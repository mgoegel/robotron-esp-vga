#include <stdio.h>
#include <unistd.h>
#include "driver/gpio.h"
#include <driver/spi_master.h>
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include <soc/gdma_reg.h>
#include <soc/spi_reg.h>
#include <soc/interrupt_core1_reg.h>
#include <freertos/task.h>
#include <xtensa/core-macros.h>
#include "nvs_flash.h"
#include "nvs.h"

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

#define PIN_NUM_ABG_BSYNC1 21
#define PIN_NUM_ABG_BSYNC2 45
#define PIN_NUM_ABG_VIDEO1 47
#define PIN_NUM_ABG_VIDEO2 48

#define PIN_NUM_TAST_LEFT 38
#define PIN_NUM_TAST_UP 39
#define PIN_NUM_TAST_DOWN 40
#define PIN_NUM_TAST_RIGHT 41

#ifdef DEBUG
#define PIN_NUM_DEBUG_SPI 13//nur für debug
#define PIN_NUM_DEBUG_COPY 14//nur für debug
#endif

// Computer Typen
#define A7100 1
#define PC1715 2

#define ZIELTYP PC1715

// Konstanten
#if ZIELTYP == A7100
#define _MODE "A7100"
#define _PIXEL_START 2.0f
uint8_t COLORS[] = {0, 0b00000100, 0b00001000, 0b00001100};  // Farbdefinition
#define _BS_PIXEL_ABSTAND  	1618
#define _START_LINE 		28
#define _INT_DELAY 			1
#define _PIXEL_PER_LINE		736

#elif ZIELTYP == PC1715
#define _MODE "PC1715"
#define _PIXEL_START 28.4f
uint8_t COLORS[] = {0b00101010, 0b00010101, 0, 0b00111111};  // Farbdefinition
#define _BS_PIXEL_ABSTAND  	2049
#define _START_LINE 		6
#define _INT_DELAY 			6
#define _PIXEL_PER_LINE		864
#endif

// Deklaration der Modi

// Systemnamen
const char* SYSNAME[] = { "A7100", "PC1715" };

// Mapping der "Farben"
static uint8_t SYSCOLORS[][4] = {
	{0, 0b00000100, 0b00001000, 0b00001100}, // A7100
	{0b00101010, 0b00010101, 0, 0b00111111} // PC1715
};

// Initialisierte Daten aus dem NVS
struct SYSMODE {
	uint16_t mode;
	float pixel_start;
	uint16_t pixel_abstand;
	uint16_t start_line;
	uint16_t pixel_per_line;
};

// diese Definition scheint in den Header-Dateien von ESP zu fehlen!
#define REG_SPI_BASE(i)     (DR_REG_SPI1_BASE + (((i)>1) ? (((i)* 0x1000) + 0x20000) : (((~(i)) & 1)* 0x1000 )))

// globale Variablen

// Aktives System
static uint16_t ACTIVESYS = 0;

volatile uint32_t* ABG_DMALIST;
volatile uint32_t ABG_Scan_Line = 0;
volatile uint32_t ABG_PIXEL_PER_LINE = _PIXEL_PER_LINE;
volatile uint32_t BSYNC_PIXEL_ABSTAND = _BS_PIXEL_ABSTAND;
volatile uint32_t ABG_START_LINE = _START_LINE;
volatile double ABG_PIXEL_START = _PIXEL_START;
volatile bool ABG_RUN = false;
volatile uint32_t ABG_INT_DELAY = _INT_DELAY;

int BSYNC_SUCHE_START = 0;
uint8_t* PIXEL_STEP_LIST;
uint8_t* VGA_BUF;
uint8_t* OSD_BUF;
volatile uint32_t bsyn_clock_diff = 0;
volatile uint32_t bsyn_clock_last = 0;

// Zeichensatz für das OSD-Textfeld
unsigned char CHARSET[] =
{
	0x80, 0x80, 0x80, 0x00, 0x00, 0x00,  //
	0x5F, 0x5F, 0x00, 0x00, 0x00, 0x00,  // !
	0x03, 0x03, 0x03, 0x03, 0x00, 0x00,  // "
	0x36, 0x7F, 0x36, 0x7F, 0x36, 0x00,  // #
	0x2C, 0x7F, 0x2A, 0x7F, 0x1A, 0x00,  // $
	0x03, 0x63, 0x38, 0x0E, 0x67, 0x60,  // %
	0x3A, 0x7F, 0x4D, 0x7F, 0x32, 0x78,  // &
	0x03, 0x03, 0x00, 0x00, 0x00, 0x00,  // '
	0x3E, 0x7F, 0x41, 0x00, 0x00, 0x00,  // (
	0x41, 0x7F, 0x3E, 0x00, 0x00, 0x00,  // )
	0x54, 0x7C, 0x38, 0x7C, 0x54, 0x00,  // *
	0x10, 0x10, 0x7C, 0x10, 0x10, 0x00,  // +
	0x60, 0x30, 0x00, 0x00, 0x00, 0x00,  // ,
	0x10, 0x10, 0x10, 0x10, 0x00, 0x00,  // -
	0x80, 0x30, 0x30, 0x00, 0x00, 0x00,  // .
	0x70, 0x1C, 0x07, 0x00, 0x00, 0x00,  // /
	0x3E, 0x7F, 0x41, 0x7F, 0x3E, 0x00,  // 0
	0x80, 0x02, 0x7F, 0x7F, 0x00, 0x00,  // 1
	0x71, 0x79, 0x49, 0x4F, 0x46, 0x00,  // 2
	0x41, 0x41, 0x49, 0x7F, 0x36, 0x00,  // 3
	0x18, 0x14, 0x12, 0x7F, 0x7F, 0x00,  // 4
	0x47, 0x45, 0x45, 0x7D, 0x38, 0x00,  // 5
	0x3E, 0x7F, 0x45, 0x7D, 0x38, 0x00,  // 6
	0x01, 0x71, 0x7D, 0x0F, 0x03, 0x00,  // 7
	0x36, 0x7F, 0x49, 0x7F, 0x36, 0x00,  // 8
	0x0E, 0x5F, 0x51, 0x7F, 0x3E, 0x00,  // 9
	0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00,  // :
	0x4C, 0x6C, 0x00, 0x00, 0x00, 0x00,  // ;
	0x10, 0x38, 0x6C, 0x44, 0x00, 0x00,  // <
	0x28, 0x28, 0x28, 0x28, 0x00, 0x00,  // =
	0x44, 0x6C, 0x38, 0x10, 0x00, 0x00,  // >
	0x01, 0x59, 0x5D, 0x07, 0x02, 0x00,  // ?
	0x3E, 0x7F, 0x55, 0x5D, 0x5F, 0x1E,  // @
	0x7E, 0x7F, 0x11, 0x7F, 0x7E, 0x00,  // A
	0x7F, 0x7F, 0x45, 0x7F, 0x3A, 0x00,  // B
	0x3E, 0x7F, 0x41, 0x41, 0x41, 0x00,  // C
	0x7F, 0x7F, 0x41, 0x7F, 0x3E, 0x00,  // D
	0x7F, 0x7F, 0x45, 0x45, 0x00, 0x00,  // E
	0x7F, 0x7F, 0x05, 0x05, 0x01, 0x00,  // F
	0x3E, 0x7F, 0x41, 0x79, 0x78, 0x00,  // G
	0x7F, 0x7F, 0x04, 0x7F, 0x7F, 0x00,  // H
	0x7F, 0x7F, 0x00, 0x00, 0x00, 0x00,  // I
	0x00, 0x40, 0x40, 0x7F, 0x3F, 0x00,  // J
	0x7F, 0x7F, 0x1C, 0x36, 0x63, 0x41,  // K
	0x7F, 0x7F, 0x40, 0x40, 0x40, 0x00,  // L
	0x7F, 0x7F, 0x0F, 0x3C, 0x0F, 0x7F,  // M
	0x7F, 0x7E, 0x0C, 0x18, 0x3F, 0x7F,  // N
	0x3E, 0x7F, 0x41, 0x41, 0x7F, 0x3E,  // O
	0x7F, 0x7F, 0x11, 0x1F, 0x0E, 0x00,  // P
	0x3E, 0x7F, 0x41, 0x51, 0x7F, 0x7E,  // Q
	0x7F, 0x7F, 0x11, 0x7F, 0x4E, 0x00,  // R
	0x46, 0x4F, 0x79, 0x31, 0x00, 0x00,  // S
	0x01, 0x01, 0x7F, 0x7F, 0x01, 0x01,  // T
	0x3F, 0x7F, 0x40, 0x7F, 0x3F, 0x00,  // U
	0x07, 0x1F, 0x7C, 0x7C, 0x1F, 0x07,  // V
	0x07, 0x3F, 0x78, 0x3C, 0x78, 0x3F,  // W
	0x63, 0x77, 0x1C, 0x1C, 0x77, 0x63,  // X
	0x03, 0x07, 0x7C, 0x7C, 0x07, 0x03,  // Y
	0x71, 0x79, 0x5D, 0x4F, 0x47, 0x00,  // Z
	0x7F, 0x7F, 0x41, 0x00, 0x00, 0x00,  // [
	0x07, 0x1C, 0x70, 0x00, 0x00, 0x00,
	0x41, 0x7F, 0x7F, 0x00, 0x00, 0x00,  // ]
	0x06, 0x03, 0x06, 0x00, 0x00, 0x00,  // ^
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40,  // _
	0x01, 0x03, 0x02, 0x00, 0x00, 0x00,  // `
	0x20, 0x74, 0x54, 0x7C, 0x78, 0x00,  // a
	0x7F, 0x7F, 0x44, 0x7C, 0x38, 0x00,  // b
	0x38, 0x7C, 0x44, 0x44, 0x00, 0x00,  // c
	0x38, 0x7C, 0x44, 0x7F, 0x7F, 0x00,  // d
	0x38, 0x7C, 0x54, 0x5C, 0x58, 0x00,  // e
	0x7E, 0x7F, 0x05, 0x00, 0x00, 0x00,  // f
	0x08, 0x5C, 0x54, 0x7C, 0x3C, 0x00,  // g
	0x7F, 0x7F, 0x04, 0x7C, 0x78, 0x00,  // h
	0x7D, 0x7D, 0x00, 0x00, 0x00, 0x00,  // i
	0x40, 0x7D, 0x3D, 0x00, 0x00, 0x00,  // j
	0x7F, 0x7F, 0x38, 0x6C, 0x44, 0x00,  // k
	0x7F, 0x7F, 0x00, 0x00, 0x00, 0x00,  // l
	0x7C, 0x7C, 0x04, 0x7C, 0x04, 0x78,  // m
	0x7C, 0x7C, 0x04, 0x7C, 0x78, 0x00,  // n
	0x38, 0x7C, 0x44, 0x7C, 0x38, 0x00,  // o
	0x7C, 0x7C, 0x14, 0x1C, 0x08, 0x00,  // p
	0x08, 0x1C, 0x14, 0x7C, 0x7C, 0x00,  // q
	0x7C, 0x7C, 0x08, 0x0C, 0x00, 0x00,  // r
	0x58, 0x5C, 0x74, 0x34, 0x00, 0x00,  // s
	0x3F, 0x7F, 0x44, 0x00, 0x00, 0x00,  // t
	0x3C, 0x7C, 0x40, 0x7C, 0x7C, 0x00,  // u
	0x0C, 0x3C, 0x70, 0x3C, 0x0C, 0x00,  // v
	0x1C, 0x7C, 0x38, 0x38, 0x7C, 0x1C,  // w
	0x6C, 0x7C, 0x10, 0x7C, 0x6C, 0x00,  // x
	0x0C, 0x5C, 0x50, 0x7C, 0x3C, 0x00,  // y
	0x64, 0x74, 0x5C, 0x4C, 0x44, 0x00,  // z
	0x08, 0x7F, 0x77, 0x00, 0x00, 0x00,  // {
	0x7F, 0x7F, 0x00, 0x00, 0x00, 0x00,  // |
	0x77, 0x7F, 0x08, 0x00, 0x00, 0x00,  // }
	0x21, 0x75, 0x54, 0x7D, 0x79, 0x00,  // ä
	0x7F, 0x7F, 0x00, 0x00, 0x00, 0x00
};

