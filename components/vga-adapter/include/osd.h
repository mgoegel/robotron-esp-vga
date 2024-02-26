#pragma once
#include <esp_heap_caps.h>

void setup_flash();
bool restore_settings();
bool write_settings();
void switch_system();

extern void osd_task(void*);