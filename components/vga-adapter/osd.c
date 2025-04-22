#include <unistd.h>
#include <driver/gpio.h>
#include "esp_ota_ops.h"

#include "globalvars.h"
#include "pins.h"
#include "osd.h"
#include "vga.h"
#include "wlan.h"


#define Color_Back 0
#define Color_BackSelect 0x25
#define Color_TextSelect 0x03
#define Color_Menutext 0x3f
#define Color_Statictext 0x03
#define Color_Frame 0x3f
#define Color_Values 0x2e
#define Color_Disabled 0x07
#define Color_TextHiLight 0x35

#define font_w 7
#define font_h 10

uint8_t menu_top = 0;
uint8_t menu_sel = 0;
uint8_t menu_subsel = 0;
uint8_t menu_row = 0;

// Zeichensatz für das OSD-Textfeld
const unsigned char CHARSET[] =
{
	0x80, 0x80, 0x80, 0x00, 0x00, 0x00,  //
	0x5F, 0x5F, 0x00, 0x00, 0x00, 0x00,  // !
	0x03, 0x03, 0x03, 0x03, 0x00, 0x00,  // "
	0x36, 0x7F, 0x36, 0x7F, 0x36, 0x00,  // #
	0x2C, 0x7F, 0x2A, 0x7F, 0x1A, 0x00,  // $
	0x03, 0x63, 0x38, 0x0E, 0x67, 0x60,  // %
	0x3A, 0x7F, 0x4D, 0x7F, 0x32, 0x78,  // &
	0x03, 0x03, 0x00, 0x00, 0x00, 0x00,  // '
	0x3E, 0x7F, 0x41, 0x00, 0x00, 0x00,  // (
	0x41, 0x7F, 0x3E, 0x00, 0x00, 0x00,  // )
	0x54, 0x7C, 0x38, 0x7C, 0x54, 0x00,  // *
	0x10, 0x10, 0x7C, 0x10, 0x10, 0x00,  // +
	0x60, 0x30, 0x00, 0x00, 0x00, 0x00,  // ,
	0x10, 0x10, 0x10, 0x10, 0x00, 0x00,  // -
	0x80, 0x30, 0x30, 0x00, 0x00, 0x00,  // .
	0x70, 0x1C, 0x07, 0x00, 0x00, 0x00,  // /
	0x3E, 0x7F, 0x41, 0x7F, 0x3E, 0x00,  // 0
	0x80, 0x02, 0x7F, 0x7F, 0x00, 0x00,  // 1
	0x71, 0x79, 0x49, 0x4F, 0x46, 0x00,  // 2
	0x41, 0x41, 0x49, 0x7F, 0x36, 0x00,  // 3
	0x18, 0x14, 0x12, 0x7F, 0x7F, 0x00,  // 4
	0x47, 0x45, 0x45, 0x7D, 0x38, 0x00,  // 5
	0x3E, 0x7F, 0x45, 0x7D, 0x38, 0x00,  // 6
	0x01, 0x71, 0x7D, 0x0F, 0x03, 0x00,  // 7
	0x36, 0x7F, 0x49, 0x7F, 0x36, 0x00,  // 8
	0x0E, 0x5F, 0x51, 0x7F, 0x3E, 0x00,  // 9
	0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00,  // :
	0x4C, 0x6C, 0x00, 0x00, 0x00, 0x00,  // ;
	0x10, 0x38, 0x6C, 0x44, 0x00, 0x00,  // <
	0x28, 0x28, 0x28, 0x28, 0x00, 0x00,  // =
	0x44, 0x6C, 0x38, 0x10, 0x00, 0x00,  // >
	0x01, 0x59, 0x5D, 0x07, 0x02, 0x00,  // ?
	0x3E, 0x7F, 0x55, 0x5D, 0x5F, 0x1E,  // @
	0x7E, 0x7F, 0x11, 0x7F, 0x7E, 0x00,  // A
	0x7F, 0x7F, 0x45, 0x7F, 0x3A, 0x00,  // B
	0x3E, 0x7F, 0x41, 0x41, 0x41, 0x00,  // C
	0x7F, 0x7F, 0x41, 0x7F, 0x3E, 0x00,  // D
	0x7F, 0x7F, 0x45, 0x45, 0x00, 0x00,  // E
	0x7F, 0x7F, 0x05, 0x05, 0x01, 0x00,  // F
	0x3E, 0x7F, 0x41, 0x79, 0x78, 0x00,  // G
	0x7F, 0x7F, 0x04, 0x7F, 0x7F, 0x00,  // H
	0x00, 0x7F, 0x7F, 0x00, 0x00, 0x00,  // I
	0x00, 0x40, 0x40, 0x7F, 0x3F, 0x00,  // J
	0x7F, 0x7F, 0x1C, 0x36, 0x63, 0x41,  // K
	0x7F, 0x7F, 0x40, 0x40, 0x40, 0x00,  // L
	0x7F, 0x7F, 0x0F, 0x3C, 0x0F, 0x7F,  // M
	0x7F, 0x7E, 0x0C, 0x18, 0x3F, 0x7F,  // N
	0x3E, 0x7F, 0x41, 0x41, 0x7F, 0x3E,  // O
	0x7F, 0x7F, 0x11, 0x1F, 0x0E, 0x00,  // P
	0x3E, 0x7F, 0x41, 0x51, 0x7F, 0x7E,  // Q
	0x7F, 0x7F, 0x11, 0x7F, 0x4E, 0x00,  // R
	0x46, 0x4F, 0x79, 0x31, 0x00, 0x00,  // S
	0x01, 0x01, 0x7F, 0x7F, 0x01, 0x01,  // T
	0x3F, 0x7F, 0x40, 0x7F, 0x3F, 0x00,  // U
	0x07, 0x1F, 0x7C, 0x7C, 0x1F, 0x07,  // V
	0x07, 0x3F, 0x78, 0x3C, 0x78, 0x3F,  // W
	0x63, 0x77, 0x1C, 0x1C, 0x77, 0x63,  // X
	0x03, 0x07, 0x7C, 0x7C, 0x07, 0x03,  // Y
	0x71, 0x79, 0x5D, 0x4F, 0x47, 0x00,  // Z
	0x7F, 0x7F, 0x41, 0x00, 0x00, 0x00,  // [
	0x07, 0x1C, 0x70, 0x00, 0x00, 0x00,
	0x41, 0x7F, 0x7F, 0x00, 0x00, 0x00,  // ]
	0x06, 0x03, 0x06, 0x00, 0x00, 0x00,  // ^
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40,  // _
	0x01, 0x03, 0x02, 0x00, 0x00, 0x00,  // `
	0x20, 0x74, 0x54, 0x7C, 0x78, 0x00,  // a
	0x7F, 0x7F, 0x44, 0x7C, 0x38, 0x00,  // b
	0x38, 0x7C, 0x44, 0x44, 0x00, 0x00,  // c
	0x38, 0x7C, 0x44, 0x7F, 0x7F, 0x00,  // d
	0x38, 0x7C, 0x54, 0x5C, 0x58, 0x00,  // e
	0x7E, 0x7F, 0x05, 0x00, 0x00, 0x00,  // f
	0x08, 0x5C, 0x54, 0x7C, 0x3C, 0x00,  // g
	0x7F, 0x7F, 0x04, 0x7C, 0x78, 0x00,  // h
	0x00, 0x7D, 0x7D, 0x00, 0x00, 0x00,  // i
	0x40, 0x7D, 0x3D, 0x00, 0x00, 0x00,  // j
	0x7F, 0x7F, 0x38, 0x6C, 0x44, 0x00,  // k
	0x00, 0x7F, 0x7F, 0x00, 0x00, 0x00,  // l
	0x7C, 0x7C, 0x04, 0x7C, 0x04, 0x78,  // m
	0x7C, 0x7C, 0x04, 0x7C, 0x78, 0x00,  // n
	0x38, 0x7C, 0x44, 0x7C, 0x38, 0x00,  // o
	0x7C, 0x7C, 0x14, 0x1C, 0x08, 0x00,  // p
	0x08, 0x1C, 0x14, 0x7C, 0x7C, 0x00,  // q
	0x7C, 0x7C, 0x08, 0x0C, 0x00, 0x00,  // r
	0x58, 0x5C, 0x74, 0x34, 0x00, 0x00,  // s
	0x3F, 0x7F, 0x44, 0x00, 0x00, 0x00,  // t
	0x3C, 0x7C, 0x40, 0x7C, 0x7C, 0x00,  // u
	0x0C, 0x3C, 0x70, 0x3C, 0x0C, 0x00,  // v
	0x1C, 0x7C, 0x38, 0x38, 0x7C, 0x1C,  // w
	0x6C, 0x7C, 0x10, 0x7C, 0x6C, 0x00,  // x
	0x0C, 0x5C, 0x50, 0x7C, 0x3C, 0x00,  // y
	0x64, 0x74, 0x5C, 0x4C, 0x44, 0x00,  // z
	0x08, 0x7F, 0x77, 0x00, 0x00, 0x00,  // {
	0x7F, 0x7F, 0x00, 0x00, 0x00, 0x00,  // |
	0x77, 0x7F, 0x08, 0x00, 0x00, 0x00,  // }
	0x21, 0x75, 0x54, 0x7D, 0x79, 0x00,  // \x7e ä
	0x10, 0x38, 0x54, 0x10, 0x1F, 0x00,  // \x7f enter
	0x0c, 0x06, 0x7f, 0x06, 0x0c, 0x00,  // \x80 pfeil hoch
	0x18, 0x30, 0x7f, 0x30, 0x18, 0x00,  // \x81 pfeil runter
	0x08, 0x08, 0x2a, 0x1c, 0x08, 0x00,  // \x82 pfeil rechts
	0x08, 0x1c, 0x2a, 0x08, 0x08, 0x00,  // \x83 pfeil links
	0x3D, 0x7D, 0x40, 0x7D, 0x7D, 0x00,  // \x84 ü
	0x39, 0x7D, 0x44, 0x7D, 0x39, 0x00,  // \x85 ö
};

