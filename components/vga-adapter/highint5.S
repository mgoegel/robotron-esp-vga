#include <xtensa_context.h>
#include <soc/gpio_reg.h>
#include <soc/gdma_reg.h>
#include <soc/spi_reg.h>
#include "pins.h"

    .data
Reg_Save:
    .space      24

    .section .iram1,"ax"
    .global     xt_highint5
    .type       xt_highint5,@function
    .align      4
xt_highint5:

    // Register sichern
    movi    a0, Reg_Save
    s32i    a15, a0, 0
    s32i    a14, a0, 4
    s32i    a13, a0, 8
    s32i    a12, a0, 12

/*
    // DEBUG-Impuls auf GPIO14, bei Bedarf an beliebige Stelle kopieren
    s32i    a12, a0, 16         // Register sichern
    s32i    a13, a0, 20
    movi    a13, (1 << 14)      // GPIO 14
    movi    a12, GPIO_OUT_W1TS_REG
    s32i    a13, a12, 0         // auf High
    s32i    a13, a12, 4         // auf Low
    l32i    a12, a0, 16         // Register wiederherstellen
    l32i    a13, a0, 20
    // DEBUG Ende
*/
/*  dieser Softwar-Filter scheint eher kontraproduktiv zu sein, deshalb erstmal deaktiviert
    // warten, bis das BSYN-Signal sich nicht mehr ändert
xt_highint5_L12:
    movi    a14, 5                                  // Anzahl Schleifendurchgänge
xt_highint5_L13:
    mov     a12, a13                                // GPIO-Wert des letzen Durchgangs sichern. Beim ersten Durchgang steht hier Müll drin, das macht aber nichts
    movi    a13, SYNC_PIN_REG                       // Adresse vom GPIO-Port1
    movi    a15, SYNC_BIT_VAL                       // nur 1 Bit davon
    l32i    a13, a13, 0                             // Wert laden
    and     a13, a13, a15                           // AND 1-Bitmuster
    bne     a12, a13, xt_highint5_L12               // Schleife von ganz vorn wiederholen, wenn sich der GPIO-Wert geändert hat
    addi    a14, a14, -1                            // Schleifenzähler reduzieren
    bnez    a14, xt_highint5_L13                    // bis 0 wiederholen
    bnez    a13, xt_highint5_L14                    // sollte an der Stelle das BSYN bereits HIGH sein, dann war das ein Fehl-Interrupt: Ende
*/

    // SPI Transfer stoppen
    movi    a15, DR_REG_SPI1_BASE + 0x22000         // Adresse des SPI-Steuerregisters
    movi    a13, 0
    s32i    a13, a15, 0

    // Scanline laden
    movi    a14, ABG_Scan_Line                      // Adresse der Variable
    l32i    a14, a14, 0                             // Wert der Variable

    // Scanline +1 hochzählen
    addi    a14, a14, 1

    // Behandlung des Interleaves: Zähler hochzählen
    movi    a15, ABG_Interleave                     // Interleave-Zähler laden
    l32i    a13, a15, 0
    movi    a12, ABG_Interleave_Mask                // Interleave-Maske laden
    l32i    a12, a12, 0
    bnei    a14, 1, xt_highint5_L11                 // Wenn Scanline != 1, dann weiter
    addi    a13, a13, 1                             // Interleave-Zähler +1
    and     a13, a13, a12                           // mit Zähler UND-verknüpfen
    s32i    a13, a15, 0                             // und speichern
xt_highint5_L11:

    // Warteschleife initialisieren
    movi    a12, 100                                // Timeout-Counter (~10µs)

xt_highint5_L2:

    // GPIO's lesen
    movi    a13, SYNC_PIN_REG                       // Adresse vom GPIO-Port1
    movi    a15, SYNC_BIT_VAL                       // nur 1 Bit davon
    l32i    a13, a13, 0                             // Wert laden
    and     a13, a13, a15                           // AND 1-Bitmuster
    beqz    a13, xt_highint5_L1                     // alles 0 ? --> zum timeout-test

    // bei Scanline 10 den CPU-Takt-Zähler speichern, und auf 0 setzen
    // der Wert wird im Pixel-Debugger zur Berechnung der HSYNC-Frequenz verwendet
    movi    a13, 20                                
    bne     a14, a13, xt_highint5_L4                // wenn nicht 20, dann weiter
    rsr.ccount a13                                  // Zähler auslesen
    movi    a15, bsyn_clock_frame                   // Adresse der Variable
    s32i    a13, a15, 0                             // Wert schreiben
    movi    a13, 0                                  // ansonsten ccount=0
    wsr.ccount a13
xt_highint5_L4:

    // bei Zeile 220 den CPU-Takt-Zähler auslesen und speichern, 
    // der wird später für die Berechnung der Frequenzen und Pixelpositionen gebraucht
    // wir haben damit genau die CPU-Takte für 200 Zeilen
    movi    a13, 220                                
    bne     a14, a13, xt_highint5_L5                // wenn nicht 220, dann weiter
    rsr.ccount a13                                  // ansonsten bsyn_clock_diff=ccount
    movi    a15, bsyn_clock_diff                    // Adresse der Variable
    s32i    a13, a15, 0                             // Wert schreiben
