// main/demo_cover.c —— 赛博朋克大肥鱼 DeepSeek 娘化形象全屏封面展示
#include "demo_cover.h"
#include "demo.h"
#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "demo_cover";

extern const uint8_t deepseek_cover_bin_start[] asm("_binary_deepseek_cover_bin_start");
extern const uint8_t deepseek_cover_bin_end[]   asm("_binary_deepseek_cover_bin_end");

static const lv_image_dsc_t s_deepseek_cover_dsc = {
    .header = {
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_RGB565,
        .flags = 0,
        .w = 240,
        .h = 320,
        .stride = 240 * 2,
    },
    .data_size = 240 * 320 * 2,
    .data = deepseek_cover_bin_start,
};

static lv_obj_t *s_cover_scr;

void demo_cover_enter(void) {
    ESP_LOGI(TAG, "加载赛博朋克大肥鱼 DeepSeek 封面");
    s_cover_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_cover_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_cover_scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_pad_all(s_cover_scr, 0, 0);

    // 全屏底图 (240x320 RGB565)
    lv_obj_t *img = lv_image_create(s_cover_scr);
    lv_image_set_src(img, &s_deepseek_cover_dsc);
    lv_obj_set_pos(img, 0, 0);

    // 顶部科技 HUD 铭牌
    lv_obj_t *top_badge = lv_obj_create(s_cover_scr);
    lv_obj_remove_flag(top_badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(top_badge, 8, 8);
    lv_obj_set_size(top_badge, 224, 28);
    lv_obj_set_style_bg_color(top_badge, lv_color_hex(0x060B14), 0);
    lv_obj_set_style_bg_opa(top_badge, LV_OPA_80, 0);
    lv_obj_set_style_border_color(top_badge, lv_color_hex(0x00F0FF), 0);
    lv_obj_set_style_border_width(top_badge, 1, 0);
    lv_obj_set_style_radius(top_badge, 0, 0);
    lv_obj_set_style_pad_all(top_badge, 3, 0);

    lv_obj_t *lbl_title = lv_label_create(top_badge);
    lv_label_set_text(lbl_title, "> DEEPSEEK // CYBER");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x00F0FF), 0);
    lv_obj_align(lbl_title, LV_ALIGN_LEFT_MID, 6, 0);

    lv_obj_t *lbl_stat = lv_label_create(top_badge);
    lv_label_set_text(lbl_stat, "SYS:OK");
    lv_obj_set_style_text_font(lbl_stat, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_stat, lv_color_hex(0x00FF66), 0);
    lv_obj_align(lbl_stat, LV_ALIGN_RIGHT_MID, -6, 0);

    // 底部脉冲交互提示条
    lv_obj_t *bot_badge = lv_obj_create(s_cover_scr);
    lv_obj_remove_flag(bot_badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(bot_badge, 16, 284);
    lv_obj_set_size(bot_badge, 208, 26);
    lv_obj_set_style_bg_color(bot_badge, lv_color_hex(0x060B14), 0);
    lv_obj_set_style_bg_opa(bot_badge, LV_OPA_80, 0);
    lv_obj_set_style_border_color(bot_badge, lv_color_hex(0x00F0FF), 0);
    lv_obj_set_style_border_width(bot_badge, 1, 0);
    lv_obj_set_style_radius(bot_badge, 0, 0);
    lv_obj_set_style_pad_all(bot_badge, 3, 0);

    lv_obj_t *lbl_bot = lv_label_create(bot_badge);
    lv_label_set_text(lbl_bot, "> PRESS OK TO START <");
    lv_obj_set_style_text_font(lbl_bot, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_bot, lv_color_hex(0x00FF66), 0);
    lv_obj_center(lbl_bot);

    lv_screen_load(s_cover_scr);
}

void demo_cover_exit(void) {
    ESP_LOGI(TAG, "退出赛博朋克大肥鱼 DeepSeek 封面");
    if (s_cover_scr) {
        lv_obj_delete(s_cover_scr);
        s_cover_scr = NULL;
    }
}

void demo_cover_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    (void)btn;
    // 任意按键触发退出封面，返回主菜单
    if (ev == BSP_BTN_PRESS || ev == BSP_BTN_CLICK || ev == BSP_BTN_LONG) {
        demo_request_exit();
    }
}

esp_err_t demo_cover_start(void) {
    return ESP_OK;
}

esp_err_t demo_cover_stop(void) {
    return ESP_OK;
}