char* osd_ovrstatus = NULL;
bool osd_repeat = false;
wifi_ap_record_t* ap_list = NULL;
uint16_t ap_count = 0;


enum MENUTYPE 
{
	MT_STOP,
	MT_SEPARATOR,
	MT_REALVALUE,
	/* Realzahl
		value1 = Zeiger auf Variable (double*)
		value2 = Untergrenze
		value3 = Obergrenze
		value4 = Anzahl Nachkommastellen
		value5 = Schrittweite (*1000)
	*/
	MT_INTVALUE,
	/* Integerzahl 32bit
		value1 = Zeiger auf Variable (uint32_t*)
		value2 = Untergrenze
		value3 = Obergrenze
	*/
	MT_ENUM,
	/*  Aufzählung
		value1 = typ
				0: Computer-Profil
				1: SPI 4bit / 8bit
				2: VGA-Auflösung
				3: Farbschema, Auswahl
				4: Anwender-Farben
				5: Wlan-Modus
				6: WPS-Setup
				7: Wlan SSID
				8: Wlan Passwort
				9: Streaming aktivieren
				10: Sprache
				11: Transparenz einstellen
				12: Keybinding
				13: Profil defaults laden
				14: alles speichern
	*/
};

struct MENUITEM
{
	char* name[Language_count];
	char* description[Language_count]; // max 21 zeichen pro zeile
	enum MENUTYPE typ;
	uint32_t value1;
	uint32_t value2;
	uint32_t value3;
	uint32_t value4;
	uint32_t value5;
};

