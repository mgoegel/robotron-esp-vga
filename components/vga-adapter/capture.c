#include <math.h>
#include "driver/gpio.h"
#include "esp_private/periph_ctrl.h"
#include <driver/spi_master.h>
#include <xtensa_context.h>
#include <soc/gpio_reg.h>
#include <soc/gdma_reg.h>
#include <soc/spi_reg.h>
#include "esp_intr_alloc.h"
#include <rom/ets_sys.h>
#include <string.h>
#include "esp_private/gdma.h"

#include "driver/gpio_filter.h"

#include "capture.h"
#include "globalvars.h"
#include "main.h"
#include "pins.h"
#include "osd.h"

// einen bestimmten DMA-Kanal reservieren
void alloc_abg_dma(uint8_t channel)
{
	gdma_channel_handle_t s_rx_channel;
	gdma_channel_alloc_config_t rx_alloc_config = 
	{
		.direction = GDMA_CHANNEL_DIRECTION_RX,
		.sibling_chan = NULL,
	};
	if (gdma_new_channel(&rx_alloc_config, &s_rx_channel) != ESP_OK) return;
	int i=10;
	gdma_get_channel_id(s_rx_channel, &i);
	if (i != channel)
	{
		alloc_abg_dma(channel);
		gdma_del_channel(s_rx_channel);
	}
}

// alles für die Abtastung der Video-Signale vom ABG mit SPI vorbereiten
void setup_abg()
{
	// Buffer holen
	uint8_t* ABG_PIXBUF1 = heap_caps_malloc(4096, MALLOC_CAP_DMA | MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);
	uint8_t* ABG_PIXBUF2 = heap_caps_malloc(4096, MALLOC_CAP_DMA | MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);
	ABG_DMALIST = heap_caps_malloc(32, MALLOC_CAP_DMA | MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);
	PIXEL_STEP_LIST = heap_caps_malloc(ABG_XRes, MALLOC_CAP_DMA | MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);

	// Pin-Konfiguration
	spi_bus_config_t buscfg=
	{
		.data0_io_num = 0,
		.data1_io_num = PIN_NUM_ABG_VIDEO1,  // Farb-Signal
    	.data2_io_num = PIN_NUM_ABG_VIDEO2,  // Farb-Signal
    	.data3_io_num = PIN_NUM_ABG_BSYNC1,  // wir zeichnen auch das BSYN auf, daran können wir später die Spalten ausrichten
    	.max_transfer_sz = 4096,
    	.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS,
    };
	ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

	// SPI-Konfiguration
	spi_device_interface_config_t devcfg=
    {
    	.cs_ena_pretrans = 0,
		.cs_ena_posttrans = 0,
        .clock_speed_hz = SPI_MASTER_FREQ_80M, // Abtast-Frequenz 80MHz - das ist mehr als wir eigendlich brauchen,
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
	    .flags = (ABG_Bits_per_sample == 4) ? SPI_TRANS_MODE_QIO : SPI_TRANS_MODE_OCT,           // 4 oder 8 Bit gleichzeitig einlesen
	    .length = 0,                           // nix ausgeben...
	    .rxlength = 4090 * 8,
	    .rx_buffer = ABG_PIXBUF1,
	};

	// Transfer starten. Wir brauchen die Daten hier noch nicht, aber der Transfer setzt alle Parameter in dem SPI-Kontroller
	ESP_ERROR_CHECK(spi_device_queue_trans(spi, &transfer, 0));

	// Aus dieser Tabelle nimmt der DMA-Kontroller die Speicheraddressen
	ABG_DMALIST[0] = 0xc0000000;
	ABG_DMALIST[1] = (int)ABG_PIXBUF1;
	ABG_DMALIST[2] = 0;
	ABG_DMALIST[3] = 0;
	ABG_DMALIST[4] = 0xc0000000;
	ABG_DMALIST[5] = (int)ABG_PIXBUF2;
	ABG_DMALIST[6] = 0;
	ABG_DMALIST[7] = 0;

	// wir haben uns den DMA-Kanal 1 reserviert, nun weisen wir den Kanal wieder dem SPI-Kontroller zu
	if (REG_READ(GDMA_IN_PERI_SEL_CH0_REG) == 0) REG_WRITE(GDMA_IN_PERI_SEL_CH0_REG, 63);
	REG_WRITE(GDMA_IN_PERI_SEL_CH1_REG, 0);
	if (REG_READ(GDMA_IN_PERI_SEL_CH2_REG) == 0) REG_WRITE(GDMA_IN_PERI_SEL_CH2_REG, 63);
	if (REG_READ(GDMA_IN_PERI_SEL_CH3_REG) == 0) REG_WRITE(GDMA_IN_PERI_SEL_CH3_REG, 63);

	// DMA-Kontroller 1 auf unseren Puffer umstellen und starten
	REG_SET_BIT(GDMA_IN_LINK_CH1_REG, GDMA_INLINK_STOP_CH1);
	REG_SET_BIT(GDMA_IN_CONF1_CH1_REG, GDMA_IN_CHECK_OWNER_CH1);
	REG_SET_FIELD(GDMA_IN_LINK_CH1_REG, GDMA_INLINK_ADDR_CH1, (int)ABG_DMALIST);
	REG_SET_BIT(GDMA_IN_LINK_CH1_REG, GDMA_INLINK_START_CH1);

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


    gpio_glitch_filter_handle_t filter = NULL;
    gpio_pin_glitch_filter_config_t config = 
	{
        .clk_src = GLITCH_FILTER_CLK_SRC_DEFAULT,
		.gpio_num = PIN_NUM_ABG_BSYNC1,
    };
    ESP_ERROR_CHECK(gpio_new_pin_glitch_filter(&config, &filter));
    ESP_ERROR_CHECK(gpio_glitch_filter_enable(filter));
	config.gpio_num = PIN_NUM_ABG_BSYNC2;
    ESP_ERROR_CHECK(gpio_new_pin_glitch_filter(&config, &filter));
    ESP_ERROR_CHECK(gpio_glitch_filter_enable(filter));


	ESP_INTR_DISABLE(31);
	intr_matrix_set(1, ETS_GPIO_INTR_SOURCE, 31);  // Assembler ISR aktivieren
	ESP_INTR_ENABLE(31);

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

	gpio_config_t pincfg3 =
	{
	    .pin_bit_mask = 1ULL << PIN_NUM_LED_WIFI | 1ULL << PIN_NUM_LED_SYNC,
	    .mode = GPIO_MODE_OUTPUT,
	    .pull_up_en = false,
	    .pull_down_en = false,
	};
	ESP_ERROR_CHECK(gpio_config(&pincfg3));
	gpio_set_level(PIN_NUM_LED_WIFI,0);
	gpio_set_level(PIN_NUM_LED_SYNC,0);
}


