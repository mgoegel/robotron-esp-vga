#pragma once
#include <esp_heap_caps.h>

void setup_abg();
void IRAM_ATTR capture_task(void*);
