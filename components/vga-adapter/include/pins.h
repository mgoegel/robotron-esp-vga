#pragma once

#include "main.h"

// Pin-Definition --> muss gegebenenfalls auf die Schaltung angepasst wereden!
#define PIN_NUM_VGA_R0 4
#define PIN_NUM_VGA_R1 5
#define PIN_NUM_VGA_G0 6
#define PIN_NUM_VGA_G1 7
#define PIN_NUM_VGA_B0 15
#define PIN_NUM_VGA_B1 16
#define PIN_NUM_VGA_VSYNC 17
#define PIN_NUM_VGA_HSYNC 18

#define PIN_NUM_ABG_BSYNC1 21  // SPI Pin
#define PIN_NUM_ABG_BSYNC2 45  // ISR Pin, Negative Edge, Fallende Flanke
#define PIN_NUM_ABG_VIDEO1 47
#define PIN_NUM_ABG_VIDEO2 48

#define PIN_NUM_TAST_LEFT 38
#define PIN_NUM_TAST_UP 39
#define PIN_NUM_TAST_DOWN 40
#define PIN_NUM_TAST_RIGHT 41

#define PIN_NUM_LED_SYNC 42
#define PIN_NUM_LED_WIFI 46

#ifdef DEBUG
#define PIN_NUM_DEBUG_SPI 13//nur für debug
#define PIN_NUM_DEBUG_COPY 14//nur für debug
#endif

// Pin-Definition vor-ausgerechnet für die highint5.S --> der Assembler scheint keine Symbole auszurechnen!
#if PIN_NUM_ABG_BSYNC2 < 32
#define SYNC_QUIT_REG GPIO_STATUS_W1TC_REG
#define SYNC_BIT_VAL (1 << PIN_NUM_ABG_BSYNC2)
#define SYNC_PIN_REG GPIO_IN_REG
#else
#define SYNC_QUIT_REG GPIO_STATUS1_W1TC_REG
#define SYNC_BIT_VAL (1 << (PIN_NUM_ABG_BSYNC2 - 32))
#define SYNC_PIN_REG GPIO_IN1_REG
#endif
