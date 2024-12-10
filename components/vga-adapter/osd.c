#include <unistd.h>
#include <esp_heap_caps.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <driver/gpio.h>
#include "esp_system.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"

#include "globalvars.h"
#include "main.h"
#include "pins.h"
#include "osd.h"

char last_app_name[17];
uint8_t last_app_id;
char this_app_name[17];
uint8_t this_app_id;
char next_app_name[17];
uint8_t next_app_id;
uint8_t wps_app_id = 0;

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
	0x00, 0x7F, 0x7F, 0x00, 0x00, 0x00,  // I
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
	0x00, 0x7D, 0x7D, 0x00, 0x00, 0x00,  // i
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
	0x21, 0x75, 0x54, 0x7D, 0x79, 0x00,  // \x7e ä
	0x7F, 0x7F, 0x00, 0x00, 0x00, 0x00,
	0x0c, 0x06, 0x7f, 0x06, 0x0c, 0x00,  // \x80 pfeil hoch
	0x18, 0x30, 0x7f, 0x30, 0x18, 0x00,  // \x81 pfeil runter
	0x08, 0x08, 0x2a, 0x1c, 0x08, 0x00,  // \x82 pfeil rechts
	0x08, 0x1c, 0x2a, 0x08, 0x08, 0x00,  // \x83 pfeil links
	0x3D, 0x7D, 0x40, 0x7D, 0x7D, 0x00,  // \x84 ü
	0x39, 0x7D, 0x44, 0x7D, 0x39, 0x00,  // \x85 ö
};

// Text in die OSD-Zeile drucken
static void drawtext(char* txt, int count, int pos, int fill, bool selected)
{
	char color = selected ? 0x03 : ((pos>64 && count>1) ? 0x1a : 0x3f);
	char bkcolor = selected ? 0x25 : 0;
	for (int a=0;a<fill;a++)
	{
		for (int b=0;b<6;b++)
		{
			char c=0;
			if (a<count)
			{
				c = CHARSET[(txt[a]-32)*6+b];
			}
			for (int d=0;d<7;d++)
			{
				OSD_BUF[b+(pos+a)*7+(d+1)*ABG_XRes+1] = (((1 << d) & c) != 0) ? color : bkcolor;
			}
			OSD_BUF[(pos+a)*7+b+1] = bkcolor;
			OSD_BUF[(pos+a)*7+b+8*ABG_XRes+1] = bkcolor;
		}
		for (int d=0;d<9;d++)
		{
			OSD_BUF[(pos+a)*7+d*ABG_XRes] = bkcolor;
		}
	}
}

// Text in die OSD-Zeile drucken
static void drawhint(char* txt, int count, int pos, int fill)
{
	for (int a=0;a<fill;a++)
	{
		for (int b=0;b<6;b++)
		{
			char c=0;
			if (a<count)
			{
				c = CHARSET[(txt[a]-32)*6+b];
			}
			for (int d=0;d<7;d++)
			{
				OSD_BUF[b+(pos+a)*7+(d+11)*ABG_XRes] = (((1 << d) & c) != 0) ? 0x1a : 0x00;
			}
		}
	}
}

