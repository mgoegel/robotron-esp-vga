#include "globalvars.h"
#include "pins.h"
#include <hal/gpio_hal.h>
#include <driver/periph_ctrl.h>
#include <driver/gpio.h>
#include <soc/lcd_cam_struct.h>
#include <math.h>
#include <esp_private/gdma.h>
#include <hal/dma_types.h>

gdma_channel_handle_t dmaCh = NULL;
dma_descriptor_t* descriptors = NULL;
bool osd_enabled = true;

#define HAL_FORCE_MODIFY_U32_REG_FIELD(base_reg, reg_field, field_val)    \
{                                                           \
	uint32_t temp_val = base_reg.val;                       \
	typeof(base_reg) temp_reg;                              \
	temp_reg.val = temp_val;                                \
	temp_reg.reg_field = (field_val);                       \
	(base_reg).val = temp_reg.val;                          \
}

void attachPinToSignal(int pin, int signal)
{
	esp_rom_gpio_connect_out_signal(pin, signal, false, false);
	gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[pin], PIN_FUNC_GPIO);
	gpio_set_drive_capability((gpio_num_t)pin, (gpio_drive_cap_t)3);
}

void enable_osd(bool enable)
{
    if (enable && !osd_enabled)
    {
        if (OSD_TRANSPARENT)
        {
            descriptors[ABG_YRes+OSD_HIGHT+OSD_HIGHT-1].next = &descriptors[ABG_YRes+OSD_HIGHT+OSD_HIGHT];
            descriptors[ABG_YRes+OSD_HIGHT+OSD_HIGHT+ABG_YRes-1].next = descriptors;
        }
        else
        {
            descriptors[ABG_YRes+OSD_HIGHT+OSD_HIGHT-1].next = descriptors;
            descriptors[ABG_YRes+OSD_HIGHT+OSD_HIGHT+ABG_YRes-1].next = descriptors;
        }
        osd_enabled = true;
    }
    if (!enable && osd_enabled)
    {
        descriptors[ABG_YRes+OSD_HIGHT+OSD_HIGHT-1].next = &descriptors[ABG_YRes+OSD_HIGHT+OSD_HIGHT];
        descriptors[ABG_YRes+OSD_HIGHT+OSD_HIGHT+ABG_YRes-1].next = &descriptors[ABG_YRes+OSD_HIGHT+OSD_HIGHT];
        osd_enabled = false;
    }
}

