#include <unistd.h>
#include <esp_heap_caps.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <driver/gpio.h>
#include "esp_system.h"

#include "globalvars.h"
#include "main.h"
#include "pins.h"
#include "osd.h"

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
	0x7F, 0x7F, 0x00, 0x00, 0x00, 0x00,
	0x0c, 0x06, 0x7f, 0x06, 0x0c, 0x00,  // \x80 pfeil hoch
	0x18, 0x30, 0x7f, 0x30, 0x18, 0x00,  // \x81 pfeil runter
	0x08, 0x08, 0x2a, 0x1c, 0x08, 0x00,  // \x82 pfeil rechts
	0x08, 0x1c, 0x2a, 0x08, 0x08, 0x00   // \x83 pfeil links
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
				OSD_BUF[b+(pos+a)*7+d*ABG_XRes] = (((1 << d) & c) != 0) ? color : 0x00;
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

	printf("\n");
    printf("Öffne NVS handle... ");

	err = nvs_open("storage", NVS_READWRITE, &sys_nvs_handle);
    if (err != ESP_OK) {
        printf("Fehler (%s) beim öffnen des NVS handle!\n", esp_err_to_name(err));
    } else {
		 printf("Erledigt\n");
	}
}

// Einstellungen vom Flash laden
bool restore_settings() {
	if (sys_nvs_handle == 0)
		return false;

	// Read
	printf("Lese Einstellungen vom Flash ...\n");

	int16_t nvs_mode = -1;
	uint32_t nvs_pixel_abstand = 0;
	uint32_t nvs_start_line = 0;
	uint32_t nvs_pixel_per_line = 0;

	char tb[40];
    
	if (nvs_get_i16(sys_nvs_handle, _NVS_SETTING_MODE, &nvs_mode) != ESP_OK) {
		printf("Modus-Einstellung nicht gefunden, nutze default = A7100\n");
		nvs_mode = 0;
	}
	if (nvs_mode<0 || nvs_mode>_SETTINGS_COUNT) {
		printf("Modus-Einstellung ungültig (%d), nutze default = A7100\n",nvs_mode);
		nvs_mode = 0;
	}

	snprintf(tb, 40, _NVS_SETTING_PIXEL_ABSTAND, nvs_mode);
	if (nvs_get_u32(sys_nvs_handle, tb, &nvs_pixel_abstand) != ESP_OK) {
		nvs_pixel_abstand = _STATIC_SYS_VALS[nvs_mode].default_pixel_abstand;
		printf("Pixelabstand-Einstellung nicht gefunden, nutze default = %ld\n",nvs_pixel_abstand);
	}
	snprintf(tb, 40, _NVS_SETTING_START_LINE, nvs_mode);
	if (nvs_get_u32(sys_nvs_handle, tb, &nvs_start_line) != ESP_OK) {
		nvs_start_line =  _STATIC_SYS_VALS[nvs_mode].default_start_line;
		printf("Startline-Einstellung nicht gefunden, nutze default = %ld\n",nvs_start_line);
	}
	snprintf(tb, 40, _NVS_SETTING_PIXEL_PER_LINE, nvs_mode);
	if (nvs_get_u32(sys_nvs_handle, tb, &nvs_pixel_per_line) != ESP_OK) {
		nvs_pixel_per_line =  _STATIC_SYS_VALS[nvs_mode].default_pixel_per_line;
		printf("Pixelperline-Einstellung nicht gefunden, nutze default = %ld\n",nvs_pixel_per_line);
	}

	ACTIVESYS = nvs_mode;
	BSYNC_PIXEL_ABSTAND = (float)nvs_pixel_abstand / 100;
	ABG_START_LINE = nvs_start_line;
	ABG_PIXEL_PER_LINE = (float)nvs_pixel_per_line / 100;
	ABG_Interleave_Mask = _STATIC_SYS_VALS[nvs_mode].interleave_mask;
	ABG_XRes = _STATIC_SYS_VALS[nvs_mode].xres;
	ABG_YRes = _STATIC_SYS_VALS[nvs_mode].yres;
	ABG_Bits_per_sample = _STATIC_SYS_VALS[nvs_mode].bits_per_sample;
	printf("Zuweisung erledigt (%d, %f, %ld, %f)\n", ACTIVESYS, BSYNC_PIXEL_ABSTAND, ABG_START_LINE, ABG_PIXEL_PER_LINE);
	DEBUG_MODE = _STATIC_SYS_VALS[nvs_mode].is_debugger;
	return true;
}

