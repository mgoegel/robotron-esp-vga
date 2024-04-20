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

#include "capture.h"
#include "globalvars.h"
#include "main.h"
#include "pins.h"

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
    	.max_transfer_sz = 4092,
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
	    .rxlength = 1000,                      // Platzhalter, wird später aus dem hsync-timing errechnet
	    .rx_buffer = ABG_PIXBUF1,
	};

	// Transfer starten. Wir brauchen die Daten hier noch nicht, aber der Transfer setzt alle Parameter in dem SPI-Kontroller
	ESP_ERROR_CHECK(spi_device_queue_trans(spi, &transfer, 0));

	// Aus dieser Tabelle nimmt der DMA-Kontroller die Speicheraddressen
	ABG_DMALIST[0] = 0x80ffffff;
	ABG_DMALIST[1] = (int)ABG_PIXBUF1;
	ABG_DMALIST[2] = (int)&ABG_DMALIST[4];
	ABG_DMALIST[4] = 0x80ffffff;
	ABG_DMALIST[5] = (int)ABG_PIXBUF2;
	ABG_DMALIST[6] = (int)&ABG_DMALIST[0];

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
			if (bsyn_clock_last != bsyn_clock_diff && ABG_Scan_Line == 0)
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

				// :200 Zeilen
				// :240 MHz CPU-Takt
				// *80 MHz Sample Rate
				// *4 Bit per Sample
				// = 150
				int spi_len = (bsyn_clock_diff / ((ABG_Bits_per_sample == 4) ? 150 : 75)) - 600;
				REG_SET_FIELD(SPI_MS_DLEN_REG(2), SPI_MS_DATA_BITLEN, spi_len);
				REG_SET_BIT(SPI_CMD_REG(2), SPI_UPDATE);
				ABG_RUN = true;
#ifdef PIN_NUM_DEBUG_COPY
		gpio_set_level(PIN_NUM_DEBUG_COPY,0);
#endif
			}

			if (a==0)						 // Die Wartezeit ist abgelaufen!
			{
				for (int b=0;b<ABG_XRes*ABG_YRes;b++)  // VGA-Puffer leeren
				{
				    VGA_BUF[b]=0;
				}
				a=1000000;
				ABG_RUN = false;
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
		// Anzahl der empfangenen Bytes, wird vom DMA-Kontroller gesetzt
		int b=(next[0] & 0xfff000) >> 12;

		int sync = 0;
		uint8_t* buf = (uint8_t*)next[1];

		if ((buf[BSYNC_SUCHE_START] & 0x08)!=0x08)
		{
			BSYNC_SUCHE_START = (BSYNC_SAMPLE_ABSTAND>>1)+1;
		}

		// Wir suchen den nächsten BSYNC-Impuls


		if (ABG_Bits_per_sample == 4)
		{
			for (int a=BSYNC_SUCHE_START; a<b; a++)
			{
				if ((buf[a] & 0x88)!=0x88)
				{
					sync = a * 2;  // gefunden!
					if ((buf[a] & 0x88) == 0x80)
					{
						sync++;
					}
					BSYNC_SUCHE_START = a - 20;
					break;
				}
			}
		}
		else
		{
			for (int a=BSYNC_SUCHE_START; a<b; a++)
			{
				if ((buf[a] & 0x08)!=0x08)
				{
					sync = a;  // gefunden!
					BSYNC_SUCHE_START = a - 20;
					break;
				}
			}
		}

		uint32_t line = next[3];

		// _STATIC_SYS_VALS liegt jetzt im externen Flash-Rom - für die Kopierschleife etwas zu langsam
		uint8_t colors[4];
		for (int i=0;i<4;i++)
		{
			colors[i] = _STATIC_SYS_VALS[ACTIVESYS].colors[i];
		}

		// und nun die Pixel in den VGA-Puffer kopieren
		if (sync>0 && line>=ABG_START_LINE && line<=ABG_START_LINE+(ABG_YRes-1))
		{
			if (ABG_Bits_per_sample == 4)
			{
				uint32_t bufpos = sync - BSYNC_SAMPLE_ABSTAND;
				uint8_t* vgapos = (uint8_t*)((line - ABG_START_LINE)*ABG_XRes + (int)VGA_BUF);
				uint8_t* stepende = (uint8_t*)((int)PIXEL_STEP_LIST + ABG_XRes);
				for (uint8_t* steplist=PIXEL_STEP_LIST;steplist<stepende;steplist++)
				{
					if (bufpos & 1)
					{
						*vgapos = colors[((*((bufpos>>1)+buf))>>1) & 3];
					}
					else
					{
						*vgapos = colors[((*((bufpos>>1)+buf))>>5) & 3];
					}
					vgapos++;
					bufpos+=*steplist;
				}
			}
			else // 8 bit samples
			{
				uint8_t* bufpos = (uint8_t*)((sync - BSYNC_SAMPLE_ABSTAND) + (int)buf);
				uint8_t* vgapos = (uint8_t*)((line - ABG_START_LINE)*ABG_XRes + (int)VGA_BUF);
				uint8_t* stepende = (uint8_t*)((int)PIXEL_STEP_LIST + ABG_XRes);
				for (uint8_t* steplist=PIXEL_STEP_LIST;steplist<stepende;steplist++)
				{
					*vgapos = colors[(*bufpos)>>1 & 3];
					vgapos++;
					bufpos+=*steplist;
				}
			}
		}
		else
		{
			BSYNC_SUCHE_START = (BSYNC_SAMPLE_ABSTAND>>1)+1;
		}

		// Puffer wieder für den DMA-Kontroller freigeben
		next[0]=0x80ffffff;
	}
}
