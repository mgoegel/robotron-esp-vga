idf_component_register(SRCS "globalvars.c" "osd.c" "vga.c" "capture.c" "main.c" "highint5.S"
                    REQUIRES "driver" "esp_lcd" "soc" "nvs_flash" "app_update" "esp_partition"
                    INCLUDE_DIRS "include")
target_link_libraries(${COMPONENT_TARGET} "-u ld_include_xt_highint5")
