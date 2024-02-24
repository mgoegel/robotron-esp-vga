#include <stdio.h>
#include <unistd.h>
#include "driver/gpio.h"
#include <driver/spi_master.h>
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include <soc/gdma_reg.h>
#include <soc/spi_reg.h>
#include "nvs_flash.h"
#include "nvs.h"
#include <math.h>
#include "esp_intr_alloc.h"
#include <rom/ets_sys.h>
#include <soc/periph_defs.h>


#define DEBUG 1

#include "pins.h"

// Computer Typen
#define A7100 1
#define PC1715 2

#define ZIELTYP A7100

// Deklaration der Modi

// Statische Struktur - Systemkonstanten
struct SYSSTATIC {
	char* name;
	uint8_t colors[4];
};

// Statische Werte vorinitialisiert
const struct SYSSTATIC _STATIC_SYS_VALS[] = {
	{ 
		.name = "A7100 ",
		.colors = {0, 0b00000100, 0b00001000, 0b00001100},
	},
	{ 
		.name = "PC1715",
		.colors = {0b00001000, 0b00000100, 0, 0b00001100}, // 0b--rrggbb
	}
};

// Modusstruktur - Systemvariablen, änderbar durch den Anwender
struct SYSVARS {
	uint16_t mode;
	float pixel_abstand;
	uint32_t start_line;
	float pixel_per_line;
};

// Initialisierte Daten aus dem NVS - Werte, die durch den Anwender geändert werden können
// Standardwerte für Modusinitialisierung nach Umschaltung des Modus
const struct SYSVARS _DEFAULT_SYS_VARS[] = {
	{.mode = 0, .pixel_abstand = 89.86f, .start_line = 29, .pixel_per_line = 736}, // A7100
	{.mode = 1, .pixel_abstand = 155.67f /* schwankt bis auf 155.88 */, .start_line = 6, .pixel_per_line = 864.1} // PC1715
};

// Bezeichner für NVS KEY - max 15 Zeichen!
#define _NVS_SETTING_MODE	"SMODE"
#define _NVS_SETTING_PIXEL_ABSTAND "SPIXABST"
#define _NVS_SETTING_START_LINE	"SSTARTLINE"
#define _NVS_SETTING_PIXEL_PER_LINE "SPIXPERLINE"

// globale Variablen

// Aktives System
static uint16_t ACTIVESYS = ZIELTYP-1;

nvs_handle_t sys_nvs_handle;

volatile uint32_t* ABG_DMALIST;
volatile uint32_t ABG_Scan_Line = 0;
volatile double ABG_PIXEL_PER_LINE;
volatile double BSYNC_PIXEL_ABSTAND;
volatile uint32_t ABG_START_LINE;
volatile bool ABG_RUN = false;

int BSYNC_SUCHE_START = 0;
uint8_t* PIXEL_STEP_LIST;
uint8_t* VGA_BUF;
uint8_t* OSD_BUF;
volatile uint32_t bsyn_clock_diff = 0;
volatile uint32_t bsyn_clock_last = 0;
volatile uint32_t BSYNC_SAMPLE_ABSTAND = 0;

#include "capture.c"
#include "osd.c"
#include "vga.c"


// Hauptprogramm
void IRAM_ATTR app_main(void)
{
	setup_vga();
	setup_flash();
	// Standardwerte initialisieren
	switch_system();
	// NVS Einstellungen laden, wenn vorhanden
	restore_settings();

	xTaskCreatePinnedToCore(osd_task,"osd_task",6000,NULL,0,NULL,0);
	xTaskCreatePinnedToCore(capture_task,"capture_task",6000,NULL,10,NULL,1);
}