void IRAM_ATTR capture_task(void*)
{
	setup_abg();
	volatile uint32_t* next = ABG_DMALIST;

	while (1)
	{
#ifdef PIN_NUM_DEBUG_COPY
		gpio_set_level(PIN_NUM_DEBUG_COPY,0);
#endif

		int a=1000000;
		while (((ABG_DMALIST[0] & 0x80000000) != 0) && ((ABG_DMALIST[4] & 0x80000000) != 0))  // darauf warten, dass der DMA-Kontroller den Transfer in den Puffer fertig meldet - der löscht das höchste Bit
		{
			if (bsyn_clock_last != bsyn_clock_diff && ABG_Scan_Line < 5)
			{
				// :200 Zeilen
				// :240 MHz CPU-Takt
				// *80 MHz Sample Rate
				// = 600
				// :ABG_PIXEL_PER_LINE
#ifdef PIN_NUM_DEBUG_COPY
		gpio_set_level(PIN_NUM_DEBUG_COPY,1);
#endif
				bsyn_clock_last = bsyn_clock_diff;
				double weite = bsyn_clock_diff / (600.0f * ABG_PIXEL_PER_LINE);
				double abst = 0.0f;
				double step = modf((weite * ((double)(ABG_PIXEL_PER_LINE - BSYNC_PIXEL_ABSTAND))), &abst);
				BSYNC_SAMPLE_ABSTAND = (int)abst;
				int istep = round(step); 
				int last = istep;
				for (int i=0;i<ABG_XRes;i++)
				{
					step += weite;
					istep = round(step);
					PIXEL_STEP_LIST[i] = istep - last;
					last = istep;
				}

				ABG_RUN = true;
				gpio_set_level(PIN_NUM_LED_SYNC,1);
#ifdef PIN_NUM_DEBUG_COPY
		gpio_set_level(PIN_NUM_DEBUG_COPY,0);
#endif
			}

			if (a==0)						 // Die Wartezeit ist abgelaufen!
			{
				uint8_t nullcolor = (Current_Color_Scheme == _COLORSCHEME_COUNT) ? Custom_Colors[0] : _STATIC_COLOR_VALS[Current_Color_Scheme].colors[0];
				for (int b=0;b<ABG_YRes;b++)  // VGA-Puffer leeren
					for (int c=0;c<ABG_XRes;c++)
						VGA_BUF[b][c]=nullcolor;
				a=1000000;
				ABG_RUN = false;
				ABG_DMALIST[0] = 0xc0000000;
				ABG_DMALIST[4] = 0xc0000000;
				bsyn_clock_last = 0;
				bsyn_clock_diff = 0;
				gpio_set_level(PIN_NUM_LED_SYNC,0);
			}
			a--;
		}

		if ((ABG_DMALIST[0] & 0x80000000) == 0)
		{
			next = (uint32_t*)ABG_DMALIST;
		}
		else
		{
			next = (uint32_t*)ABG_DMALIST + 4;
		}

#ifdef PIN_NUM_DEBUG_COPY
		gpio_set_level(PIN_NUM_DEBUG_COPY,1);
#endif

		uint32_t line = next[3];

		if (line>=ABG_START_LINE && line<=ABG_START_LINE+(ABG_YRes-1))
		{
			// Anzahl der empfangenen Bytes, wird vom DMA-Kontroller gesetzt
			int b=(next[0] & 0xfff000) >> 12;

			int sync = 0;
			uint8_t* buf = (uint8_t*)next[1];

			// Wir suchen den nächsten BSYNC-Impuls
			if (ABG_Bits_per_sample == 4)
			{
				for (int a=b-25; a<b; a++)
				{
					if ((buf[a] & 0x88)!=0x88)
					{
						sync = a * 2;  // gefunden!
						if ((buf[a] & 0x88) == 0x80)
						{
							sync++;
						}
						break;
					}
				}
			}
			else
			{
				for (int a=b-50; a<b; a++)
				{
					if ((buf[a] & 0x08)!=0x08)
					{
						sync = a;  // gefunden!
						break;
					}
				}
			}

			if (sync<BSYNC_SAMPLE_ABSTAND)
			{
				sync = BSYNC_SAMPLE_ABSTAND;
			}

			// und nun die Pixel in den VGA-Puffer kopieren
			if (sync>0)
			{
				if (ABG_Bits_per_sample == 4)
				{
					uint32_t bufpos = sync - BSYNC_SAMPLE_ABSTAND;
					uint8_t* vgapos = VGA_BUF[line - ABG_START_LINE];
					uint8_t* stepende = (uint8_t*)((int)PIXEL_STEP_LIST + ABG_XRes);
					for (uint8_t* steplist=PIXEL_STEP_LIST;steplist<stepende;steplist++)
					{
						if (bufpos & 1)
						{
							*vgapos = Current_Colors[((*((bufpos>>1)+buf))>>1) & 3];
						}
						else
						{
							*vgapos = Current_Colors[((*((bufpos>>1)+buf))>>5) & 3];
						}
						vgapos++;
						bufpos+=*steplist;
					}
				}
				else // 8 bit samples
				{
					uint8_t* bufpos = (uint8_t*)((sync - BSYNC_SAMPLE_ABSTAND) + (int)buf);
					uint8_t* vgapos = VGA_BUF[line - ABG_START_LINE];
					uint8_t* stepende = (uint8_t*)((int)PIXEL_STEP_LIST + ABG_XRes);
					for (uint8_t* steplist=PIXEL_STEP_LIST;steplist<stepende;steplist++)
					{
						*vgapos = Current_Colors[(*bufpos)>>1 & 3];
						vgapos++;
						bufpos+=*steplist;
					}
				}
			}
		}

		// Puffer für den ISR freigeben
		next[0]=0xc0000000;
	}
}