// Einstellungen im Flash sichern
bool write_settings(bool full) {
	if (sys_nvs_handle == 0)
		return false;

	uint16_t nvs_mode = ACTIVESYS;
	uint32_t nvs_pixel_abstand = (int)(BSYNC_PIXEL_ABSTAND * 100);
	uint32_t nvs_start_line = ABG_START_LINE;
	uint32_t nvs_pixel_per_line = (int)(ABG_PIXEL_PER_LINE * 100);
	char tb[40];

	esp_err_t err;
	bool valid_settings = true;
	err = nvs_set_i16(sys_nvs_handle, _NVS_SETTING_MODE, nvs_mode);
	if (err != ESP_OK) valid_settings = false;
	if (full) {
		snprintf(tb, 40, _NVS_SETTING_PIXEL_ABSTAND, nvs_mode);
		err = nvs_set_u32(sys_nvs_handle, tb, nvs_pixel_abstand);
		if (err != ESP_OK) valid_settings = false;
		snprintf(tb, 40, _NVS_SETTING_START_LINE, nvs_mode);
		err = nvs_set_u32(sys_nvs_handle, tb, nvs_start_line);
		if (err != ESP_OK) valid_settings = false;
		snprintf(tb, 40, _NVS_SETTING_PIXEL_PER_LINE, nvs_mode);
		err = nvs_set_u32(sys_nvs_handle, tb, nvs_pixel_per_line);
		if (err != ESP_OK) valid_settings = false;
	}

	if (!valid_settings) {
		printf("Einstellungen nicht gespeichert.");
		return false;
	}
	printf("Einstellungen gespeichert.");
	return true;
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

	char* tb = heap_caps_malloc(60, MALLOC_CAP_INTERNAL);
	int cursor = 0;
	for (int h=0;h<20*640;h++) OSD_BUF[h]=0x0;

	int j = 0;
	bool nvs_saved = false;

	while (1)
	{
		// Menü ausgeben
		int l = snprintf(tb, 40, _STATIC_SYS_VALS[ACTIVESYS].name);
		drawtext(tb,l,1,cursor==0 ? 0x03 : 0x3f);
		if (cursor < 4) {
			l = snprintf(tb, 40, "Pixel pro Zeile=%.1f    ",ABG_PIXEL_PER_LINE);
			drawtext(tb,l,9,cursor==1 ? 0x03 : 0x3f);
			l = snprintf(tb, 40, "Pixel Abstand=%.2f    ",BSYNC_PIXEL_ABSTAND);
			drawtext(tb,l,32,cursor==2 ? 0x03 : 0x3f);
			l = snprintf(tb, 40, "Startzeile=%ld    ",ABG_START_LINE);
			drawtext(tb,l,57,cursor==3 ? 0x03 : 0x3f);
		} else if (cursor > 3)
		{
			l = snprintf(tb, 60, "Einstellungen sichern \x80 / default \x81 / laden \x82              ");
			drawtext(tb,l,9,cursor==4 ? 0x03 : 0x3f);
		}
		

		// darauf warten, dass alle Tasten losgelassen werden
		while (gpio_get_level(PIN_NUM_TAST_LEFT)==0 || gpio_get_level(PIN_NUM_TAST_UP)==0 || gpio_get_level(PIN_NUM_TAST_DOWN)==0 || gpio_get_level(PIN_NUM_TAST_RIGHT)==0)
		{
			usleep(10000);
			if (j<=0)
			{
				j = 5;
				break;
			}
			j--;
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
			if (i>0 && ABG_RUN && !DEBUG_MODE) i--;
			j = 50;
		}

		// cursor nach links
		if (gpio_get_level(PIN_NUM_TAST_LEFT)==0)
		{
			cursor--;
			if (cursor==-1) cursor=4;
		}

		// cursor nach rechts
		if (gpio_get_level(PIN_NUM_TAST_RIGHT)==0)
		{
			cursor++;
			if (cursor==5) {
				nvs_saved = restore_settings();
				cursor = 0;
			}
		}

		// wert erhöhen
		if (gpio_get_level(PIN_NUM_TAST_UP)==0)
		{
			switch (cursor)
			{
				case 0:
					if (ACTIVESYS < (_SETTINGS_COUNT-1))
					{
						ACTIVESYS++;
						write_settings(false);
						esp_restart();
					}
					break;
				case 1:
					ABG_PIXEL_PER_LINE+= (j==5) ? 1.0f : 0.1f;
					nvs_saved = false;
					break;
				case 2:
					BSYNC_PIXEL_ABSTAND+= (j==5) ? 0.2f : 0.02f;
					nvs_saved = false;
					break;
				case 3:
					ABG_START_LINE++;
					nvs_saved = false;
					break;
				case 4:
					if (!nvs_saved) // Sicherung gegen unnötiges schreiben
						nvs_saved = write_settings(true);
					cursor=0;
					break;

			}
		}

		// wert senken
		if (gpio_get_level(PIN_NUM_TAST_DOWN)==0)
		{
			switch (cursor)
			{
				case 0:
					if (ACTIVESYS > 0)
					{
						ACTIVESYS--;
						write_settings(false); // nur aktives System sichern, keine Parameter
						esp_restart();
					}
					break;
				case 1:
					ABG_PIXEL_PER_LINE-= (j==5) ? 1.0f : 0.1f;
					nvs_saved = false;
					break;
				case 2:
					BSYNC_PIXEL_ABSTAND-= (j==5) ? 0.20f : 0.02f;
					nvs_saved = false;
					break;
				case 3:
					ABG_START_LINE--;
					nvs_saved = false;
					break;
				case 4:
					// default Einstellungen setzen
					BSYNC_PIXEL_ABSTAND = (float)_STATIC_SYS_VALS[ACTIVESYS].default_pixel_abstand / 100;
					ABG_START_LINE = _STATIC_SYS_VALS[ACTIVESYS].default_start_line;
					ABG_PIXEL_PER_LINE = (float)_STATIC_SYS_VALS[ACTIVESYS].default_pixel_per_line / 100;
					cursor=0;
					break;
			}
		}
	}
}

