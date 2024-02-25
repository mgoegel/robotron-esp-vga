#include <driver/spi_master.h>
#include "esp_intr_alloc.h"
#include <xtensa/core-macros.h>
#include <freertos/task.h>
#include <rom/ets_sys.h>
#include <soc/periph_defs.h>

#define DEBUG 1

#include "globalvars.h"
#include "main.h"
#include "osd.h"
#include "vga.h"
#include "capture.h"
#include "pins.h"


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