const struct MENUITEM osd_menu[] =
{
	{
		.name = {"Eingang","Input"},
		.typ = MT_SEPARATOR,
	},
	{
		.name = {"Profil", "Profile"},
		.description = {"Vorgabe-Einstellungen\nf\x84r verschiedene\nComputer", "Default settings for\nsome computer types"},
		.typ = MT_ENUM,
		.value1 = 0,
	},
	{
		.name = {"Vorgaben laden", "Load defaults"} ,
		.description = {"Profil-Vorgaben\neinstellen", "Set profile settings\nto default values"},
		.typ = MT_ENUM,
		.value1 = 13,
	},
	{
		.name = {"SPI-Bitanzahl", "SPI-Bitcount"} ,
		.description = {"Anzahl gleichzeitig\neingelesener Bits,\nentscheidend f\x84r das\nVideo-Timing", "Sample-count,\nimportant for video\ntiming"},
		.typ = MT_ENUM,
		.value1 = 1,
	},
	{
		.name = {"Pixel pro Zeile", "Pixel per line"},
		.description = {"Pixel pro Zeile\n(inklusive Vor,-und\nNachlauf)", "Pixel count inclusive\nblank times"},
		.typ = MT_REALVALUE,
		.value1 = (uint32_t)&ABG_PIXEL_PER_LINE,
		.value2 = 400,
		.value3 = 2000,
		.value4 = 1,
		.value5 = 100,
	},
	{
		.name = {"Pixel Abstand", "Pixel distance"},
		.description = {"Pixel Abstand zwischen\ndem 1. Pixel und HSync", "Pixel distance between\n1st Pixel and HSync"},
		.typ = MT_REALVALUE,
		.value1 = (uint32_t)&BSYNC_PIXEL_ABSTAND,
		.value2 = 10,
		.value3 = 1000,
		.value4 = 2,
		.value5 = 20,
	},
	{
		.name = {"Startzeile", "Start line"},
		.description = {"Anzahl Zeilen zwischen\n1. Zeile und VSync", "Lines between 1st\nline and HSync"},
		.typ = MT_INTVALUE,
		.value1 = (uint32_t)&ABG_START_LINE,
		.value2 = 3,
		.value3 = 100,
	},
	{
		.name = {"Ausgang","Output"},
		.typ = MT_SEPARATOR,
	},
	{
		.name = {"VGA-Modus", "VGA-Mode"},
		.description = {"Aufl\x85sung und Frequenz\ndes VGA-Signals", "Resolution and\nfrequency of VGA\nsignal"},
		.typ = MT_ENUM,
		.value1 = 2,
	},
	{	
		.name = {"Farbschema", "Color sheme"},
		.description = {"Die Grundfarben\neinstellen", "Setup the base colors"},
		.typ = MT_ENUM,
		.value1 = 3,
	},
	{
		.name = {"Farben", "Colors"},
		.description = {"Farben f\x84r das Custom-\nFarbschema", "Colors for the custom\ncolor sheme"},
		.typ = MT_ENUM,
		.value1 = 4,
	},
	{
		.name = {"Wlan-Einstellungen","Wlan settings"},
		.typ = MT_SEPARATOR,
	},
	{
		.name = {"Wlan-Modus","Wlan mode"},
		.description = {"Betriebsart des Wlan.\nACHTUNG: Benutzung\ndes Wlan kann zu\nBildst\x85rungen f\x84hren.", "Operation mode of wlan\nATTENTION: using the\nwlan can lead to\nimage distorsions"},
		.typ = MT_ENUM,
		.value1 = 5,
	},
	{
		.name = {"SSID", "SSID"},
		.description = {"Wlan Netzwerkname", "Wlan network\nidentifier"},
		.typ = MT_ENUM,
		.value1 = 7,
	},
	{
		.name = {"Passwort", "Password"},
		.description = {"Das Passwort kann nur\nin einem St\x84""ck\nneu eingegeben werden.", "The password can only\nbe entered new in one\npice"},
		.typ = MT_ENUM,
		.value1 = 8,
	},
	{
		.name = {"Streaming Modus", "Streaming mode"},
		.description = {"Aktivierung des\nStreamings stoppt die\nBildwidergabe auf VGA", "Enabling the streaming\nfeature stops the VGA\ndisplay"},
		.typ = MT_ENUM,
		.value1 = 9,
	},
	{
		.name = {"OSD-Einstellungen","OSD settings"},
		.typ = MT_SEPARATOR,
	},
	{
		.name = {"Sprache", "Language"},
		.description = {"Auswahl der Sprache\ndes OSD-Men\x84s", "Select the language\nof the OSD-Menue"},
		.typ = MT_ENUM,
		.value1 = 10,
	},
	{
		.name = {"Transparenz", "Transparency"},
		.description = {"Das OSD-Men\x84 halb-\ntransparent erscheinen\nlassen - flimmert\nallerdings etwas", "Let the OSD-Menu\nappear half\ntransnparent -does\nflicker a bit"},
		.typ = MT_ENUM,
		.value1 = 11,
	},
	{	
		.name = {"Tastenbelegung", "Keybinding"},
		.description = {"Dreht die Belegung der\nOSD-Tasten: wenn Sie\ndas Keypad falsch\nherum benutzen wollen", "Rotates or switches\nthe OSD-Keys, if you\nwant to use the keypad\nrotated"},
		.typ = MT_ENUM,
		.value1 = 12,
	},
	{
		.name = {"Alle-Einstellungen","All settings"},
		.typ = MT_SEPARATOR,
	},
	{
		.name = {"Speichern", "save"},
		.description = {"Alle Einstellungen\nspeichern", "Safe all settings"},
		.typ = MT_ENUM,
		.value1 = 14,
	},
	{
		.typ = MT_STOP,
	},
};

const char* TextCustoms[Language_count] = {"Anwender","Custom"};
const char* TextLanguage[Language_count] = {"Deutsch","English"};
const char* TextON[Language_count] = {"EIN","ON"};
const char* TextOFF[Language_count] = {"AUS","OFF"};
const char* TextAccept[Language_count] = {"Annehmen","Accept"};
const char* TextCancel[Language_count] = {"Abbrechen","Cancel"};
const char* TextScan[Language_count] = {"Suchen","Scan"};
const char* TextEnter[Language_count] = {"Eingeben","Enter"};
const char* TextSet[Language_count] = {"Gesetzt","Entered"};
const char* TextLoaded[Language_count] = {"Geladen","Loaded"};
const char* TextSaved[Language_count] = {"Gespeichert","Saved"};
const char* TextNone[Language_count] = {"Keins","None"};

static void draw_text_color(const char* txt, uint8_t count, uint16_t xpos, uint16_t ypos, uint8_t color, uint8_t bkcolor)
{
	bool run=true;
	uint16_t x = xpos;
	uint16_t y = ypos;
	for (int a=0;a<count;a++)
	{
		if (y+font_h>=OSD_HIGHT) return;
		if (run && (txt[a]=='\n'))
		{
			y += font_h;
			x = xpos;
		}
		else
		{
			for (int b=0;b<6;b++)
			{
				char c=0;
				if (run)
				{
					if (txt[a]==0) 
					{
						c = 0;
						run = false;
						if (count > 250) return;
					}
					else
					{
						c = CHARSET[(txt[a]-32)*6+b];
					}
				}
				if (b+x>=OSD_WIDTH-4) return;
				for (int d=0;d<font_w;d++)
				{
					OSD_BUF[y+d+2][b+x+2] = (((1 << d) & c) != 0) ? color : bkcolor;
				}
				OSD_BUF[y+1][x+b+2] = bkcolor;
				OSD_BUF[y+9][x+b+2] = bkcolor;
			}
			for (int d=0;d<font_h-1;d++)
			{
				OSD_BUF[y+d+1][x+1] = bkcolor;
			}
			x += font_w;
		}
	}
}

// Text in die OSD-Zeile drucken
static void draw_text_select(const char* txt, uint8_t count, uint8_t col, uint8_t row, bool selected)
{
	char color = selected ? Color_TextSelect : Color_Menutext;
	char bkcolor = selected ? Color_BackSelect : Color_Back;
	draw_text_color(txt, count, col*font_w, row*font_h, color, bkcolor);
}

