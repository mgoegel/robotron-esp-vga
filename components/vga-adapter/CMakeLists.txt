idf_component_register(SRCS "main.c" "highint5.S"
                    REQUIRES "driver" "esp_lcd" "soc" "nvs_flash"
                    INCLUDE_DIRS "include")
target_link_libraries(${COMPONENT_TARGET} "-u ld_include_xt_highint5")