// VGA-Buffer holen, Aufruf nur einmal erlaubt
void alloc_vga_buffer()
{
    // Zeilen-Zeigerliste für DMA-Kontroller
    descriptors = (dma_descriptor_t *)heap_caps_aligned_calloc(4, 1, ((ABG_YRes*2)+(OSD_HIGHT*2)+2) * sizeof(dma_descriptor_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!descriptors)
    {
        printf("Fehler: descriptors==null\n");
        return;
    }

    // VGA-Zeilen-Zeigerliste für uns
    VGA_BUF = heap_caps_aligned_calloc(4, 1, ABG_YRes * 4, MALLOC_CAP_INTERNAL);
    if (!VGA_BUF)
    {
        printf("Fehler: VGA_BUF==null\n");
        return;
    }

    // OSD-Zeilen-Zeigerliste für uns
    OSD_BUF = heap_caps_malloc(OSD_HIGHT * 4, MALLOC_CAP_SPIRAM);
    if (!OSD_BUF)
    {
        printf("Fehler: OSD_BUF==null\n");
        return;
    }

    // Pufferspeicher der einzelnen Zeilen
    for (int i=0; i<ABG_YRes; i++) 
    {
        VGA_BUF[i] = heap_caps_aligned_calloc(4, 1, ABG_XRes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
        if (!VGA_BUF[i])
        {
            printf("Fehler: VGA_BUF[%d]==null\n",i);
            return;
        }
    }

    // Pufferspeicher der OSD-Zeilen
    for (int i=0; i<OSD_HIGHT; i++)
    {
        OSD_BUF[i] = heap_caps_aligned_calloc(64, 1, OSD_WIDTH, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
        if (!OSD_BUF[i])
        {
            printf("Fehler: OSD_BUF[%d]==null\n",i);
            return;
        }
    }
}

// DMA-Descriptoren zuweisen, kann wiederholt werden
void setup_vga_buffer()
{
    // Erster Teil der DMA-Tabelle ist ein Frame mit eingebundenem OSD-Menü
    int j=0;
    for (int i=0; i<ABG_YRes; i++) 
    {
        if (i<OSD_TOP || i>=(OSD_TOP+OSD_HIGHT))
        {
            // Zeilen ohne OSD komplett auf den VGA-Buffer linken
            descriptors[j].buffer = VGA_BUF[i];
            descriptors[j].dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_CPU;
            descriptors[j].dw0.suc_eof = 0;
            descriptors[j].next = &descriptors[j + 1];
            descriptors[j].dw0.size = ABG_XRes;
            descriptors[j].dw0.length = ABG_XRes;
            j++;
        }
        else
        {
            // Zeilen mit OSD erstmal ein Stück VGA-Buffer...
            descriptors[j].buffer = VGA_BUF[i];
            descriptors[j].dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_CPU;
            descriptors[j].dw0.suc_eof = 0;
            descriptors[j].next = &descriptors[j + 1];
            descriptors[j].dw0.size = OSD_LEFT;
            descriptors[j].dw0.length = OSD_LEFT;
            j++;
            // ... dann ein Stück OSD-Buffer...
            descriptors[j].buffer = OSD_BUF[i-OSD_TOP];
            descriptors[j].dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_CPU;
            descriptors[j].dw0.suc_eof = 0;
            descriptors[j].next = &descriptors[j + 1];
            descriptors[j].dw0.size = OSD_WIDTH;
            descriptors[j].dw0.length = OSD_WIDTH;
            j++;
            // ... und den Rest VGA.
            descriptors[j].buffer = VGA_BUF[i]+OSD_LEFT+OSD_WIDTH;
            descriptors[j].dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_CPU;
            descriptors[j].dw0.suc_eof = 0;
            descriptors[j].next = &descriptors[j + 1];
            descriptors[j].dw0.size = ABG_XRes-OSD_WIDTH-OSD_LEFT;
            descriptors[j].dw0.length = ABG_XRes-OSD_WIDTH-OSD_LEFT;
            j++;
        }
    }
    // Der zweite Teil der DMA-Tabelle ist ein Frame ohne OSD-Menü, nur VGA-Buffer
    for (int i=0; i<ABG_YRes; i++) 
    {
        descriptors[j].buffer = VGA_BUF[i];
        descriptors[j].dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_CPU;
        descriptors[j].dw0.suc_eof = 0;
        descriptors[j].next = &descriptors[j + 1];
        descriptors[j].dw0.size = ABG_XRes;
        descriptors[j].dw0.length = ABG_XRes;
        j++;
    }
    descriptors[j-1].next = descriptors;
}

// VGA mit der ausgewählten Auflösung starten, kann wiederholt werden
void setup_vga_mode()
{
	periph_module_enable(PERIPH_LCD_CAM_MODULE);
	periph_module_reset(PERIPH_LCD_CAM_MODULE);
	LCD_CAM.lcd_user.lcd_reset = 1;
	esp_rom_delay_us(100);

	int N = round(240000000.0/(double)_STATIC_VGA_VALS[ACTIVEVGA].frequency);
	if(N < 2) N = 2;
	LCD_CAM.lcd_clock.clk_en = 1;
	LCD_CAM.lcd_clock.lcd_clk_sel = 2;
	LCD_CAM.lcd_clock.lcd_clkm_div_a = 0;
	LCD_CAM.lcd_clock.lcd_clkm_div_b = 0;
	LCD_CAM.lcd_clock.lcd_clkm_div_num = N;
	LCD_CAM.lcd_clock.lcd_ck_out_edge = 0;		
	LCD_CAM.lcd_clock.lcd_ck_idle_edge = 0;
	LCD_CAM.lcd_clock.lcd_clk_equ_sysclk = 1;
	LCD_CAM.lcd_ctrl.lcd_rgb_mode_en = 1;
	LCD_CAM.lcd_user.lcd_2byte_en = 0;
    LCD_CAM.lcd_user.lcd_cmd = 0;
    LCD_CAM.lcd_user.lcd_dummy = 0;
    LCD_CAM.lcd_user.lcd_dout = 1;
    LCD_CAM.lcd_user.lcd_cmd_2_cycle_en = 0;
    LCD_CAM.lcd_user.lcd_dummy_cyclelen = 0;
    LCD_CAM.lcd_user.lcd_dout_cyclelen = 0;
	LCD_CAM.lcd_user.lcd_always_out_en = 1;

    LCD_CAM.lcd_ctrl2.lcd_hsync_idle_pol = _STATIC_VGA_VALS[ACTIVEVGA].hPol ^ 1;
    LCD_CAM.lcd_ctrl2.lcd_vsync_idle_pol = _STATIC_VGA_VALS[ACTIVEVGA].vPol ^ 1;
    LCD_CAM.lcd_ctrl2.lcd_de_idle_pol = 1;	

	LCD_CAM.lcd_misc.lcd_bk_en = 1;	
    LCD_CAM.lcd_misc.lcd_vfk_cyclelen = 0;
    LCD_CAM.lcd_misc.lcd_vbk_cyclelen = 0;

	LCD_CAM.lcd_ctrl2.lcd_hsync_width = _STATIC_VGA_VALS[ACTIVEVGA].hSync - 1; 
    LCD_CAM.lcd_ctrl.lcd_hb_front = _STATIC_VGA_VALS[ACTIVEVGA].hFront + _STATIC_VGA_VALS[ACTIVEVGA].hSync + _STATIC_VGA_VALS[ACTIVEVGA].hBack + ((_STATIC_VGA_VALS[ACTIVEVGA].hRes-ABG_XRes)/2) - 1;
    LCD_CAM.lcd_ctrl1.lcd_ha_width = ABG_XRes - 1;
    LCD_CAM.lcd_ctrl1.lcd_ht_width = _STATIC_VGA_VALS[ACTIVEVGA].hFront + _STATIC_VGA_VALS[ACTIVEVGA].hSync + _STATIC_VGA_VALS[ACTIVEVGA].hBack + _STATIC_VGA_VALS[ACTIVEVGA].hRes - 1;
	
    LCD_CAM.lcd_ctrl2.lcd_vsync_width = _STATIC_VGA_VALS[ACTIVEVGA].vSync - 1;
    HAL_FORCE_MODIFY_U32_REG_FIELD(LCD_CAM.lcd_ctrl1, lcd_vb_front, _STATIC_VGA_VALS[ACTIVEVGA].vSync + _STATIC_VGA_VALS[ACTIVEVGA].vBack + ((_STATIC_VGA_VALS[ACTIVEVGA].vRes-ABG_YRes)/2) - 1);
    LCD_CAM.lcd_ctrl.lcd_va_height = ABG_YRes - 1;
    LCD_CAM.lcd_ctrl.lcd_vt_height = _STATIC_VGA_VALS[ACTIVEVGA].vFront + _STATIC_VGA_VALS[ACTIVEVGA].vSync + _STATIC_VGA_VALS[ACTIVEVGA].vBack + _STATIC_VGA_VALS[ACTIVEVGA].vRes - 1;

	LCD_CAM.lcd_ctrl2.lcd_hs_blank_en = 1;
	HAL_FORCE_MODIFY_U32_REG_FIELD(LCD_CAM.lcd_ctrl2, lcd_hsync_position, 0);
	LCD_CAM.lcd_misc.lcd_next_frame_en = 1;

    if (!dmaCh) // Zuweisung des DMA-Kanals, und der Pins nur beim ersten mal machen
    {
        attachPinToSignal(PIN_NUM_VGA_B0, LCD_DATA_OUT0_IDX);
        attachPinToSignal(PIN_NUM_VGA_B1, LCD_DATA_OUT1_IDX);
        attachPinToSignal(PIN_NUM_VGA_G0, LCD_DATA_OUT2_IDX);
        attachPinToSignal(PIN_NUM_VGA_G1, LCD_DATA_OUT3_IDX);
        attachPinToSignal(PIN_NUM_VGA_R0, LCD_DATA_OUT4_IDX);
        attachPinToSignal(PIN_NUM_VGA_R1, LCD_DATA_OUT5_IDX);
        attachPinToSignal(PIN_NUM_VGA_HSYNC, LCD_H_SYNC_IDX);
        attachPinToSignal(PIN_NUM_VGA_VSYNC, LCD_V_SYNC_IDX);

        gdma_channel_alloc_config_t dma_chan_config = 
        {
            .direction = GDMA_CHANNEL_DIRECTION_TX,
        };
	    gdma_new_channel(&dma_chan_config, &dmaCh);
        gdma_connect(dmaCh, GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_LCD, 0));
        gdma_transfer_ability_t ability = 
        {
            .sram_trans_align = 4,
            .psram_trans_align = 64,
        };
        gdma_set_transfer_ability(dmaCh, &ability);
    }

	gdma_reset(dmaCh);
    esp_rom_delay_us(1);	
    LCD_CAM.lcd_user.lcd_start = 0;
    LCD_CAM.lcd_user.lcd_update = 1;
	esp_rom_delay_us(1);
	LCD_CAM.lcd_misc.lcd_afifo_reset = 1;
    LCD_CAM.lcd_user.lcd_update = 1;
	gdma_start(dmaCh, (intptr_t)descriptors);
    esp_rom_delay_us(1);
    LCD_CAM.lcd_user.lcd_update = 1;
	LCD_CAM.lcd_user.lcd_start = 1;
}