// Text in die OSD-Zeile drucken
static void draw_text_value(const char* txt, uint8_t line)
{
	uint8_t f = 0;
	for (f=0;f<20;f++)
	{
		if (txt[f]==0) break;
	}
	if (f<6)
	{
		draw_text_color(" ",6-f,16*font_w,(line-menu_top)*font_h,Color_Values,Color_Back);
	}
	draw_text_color(txt,f,((OSD_WIDTH/7)-f)*font_w,(line-menu_top)*font_h,Color_Values,Color_Back);
}

uint32_t status_line = 0;
void force_status_type(uint8_t type)
{
	status_line = type<<4;
}

// Statuszeile aktualisieren
static void draw_status_line()
{
	if (osd_ovrstatus[0] != 0)
	{
		draw_text_color(osd_ovrstatus,OSD_WIDTH/font_w,0,OSD_HIGHT-font_h-1,Color_TextHiLight,Color_Back);
	}
	else
	{
		char tb[40];
		status_line = (status_line+1)&0xff;
		switch ((status_line>>4) & 0x3)
		{
			case 1:
				size_t s1 = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
				size_t s2 = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
				snprintf(tb, 40, "IRAM=%dKB SPI=%dKB",s1/1024,s2/1024);
				break;
			case 2:
				snprintf(tb, 40, wlan_state);
				break;
			case 3:
				snprintf(tb, 40, "Version 2.0 beta 1");
				break;
			default:
				if (bsyn_clock_diff>0 && bsyn_clock_frame>0)
				{
					double hfreq = 48000000.0f / (double)bsyn_clock_diff; // 200 Zeilen * 240mhz = 48000000000
					double vfreq = 240000000.0f / (double)bsyn_clock_frame; // 240mhz
					snprintf(tb, 40, "H=%.3fkHz V=%.3fHz",hfreq,vfreq);
				}
				else
				{
					snprintf(tb, 40, "H=none      V=none");
				}
				break;
		}
		draw_text_color(tb,OSD_WIDTH/font_w,0,OSD_HIGHT-font_h-1,Color_Disabled,Color_Back);
	}
}

// Farbschema je nach Computer-Typ in Variable kopieren
void set_colorscheme()
{
	if (Current_Color_Scheme == _COLORSCHEME_COUNT)
	{
		for (int i=0;i<4;i++)
		{
			Current_Colors[i] = Custom_Colors[i];
		}
	}
	else
	{
		for (int i=0;i<4;i++)
		{
			Current_Colors[i] = _STATIC_COLOR_VALS[Current_Color_Scheme].colors[_STATIC_SYS_VALS[ACTIVESYS].swap_colors[i]];
		}
	}
}

bool is_visable(uint8_t row)
{
	if (row>sizeof(osd_menu)/sizeof(struct MENUITEM)) return false;
	if (osd_menu[row].typ==MT_STOP) return false;
	if (osd_menu[row].typ==MT_ENUM && osd_menu[row].value1==4 && Current_Color_Scheme<_COLORSCHEME_COUNT) return false;
	if (osd_menu[row].typ==MT_ENUM && osd_menu[row].value1==1 && _SETTINGS_COUNT!=ACTIVESYS) return false;
	if (osd_menu[row].typ==MT_ENUM && osd_menu[row].value1==9 && this_app_id==next_app_id) return false;
	return true;
}

bool is_selecatable(uint8_t row)
{
	if (!is_visable(row)) return false;
	if (osd_menu[row].typ==MT_SEPARATOR) return false;
	return true;
}

char* auth_mode(int authmode)
{
    switch (authmode) 
	{
		case WIFI_AUTH_OPEN: return "OPEN";
		case WIFI_AUTH_OWE:  return "OWE";
		case WIFI_AUTH_WEP:  return "WEP";
		case WIFI_AUTH_WPA_PSK: return "WPA-PSK";
		case WIFI_AUTH_WPA2_PSK: return "WPA2-PSK";
		case WIFI_AUTH_WPA_WPA2_PSK: return "WPA-WPA2-PSK";
		case WIFI_AUTH_ENTERPRISE: return "ENTERPRISE";
		case WIFI_AUTH_WPA3_PSK: return "WPA3-PSK";
		case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2-WPA3-PSK";
		case WIFI_AUTH_WPA3_ENT_192: return "WPA3-ENT-192";
		default: return "UNKNOWN";
    }
}


// das OSD-Menue
void osd_task(void*)
{
	enable_osd(false);
	// Taster-Pins einstellen
	gpio_config_t pincfg =
	{
		.pin_bit_mask = 1ULL<<PIN_NUM_TAST_LEFT | 1ULL<<PIN_NUM_TAST_UP | 1ULL<<PIN_NUM_TAST_DOWN | 1ULL<<PIN_NUM_TAST_RIGHT,
	    .mode = GPIO_MODE_INPUT,
	    .pull_up_en = true,
	    .pull_down_en = false,
	};
	ESP_ERROR_CHECK(gpio_config(&pincfg));

	uint8_t MAP_PIN_LEFT = PIN_NUM_TAST_LEFT;
	uint8_t MAP_PIN_UP = PIN_NUM_TAST_UP;
	uint8_t MAP_PIN_DOWN = PIN_NUM_TAST_DOWN;
	uint8_t MAP_PIN_RIGHT = PIN_NUM_TAST_RIGHT;

	for (int h=0;h<OSD_HIGHT;h++)
	{
		for (int i=0;i<OSD_WIDTH;i++) 
		{
			OSD_BUF[h][i]=Color_Back;
		}
		OSD_BUF[h][0]=Color_Frame;
		OSD_BUF[h][OSD_WIDTH-1]=Color_Frame;
	}
	for (int i=0;i<OSD_WIDTH;i++)
	{
		OSD_BUF[0][i]=Color_Frame;
		OSD_BUF[OSD_HIGHT-1][i]=Color_Frame;
		OSD_BUF[11*font_h][i]=Color_Frame;
		OSD_BUF[15*font_h+2][i]=Color_Frame;
	}

	menu_sel=1;

	char* pw = heap_caps_malloc(64, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);
	char* tb = heap_caps_malloc(256, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);
	osd_ovrstatus = heap_caps_malloc(25, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);

	memset(pw,0,64);
	memset(osd_ovrstatus,0,25);

	while (1)
	{
		// Tasten-Rotation anwenden ********************************************************************************************************************
		switch (OSD_KEY_ROTATE)
		{
			case 0: 
				MAP_PIN_UP = PIN_NUM_TAST_UP;
				MAP_PIN_RIGHT = PIN_NUM_TAST_RIGHT;
				MAP_PIN_DOWN = PIN_NUM_TAST_DOWN;
				MAP_PIN_LEFT = PIN_NUM_TAST_LEFT;
				break;
			case 1: 
				MAP_PIN_UP = PIN_NUM_TAST_LEFT;
				MAP_PIN_RIGHT = PIN_NUM_TAST_UP;
				MAP_PIN_DOWN = PIN_NUM_TAST_RIGHT;
				MAP_PIN_LEFT = PIN_NUM_TAST_DOWN;
				break;
			case 2: 
				MAP_PIN_UP = PIN_NUM_TAST_DOWN;
				MAP_PIN_RIGHT = PIN_NUM_TAST_LEFT;
				MAP_PIN_DOWN = PIN_NUM_TAST_UP;
				MAP_PIN_LEFT = PIN_NUM_TAST_RIGHT;
				break;
			case 3: 
				MAP_PIN_UP = PIN_NUM_TAST_RIGHT;
				MAP_PIN_RIGHT = PIN_NUM_TAST_DOWN;
				MAP_PIN_DOWN = PIN_NUM_TAST_LEFT;
				MAP_PIN_LEFT = PIN_NUM_TAST_UP;
				break;
		}

		uint32_t i = 0;
		uint32_t l = 0;

		// Menü ausgeben **********************************************************************************************************************************
		do
		{
			if (i<=menu_top+10 && i>=menu_top)
			{
				while (!is_visable(l)) l++;
				if (menu_sel==l) menu_row = i;
				switch (osd_menu[l].typ)
				{
					case MT_SEPARATOR: 
						for (uint16_t y=1;y<font_h;y++)
						{
							for (uint16_t x=1;x<OSD_WIDTH-2;x++)
							{
								OSD_BUF[(i-menu_top)*font_h+y][x] = Color_Back;
							}
						}
						for (uint16_t x=3;x<OSD_WIDTH-5;x++)
						{
							OSD_BUF[(i-menu_top)*10+5][x] = Color_Statictext;
							OSD_BUF[(i-menu_top)*10+7][x] = Color_Statictext;
						}
						draw_text_color(osd_menu[l].name[Language],255,2*font_w,(i-menu_top)*font_h,Color_Statictext,Color_Back);
						break;
					case MT_REALVALUE:
						draw_text_select(osd_menu[l].name[Language],16,0,i-menu_top,menu_sel==l);
						double* d = (double*)osd_menu[l].value1;
						switch (osd_menu[l].value4)
						{
							case 0:	snprintf(tb, 50, "%.0f",*d);break;
							case 1:	snprintf(tb, 50, "%.1f",*d);break;
							case 2:	snprintf(tb, 50, "%.2f",*d);break;
							case 3:	snprintf(tb, 50, "%.3f",*d);break;
							case 4:	snprintf(tb, 50, "%.4f",*d);break;
						}
						draw_text_value(tb,i);
						break;
					case MT_INTVALUE:
						draw_text_select(osd_menu[l].name[Language],16,0,i-menu_top,menu_sel==l);
						uint32_t* f = (uint32_t*)osd_menu[l].value1;
						snprintf(tb, 50, "%ld",*f);
						draw_text_value(tb,i);
						break;
					case MT_ENUM:
						draw_text_select(osd_menu[l].name[Language],16,0,i-menu_top,menu_sel==l);
						switch (osd_menu[l].value1)
						{
							case 0: //Computer-Profil
								if ((menu_sel==l) && (menu_subsel!=0))
								{
									if (menu_subsel == _SETTINGS_COUNT+1)
									{
										draw_text_value(TextCustoms[Language],i);
									}
									else
									{
										draw_text_value(_STATIC_SYS_VALS[menu_subsel-1].name,i);
									}
								}
								else
								{
									draw_text_value(_STATIC_SYS_VALS[ACTIVESYS].name,i);
								}
								break;
							case 1: //SPI 4bit / 8bit
								draw_text_value(ABG_Bits_per_sample==4 ? "4 Bit" : "8 Bit",i);
								break;
							case 2: //VGA-Auflösung
								draw_text_value(_STATIC_VGA_VALS[ACTIVEVGA].name,i);
								break;
							case 3: //Farbschema, Auswahl
								draw_text_value(Current_Color_Scheme==_COLORSCHEME_COUNT ? TextCustoms[Language] : _STATIC_COLOR_VALS[Current_Color_Scheme].name[Language],i);
								break;
							case 4: //Farbschema, Anwenderfarben
								// 0=rgb 1=rgb 2=rgb 3=rgb
								for (uint8_t m=0;m<4;m++)
								{
									for (uint8_t n=0;n<4;n++)
									{
										tb[m*4+n] = ((Custom_Colors[m] >> (n*2)) & 3) + '0';
									}
									tb[m*4+3] = ' ';
								}
								tb[15] = 0;
								draw_text_value(tb,i);
								if ((menu_sel==l) && (menu_subsel!=0))
								{
									uint8_t o = (menu_subsel-1) / 3;
									uint8_t p = (menu_subsel-1) % 3;
									draw_text_color(&tb[o*4+p],1,(o*4+p+7)*font_w,(i-menu_top)*font_h,Color_TextSelect,Color_BackSelect);
								}
								break;
							case 5: //Wlan-Modus
								switch (((menu_subsel!=0) && (menu_sel==l)) ? menu_subsel-1 : wlan_mode)
								{
									case 0:
										draw_text_value(TextOFF[Language],i);
										break;
									case 1:
										draw_text_value("WPS",i);
										break;
									case 2:
										draw_text_value("Station",i);
										break;
									case 3:
										draw_text_value("Hotspot",i);
										break;
								}
								break;
							case 6: //WPS-Setup
								draw_text_value("fehlt",i);
								break;
							case 7: //Wlan SSID
								if (wlan_mode==3)
								{
									draw_text_value(ap_ssid,i);
								}
								else
								{
									if (menu_sel==l && menu_subsel!=0 && ap_list!=NULL)
									{
										draw_text_value((char *)ap_list[menu_subsel-1].ssid,i);
									}
									else
									{
										if (wlan_ssid[0]==0)
										{
											snprintf(tb,10,"\x82 %s",TextScan[Language]);
											draw_text_value(tb,i);
										}
										else
										{
											draw_text_value(wlan_ssid,i);
										}
									}
								}
								break;
							case 8: //Wlan Passwort
								if (wlan_mode==3)
								{
									draw_text_value(TextNone[Language],i);
								}
								else
								{
									if (menu_sel==l && menu_subsel!=0)
									{
										for (uint8_t k=0;k<64;k++)
										{
											if (pw[k]==0)
											{
												tb[k]=0x7f;
												tb[k+1]=0;
												break;
											}
											tb[k]=pw[k];
										}
										uint8_t k = menu_subsel>11 ? menu_subsel-11 : 0;
										draw_text_color(" ",1,9*font_w,(i-menu_top)*font_h,Color_Values,Color_Back);
										draw_text_color(&tb[k],12,10*font_w,(i-menu_top)*font_h,Color_Values,Color_Back);
										draw_text_color(&tb[menu_subsel-1],1,(9+menu_subsel-k)*font_w,(i-menu_top)*font_h,Color_TextSelect,Color_BackSelect);
									}
									else
									{
										if (wlan_passwd[0]==0)
										{
											snprintf(tb,11,"\x82 %s",TextEnter[Language]);
											draw_text_value(tb,i);
										}
										else
										{
											snprintf(tb,10,"(%s)",TextSet[Language]);
											draw_text_value(tb,i);
										}
									}
								}
								break;
							case 9: //Streaming aktivieren
								snprintf(tb, 50, "%s 2s\x82",TextOFF[Language]);
								draw_text_value(tb,i);
								break;
							case 10: //Sprache
								draw_text_value(TextLanguage[Language],i);
								break;
							case 11: //Transparenz einstellen
								draw_text_value(OSD_TRANSPARENT ? TextON[Language] : TextOFF[Language], i);
								break;
							case 12: //Keybinding
								snprintf(tb, 50, "%d",(((menu_subsel!=0) && (menu_sel==l)) ? menu_subsel-1 : OSD_KEY_ROTATE)*90);
								draw_text_value(tb,i);
								break;
							case 13: //Load defaults aktion
							case 14: //alles speichern
								draw_text_value("2s\x82",i);
								break;
						}
						break;
					case MT_STOP:
						break;
				}
			}
			i++;
			l++;
		}
		while ((osd_menu[i].typ != MT_STOP));
	
		// Beschreibungs-Text schreiben ****************************************************************************************************************
		for (uint16_t y=0;y<font_h*4;y++)
		{
			for (uint16_t x=1; x<OSD_WIDTH-1;x++)
			{
				OSD_BUF[y+11*font_h+1][x]=Color_Back;
			}
		}
		if (osd_menu[menu_sel].typ==MT_ENUM && osd_menu[menu_sel].value1==7 && menu_subsel!=0)
		{
			snprintf(tb,256,"%s\n%ddb Ch%d\n%s\n(%d/%d)",ap_list[menu_subsel-1].ssid,ap_list[menu_subsel-1].rssi,ap_list[menu_subsel-1].primary,auth_mode(ap_list[menu_subsel-1].authmode),menu_subsel,ap_count);
			draw_text_color(tb,255,0,11*font_h+1,Color_Statictext,Color_Back);
		}
		else
		{
			draw_text_color(osd_menu[menu_sel].description[Language],255,0,11*font_h+1,Color_Statictext,Color_Back);
		}

		l = osd_repeat ? 10 : 50;
		// darauf warten, dass alle Tasten losgelassen werden *******************************************************************************************
		while (gpio_get_level(MAP_PIN_LEFT)==0 || gpio_get_level(MAP_PIN_UP)==0 || gpio_get_level(MAP_PIN_DOWN)==0 || gpio_get_level(MAP_PIN_RIGHT)==0)
		{
			usleep(10000);
			l--;
			if (l==0)
			{
				osd_repeat = true;
				break;
			}
		}

		i = 1023;
		// darauf warten, dass eine Taste gedrückt wird **************************************************************************************************
		while (gpio_get_level(MAP_PIN_LEFT)!=0 && gpio_get_level(MAP_PIN_UP)!=0 && gpio_get_level(MAP_PIN_DOWN)!=0 && gpio_get_level(MAP_PIN_RIGHT)!=0)
		{
			osd_repeat = false;
			if ((i & 15)==15) draw_status_line();
			usleep(10000);
			if (i==1)
			{
				if (ABG_RUN && menu_subsel==0)
				{
					enable_osd(false);
				}
				else
				{
					break;
				}
			}
			if (i>0) i--;
		}
		enable_osd(true);

		// Button: UP **********************************************************************************************************************
		if (gpio_get_level(MAP_PIN_UP)==0)
		{
			if (menu_subsel!=0)
			{
				switch (osd_menu[menu_sel].typ)
				{
					case MT_ENUM:
						switch (osd_menu[menu_sel].value1)
						{
							case 0: //Computer-Profil
								if (osd_repeat)
								{
									nvs_set_u16(sys_nvs_handle, _NVS_SETTING_MODE, menu_subsel-1);
									esp_restart();
								}
								break;
							case 4: //Farbschema, Anwenderfarben
								uint8_t o = (menu_subsel-1) / 3;
								uint8_t p = (menu_subsel-1) % 3;
								uint8_t q = 3<<(p*2);
								Custom_Colors[o] = (((Custom_Colors[o] & q)+0b10101) & q) | (Custom_Colors[o] & (q^0xff));
								set_colorscheme();
								break;
							case 5: //Wlan-modus
								if (osd_repeat)
								{
									wlan_mode = menu_subsel-1;
									setup_wlan(wlan_mode);
									menu_subsel = 0;
									osd_ovrstatus[0] = 0;
								}
								break;
							case 7: // SSID
								if (osd_repeat)
								{
									snprintf(wlan_ssid,64,(char*)ap_list[menu_subsel-1].ssid);
									menu_subsel = 0;
									osd_ovrstatus[0] = 0;
									if (ap_list != NULL)
									{
										free(ap_list);
										ap_list = NULL;
									}
								}
								break;
							case 8: // passwort
								char a = pw[menu_subsel-1];
								if (a=='/') a=0;
								else if (a==0) a='0';
								else if (a>=0x7d) a=0x20;
								else a++;
								pw[menu_subsel-1]=a;
								break;
							case 12: // Keybinding
								if (osd_repeat)
								{
									OSD_KEY_ROTATE = menu_subsel-1;
									menu_subsel = 0;
									osd_ovrstatus[0] = 0;
									while (gpio_get_level(MAP_PIN_LEFT)==0 || gpio_get_level(MAP_PIN_UP)==0 || gpio_get_level(MAP_PIN_DOWN)==0 || gpio_get_level(MAP_PIN_RIGHT)==0);
								}
								break;
							default:
								break;
						}
					default:
						break;
				}
			}
			else if (!osd_repeat)
			{
				uint8_t old = menu_sel;
				do
				{
					menu_sel--;
					if (menu_sel==0)
					{
						menu_sel = old;
						break;
					}
				}
				while (!is_selecatable(menu_sel));
				if (menu_top > (menu_sel-1))
				{
					menu_top = menu_sel - 1;
				}
			}
		}

		// Button: Down **********************************************************************************************************************
		if (gpio_get_level(MAP_PIN_DOWN)==0)
		{
			if (menu_subsel!=0)
			{
				switch (osd_menu[menu_sel].typ)
				{
					case MT_ENUM:
						switch (osd_menu[menu_sel].value1)
						{
							case 0: //Computer-Profil
							case 5: //Wlan-modus
							case 12: // Keybinding
								menu_subsel = 0;
								osd_ovrstatus[0] = 0;
								while (gpio_get_level(MAP_PIN_LEFT)==0 || gpio_get_level(MAP_PIN_UP)==0 || gpio_get_level(MAP_PIN_DOWN)==0 || gpio_get_level(MAP_PIN_RIGHT)==0);								
								break;
							case 4: //Farbschema, Anwenderfarben
								uint8_t o = (menu_subsel-1) / 3;
								uint8_t p = (menu_subsel-1) % 3;
								uint8_t q = 3<<(p*2);
								Custom_Colors[o] = (((Custom_Colors[o] & q)-(0b10101 & q)) & q) | (Custom_Colors[o] & (q^0xff));
								set_colorscheme();
								break;
							case 7: //SSID
								menu_subsel = 0;
								osd_ovrstatus[0] = 0;
								if (ap_list != NULL)
								{
									free(ap_list);
									ap_list = NULL;
								}
								break;
							case 8: //passwort
								char a = pw[menu_subsel-1];
								if (a=='0') a=0;
								else if (a==0) a='/';
								else if (a<=0x20) a=0x7d;
								else a--;
								pw[menu_subsel-1]=a;
								break;
							default:
								break;
						}
					default:
						break;
				}
			}
			else if (!osd_repeat)
			{
				uint8_t old = menu_sel;
				do
				{
					menu_sel++;
					if (osd_menu[menu_sel].typ==MT_STOP)
					{
						menu_sel = old;
						break;
					}
				}
				while (!is_selecatable(menu_sel));
				uint8_t m=menu_sel;
				do
				{
					m++;
					if (osd_menu[m].typ==MT_STOP)
					{
						m = menu_sel;
						break;
					}
				}
				while (!is_selecatable(menu_sel));
				uint8_t n = 0;
				for (; m!=0; m--)
				{
					if (is_visable(m)) n++;
					if (n==11) menu_top = m;
					if (m<menu_top) break;
				}
			}
		}

		// Button: Left **********************************************************************************************************************
		if (gpio_get_level(MAP_PIN_LEFT)==0)
		{
			switch (osd_menu[menu_sel].typ)
			{
				case MT_INTVALUE:
					uint32_t* v = (uint32_t*)osd_menu[menu_sel].value1;
					if (*v>osd_menu[menu_sel].value2) *v = *v - 1;
					break;
				case MT_REALVALUE:
					double* w = (double*)osd_menu[menu_sel].value1;
					if (*w>osd_menu[menu_sel].value2) *w = *w - ((double)osd_menu[menu_sel].value5 / (osd_repeat ? 200.0 : 1000.0));
					break;
				case MT_ENUM:
					switch (osd_menu[menu_sel].value1)
					{
						case 0: //Computer-Profil
							if (menu_subsel==0)
							{
								menu_subsel = ACTIVESYS;
							}
							else
							{
								menu_subsel -= 1;
							}
							if (menu_subsel==0)
							{
								menu_subsel = _SETTINGS_COUNT; // _SETTINGS_COUNT+1 für Custom Profil - dieses Feature vervollständigen wir später
							}
							snprintf(osd_ovrstatus,25,"2s\x80%s \x81%s",TextAccept[Language],TextCancel[Language]);
							break;
						case 1: // SPI 4bit / 8bit // für Custom Profil - dieses Feature vervollständigen wir später
							break;
						case 2: // VGA-Auflösung
							do
							{
								ACTIVEVGA--;
								if (ACTIVEVGA>_VGAMODE_COUNT) ACTIVEVGA = _VGAMODE_COUNT;
							}
							while (((1 << ACTIVEVGA) & _STATIC_SYS_VALS[ACTIVESYS].accept_vga_modes) == 0);
							setup_vga_mode();
							break;
						case 3: // Farbschema, Auswahl
							Current_Color_Scheme--;
							if (Current_Color_Scheme>_COLORSCHEME_COUNT) Current_Color_Scheme = _COLORSCHEME_COUNT;
							set_colorscheme();
							break;
							if (menu_subsel>0 && !osd_repeat) menu_subsel--;
							break;
							break;
						case 4: // Farbschema, Anwenderfarben
							if (menu_subsel>0 && !osd_repeat) menu_subsel--;
							break;
						case 5: // Wlan-Modus
							if (menu_subsel==0)
							{
								menu_subsel = wlan_mode+1;
								snprintf(osd_ovrstatus,25,"2s\x80%s \x81%s",TextAccept[Language],TextCancel[Language]);
							}
							if (menu_subsel>1 && !osd_repeat) menu_subsel--;
							break;
						case 7: // Wlan SSID
							if (menu_subsel>1 && !osd_repeat) menu_subsel--;
							break;
						case 8: // Wlan Passwort
							if (!osd_repeat && menu_subsel>1)
							{
								menu_subsel--;
								break;
							}
							if (osd_repeat)
							{
								memcpy(wlan_passwd,pw,64);
								menu_subsel = 0;
								osd_ovrstatus[0] = 0;
								setup_wlan(wlan_mode);
								while (gpio_get_level(MAP_PIN_LEFT)==0 || gpio_get_level(MAP_PIN_UP)==0 || gpio_get_level(MAP_PIN_DOWN)==0 || gpio_get_level(MAP_PIN_RIGHT)==0);
							}
							break;
						case 9: // Streaming aktivieren
							break;
						case 10: // Sprache
							Language = (Language-1) & 1;
							break;
						case 11: // Transparenz einstellen
							enable_osd(false);
							OSD_TRANSPARENT = ! OSD_TRANSPARENT;
							enable_osd(true);
							break;
						case 12: // Keybinding
							if (menu_subsel==0)
							{
								menu_subsel = OSD_KEY_ROTATE+1;
								snprintf(osd_ovrstatus,25,"2s\x80%s \x81%s",TextAccept[Language],TextCancel[Language]);
							}
							if (menu_subsel>1) menu_subsel--;
							break;
					}
					break;
				default:
				    break;
			}
		}

		// Button: Right **********************************************************************************************************************
		if (gpio_get_level(MAP_PIN_RIGHT)==0)
		{
			switch (osd_menu[menu_sel].typ)
			{
				case MT_INTVALUE:
					uint32_t* v = (uint32_t*)osd_menu[menu_sel].value1;
					if (*v<osd_menu[menu_sel].value3) *v = *v + 1;
					break;
				case MT_REALVALUE:
					double* w = (double*)osd_menu[menu_sel].value1;
					if (*w<osd_menu[menu_sel].value3) *w = *w + ((double)osd_menu[menu_sel].value5 / (osd_repeat ? 200.0 : 1000.0));
					break;
				case MT_ENUM:
					switch (osd_menu[menu_sel].value1)
					{
						case 0: //Computer-Profil
							if (menu_subsel==0)
							{
								menu_subsel = ACTIVESYS+2;
							}
							else
							{
								menu_subsel += 1;
							}
							if (menu_subsel>_SETTINGS_COUNT)  // _SETTINGS_COUNT+1 für Custom Profil - dieses Feature vervollständigen wir später
							{
								menu_subsel = 1;
							}
							snprintf(osd_ovrstatus,25,"2s\x80%s \x81%s",TextAccept[Language],TextCancel[Language]);
							break;
						case 1: // SPI 4bit / 8bit // für Custom Profil - dieses Feature vervollständigen wir später
							break;
						case 2: // VGA-Auflösung
							do
							{
								ACTIVEVGA++;
								if (ACTIVEVGA>=_VGAMODE_COUNT) ACTIVEVGA = 0;
							}
							while (((1 << ACTIVEVGA) & _STATIC_SYS_VALS[ACTIVESYS].accept_vga_modes) == 0);
							setup_vga_mode();
							break;
						case 3: // Farbschema, Auswahl
							Current_Color_Scheme++;
							if (Current_Color_Scheme>_COLORSCHEME_COUNT) Current_Color_Scheme = 0;
							set_colorscheme();
							break;
						case 4: // Farbschema, Anwenderfarben
							if (menu_subsel<12 && !osd_repeat) menu_subsel++;
							break;
						case 5: // Wlan-Modus
							if (menu_subsel==0)
							{
								menu_subsel = wlan_mode+1;
								snprintf(osd_ovrstatus,25,"2s\x80%s \x81%s",TextAccept[Language],TextCancel[Language]);
							}
							if (menu_subsel<4 && !osd_repeat) menu_subsel++;
							break;
						case 7: // Wlan SSID
							if (menu_subsel>0)
							{
								if (menu_subsel<ap_count && !osd_repeat) menu_subsel++;
							}
							else
							{
								if (ap_list==NULL)
								{
									ap_list = heap_caps_malloc(sizeof(wifi_ap_record_t) * 64, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM);;
								}
								draw_text_value("       Scan...", menu_row);
								wifi_scan(&ap_count,ap_list);
								menu_subsel = 1;
								snprintf(osd_ovrstatus,25,"2s\x80%s \x81%s",TextAccept[Language],TextCancel[Language]);
							}
							break;
						case 8: // Wlan Passwort
							if (menu_subsel>0)
							{
								if (pw[menu_subsel-1]!=0 && !osd_repeat) menu_subsel++;
								else if (osd_repeat)
								{
									menu_subsel = 0;
									osd_ovrstatus[0] = 0;
									while (gpio_get_level(MAP_PIN_LEFT)==0 || gpio_get_level(MAP_PIN_UP)==0 || gpio_get_level(MAP_PIN_DOWN)==0 || gpio_get_level(MAP_PIN_RIGHT)==0);
								}
							}
							else
							{
								memset(pw,0,64);
								menu_subsel = 1;
								snprintf(osd_ovrstatus,25,"2s\x83%s \x82%s",TextAccept[Language],TextCancel[Language]);
								while (gpio_get_level(MAP_PIN_LEFT)==0 || gpio_get_level(MAP_PIN_UP)==0 || gpio_get_level(MAP_PIN_DOWN)==0 || gpio_get_level(MAP_PIN_RIGHT)==0);
							}
							break;
						case 9: // Streaming aktivieren
							if (osd_repeat && next_app_id!=this_app_id) 
							{
								const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, next_app_id, NULL);
								if (part!=NULL)
								{
									esp_ota_set_boot_partition(part);
									esp_restart();
								}
							}
							break;
						case 10: // Sprache
							Language = (Language+1) & 1;
							break;
						case 11: // Transparenz einstellen
							enable_osd(false);
							OSD_TRANSPARENT = ! OSD_TRANSPARENT;
							enable_osd(true);
							break;
						case 12: // Keybinding
							if (menu_subsel==0)
							{
								menu_subsel = OSD_KEY_ROTATE+1;
								snprintf(osd_ovrstatus,25,"2s\x80%s \x81%s",TextAccept[Language],TextCancel[Language]);
							}
							if (menu_subsel<4) menu_subsel++;
							break;
						case 13: // profil-defaults laden
							if (osd_repeat)
							{
								BSYNC_PIXEL_ABSTAND = (float)_STATIC_SYS_VALS[ACTIVESYS].default_pixel_abstand / 100;
								ABG_START_LINE = _STATIC_SYS_VALS[ACTIVESYS].default_start_line;
								ABG_PIXEL_PER_LINE = (float)_STATIC_SYS_VALS[ACTIVESYS].default_pixel_per_line / 100;
								if (ACTIVEVGA != _STATIC_SYS_VALS[ACTIVESYS].default_vga_mode)
								{
									ACTIVEVGA = _STATIC_SYS_VALS[ACTIVESYS].default_vga_mode;
									setup_vga_mode();
								}
								draw_text_value(TextLoaded[Language], menu_row);
								while (gpio_get_level(MAP_PIN_LEFT)==0 || gpio_get_level(MAP_PIN_UP)==0 || gpio_get_level(MAP_PIN_DOWN)==0 || gpio_get_level(MAP_PIN_RIGHT)==0);
							}
							break;
						case 14: // alles speichern
							if (osd_repeat)
							{
								write_settings(true);
								draw_text_value(TextSaved[Language], menu_row);
								while (gpio_get_level(MAP_PIN_LEFT)==0 || gpio_get_level(MAP_PIN_UP)==0 || gpio_get_level(MAP_PIN_DOWN)==0 || gpio_get_level(MAP_PIN_RIGHT)==0);
							}
							break;
					}
				default:
				    break;
			}
		}
	}
}