xt_highint5_L5:

    // Testen, ob vom Interleave her diese Zeile eingelesen werden soll
    movi    a13, ABG_Interleave                     // Interleave-Zähler laden
    l32i    a13, a13, 0
    movi    a12, ABG_Interleave_Mask                // Interleave-Maske laden
    l32i    a12, a12, 0
    and     a15, a14, a12                           // Scanline mit Interleave-Maske UND-Verknüpfen
    bne     a13, a15, xt_highint5_L3                // wenn Interleave-Zähler != Scanline --> Ende

    // Testen, ob wir schon an der Startline vorbei sind
    movi    a15, ABG_START_LINE                     // Startline laden
    l32i    a15, a15, 0
    blt     a14, a15, xt_highint5_L3                // wenn Scanline<Startline --> Ende

    // Testen, ob das RUN-Flag gesetzt ist
    movi    a15, ABG_RUN                            // RUN-Flag laden
    l32i    a15, a15, 0
    beqz    a15, xt_highint5_L3                     // wenn RUN=0 --> Ende

    // eine kurze Warteschleife, wir müssen sicherstellen, dass der SPI/DMA mit der vorherigen Übertragung fertig ist
    movi    a12, 10
xt_highint5_L10:
    addi    a12, a12, -1
    bnez    a12, xt_highint5_L10

    // DMA-Buffer-Parameter-Liste bearbeiten
    movi    a15, ABG_DMALIST                        // Zeiger auf Zeiger auf die Parameterliste vom DMA-Kontroller
    l32i    a15, a15, 0                             // Zeiger laden
    movi    a13, 0xc0000000                         // Frei-Kennung
    l32i    a12, a15, 0                             // Parameter vom 1. Buffer laden
    beq     a12, a13, xt_highint5_L6                // Frei? --> L6
    addi    a15, a15, 16                            // ein Buffer weiter
    l32i    a12, a15, 0                             // Parameter vom 2. Buffer laden
    bne     a12, a13, xt_highint5_L3                // auch nicht frei? --> L3
xt_highint5_L6:
    movi    a13, 0xc0ffffff                         // DMA-Parameter
    s32i    a13, a15, 0                             // in die Parameterliste schreiben
    s32i    a14, a15, 12                            // Scanline an die Parameterliste schreiben
    memw                                            // cache flush
    movi    a13, 0xfffff
    and     a12, a15, a13                           // 20 bits von der Parameterliste-Adresse trennen

    // DMA-Kontroller neustarten
    movi    a15, GDMA_IN_LINK_CH1_REG               // Adresse des DMA-Steuerregisters
    movi    a13, GDMA_INLINK_START_CH1              // Bit für Start
    or      a12, a12, a13                           // Start-Bit setzen + Adresse von Parameterliste
    s32i    a12, a15, 0                             // Register schreiben

    // SPI-Interrupt-Flaggs resetten, der DMA macht sonst ab und zu Fehler
    movi    a15, DR_REG_SPI1_BASE + 0x22000         // Adresse des SPI-Steuerregisters
    movi    a12, 0xffffffff
    s32i    a12, a15, 0x38                          // SPI_DMA_INT_CLR_REG

    // SPI-Kontroller USR-Bit setzen (startet einlesen)
    movi    a13, SPI_USR | SPI_UPDATE               // USR-Bit und UPDATE-Bit
    l32i    a12, a15, 0                             // Register lesen
    or      a12, a12, a13                           // Bit setzen (wird vom Kontroller selber wieder auf 0 gesetzt)
    s32i    a12, a15, 0                             // Register schreiben
    
    j       xt_highint5_L3                          // --> Ende
    
    // BSYNC ist immer noch low! Ist das vielleicht ein VSYNC?
xt_highint5_L1:
    addi    a12, a12, -1                            // timeout runterzählen
    bnez    a12, xt_highint5_L2                     // noch kein timeout? --> gpio nochmal lesen

    // die Zeit ist um! Das ist tatsächlich ein VSYNC!
    blti    a14, 16, xt_highint5_L3                 // Scanline <=15 ? dann ist das nicht der erste VSYNC in diesem Frame!

    // Scanline auf 0 setzen
    movi    a14, 0

xt_highint5_L3:
    movi    a15, ABG_Scan_Line                      // Scanline schreiben
    s32i    a14, a15, 0
    memw

xt_highint5_L14:
    // GPIO Interrupt Quittieren
    movi    a14, SYNC_QUIT_REG                      // Adresse Quitt-Register
    movi    a15, SYNC_BIT_VAL                       // Bit GPIO
    s32i    a15, a14, 0                             // Quittieren
    memw                                            // cache flush!

    movi a14, GPIO_STATUS_REG                       // Einfach nur Quittieren scheint nicht zu reichen. Diese 3 ansich unsinnigen 
    l32i a14, a14, 0                                // Anweisungen stellen sicher, dass die Quittierung sofort wirksam wird, 
    memw                                            // ansonsten wirkt die erst später, und es wird sofort noch ein 2. Interrupt ausgelöst

    // Register wiederherstellen
    l32i    a15, a0, 0                              
    l32i    a14, a0, 4
    l32i    a13, a0, 8
    l32i    a12, a0, 12

    // Interrupt beenden
    rsr     a0, EXCSAVE_5
    rfi     5

    .global ld_include_xt_highint5
ld_include_xt_highint5:
