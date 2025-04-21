#include "esp_ota_ops.h"

#include "globalvars.h"
#include "osd.h"

// NVS Partition initialisieren und Daten vom Flash laden, wenn vorhanden
void setup_flash() 
{
	// Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
	{
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

	printf("\n");
    printf("Öffne NVS handle... ");

	err = nvs_open("storage", NVS_READWRITE, &sys_nvs_handle);
    if (err != ESP_OK) 
	{
        printf("Fehler (%s) beim öffnen des NVS handle!\n", esp_err_to_name(err));
    } 
	else 
	{
		 printf("Erledigt\n");
	}
}

// Einstellungen vom Flash laden
bool restore_settings() 
{
	if (sys_nvs_handle == 0)
		return false;

	wlan_state = heap_caps_malloc(64, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);
	wlan_ssid = heap_caps_malloc(64, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);
	ap_ssid = heap_caps_malloc(64, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);
	wlan_passwd = heap_caps_malloc(64, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);

	printf("Lese Partitionstabelle...\n");

	const esp_partition_t* part = esp_ota_get_running_partition();
	this_app_id = part->subtype;
	
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

	if (my_num == app_count-1)
	{
		next_app_id = app_ids[0];
	}
	else
	{
		next_app_id = app_ids[my_num+1];
	}

	// Read
	printf("Lese Einstellungen vom Flash ...\n");

	uint32_t nvs_pixel_abstand = 0;
	uint32_t nvs_abg_start_line = 0;
	uint32_t nvs_pixel_per_line = 0;

	char tb[40];
    
	if (nvs_get_u16(sys_nvs_handle, _NVS_SETTING_MODE, &ACTIVESYS) != ESP_OK) 
	{
		printf("Modus-Einstellung nicht gefunden, nutze default = A7100\n");
		ACTIVESYS = 0;
	}
	if (ACTIVESYS>=_SETTINGS_COUNT) 
	{
		printf("Modus-Einstellung ungültig (%d), nutze default = A7100\n",ACTIVESYS);
		ACTIVESYS = 0;
	}
	snprintf(tb, 40, _NVS_SETTING_VGAMODE, ACTIVESYS);
	if (nvs_get_u8(sys_nvs_handle, tb, &ACTIVEVGA) != ESP_OK) 
	{
		printf("VGA-Einstellung nicht gefunden, nutze default = %d\n",_STATIC_SYS_VALS[ACTIVESYS].default_vga_mode);
		ACTIVEVGA = _STATIC_SYS_VALS[ACTIVESYS].default_vga_mode;
	}
	if (ACTIVEVGA>=_VGAMODE_COUNT) 
	{
		printf("VGA-Einstellung ungültig (%d), nutze default = %d\n",ACTIVEVGA,_STATIC_SYS_VALS[ACTIVESYS].default_vga_mode);
		ACTIVEVGA = _STATIC_SYS_VALS[ACTIVESYS].default_vga_mode;
	}
	snprintf(tb, 40, _NVS_SETTING_PIXEL_ABSTAND, ACTIVESYS);
	if (nvs_get_u32(sys_nvs_handle, tb, &nvs_pixel_abstand) != ESP_OK) 
	{
		nvs_pixel_abstand = _STATIC_SYS_VALS[ACTIVESYS].default_pixel_abstand;
		printf("Pixelabstand-Einstellung nicht gefunden, nutze default = %ld\n",nvs_pixel_abstand);
	}
	snprintf(tb, 40, _NVS_SETTING_START_LINE, ACTIVESYS);
	if (nvs_get_u32(sys_nvs_handle, tb, &nvs_abg_start_line) != ESP_OK) 
	{
		nvs_abg_start_line =  _STATIC_SYS_VALS[ACTIVESYS].default_start_line;
		printf("Startline-Einstellung nicht gefunden, nutze default = %ld\n",nvs_abg_start_line);
	}
	snprintf(tb, 40, _NVS_SETTING_PIXEL_PER_LINE, ACTIVESYS);
	if (nvs_get_u32(sys_nvs_handle, tb, &nvs_pixel_per_line) != ESP_OK) 
	{
		nvs_pixel_per_line =  _STATIC_SYS_VALS[ACTIVESYS].default_pixel_per_line;
		printf("Pixelperline-Einstellung nicht gefunden, nutze default = %ld\n",nvs_pixel_per_line);
	}
	if (nvs_get_u8(sys_nvs_handle, _NVS_SETTING_COLORSCHEMA, &Current_Color_Scheme) != ESP_OK) {
		Current_Color_Scheme =  0;
		printf("Colorschema-Einstellung nicht gefunden, nutze default = %d\n",Current_Color_Scheme);
	}
	if (Current_Color_Scheme>_COLORSCHEME_COUNT) Current_Color_Scheme=_COLORSCHEME_COUNT;
	if (Current_Color_Scheme<0) Current_Color_Scheme=0;
	if (nvs_get_u32(sys_nvs_handle, _NVS_SETTING_CUSTOMCOLORS, (uint32_t*)&Custom_Colors[0]) != ESP_OK) 
	{
		for (int i=0;i<4;i++)
		{
			Custom_Colors[i] = _STATIC_COLOR_VALS[0].colors[i];
		}
		printf("Customcolors-Einstellung nicht gefunden, nutze default\n");
	}

	set_colorscheme();
	BSYNC_PIXEL_ABSTAND = (float)nvs_pixel_abstand / 100;
	ABG_START_LINE = nvs_abg_start_line;
	ABG_PIXEL_PER_LINE = (float)nvs_pixel_per_line / 100;
	ABG_Interleave_Mask = _STATIC_SYS_VALS[ACTIVESYS].interleave_mask;	
	ABG_XRes = _STATIC_SYS_VALS[ACTIVESYS].xres;
	ABG_YRes = _STATIC_SYS_VALS[ACTIVESYS].yres;
	ABG_Bits_per_sample = _STATIC_SYS_VALS[ACTIVESYS].bits_per_sample;
	printf("Zuweisung erledigt (%d, %f, %ld, %f)\n", ACTIVESYS, BSYNC_PIXEL_ABSTAND, ABG_START_LINE, ABG_PIXEL_PER_LINE);

	uint8_t a = 0;
	if (nvs_get_u8(sys_nvs_handle, _NVS_SETTING_TRANSPARENT, &a)!=ESP_OK) a = 0;
	OSD_TRANSPARENT = (a!=0);
	if (nvs_get_u8(sys_nvs_handle, _NVS_SETTING_ROTATE, &OSD_KEY_ROTATE)!=ESP_OK) OSD_KEY_ROTATE = 0;
	if (nvs_get_u8(sys_nvs_handle, _NVS_SETTING_WLANMODE, &wlan_mode) != ESP_OK) wlan_mode = 0;
	if (wlan_mode>3) wlan_mode = 0;
	if (wlan_mode==1) wlan_mode = 0; // WPS mode für diese Anwendung ungültig!
	if (nvs_get_u8(sys_nvs_handle, _NVS_SETTING_LANGUAGE, &Language) != ESP_OK) Language = 0;
	if (Language>=Language_count) Language = 0;
	size_t i = 63;
	if (nvs_get_str(sys_nvs_handle, _NVS_SETTING_SSID, wlan_ssid, &i) != ESP_OK) wlan_ssid[0]=0;
	i = 63;
	if (nvs_get_str(sys_nvs_handle, _NVS_SETTING_PASSWD, wlan_passwd, &i) != ESP_OK) wlan_passwd[0]=0;

	OSD_TOP = (ABG_YRes-OSD_HIGHT) / 2;
	OSD_LEFT = (ABG_XRes-OSD_WIDTH) / 2;

	return true;
}

// Einstellungen im Flash sichern
bool write_settings() 
{
	if (sys_nvs_handle == 0)
		return false;

	uint32_t nvs_pixel_abstand = (int)(BSYNC_PIXEL_ABSTAND * 100);
	uint32_t nvs_abg_start_line = ABG_START_LINE;
	uint32_t nvs_pixel_per_line = (int)(ABG_PIXEL_PER_LINE * 100);
	int8_t nvs_current_colorscheme = Current_Color_Scheme;
	uint32_t* nvs_custom_colors = ((uint32_t*)&Custom_Colors[0]);
	char tb[40];

	esp_err_t err;
	bool valid_settings = true;
	err = nvs_set_u16(sys_nvs_handle, _NVS_SETTING_MODE, ACTIVESYS);
	if (err != ESP_OK) valid_settings = false;
	snprintf(tb, 40, _NVS_SETTING_PIXEL_ABSTAND, ACTIVESYS);
	if (nvs_set_u32(sys_nvs_handle, tb, nvs_pixel_abstand) != ESP_OK) valid_settings = false;
	snprintf(tb, 40, _NVS_SETTING_START_LINE, ACTIVESYS);
	if (nvs_set_u32(sys_nvs_handle, tb, nvs_abg_start_line) != ESP_OK) valid_settings = false;
	snprintf(tb, 40, _NVS_SETTING_PIXEL_PER_LINE, ACTIVESYS);
	if (nvs_set_u32(sys_nvs_handle, tb, nvs_pixel_per_line) != ESP_OK) valid_settings = false;
	if (nvs_set_u8(sys_nvs_handle, _NVS_SETTING_COLORSCHEMA, nvs_current_colorscheme) != ESP_OK) valid_settings = false;
	if (nvs_set_u32(sys_nvs_handle, _NVS_SETTING_CUSTOMCOLORS, *nvs_custom_colors) != ESP_OK) valid_settings = false;
	snprintf(tb, 40, _NVS_SETTING_VGAMODE, ACTIVESYS);
	if (nvs_set_u8(sys_nvs_handle, tb, ACTIVEVGA) != ESP_OK) valid_settings = false;
	if (nvs_set_u8(sys_nvs_handle, _NVS_SETTING_TRANSPARENT, OSD_TRANSPARENT ? 1 : 0) != ESP_OK) valid_settings = false;
	if (nvs_set_u8(sys_nvs_handle, _NVS_SETTING_ROTATE, OSD_KEY_ROTATE)!=ESP_OK) valid_settings = false;
	if (nvs_set_u8(sys_nvs_handle, _NVS_SETTING_WLANMODE, wlan_mode) != ESP_OK) valid_settings = false;
	if (nvs_set_u8(sys_nvs_handle, _NVS_SETTING_LANGUAGE, Language) != ESP_OK) valid_settings = false;
	if (nvs_set_str(sys_nvs_handle, _NVS_SETTING_SSID, wlan_ssid) != ESP_OK) valid_settings = false;
	if (nvs_set_str(sys_nvs_handle, _NVS_SETTING_PASSWD, wlan_passwd) != ESP_OK) valid_settings = false;

	if (!valid_settings) 
	{
		printf("Einstellungen nicht gespeichert.\n");
		return false;
	}
	printf("Einstellungen gespeichert.\n");
	return true;
}
