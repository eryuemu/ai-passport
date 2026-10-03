// main/demo_cover.c —— 蓝色大肥鱼二创全屏封面展示
#include "demo_cover.h"
#include "demo.h"
#include "bsp_display.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "lvgl.h"
#include <string.h>

static const char *TAG = "demo_cover";

extern const uint8_t deepseek_cover_bin_start[] asm("_binary_deepseek_cover_bin_start");
extern const uint8_t deepseek_cover_bin_end[]   asm("_binary_deepseek_cover_bin_end");

#define SLICE_LINES 16
#define SLICE_BYTES (240 * SLICE_LINES * sizeof(uint16_t))
static uint16_t s_slice_buf[240 * SLICE_LINES];
static lv_obj_t *s_cover_scr = NULL;

void demo_cover_enter(void) {
    ESP_LOGI(TAG, "推流展示蓝色大肥鱼二创全屏立绘");

    // 创建合法的 LVGL 屏幕，保证 LVGL 调度器始终拥有 active screen
    s_cover_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_cover_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_cover_scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_pad_all(s_cover_scr, 0, 0);
    lv_screen_load(s_cover_scr);

    esp_lcd_panel_handle_t panel = bsp_display_panel();
    if (!panel) return;

    // 确保竖屏
    esp_lcd_panel_swap_xy(panel, false);
    esp_lcd_panel_mirror(panel, false, false);

    // 分片直接刷屏 (16 行一批，20 批完成)
    // deepseek_cover.bin 已经是 Big-Endian RGB565，直接 DMA 刷屏
    const uint16_t *src = (const uint16_t *)deepseek_cover_bin_start;
    for (int y = 0; y < 320; y += SLICE_LINES) {
        memcpy(s_slice_buf, &src[y * 240], SLICE_BYTES);
        esp_lcd_panel_draw_bitmap(panel, 0, y, 240, y + SLICE_LINES, s_slice_buf);
    }
}

void demo_cover_exit(void) {
    ESP_LOGI(TAG, "退出蓝色大肥鱼封面");
    if (s_cover_scr) {
        lv_obj_delete(s_cover_scr);
        s_cover_scr = NULL;
    }
}

void demo_cover_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    (void)btn;
    // 任意按键触发返回主菜单
    if (ev == BSP_BTN_PRESS || ev == BSP_BTN_CLICK || ev == BSP_BTN_LONG || ev == BSP_BTN_DOUBLE) {
        demo_request_exit();
    }
}

esp_err_t demo_cover_start(void) {
    return ESP_OK;
}

esp_err_t demo_cover_stop(void) {
    return ESP_OK;
}
