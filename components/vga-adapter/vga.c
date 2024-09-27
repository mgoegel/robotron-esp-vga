
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>

#include "globalvars.h"
#include "main.h"
#include "pins.h"

// alles für das VGA-Signal vorbereiten
void setup_vga()
{
	// Einstellungen
    esp_lcd_rgb_panel_config_t panel_config =
    {
        .data_width = 8,
        .psram_trans_align = 64,
        .num_fbs = 1,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .vsync_gpio_num = PIN_NUM_VGA_VSYNC,
        .hsync_gpio_num = PIN_NUM_VGA_HSYNC,
        .data_gpio_nums =
        {
			PIN_NUM_VGA_B0,
			PIN_NUM_VGA_B1,
			PIN_NUM_VGA_G0,
			PIN_NUM_VGA_G1,
        	PIN_NUM_VGA_R0,
			PIN_NUM_VGA_R1,
        },
        .timings =
        {
        	// Timing von 800x600 70Hz VGA. Andere Auflösungen geben kein stabiles Bild.
            .pclk_hz = 40000000,  		// 40MHz Pixelfrequenz
            .h_res = ABG_XRes, 			// 800 auf VGA
            .v_res = ABG_YRes+20,   	// 600 auf VGA
			//line
            .hsync_back_porch = 88+((800-ABG_XRes)/2),
            .hsync_front_porch = 40+((800-ABG_XRes)/2),
            .hsync_pulse_width = 128,
			//frame
            .vsync_back_porch = 23+((600-ABG_YRes)/2)-20,
            .vsync_front_porch = 1+((600-ABG_YRes)/2),
            .vsync_pulse_width = 4,
			.flags.hsync_idle_low = 1,
			.flags.vsync_idle_low = 1,
        },
        .flags.fb_in_psram = false,
    };

    esp_lcd_panel_handle_t VGA_Handle;
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &VGA_Handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(VGA_Handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(VGA_Handle));
    esp_lcd_rgb_panel_get_frame_buffer(VGA_Handle, 1, (void*)&OSD_BUF); // der OSD-Buffer ist genaugenommen der gesamte Bildschirm
    VGA_BUF = 20 * ABG_XRes + OSD_BUF;  // der Bereich für das eigendliche Bild beginnt erst ab Zeile 20
}
