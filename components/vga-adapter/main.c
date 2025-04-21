
#include "globalvars.h"
#include "main.h"
#include "osd.h"
#include "vga.h"
#include "capture.h"
#include "nvs.h"
#include "wlan.h"

void printramfree(char* text)
{
	printf("%s: IRAM=%dKB SPI=%dKB\n", text, heap_caps_get_free_size(MALLOC_CAP_INTERNAL)/1024, heap_caps_get_free_size(MALLOC_CAP_SPIRAM)/1024);
}

// Hauptprogramm
void app_main(void)
{
	printramfree("start");
	
	setup_flash();
	// NVS Einstellungen laden, wenn vorhanden
	restore_settings();
	printramfree("+settings");

	alloc_abg_dma(1);
	alloc_vga_buffer();
	setup_vga_buffer();
	setup_vga_mode();
	printramfree("+vga");

	xTaskCreatePinnedToCore(osd_task,"osd_task",8000,NULL,0,NULL,0);
	printramfree("+osdtask");

	xTaskCreatePinnedToCore(capture_task,"capture_task",3000,NULL,10,NULL,1);
	printramfree("+capturetask");

	setup_wlan(wlan_mode);
	printramfree("+wlan");
}

