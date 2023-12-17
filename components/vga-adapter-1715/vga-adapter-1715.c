#include <stdio.h>
#include <unistd.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include <soc/gdma_reg.h>
#include <soc/spi_reg.h>
#include <soc/interrupt_core1_reg.h>
#include "vga-adapter-1715.h"

#define DEBUG 1

// Pin-Definition --> muss gegebenenfalls auf die Schaltung angepasst wereden!
#define PIN_NUM_VGA_R0 4
#define PIN_NUM_VGA_R1 5
#define PIN_NUM_VGA_G0 6
#define PIN_NUM_VGA_G1 7
#define PIN_NUM_VGA_B0 15
#define PIN_NUM_VGA_B1 16
#define PIN_NUM_VGA_VSYNC 17
#define PIN_NUM_VGA_HSYNC 18

#define PIN_NUM_ABG_BSYNC1 40
#define PIN_NUM_ABG_BSYNC2 41
#define PIN_NUM_ABG_VIDEO1 38
#define PIN_NUM_ABG_VIDEO2 39

#ifdef DEBUG
#define PIN_NUM_DEBUG_SPI 10//nur für debug
#define PIN_NUM_DEBUG_COPY 11//nur für debug
#endif

// Konstanten
#define BSYNC_PIXEL_ABSTAND 3235 // Zeitabstand zwischen dem ersten Pixel und der fallenden Flanke des nachfolgendem BSYN-Impulses in 80MHz-Samples
#define BSYNC_SUCHE_START 3380   // Sample, ab dem nach dem BSYN-Impuls gesucht wird
uint8_t COLORS[] = {0, 0b00010101, 0b00101010, 0b00111111};  // Farbdefinition

// diese Definition scheint in den Header-Dateien von ESP zu fehlen!
#define REG_SPI_BASE(i)     (DR_REG_SPI1_BASE + (((i)>1) ? (((i)* 0x1000) + 0x20000) : (((~(i)) & 1)* 0x1000 )))

// globale Variablen
volatile uint32_t* ABG_DMALIST;
volatile uint32_t ABG_Scan_Line = 0;
uint8_t* VGA_BUF;

// Interrupt: wird bei fallender Flanke von BSYN aufgerufen
static void IRAM_ATTR abg_bsync_interrupt(void *args)
{
	usleep(1);  // kurz warten. Je nachdem wie der ESP heute drauf ist, ist der kurze BSYN-Impuls noch nicht ganz vorbei
	if (gpio_get_level(PIN_NUM_ABG_BSYNC2)!=0)
	{
		// bsync Impuls ist vorbei - war ein VSYNC
		if (ABG_Scan_Line > 28)
		{
		    REG_SET_BIT(GDMA_IN_LINK_CH1_REG, GDMA_INLINK_RESTART_CH1);  // das bewirkt, dass der DMA-Kontroller die Address-Tabelle neu einliest
		    REG_SET_BIT(SPI_CMD_REG(2), SPI_USR);                        // neuen SPI-Transfer starten - die Parameter sind ja schon drin
		}
		ABG_Scan_Line++;
	}
	else
	{
		// bsync Impuls liegt immer noch an - ist ein HSYNC
		ABG_Scan_Line = 0;
	}
}

