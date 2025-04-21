idf_component_register(SRCS "wlan.c" "webserver.c" "globalvars.c" "osd.c" "vga.c" "capture.c" "main.c" "highint5.S" "nvs.c" 
                    REQUIRES "driver" "esp_lcd" "soc" "nvs_flash" "app_update" "esp_partition" "esp_wifi" "esp_http_server" "wpa_supplicant"
                    INCLUDE_DIRS "include")
target_link_libraries(${COMPONENT_TARGET} "-u ld_include_xt_highint5")