// Text in den VGA-Buffer drucken
static void vgadraw(char* txt, int count, int xpos, int ypos, char color)
{
	for (int a=0;a<count;a++)
	{
		for (int b=0;b<6;b++)
		{
			char c=CHARSET[(txt[a]-32)*6+b];
			for (int d=0;d<7;d++)
			{
				VGA_BUF[b+(xpos+a)*7+(d+ypos*10)*ABG_XRes] = (((1 << d) & c) != 0) ? color : 0x00;
			}
		}
	}
}

// Speicher für Debugger zuweisen und festen Teil des Debugger-Screens zeichnen
void osd_setup_debugger()
{
	DEBUG_SCAN_BUF = heap_caps_malloc(4096 * 16, MALLOC_CAP_32BIT | MALLOC_CAP_INTERNAL);
	DEBUG_HISTORY_BUF = heap_caps_malloc(4096*2*2*16, MALLOC_CAP_SPIRAM);

	if (DEBUG_SCAN_BUF==0 || DEBUG_HISTORY_BUF==0)
	{
		DEBUG_MODE = false;
		printf("Speicherzuordnung für Pixel-Debugger fehlgeschlagen!\n");
		return;
	}

	for (int b=0;b<ABG_XRes*ABG_YRes;b++)  // VGA-Puffer leeren
	{
		VGA_BUF[b]=0;
	}
	vgadraw("Pixel-Debugger",14,0,0,0x3f);
	for (int a=0;a<100;a++)
	{
		VGA_BUF[a+10*ABG_XRes]=0x3f;
	}
	vgadraw("HSync-Frequenz:",15,0,2,0x3c);
	vgadraw("VSync-Frequenz:",15,0,4,0x3c);
	vgadraw("Samples:",8,0,6,0x3c);
	vgadraw("Feste Pixel  :",14,40,2,0x3c);
	vgadraw("Flacker Pixel:",14,40,4,0x3c);
}

const int FRAMECOLOR = 0b000001;
const int ERRORCOLOR = 0b110000;
const int FLICKERCOLOR = 0b111010;
const int STABLECOLOR = 0b001000;
const int HITCOLOR = 0b001100;