// Farbschema je nach Computer-Typ in Variable kopieren
static void set_colorscheme()
{
	if (Current_Color_Scheme == _COLORSCHEME_COUNT)
	{
		for (int i=0;i<4;i++)
		{
			Current_Colors[i] = Custom_Colors[i];
		}
	}
	else
	{
		for (int i=0;i<4;i++)
		{
			Current_Colors[i] = _STATIC_COLOR_VALS[Current_Color_Scheme].colors[_STATIC_SYS_VALS[ACTIVESYS].swap_colors[i]];
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

	printf("Lese Partitionstabelle...\n");

	const esp_partition_t* part = esp_ota_get_running_partition();
	this_app_id = part->subtype;
	snprintf(this_app_name, 17, part->label);
	
	esp_partition_iterator_t iter = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
	uint32_t startbytes = -1;
	uint8_t app_ids[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uint8_t app_count = 0;
	while (iter != NULL && app_count<16)
	{
		part = esp_partition_get(iter);
		esp_partition_read(part, 0, &startbytes, 4);
		if (startbytes != -1)
		{
			printf("App: %s\n",part->label);
			app_ids[app_count] = part->subtype;
			app_count++;
			if (part->label[0]=='W')
			{
				wps_app_id = part->subtype;
			}
		}
		iter = esp_partition_next(iter);
	}
	uint8_t my_num = 0;
	for (uint8_t a=0;a<app_count;a++)
	{
		if (app_ids[a] == this_app_id)
		{
			my_num = a;
		}
	}

	if (my_num == 0)
	{
		last_app_id = app_ids[app_count-1];
	}
	else
	{
		last_app_id = app_ids[my_num-1];
	}
	if (my_num == app_count-1)
	{
		next_app_id = app_ids[0];
	}
	else
	{
		next_app_id = app_ids[my_num+1];
	}
	part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, last_app_id, NULL);
	snprintf(last_app_name, 17, part->label);
	part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, next_app_id, NULL);
	snprintf(next_app_name, 17, part->label);
	printf("Apps: last=%s this=%s next=%s\n",next_app_name,this_app_name,last_app_name);

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
	if (nvs_mode<0 || nvs_mode>=_SETTINGS_COUNT) {
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
	if (nvs_get_i8(sys_nvs_handle, _NVS_SETTING_COLORSCHEMA, &Current_Color_Scheme) != ESP_OK) {
		Current_Color_Scheme =  0;
		printf("Colorschema-Einstellung nicht gefunden, nutze default = %d\n",Current_Color_Scheme);
	}
	if (Current_Color_Scheme>_COLORSCHEME_COUNT) Current_Color_Scheme=_COLORSCHEME_COUNT;
	if (Current_Color_Scheme<0) Current_Color_Scheme=0;
	if (nvs_get_u32(sys_nvs_handle, _NVS_SETTING_CUSTOMCOLORS, (uint32_t*)&Custom_Colors[0]) != ESP_OK) {
		for (int i=0;i<4;i++)
		{
			Custom_Colors[i] = _STATIC_COLOR_VALS[0].colors[i];
		}
		printf("Customcolors-Einstellung nicht gefunden, nutze default\n");
	}

	ACTIVESYS = nvs_mode;
	set_colorscheme();
	BSYNC_PIXEL_ABSTAND = (float)nvs_pixel_abstand / 100;
	ABG_START_LINE = nvs_start_line;
	ABG_PIXEL_PER_LINE = (float)nvs_pixel_per_line / 100;
	ABG_Interleave_Mask = _STATIC_SYS_VALS[nvs_mode].interleave_mask;	
	ABG_XRes = _STATIC_SYS_VALS[nvs_mode].xres;
	ABG_YRes = _STATIC_SYS_VALS[nvs_mode].yres;
	ABG_Bits_per_sample = _STATIC_SYS_VALS[nvs_mode].bits_per_sample;
	printf("Zuweisung erledigt (%d, %f, %ld, %f)\n", ACTIVESYS, BSYNC_PIXEL_ABSTAND, ABG_START_LINE, ABG_PIXEL_PER_LINE);

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
	int8_t nvs_current_colorscheme = Current_Color_Scheme;
	uint32_t* nvs_custom_colors = ((uint32_t*)&Custom_Colors[0]);
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
		err = nvs_set_i8(sys_nvs_handle, _NVS_SETTING_COLORSCHEMA, nvs_current_colorscheme);
		if (err != ESP_OK) valid_settings = false;
		err = nvs_set_u32(sys_nvs_handle, _NVS_SETTING_CUSTOMCOLORS, *nvs_custom_colors);
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

	char* tb = heap_caps_malloc(90, MALLOC_CAP_INTERNAL);
	int cursor = 1;
	for (int h=0;h<20*ABG_XRes;h++) OSD_BUF[h]=0x0;

	int j = 0;
	bool nvs_saved = false;

	while (1)
	{
		// Menü ausgeben
		int l = snprintf(tb, 40, this_app_name);
		drawtext(tb,l,0,15,cursor==0);
		l = snprintf(tb, 40, _STATIC_SYS_VALS[ACTIVESYS].name);
		drawtext(tb,l,16,7,cursor==1);
		l = snprintf(tb, 40, "PZ=%.1f",ABG_PIXEL_PER_LINE);
		drawtext(tb,l,24,9,cursor==2);
		l = snprintf(tb, 40, "PA=%.2f",BSYNC_PIXEL_ABSTAND);
		drawtext(tb,l,34,9,cursor==3);
		l = snprintf(tb, 40, "St=%ld",ABG_START_LINE);
		drawtext(tb,l,44,6,cursor==4);
		if (Current_Color_Scheme == _COLORSCHEME_COUNT)
		{
			l = snprintf(tb, 40, "FS=**");
		}
		else
		{
			l = snprintf(tb, 40, "FS=%s", _STATIC_COLOR_VALS[Current_Color_Scheme].shortname);
		}
		drawtext(tb,l,51,5,cursor==5);
		l = snprintf(tb, 40, "Speicher");
		drawtext(tb,l,57,8,cursor==6);

		// Frequenzen Ausgeben
		if (bsyn_clock_diff>0 && bsyn_clock_frame>0)
		{
			double hfreq = 48000000.0f / (double)bsyn_clock_diff; // 200 Zeilen * 240mhz = 48000000000
			l = snprintf(tb, 40, "H=%.3fkHz",hfreq);
			drawtext(tb,l,67,12,false);
			hfreq = 240000000.0f / (double)bsyn_clock_frame; // 240mhz
			l = snprintf(tb, 20, "V=%.3fHz",hfreq);
			drawtext(tb,l,79,12,false);
		}
		else
		{
			l = snprintf(tb, 40, "H=none");
			drawtext(tb,l,67,12,false);
			l = snprintf(tb, 20, "V=none");
			drawtext(tb,l,78,12,false);
		}
		
		// Hinweistext erstellen und schreiben
		switch (cursor)
		{
			case 0:
				if (this_app_id != next_app_id)
				{
					l = snprintf(tb, 90, "Betriebsart \x7endern  \x80=%s  \x81=%s  \x82=Computer-Typ %s",next_app_name,last_app_name, (wps_app_id==0) ? "" : " \x83(5s)=WPS Setup");
				}
				else
				{
					l = snprintf(tb, 90, "Betriebsart \x7endern  (keine weitere installiert!) \x82=Computer-Typ");
				}
				break;
			case 1:
				int sysplus = (ACTIVESYS<(_SETTINGS_COUNT-1)) ? ACTIVESYS+1 : 0;
				int sysminus = (ACTIVESYS==0) ? _SETTINGS_COUNT-1 : ACTIVESYS-1;
				l = snprintf(tb, 90, "Computer-Typ \x7endern  \x80=%s  \x81=%s  \x82=PZ  \x83=Betriebsart",_STATIC_SYS_VALS[sysplus].name,_STATIC_SYS_VALS[sysminus].name);
				break;
			case 2:
				l = snprintf(tb, 90, "Pixel pro Zeile (inklusive Vor,-und Nachlauf) \x7endern  \x80=+  \x81=-  \x82=PA  \x83=Computer-Typ");
				break;
			case 3:
				l = snprintf(tb, 90, "Pixel Abstand (zwischen 1. Pixel und HSync) \x7endern  \x80=+  \x81=-  \x82=St  \x83=PZ");
				break;
			case 4:
				l = snprintf(tb, 90, "Startzeile \x7endern  \x80= +  \x81= -  \x82=Farbschema  \x83=PA");
				break;
			case 5:
				int colplus = (Current_Color_Scheme<(_COLORSCHEME_COUNT-1)) ? Current_Color_Scheme+1 : 0;
				int colminus = (Current_Color_Scheme==0) ? _COLORSCHEME_COUNT-1 : Current_Color_Scheme-1;
				l = snprintf(tb, 90, "Farbschema \x7endern  \x80=%s  \x80(3s)=Manuelle Farben  \x81=%s  \x82=Speicher  \x83=St", _STATIC_COLOR_VALS[colplus].longname, _STATIC_COLOR_VALS[colminus].longname);
				break;
			case 6:
				l = snprintf(tb, 90, "Einstellungen  \x80=Speichern  \x81=Reset  \x82=Laden  \x83=Farbschema");
				break;
		}
		drawhint(tb,l,0,90);

		// darauf warten, dass alle Tasten losgelassen werden
		while (gpio_get_level(PIN_NUM_TAST_LEFT)==0 || gpio_get_level(PIN_NUM_TAST_UP)==0 || gpio_get_level(PIN_NUM_TAST_DOWN)==0 || gpio_get_level(PIN_NUM_TAST_RIGHT)==0)
		{
			usleep(10000);
			if (cursor==2 || cursor==3 || cursor==4)
			{
				if (j<=0)
				{
					j = 5;
					break;
				}
				j--;
			}
		}

		int i = (cursor==1) ? 500 : 50;
		// darauf warten, dass eine Taste gedrückt wird
		while (gpio_get_level(PIN_NUM_TAST_LEFT)!=0 && gpio_get_level(PIN_NUM_TAST_UP)!=0 && gpio_get_level(PIN_NUM_TAST_DOWN)!=0 && gpio_get_level(PIN_NUM_TAST_RIGHT)!=0)
		{
			usleep(10000);
			if (i==1)
			{
				if (cursor==1 && ABG_RUN)
				{
					for (int h=0;h<20*ABG_XRes;h++) OSD_BUF[h]=0x0;
				}
				else
				{
					break;
				}
			}
			if (i>0) i--;
			j = 50;
		}

		// cursor nach links
		if (gpio_get_level(PIN_NUM_TAST_LEFT)==0)
		{
			cursor--;
			if (cursor==-1) 
			{
				if (wps_app_id!=0)
				{
					i = 200;
					while (gpio_get_level(PIN_NUM_TAST_LEFT)==0)
					{
						if (i<100)
						{
							gpio_set_level(PIN_NUM_LED_WIFI, (i & 4) == 0);
						}
						else
						{
							gpio_set_level(PIN_NUM_LED_WIFI, (i & 8) == 0);
						}
						usleep(10000);
						if (i==0)
						{
							const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, wps_app_id, NULL);
							if (part!=NULL)
							{
								nvs_set_u8(sys_nvs_handle, _NVS_SETTING_WPS_MODE, 1);
								gpio_set_level(PIN_NUM_LED_WIFI, 0);
								esp_ota_set_boot_partition(part);
								esp_restart();
							}
						}
						i--;
					}
					gpio_set_level(PIN_NUM_LED_WIFI, 0);
				}
				cursor=0;
			}
		}

		// cursor nach rechts
		if (gpio_get_level(PIN_NUM_TAST_RIGHT)==0)
		{
			cursor++;
			if (cursor==7) {
				nvs_saved = restore_settings();
				cursor = 1;
			}
		}

		// wert erhöhen
		if (gpio_get_level(PIN_NUM_TAST_UP)==0)
		{
			switch (cursor)
			{
				case 0:
					if (next_app_id != this_app_id)
					{
						const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, next_app_id, NULL);
						if (part!=NULL)
						{
							esp_ota_set_boot_partition(part);
							esp_restart();
						}
					}
					break;
				case 1:
					ACTIVESYS++;
					if (ACTIVESYS >= (_SETTINGS_COUNT))
					{
						ACTIVESYS = 0;
					}
					nvs_set_i16(sys_nvs_handle, _NVS_SETTING_MODE, ACTIVESYS);
					esp_restart();
					break;
				case 2:
					ABG_PIXEL_PER_LINE+= (j==5) ? 1.0f : 0.1f;
					nvs_saved = false;
					break;
				case 3:
					BSYNC_PIXEL_ABSTAND+= (j==5) ? 0.2f : 0.02f;
					nvs_saved = false;
					break;
				case 4:
					ABG_START_LINE++;
					nvs_saved = false;
					break;
				case 5:
					Current_Color_Scheme++;
					if (Current_Color_Scheme>=_COLORSCHEME_COUNT) Current_Color_Scheme = 0;
					set_colorscheme();
					nvs_saved = false;
					i = 80;
					while (gpio_get_level(PIN_NUM_TAST_UP)==0)
					{
						usleep(10000);
						if (i==0)
						{
							Current_Color_Scheme = _COLORSCHEME_COUNT;
							set_colorscheme();
							l = snprintf(tb, 40, "FS=** 0=000 1=000 2=000 3=000");
							drawtext(tb,l,51,40,false);
							l = snprintf(tb, 90, "Manuelle Farben \x7endern \x80=+ \x81=- \x83\x82=R/G/B \x83\x82(3s)=zur\x84" "ck zu Farbschema");
							drawhint(tb,l,0,90);
							cursor = 0;
							while (1)
							{
								for (int k=0;k<4;k++)
								{
									for (int j=0;j<3;j++)
									{
										char c = (Custom_Colors[k] >> ((2-j)*2) & 3) + '0';
										drawtext(&c,1,59+j+k*6,1,cursor==(k*3+j));
									}
								}
								i = 80;
								while ((gpio_get_level(PIN_NUM_TAST_LEFT)==0 || gpio_get_level(PIN_NUM_TAST_RIGHT)==0) && i>0)
								{
									usleep(10000);
									i--;
								}
								if (i<=0)
								{
									cursor = 5;
								 	break;
								}
								while (gpio_get_level(PIN_NUM_TAST_UP)==0 || gpio_get_level(PIN_NUM_TAST_DOWN)==0)
								{
									usleep(10000);
								}
								while (gpio_get_level(PIN_NUM_TAST_LEFT)!=0 && gpio_get_level(PIN_NUM_TAST_UP)!=0 && gpio_get_level(PIN_NUM_TAST_DOWN)!=0 && gpio_get_level(PIN_NUM_TAST_RIGHT)!=0)
								{
									usleep(10000);
								}
								if (gpio_get_level(PIN_NUM_TAST_LEFT)==0)
								{
									cursor--;
									if (cursor<0) cursor=11;
								}
								if (gpio_get_level(PIN_NUM_TAST_RIGHT)==0)
								{
									cursor++;
									if (cursor>11) cursor=0;
								}
								int k = cursor / 3;
								int j = (2-(cursor % 3)) * 2;
								if (gpio_get_level(PIN_NUM_TAST_UP)==0)
								{
									Custom_Colors[k] = (Custom_Colors[k] & (~(3<<j))) | ((((Custom_Colors[k]>>j)+1) & 3)<<j);
									set_colorscheme();
								}
								if (gpio_get_level(PIN_NUM_TAST_DOWN)==0)
								{
									Custom_Colors[k] = (Custom_Colors[k] & (~(3<<j))) | ((((Custom_Colors[k]>>j)-1) & 3)<<j);
									set_colorscheme();
								}
							}
							break;
						}
						i--;
					}
					l = snprintf(tb, 90, "  ");
					drawtext(tb,l,65,2,false);
					break;
				case 6:
					if (!nvs_saved) // Sicherung gegen unnötiges schreiben
						nvs_saved = write_settings(true);
					cursor=1;
					break;

			}
		}

		// wert senken
		if (gpio_get_level(PIN_NUM_TAST_DOWN)==0)
		{
			switch (cursor)
			{
				case 0:
					if (last_app_id != this_app_id)
					{
						const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, last_app_id, NULL);
						if (part!=NULL)
						{
							esp_ota_set_boot_partition(part);
							esp_restart();
						}
					}
					break;
				case 1:
					ACTIVESYS--;
					if (ACTIVESYS > 100)
					{
						ACTIVESYS = _SETTINGS_COUNT-1;
					}
					nvs_set_i16(sys_nvs_handle, _NVS_SETTING_MODE, ACTIVESYS);
					esp_restart();
					break;
				case 2:
					ABG_PIXEL_PER_LINE-= (j==5) ? 1.0f : 0.1f;
					nvs_saved = false;
					break;
				case 3:
					BSYNC_PIXEL_ABSTAND-= (j==5) ? 0.20f : 0.02f;
					nvs_saved = false;
					break;
				case 4:
					ABG_START_LINE--;
					nvs_saved = false;
					break;
				case 5:
					Current_Color_Scheme--;
					if (Current_Color_Scheme<0) Current_Color_Scheme = _COLORSCHEME_COUNT-1;
					set_colorscheme();
					nvs_saved = false;
					break;
				case 6:
					// default Einstellungen setzen
					BSYNC_PIXEL_ABSTAND = (float)_STATIC_SYS_VALS[ACTIVESYS].default_pixel_abstand / 100;
					ABG_START_LINE = _STATIC_SYS_VALS[ACTIVESYS].default_start_line;
					ABG_PIXEL_PER_LINE = (float)_STATIC_SYS_VALS[ACTIVESYS].default_pixel_per_line / 100;
					cursor=1;
					break;
			}
		}
	}
}