// alles für die Abtastung der Video-Signale vom ABG mit SPI vorbereiten
void setup_abg()
{
	// Buffer holen
	uint8_t* ABG_PIXBUF1 = heap_caps_malloc(4096, MALLOC_CAP_DMA | MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);
	uint8_t* ABG_PIXBUF2 = heap_caps_malloc(4096, MALLOC_CAP_DMA | MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);
	ABG_DMALIST = heap_caps_malloc(24, MALLOC_CAP_DMA | MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);

	// Pin-Konfiguration
	spi_bus_config_t buscfg=
	{
		.data0_io_num = PIN_NUM_ABG_VIDEO1,  // Farb-Signal
    	.data1_io_num = PIN_NUM_ABG_VIDEO2,  // Farb-Signal
    	.data2_io_num = PIN_NUM_ABG_BSYNC1,  // wir zeichnen auch das BSYN auf, daran können wir später die Spalten ausrichten
    	.max_transfer_sz = 4092,
    	.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS,
    };
	ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

	// SPI-Konfiguration
	spi_device_interface_config_t devcfg=
    {
    	.cs_ena_pretrans = 0,
		.cs_ena_posttrans = 0,
        .clock_speed_hz = SPI_MASTER_FREQ_80M, // Abtast-Frequenz 80MHz - das ist 5x mehr als wir eigendlich brauchen,
        .flags = SPI_DEVICE_HALFDUPLEX,        // aber mit weniger gäbe es Probleme mit der Pixelsynchronisation
	    .queue_size=1,
#ifdef DEBUG
		.spics_io_num = PIN_NUM_DEBUG_SPI,     // zur Fehlersuche: an diesem Pin kann man sehen, wann das SPI aktiv ist
#endif
    };
	spi_device_handle_t spi;
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));

    // Transfer-Konfiguration
	spi_transaction_t transfer =
	{
	    .flags = SPI_TRANS_MODE_OCT,           // 8 Bit gleichzeitig einlesen. Es würde auch 4 reichen, aber dann wird die Auswertung langsamer und komplizierter
	    .length = 0,                           // nix ausgeben...
	    .rxlength = 28720,                     // nur 28k Samples einlesen
	    .rx_buffer = ABG_PIXBUF1,
	};

	// Transfer starten. Wir brauchen die Daten hier noch nicht, aber der Transfer setzt alle Parameter in dem SPI-Kontroller
	ESP_ERROR_CHECK(spi_device_queue_trans(spi, &transfer, 0));

	// Aus dieser Tabelle nimmt der DMA-Kontroller die Speicheraddressen
	ABG_DMALIST[0] = 0x80ffffff;
	ABG_DMALIST[1] = (int)ABG_PIXBUF1;
	ABG_DMALIST[2] = (int)&ABG_DMALIST[3];
	ABG_DMALIST[3] = 0x80ffffff;
	ABG_DMALIST[4] = (int)ABG_PIXBUF2;
	ABG_DMALIST[5] = (int)&ABG_DMALIST[0];

	// normalerweise nimmt der SPI-Kontroller bei diesem Projekt den DMA-Kanal 1. Sicher sein kann man sich da nicht - also überprüfen.
	assert(REG_READ(GDMA_IN_PERI_SEL_CH1_REG) == 0);

	// DMA-Kontroller 1 auf unseren Puffer umstellen und starten
	REG_SET_BIT(GDMA_IN_LINK_CH1_REG, GDMA_INLINK_STOP_CH1);
	REG_SET_BIT(GDMA_IN_CONF1_CH1_REG, GDMA_IN_CHECK_OWNER_CH1);
	REG_SET_FIELD(GDMA_IN_LINK_CH1_REG, GDMA_INLINK_ADDR_CH1, (int)ABG_DMALIST);
	REG_SET_BIT(GDMA_IN_LINK_CH1_REG, GDMA_INLINK_START_CH1);

	// jetzt noch die restlichen GPIO's konfigurieren
	// SPI und Interrupt gleichzeitig auf dem selben PIN klappt nicht, wir nehmen deshalb 2 getrennte Pins außen verbunden.
	gpio_config_t pincfg =
	{
	    .pin_bit_mask = 1ULL << PIN_NUM_ABG_BSYNC2,
	    .mode = GPIO_MODE_INPUT,
	    .pull_up_en = false,
	    .pull_down_en = false,
	    .intr_type = GPIO_INTR_NEGEDGE,
	};
	ESP_ERROR_CHECK(gpio_config(&pincfg));
	ESP_ERROR_CHECK(gpio_install_isr_service(0));
	ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_NUM_ABG_BSYNC2, abg_bsync_interrupt, 0));

#ifdef PIN_NUM_DEBUG_COPY
	// noch ein Pin zur Fehlersuche: das zeigt an, wenn das Programm Daten von dem SPI Puffer in den VGA Puffer kopiert.
	gpio_config_t pincfg2 =
	{
	    .pin_bit_mask = 1ULL << PIN_NUM_DEBUG_COPY,
	    .mode = GPIO_MODE_OUTPUT,
	    .pull_up_en = false,
	    .pull_down_en = false,
	};
	ESP_ERROR_CHECK(gpio_config(&pincfg2));
#endif
}

// alles für das VGA-Signal vorbereiten
void setup_vga()
{
	// Einstellungen
    esp_lcd_rgb_panel_config_t panel_config =
    {
        .data_width = 8,
        .psram_trans_align = 64,
        .num_fbs = 1,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .vsync_gpio_num = PIN_NUM_VGA_VSYNC,
        .hsync_gpio_num = PIN_NUM_VGA_HSYNC,
        .data_gpio_nums =
        {
        	PIN_NUM_VGA_R0,
			PIN_NUM_VGA_R1,
			PIN_NUM_VGA_G0,
			PIN_NUM_VGA_G1,
			PIN_NUM_VGA_B0,
			PIN_NUM_VGA_B1,
        },
        .timings =
        {
        	// Timing von 800x600 70Hz VGA. Andere Auflösungen geben kein stabiles Bild.
            .pclk_hz = 40000000,  		// 40MHz Pixelfrequenz
            .h_res = 640, 				// 800 auf VGA
            .v_res = 400, 				// 600 auf VGA
			//line
            .hsync_back_porch = 88+80,  // 80 Pixel Rand (800-640)/2
            .hsync_front_porch = 40+80, // 80 Pixel Rand (800-640)/2
            .hsync_pulse_width = 128,
			//frame
            .vsync_back_porch = 23+100, // 100 Pixel Rand (600-400)/2
            .vsync_front_porch = 1+100, // 100 Pixel Rand (600-400)/2
            .vsync_pulse_width = 4,
			.flags.hsync_idle_low = 1,
			.flags.vsync_idle_low = 1,
        },
        .flags.fb_in_psram = false,
    };

    esp_lcd_panel_handle_t VGA_Handle;
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &VGA_Handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(VGA_Handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(VGA_Handle));
    esp_lcd_rgb_panel_get_frame_buffer(VGA_Handle, 1, (void*)&VGA_BUF);
}

