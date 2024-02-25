#pragma once

#include <limits.h>
#include "nvs_flash.h"
#include "nvs.h"

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
extern const struct SYSSTATIC _STATIC_SYS_VALS[];

// Modusstruktur - Systemvariablen, änderbar durch den Anwender
struct SYSVARS {
	uint16_t mode;
	float pixel_abstand;
	uint32_t start_line;
	float pixel_per_line;
};

// Initialisierte Daten aus dem NVS - Werte, die durch den Anwender geändert werden können
// Standardwerte für Modusinitialisierung nach Umschaltung des Modus
extern const struct SYSVARS _DEFAULT_SYS_VARS[];

// Bezeichner für NVS KEY - max 15 Zeichen!
#define _NVS_SETTING_MODE	"SMODE"
#define _NVS_SETTING_PIXEL_ABSTAND "SPIXABST"
#define _NVS_SETTING_START_LINE	"SSTARTLINE"
#define _NVS_SETTING_PIXEL_PER_LINE "SPIXPERLINE"

// globale Variablen

// Aktives System
extern uint16_t ACTIVESYS;

extern nvs_handle_t sys_nvs_handle;

extern volatile uint32_t* ABG_DMALIST;
extern volatile uint32_t ABG_Scan_Line;
extern volatile double ABG_PIXEL_PER_LINE;
extern volatile double BSYNC_PIXEL_ABSTAND;
extern volatile uint32_t ABG_START_LINE;
extern volatile bool ABG_RUN;

extern int BSYNC_SUCHE_START;
extern uint8_t* PIXEL_STEP_LIST;
extern uint8_t* VGA_BUF;
extern uint8_t* OSD_BUF;
extern volatile uint32_t bsyn_clock_diff;
extern volatile uint32_t bsyn_clock_last;
extern volatile uint32_t BSYNC_SAMPLE_ABSTAND;

