
#include <limits.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "globalvars.h"

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

// Initialisierte Daten aus dem NVS - Werte, die durch den Anwender geändert werden können
// Standardwerte für Modusinitialisierung nach Umschaltung des Modus
const struct SYSVARS _DEFAULT_SYS_VARS[] = {
	{.mode = 0, .pixel_abstand = 90.10f, .start_line = 29, .pixel_per_line = 736}, // A7100
	{.mode = 1, .pixel_abstand = 155.67f /* schwankt bis auf 155.88 */, .start_line = 6, .pixel_per_line = 864.1} // PC1715
};

// globale Variablen

// Aktives System
uint16_t ACTIVESYS = ZIELTYP-1;
nvs_handle_t sys_nvs_handle;
volatile uint32_t* ABG_DMALIST;
volatile uint32_t ABG_Scan_Line = 0;
volatile double ABG_PIXEL_PER_LINE;
volatile double BSYNC_PIXEL_ABSTAND;
volatile uint32_t ABG_START_LINE;
volatile bool ABG_RUN = false;
uint32_t ABG_Interleave = 0;

int BSYNC_SUCHE_START = 0;
uint8_t* PIXEL_STEP_LIST;
uint8_t* VGA_BUF;
uint8_t* OSD_BUF;
volatile uint32_t bsyn_clock_diff = 0;
volatile uint32_t bsyn_clock_last = 0;
volatile uint32_t BSYNC_SAMPLE_ABSTAND = 0;

