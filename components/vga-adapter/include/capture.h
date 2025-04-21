#pragma once
#include <esp_heap_caps.h>

void setup_abg();
void capture_task(void*);
void alloc_abg_dma(uint8_t channel);