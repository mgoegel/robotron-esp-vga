
#include <limits.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "globalvars.h"

// Statische Werte vorinitialisiert
const struct SYSSTATIC _STATIC_SYS_VALS[] = {
	{ 
		.name = "A7100 ",
		.swap_colors = {0,1,2,3},
		.bits_per_sample = 8,
		.xres = 640,
		.yres = 400,
		.interleave_mask = 0,
		.default_pixel_abstand = 8950,
		.default_start_line = 32,
		.default_pixel_per_line = 73600,
	},
	{ 
		.name = "PC1715",
		.swap_colors = {2,1,0,3},
		.bits_per_sample = 4,
		.xres = 640,
		.yres = 299, /* 24 Zeilen (288 + Statuszeile) 299 im echten Leben*/
		.interleave_mask = 0,
		.default_pixel_abstand = 15532 /* schwankt bis auf 15588 */,
		.default_start_line = 24,
		.default_pixel_per_line = 86410,
	},
	{ 
		.name = "EC1834",
		.swap_colors = {0,1,2,3},
		.bits_per_sample = 8,
		.xres = 720,
		.yres = 350,
		.interleave_mask = 0,
		.default_pixel_abstand = 10717,
		.default_start_line = 18,
		.default_pixel_per_line = 86400,
	},
	{ 
		.name = "K7024",
		.swap_colors = {0,2,0,3},
		.bits_per_sample = 4,
		.xres = 640,
		.yres = 300,
		.interleave_mask = 0,
		.default_pixel_abstand = 15996,
		.default_start_line = 24,
		.default_pixel_per_line = 87200,
	},
	{ 
		.name = "VIDEO3",
		.swap_colors = {0,0,0,3},
		.bits_per_sample = 4,
		.xres = 640,
		.yres = 300,
		.interleave_mask = 0,
		.default_pixel_abstand = 20640,
		.default_start_line = 51,
		.default_pixel_per_line = 89580,
	},
};

const struct COLORSTATIC _STATIC_COLOR_VALS[] = {
	{
		.shortname = "GN",
		.longname = "Gr\x84n",
		.colors = {0, 0b00000100, 0b00001000, 0b00001100}, // 0b--rrggbb
	},
	{
		.shortname = "WS",
		.longname = "Weiss",
		.colors = {0, 0b00010101, 0b00101010, 0b00111111}, // 0b--rrggbb
	},
	{
		.shortname = "OR",
		.longname = "Orange",
		.colors = {0, 0b00010000, 0b00100100, 0b00111000}, // 0b--rrggbb
	}
};

// globale Variablen

// Aktives System
uint16_t ACTIVESYS = 0;
nvs_handle_t sys_nvs_handle;
volatile uint32_t* ABG_DMALIST;
volatile uint32_t ABG_Scan_Line = 0;
volatile double ABG_PIXEL_PER_LINE;
volatile double BSYNC_PIXEL_ABSTAND;
volatile uint32_t ABG_START_LINE;
volatile bool ABG_RUN = false;
uint32_t ABG_Interleave_Mask = 0;
uint32_t ABG_Interleave = 0;
uint16_t ABG_XRes = 0;
uint16_t ABG_YRes = 0;
uint8_t ABG_Bits_per_sample = 0;

uint8_t* PIXEL_STEP_LIST;
uint8_t* VGA_BUF;
uint8_t* OSD_BUF;
volatile uint32_t bsyn_clock_diff = 0;
volatile uint32_t bsyn_clock_last = 0;
volatile uint32_t bsyn_clock_frame = 0;
volatile uint32_t BSYNC_SAMPLE_ABSTAND = 0;
int8_t Current_Color_Scheme = 0;
uint8_t Current_Colors[4];
uint8_t Custom_Colors[4];