// dynamischen Teil des Debugger-Screens zeichnen
void osd_draw_debugger()
{
	// Frequenzen schreiben
	double hfreq = 48000000.0f / (double)bsyn_clock_diff; // 200 Zeilen * 240mhz = 48000000000
	char tb[20];
	int l = snprintf(tb, 20, "%.4f kHz    ",hfreq);
	vgadraw(tb,l,16,2,0x3f);
	hfreq = 240000000.0f / (double)bsyn_clock_frame; // 240mhz
	l = snprintf(tb, 20, "%.4f Hz    ",hfreq);
	vgadraw(tb,l,16,4,0x3f);

	uint32_t hitcount = 0;		// Zähler feste Pixel
	uint32_t flickercount = 0;	// Zähler flacker Pixel

	// 16 Zeilen
	for (uint8_t a=0;a<16;a++)
	{
		uint32_t b = (DEBUG_SCAN_BUF[a*4096] + (DEBUG_SCAN_BUF[a*4096+1]<<8)) - BSYNC_SAMPLE_ABSTAND; 		// nächster Pixel-Sample
		uint32_t c = 0; 		// Pixel-Nummer
		uint32_t d = b - 50; 	// aktueller Sample
		uint32_t e = 0; 		// Sample, relativ zum Start
		uint32_t h = 0; 		// VGA x
		uint32_t i = 0; 		// VGA y

		if (b>2*4096) return;
		
		while (d<b+50)
		{
			// Sample auspacken
			uint8_t f = (d & 1)==0 ? DEBUG_SCAN_BUF[a*4096+(d>>1)]>>5 : DEBUG_SCAN_BUF[a*4096+(d>>1)]>>1;
			f = f ^ ((DEBUG_SCAN_BUF[10]>>1) & 3);              // für den PC1715, der hat invertierte Color-Signale

			// Sample-Statistik holen
			uint8_t g1 = DEBUG_HISTORY_BUF[a*4096*2*2+(e*2)];	
			uint8_t g2 = DEBUG_HISTORY_BUF[a*4096*2*2+(e*2)+1];

			// Sample-Statistik bearbeiten
			if ((f&1) == 0)										// Sample col1 aus?
			{
				if (g1>0) g1--;									// Statistik senken
				if (g1>0x80) g1=0x80;							// damit kurze 0-Impulse die Statistik resetten
			}
			else												// Sample col1 ist an!
			{
				if (g1<0xff) g1++;								// Statistik erhöhen
				if (g1<0x80) g1=0x80;							// damit kurze 1-Impulse die Statistik resetten
			}
			if ((f&2) == 0)										// und das selbe nochmal für col2
			{
				if (g2>0) g2--;
				if (g2>0x80) g2=0x80;
			}
			else
			{
				if (g2<0xff) g2++;
				if (g2<0x80) g2=0x80;
			}
			
			// Statistik speichern
			DEBUG_HISTORY_BUF[a*4096*2*2+(e*2)] = g1;			
			DEBUG_HISTORY_BUF[a*4096*2*2+(e*2)+1] = g2;

			// Sample Farbe für VGA anhand der Statistik setzen
			uint8_t j = (d==b) ? FRAMECOLOR : 0;				// Hintergrund
			if (g1 == 0xff)										// stabiles 1-Sample
			{
				if ((d==b))
				{
					j = HITCOLOR;
					hitcount++;
				}
				else
				{
			 		j = STABLECOLOR; 
				}
			}
			else if (g1 != 0x00)								// flackerndes Sample
			{
				if ((d==b))
				{
					j = ERRORCOLOR;
					flickercount++;
				}
				else
				{
			 		j = FLICKERCOLOR; 
				}
			}
			if (g2 == 0xff)										// das selbe nochmal mit col2
			{
				if ((d==b))
				{
					j = HITCOLOR;
					hitcount++;
				}
				else
				{
			 		j = STABLECOLOR; 
				}
			}
			else if (g2 != 0x00)
			{
				if ((d==b))
				{
					j = ERRORCOLOR;
					flickercount++;
				}
				else
				{
			 		j = FLICKERCOLOR; 
				}
			}

			// Sample in den VGA-Buffer zeichnen
			VGA_BUF[(i+75+a)*ABG_XRes+h] = j;					
			
			// Rahmen zeichnen
			if (a==0)
			{
				j = (d==b) ? FRAMECOLOR : 0;						
				VGA_BUF[(i+72)*ABG_XRes+h] = FRAMECOLOR;
				VGA_BUF[(i+73)*ABG_XRes+h] = j;
				VGA_BUF[(i+74)*ABG_XRes+h] = j;
				VGA_BUF[(i+91)*ABG_XRes+h] = j;
				VGA_BUF[(i+92)*ABG_XRes+h] = j;
				VGA_BUF[(i+93)*ABG_XRes+h] = FRAMECOLOR;
			}

			// Am Ende der VGA-Zeile zum nächsten Block springen
			h++;
			if (h>=ABG_XRes)
			{
				h = 0;
				i+=25;
			}

			// Sample-Pixel zählen
			if (d==b)
			{
				if (c<ABG_XRes)
				{
					b += PIXEL_STEP_LIST[c];
				}
				c++;
			}
			e++;
			d++;
		}
	}

	// Treffer-Statistik ausgeben
	l = snprintf(tb, 20, "%ld       ",hitcount);
	vgadraw(tb,l,55,2,0x3f);
	l = snprintf(tb, 20, "%ld       ",flickercount);
	vgadraw(tb,l,55,4,0x3f);
}

// Reaktion auf BSYN Timeout auf Debugger-Screen zeichnen
void osd_timeout_debugger()
{
	vgadraw("Timeout!  ",10,16,2,0x30);
	vgadraw("Timeout!  ",10,16,4,0x30);
	for (int a=72*ABG_XRes;a<250*ABG_XRes;a++)
	{
		VGA_BUF[a] = 0;
	}
	vgadraw("          ",10,55,2,0x30);
	vgadraw("          ",10,55,4,0x30);
}