// Text in die OSD-Zeile drucken
static void drawtext(char* txt, int count, int pos, char color)
{
	for (int a=0;a<count;a++)
	{
		for (int b=0;b<6;b++)
		{
			char c=CHARSET[(txt[a]-32)*6+b];
			for (int d=0;d<7;d++)
			{
				OSD_BUF[b+(pos+a)*7+d*640] = (((1 << d) & c) != 0) ? color : 0x00;
			}
		}
	}
}

// Interrupt: wird bei fallender Flanke von BSYN aufgerufen
static void IRAM_ATTR abg_bsync_interrupt(void *args)
{
	usleep(_INT_DELAY);  // kurz warten. Je nachdem wie der ESP heute drauf ist, ist der kurze BSYN-Impuls noch nicht ganz vorbei
	if (gpio_get_level(PIN_NUM_ABG_BSYNC2)!=0)
	{
		// CPU-Zyklen 0 stellen
		if (ABG_Scan_Line == 10)
		{
			XTHAL_SET_CCOUNT(0);
		}

		// CPU-Zyklen auslesen und merken
		if (ABG_Scan_Line == 210)
		{
			bsyn_clock_diff = XTHAL_GET_CCOUNT();
		}

		// bsync Impuls ist vorbei - war ein VSYNC
		if (ABG_Scan_Line > ABG_START_LINE && ABG_RUN)
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
	PIXEL_STEP_LIST = heap_caps_malloc(640, MALLOC_CAP_DMA | MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);

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
        .clock_speed_hz = SPI_MASTER_FREQ_40M, // Abtast-Frequenz 40MHz - das ist mehr als wir eigendlich brauchen,
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
	    .rxlength = 1000,                      // Platzhalter, wird später aus dem hsync-timing errechnet
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
            .v_res = 420, 				// 600 auf VGA
			//line
            .hsync_back_porch = 88+80,  // 80 Pixel Rand (800-640)/2
            .hsync_front_porch = 40+80, // 80 Pixel Rand (800-640)/2
            .hsync_pulse_width = 128,
			//frame
            .vsync_back_porch = 23+80, // 80 Pixel Rand (600-400)/2 (-20 für osd)
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
    esp_lcd_rgb_panel_get_frame_buffer(VGA_Handle, 1, (void*)&OSD_BUF); // der OSD-Buffer ist genaugenommen der gesamte Bildschirm
    VGA_BUF = 20 * 640 + OSD_BUF;  // der Bereich für das eigendliche Bild beginnt erst ab Zeile 20
}

// das OSD-Menue
void osd_task(void*)
{
	// Taster-Pins einstellen
	gpio_config_t pincfg =
	{
		.pin_bit_mask = 1ULL<<PIN_NUM_TAST_LEFT | 1ULL<<PIN_NUM_TAST_UP | 1ULL<<PIN_NUM_TAST_DOWN | 1ULL<<PIN_NUM_TAST_RIGHT,
	    .mode = GPIO_MODE_INPUT,
	    .pull_up_en = true,
	    .pull_down_en = false,
	};
	ESP_ERROR_CHECK(gpio_config(&pincfg));

	char* tb = heap_caps_malloc(40, MALLOC_CAP_INTERNAL);
	int cursor = 0;
	for (int h=0;h<20*640;h++) OSD_BUF[h]=0x0;

	while (1)
	{
		// Menü ausgeben
		int l = snprintf(tb, 40, _MODE);
		drawtext(tb,l,1,0x3f);
		l = snprintf(tb, 40, "Pixel pro Zeile=%ld    ",ABG_PIXEL_PER_LINE);
		drawtext(tb,l,9,cursor==0 ? 0x03 : 0x3f);
		l = snprintf(tb, 40, "Pixel Abstand=%ld    ",BSYNC_PIXEL_ABSTAND);
		drawtext(tb,l,30,cursor==1 ? 0x03 : 0x3f);
		l = snprintf(tb, 40, "Startzeile=%ld    ",ABG_START_LINE);
		drawtext(tb,l,50,cursor==2 ? 0x03 : 0x3f);
		l = snprintf(tb, 40, "Subpixel-Abst.=%f  ",ABG_PIXEL_START);
		drawtext(tb,l,65,cursor==3 ? 0x03 : 0x3f);

		// darauf warten, dass alle Tasten losgelassen werden
		while (gpio_get_level(PIN_NUM_TAST_LEFT)==0 || gpio_get_level(PIN_NUM_TAST_UP)==0 || gpio_get_level(PIN_NUM_TAST_DOWN)==0 || gpio_get_level(PIN_NUM_TAST_RIGHT)==0)
		{
			usleep(10000);
		}

		int i = 500;
		// darauf warten, dass eine Taste gedrückt wird
		while (gpio_get_level(PIN_NUM_TAST_LEFT)!=0 && gpio_get_level(PIN_NUM_TAST_UP)!=0 && gpio_get_level(PIN_NUM_TAST_DOWN)!=0 && gpio_get_level(PIN_NUM_TAST_RIGHT)!=0)
		{
			usleep(10000);
			if (i==1) // nach paar Sekunden Inaktivität das OSD verschwinden lassen
			{
				for (int h=0;h<20*640;h++) OSD_BUF[h]=0x0;
			}
			if (i>0 && ABG_RUN) i--;
		}

		// cursor nach rechts
		if (gpio_get_level(PIN_NUM_TAST_LEFT)==0)
		{
			cursor--;
			if (cursor==-1) cursor=3;
		}

		// cursor nach links
		if (gpio_get_level(PIN_NUM_TAST_RIGHT)==0)
		{
			cursor++;
			if (cursor==4) cursor=0;
		}

		// wert erhöhen
		if (gpio_get_level(PIN_NUM_TAST_UP)==0)
		{
			switch (cursor)
			{
				case 0:
					ABG_PIXEL_PER_LINE++;
					break;
				case 1:
					BSYNC_PIXEL_ABSTAND++;
					break;
				case 2:
					ABG_START_LINE++;
					break;
				case 3:
					ABG_PIXEL_START+=0.1f;
			}
		}

		// wert senken
		if (gpio_get_level(PIN_NUM_TAST_DOWN)==0)
		{
			switch (cursor)
			{
				case 0:
					ABG_PIXEL_PER_LINE--;
					break;
				case 1:
					BSYNC_PIXEL_ABSTAND--;
					break;
				case 2:
					ABG_START_LINE--;
					break;
				case 3:
					ABG_PIXEL_START-=0.1f;
			}
		}
	}
}

// NVS Partition initialisieren und Daten vom Flash laden, wenn vorhanden
void setup_flash() {
	// Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
}

// Hauptprogramm
void IRAM_ATTR app_main(void)
{
	setup_vga();
	setup_abg();
	setup_flash();

	xTaskCreatePinnedToCore(osd_task,"osd_task",6000,NULL,0,NULL,1);

	volatile uint32_t* next = ABG_DMALIST;

	while (1)
	{
#ifdef PIN_NUM_DEBUG_COPY
		gpio_set_level(PIN_NUM_DEBUG_COPY,0);
#endif

		int a=100000;
		while ((next[0] & 0x80000000) != 0)  // darauf warten, dass der DMA-Kontroller den Transfer in den Puffer fertig meldet - der löscht das höchste Bit
		{
			if (bsyn_clock_last != bsyn_clock_diff && ABG_Scan_Line == 0)
			{
				// :200 Zeilen
				// :240 MHz CPU-Takt
				// *40 MHz Sample Rate
				// = 1200
				// :ABG_PIXEL_PER_LINE
#ifdef PIN_NUM_DEBUG_COPY
		gpio_set_level(PIN_NUM_DEBUG_COPY,1);
#endif
				bsyn_clock_last = bsyn_clock_diff;
				double weite = bsyn_clock_diff / (1200.0f * ABG_PIXEL_PER_LINE);
				double step = ABG_PIXEL_START;
				int last = (int)step;
				for (int i=0;i<640;i++)
				{
					step += weite;
					PIXEL_STEP_LIST[i] = ((int)step) - last;
					last = (int)step;
				}

				// :200 Zeilen
				// :240 MHz CPU-Takt
				// *40 MHz Sample Rate
				// *8 Bit per Sample
				// = 150
				int spi_len = (bsyn_clock_diff / 150) - 400;
				REG_SET_FIELD(SPI_MS_DLEN_REG(2), SPI_MS_DATA_BITLEN, spi_len);
				REG_SET_BIT(SPI_CMD_REG(2), SPI_UPDATE);

				ABG_RUN = true;
#ifdef PIN_NUM_DEBUG_COPY
		gpio_set_level(PIN_NUM_DEBUG_COPY,0);
#endif
			}

			if (a==0)						 // Die Wartezeit ist abgelaufen!
			{
				for (int b=0;b<640*400;b++)  // VGA-Puffer leeren
				{
				    VGA_BUF[b]=0;
				}
				a=100000;
				ABG_RUN = false;
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

		if ((buf[BSYNC_SUCHE_START] & 0x04)!=0x04)
		{
			BSYNC_SUCHE_START = BSYNC_PIXEL_ABSTAND+1;
		}

		// Wir suchen den nächsten BSYNC-Impuls
		for (int a=BSYNC_SUCHE_START; a<b; a++)
		{
			if ((buf[a] & 0x04)!=0x04)
			{
				sync = a;  // gefunden!
				BSYNC_SUCHE_START = a - 10;
				break;
			}
		}

		// und nun die Pixel in den VGA-Puffer kopieren
		if (sync>0 && ABG_Scan_Line>=ABG_START_LINE && ABG_Scan_Line<=ABG_START_LINE+400)
		{
			uint8_t* bufpos = (uint8_t*)((sync - BSYNC_PIXEL_ABSTAND) + (int)buf);
			uint8_t* vgapos = (uint8_t*)((ABG_Scan_Line-(ABG_START_LINE+2))*640 + (int)VGA_BUF);

			for (int i=0;i<640;i++)
			{
				*vgapos = COLORS[(*bufpos) & 3];
				vgapos++;
				bufpos+=PIXEL_STEP_LIST[i];
			}
		}
		else
		{
			BSYNC_SUCHE_START = BSYNC_PIXEL_ABSTAND+1;
		}

		// Puffer wieder für den DMA-Kontroller freigeben
		next[0]=0x80ffffff;

		// und zum nächsten Puffer springen
		next = (uint32_t*)next[2];
	}

}