// Hauptprogramm
void IRAM_ATTR app_main(void)
{
	setup_vga();
	setup_abg();

	volatile uint32_t* next = ABG_DMALIST;

	// Timer-Interrupt (Sheduler) deaktivieren
	REG_WRITE(INTERRUPT_CORE1_SYSTIMER_TARGET1_INT_MAP_REG, 6);

	while (1)
	{
#ifdef PIN_NUM_DEBUG_COPY
		gpio_set_level(PIN_NUM_DEBUG_COPY,0);
#endif

		int a=100000;
		while ((next[0] & 0x80000000) != 0)  // darauf warten, dass der DMA-Kontroller den Transfer in den Puffer fertig meldet - der löscht das höchste Bit
		{
			if (a==0)						 // Die Wartezeit ist abgelaufen!
			{
				for (int b=0;b<640*400;b++)  // VGA-Puffer leeren
				{
					//VGA_BUF[b]=0;
					VGA_BUF[b]=(b & 255); // for testing VGA
				}
				a=100000;
			}
			a--;
		}

#ifdef PIN_NUM_DEBUG_COPY
		gpio_set_level(PIN_NUM_DEBUG_COPY,1);
#endif
		// Anzahl der empfangenen Bytes, wird vom DMA-Kontroller gesetzt
		int b=(next[0] & 0xfff000) >> 12;

		int sync = 0;
		uint8_t* buf = (uint8_t*)next[1];

		// Wir suchen den nächsten BSYNC-Impuls
		for (int a=BSYNC_SUCHE_START; a<b; a++)
		{
			if ((buf[a] & 0x04)!=0x04)
			{
				sync = a;  // gefunden!
				break;
			}
		}
		// ----------- enable for debugging  vvvvvvvv
		
		printf("Sync=%d\n",sync);
		ABG_DMALIST[0] = 0x80ffffff;
		ABG_DMALIST[3] = 0x80ffffff;
		continue;
		
		// ----------- enable for debugging  ^^^^^^^^

		// und nun die Pixel in den VGA-Puffer kopieren
		if (sync>0)
		{
			uint8_t* bufpos = (uint8_t*)((sync - BSYNC_PIXEL_ABSTAND) + (int)buf);
			uint8_t* vgapos = (uint8_t*)((ABG_Scan_Line-29)*640 + (int)VGA_BUF);

			// for (int i=0;i<640;i++) /
			// {
			// 	*vgapos = COLORS[((*bufpos)|(*(bufpos+1))) & 3];  // 2 benachbarte Samples verbinden - oft sind die Pixel nur ein Sample breit, und die Position verschiebt sich zum Bildende etwas
			// 	vgapos++;
			// 	bufpos+=5;
			// }
			for (int i=0;i<128;i++) // wir machen 5 Pixel pro Durchlauf, also 128*5 = 640 Pixel
			{
				*vgapos = COLORS[((*bufpos)|(*(bufpos+1))) & 3];  // 2 benachbarte Samples verbinden - oft sind die Pixel nur ein Sample breit, und die Position verschiebt sich zum Bildende etwas
				vgapos++;
				bufpos+=6;
				*vgapos = COLORS[((*bufpos)|(*(bufpos+1))) & 3];
				vgapos++;
				bufpos+=6;
				*vgapos = COLORS[((*bufpos)|(*(bufpos+1))) & 3];
				vgapos++;
				bufpos+=6;
				*vgapos = COLORS[((*bufpos)|(*(bufpos+1))) & 3];
				vgapos++;
				bufpos+=6;
				*vgapos = COLORS[((*bufpos)|(*(bufpos+1))) & 3];
				vgapos++;
				bufpos+=5;  // <--- die 5 statt 6 ist Absicht!
			}
		}

		// Puffer wieder für den DMA-Kontroller freigeben
		next[0]=0x80ffffff;

		// und zum nächsten Puffer springen
		next = (uint32_t*)next[2];
	}

}

