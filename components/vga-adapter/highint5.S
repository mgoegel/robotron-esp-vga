#include <xtensa_context.h>
#include <soc/gpio_reg.h>
#include <soc/gdma_reg.h>
#include <soc/spi_reg.h>
#include "pins.h"

    .data
Reg_Save:
    .space      16

    .section .iram1,"ax"
    .global     xt_highint5
    .type       xt_highint5,@function
    .align      4
xt_highint5:

    movi    a0, Reg_Save                            // Register sichern
    s32i    a15, a0, 0
    s32i    a14, a0, 4
    s32i    a13, a0, 8
    s32i    a12, a0, 12
/*
    // DEBUG auf GPIO14
    movi    a14, (1 << 14)      // GPIO 14
    movi    a13, GPIO_OUT_W1TS_REG
    s32i    a14, a13, 0         // auf High
    // DEBUG Ende
*/
    movi    a14, SYNC_PIN_REG
    movi    a15, SYNC_BIT_VAL
    l32i    a13, a14, 0
    and     a13, a13, a15
    bnez    a13, xt_highint5_L9                     // Signal schon vorbei? War ein Störimpuls! --> Ende!

/*
    int i = 48;
    while (gpio_get_level(PIN_NUM_ABG_BSYNC2)!=0 && i>0) i--;
*/
    movi    a12, 100                                // Timeout-Counter (~10µs)
xt_highint5_L2:
    l32i    a13, a14, 0                             // GPIO's lesen
    and     a13, a13, a15
    beqz    a13, xt_highint5_L1                     // immer noch low? --> zum timeout-test

/*
        if (ABG_Scan_Line == 10)
        {
            XTHAL_SET_CCOUNT(0);
        }
*/  
    movi    a15, ABG_Scan_Line                      // Scanline laden
    l32i    a14, a15, 0
    bnei    a14, 10, xt_highint5_L4                 // wenn nicht 10, dann weiter
    movi    a13, 0                                  // ansonsten ccount=0
    wsr.ccount a13

/*
        if (ABG_Scan_Line == 210)
        {
            bsyn_clock_diff = XTHAL_GET_CCOUNT();
        }
*/  
xt_highint5_L4:
    movi    a13, 210                                
    bne     a14, a13, xt_highint5_L5                // wenn nicht 210, dann weiter
    rsr.ccount a13                                  // ansonsten bsyn_clock_diff=ccount
    movi    a15, bsyn_clock_diff
    s32i    a13, a15, 0
    memw

xt_highint5_L5:
    movi    a15, ABG_Interleave                     // Interleave-Zähler laden
    l32i    a13, a15, 0
    bnez    a14, xt_highint5_L11                    // Wenn Scanline==0, dann Interleave-Zähler+1
    addi    a13, a13, 1
    movi    a12, INTERLEAVE_MASK
    and     a13, a13, a12
    s32i    a13, a15, 0

/*
        ABG_Scan_Line++;
*/
xt_highint5_L11:
    addi    a14, a14, 1                             // Scanline++

    movi    a12, INTERLEAVE_MASK
    and     a15, a14, a12
    bne     a13, a15, xt_highint5_L3                // wenn Interleave-Zähler != Scanline --> Ende

    movi    a13, 1000
    beq     a14, a13, xt_highint5_L8

/*
        if (ABG_Scan_Line > ABG_START_LINE && ABG_RUN)
*/  
    movi    a15, ABG_START_LINE                     // Startline laden
    l32i    a15, a15, 0
    bge     a15, a14, xt_highint5_L3                // wenn Scanline<=Startline --> Ende
    movi    a15, ABG_RUN                            // RUN-Flag laden
    l32i    a15, a15, 0
    beqz    a15, xt_highint5_L3                     // wenn RUN=0 --> Ende

/*          if ((ABG_DMALIST[0] & 0x80000000) != 0)
            {
                ABG_DMALIST[3] = ABG_Scan_Line;
            }
*/
    movi    a15, ABG_DMALIST
    l32i    a15, a15, 0
    movi    a13, 0x80000000
    l32i    a12, a15, 0
    and     a12, a12, a13
    beqz    a12, xt_highint5_L6
    s32i    a14, a15, 12
    memw

/*
            if ((ABG_DMALIST[4] & 0x80000000) != 0)
            {
                ABG_DMALIST[7] = ABG_Scan_Line;
            }
*/
xt_highint5_L6: 
    l32i    a12, a15, 16
    and     a12, a12, a13
    beqz    a12, xt_highint5_L7
    s32i    a14, a15, 28
    memw

xt_highint5_L7:

/*
    usleep(?);
*/
    movi    a12, 100
xt_highint5_L10:
    addi    a12, a12, -1
    bnez    a12, xt_highint5_L10

/*
            REG_SET_BIT(GDMA_IN_LINK_CH1_REG, GDMA_INLINK_RESTART_CH1);
*/
    movi    a15, GDMA_IN_LINK_CH1_REG
    movi    a13, GDMA_INLINK_RESTART_CH1
    l32i    a12, a15, 0
    or      a12, a12, a13
    s32i    a12, a15, 0
//    memw

/*
            REG_SET_BIT(SPI_CMD_REG(2), SPI_USR);
*/
    movi    a15, DR_REG_SPI1_BASE + 0x22000
    movi    a13, SPI_USR
    l32i    a12, a15, 0
    or      a12, a12, a13
    s32i    a12, a15, 0
//    memw
    
    j       xt_highint5_L3
    
xt_highint5_L1:
    addi    a12, a12, -1                            // timeout runterzählen
    bnez    a12, xt_highint5_L2                     // noch kein timeout? --> gpio nochmal lesen

xt_highint5_L8:    
    movi    a14, 0                                  // Scanline=0
    
xt_highint5_L3:
    movi    a15, ABG_Scan_Line                      // Scanline schreiben
    s32i    a14, a15, 0
    memw

xt_highint5_L9:    
    movi    a14, SYNC_QUIT_REG
    movi    a15, SYNC_BIT_VAL
    s32i    a15, a14, 0                             // gpio interrupt quittieren
    memw
/*
    // DEBUG auf GPIO14
    movi    a14, (1 << 14)      // GPIO 14
    movi    a13, GPIO_OUT_W1TS_REG
    s32i    a14, a13, 4         // auf Low
    // DEBUG Ende
*/
    l32i    a15, a0, 0                              // Register wiederherstellen
    l32i    a14, a0, 4
    l32i    a13, a0, 8
    l32i    a12, a0, 12

    rsr     a0, EXCSAVE_5
    rfi     5

    .global ld_include_xt_highint5
ld_include_xt_highint5:
