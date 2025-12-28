#include "freertos/FreeRTOS.h"
#include "soc/usb_serial_jtag_struct.h"
#include "nvs.h"
#include "globalvars.h"
#include "vga.h"
#include "wlan.h"

uint32_t menulevel = 0;
char * buffer = NULL;

// Char an USB-Serial ausgeben, wird direkt in den Sendebuffer geschrieben
void printChar(char ch)
{
    uint32_t i = 0;
    while (i<100)
    {
        if (USB_SERIAL_JTAG.ep1_conf.serial_in_ep_data_free) // Ausgabe nur wenn Platz im Buffer ist
        {
            USB_SERIAL_JTAG.ep1.rdwr_byte = (uint8_t)ch; // byte ausgeben
            return;
        }
        i++;
        esp_rom_delay_us(100);
    }
}

// String direkt an USB-Serial ausgeben, im Gegensatz zu printf() muss das nicht mit \n abgeschlossen sein
void printString(char * text, uint32_t len) 
{
    for (int i=0;i<len && text[i]!=0; i++)
    {
        printChar(text[i]);
    }
    USB_SERIAL_JTAG.ep1_conf.wr_done=1; // Buffer senden
}

void processInput(char input)
{
    switch (menulevel)
    {
        case 0: 
            switch (input)
            {
                case '1':
                    printf("Auswahl:\n");
                    for (int j=0;j<_VGAMODE_COUNT;j++)
                    {
                        printf("%1d: %s %s %s\n",j+1,_STATIC_VGA_VALS[j].name,(((1 << ACTIVEVGA) & _STATIC_SYS_VALS[ACTIVESYS].accept_vga_modes) == 0)?"(zu klein!)":"",ACTIVEVGA==j?"<--aktiv":"");
                    }
                    menulevel = 1;
                    return;
                case '2':
                    memcpy(buffer,wlan_ssid,64);
                    int j;
                    for (j=0;j<64;j++) {if (buffer[j]==0) break;}
                    for (;j<64;j++) buffer[j]=0;
                    printString("SSID eingeben:",64);
                    printString(buffer,64);
                    menulevel = 2;
                    return;
                case '3':
                    memset(buffer,0,64);
                    printString("Passwort eingeben:",64);
                    menulevel = 3;
                    return;
                case '4':
                    write_settings(true);
                    return;
            }
            break;
        case 1: 
            if (input-'1'<_VGAMODE_COUNT && input>'0')
            {
                ACTIVEVGA = input-'1';
                setup_vga_mode();
                printf("Ausgewählt: %s\n",_STATIC_VGA_VALS[ACTIVEVGA].name);                
                menulevel = 0;
                break;
            }
            else if (input==27)
            {
                menulevel = 0;
                break;
            }
            return;
        case 2: 
        case 3:
            switch(input)
            {
                case 27: // Escape
                    printf("\nAbbruch\n");
                    menulevel = 0;
                    break;
                case 13: // Enter
                case 10:
                    printf("\nOk\n");
                    if (menulevel==2)
                    {
                        memcpy(wlan_ssid,buffer,64);
                    }
                    else
                    {
                        memcpy(wlan_passwd,buffer,64);
						setup_wlan(wlan_mode);
                    }
                    menulevel = 0;
                    break;
                case 8: // Backspace
                    for (int j=63;j>=0;j--)
                    {
                        if (buffer[j]!=0)
                        {
                            buffer[j] = 0;
                            printChar(8);
                            printChar(32);
                            printChar(8);
                            USB_SERIAL_JTAG.ep1_conf.wr_done=1;
                            break;
                        }
                    }
                    return;
                default: // alles andere
                    for (int j=0;j<64;j++)
                    {
                        if (buffer[j]==0)
                        {
                            buffer[j] = input;
                            printChar(input);
                            USB_SERIAL_JTAG.ep1_conf.wr_done=1;
                            break;
                        }
                    }
                    return;
            }
    }
    printf("++++ Robotron ESP32 VGA-Adapter, Version %s ++++\n",VERSION);
    printf("1: VGA-Modus\n2: Wlan-SSID\n3: Wlan-Passwort\n4: alles speichern\n");
}

void run_cmdline()
{
    if (buffer == NULL)
    {
        buffer = heap_caps_malloc(64, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);
    }
    if (USB_SERIAL_JTAG.ep1_conf.serial_out_ep_data_avail) // Byte im USB-Serial Empfangspuffer ?
    {
        processInput((char)USB_SERIAL_JTAG.ep1.rdwr_byte); // Byte aus Buffer lesen und verarbeiten
    }
}

