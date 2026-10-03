// main/ui_pixel.c —— 赛博朋克深空与大肥鱼 DeepSeek 娘化终端 UI 库
#include "ui_pixel.h"

static void start_blink(lv_obj_t *eye);

static lv_obj_t *block(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    return obj;
}

lv_obj_t *ui_pixel_label(lv_obj_t *parent, const char *text,
                         const lv_font_t *font, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

lv_obj_t *ui_pixel_screen_create(const char *title)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(CYBER_VOID), 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);

    // 四角赛博准心标尺 (HUD Viewfinder Brackets)
    // 左上角 [
    block(scr, 4, 4, 10, 2, CYBER_CYAN);
    block(scr, 4, 4, 2, 10, CYBER_CYAN);
    // 右上角 ]
    block(scr, 226, 4, 10, 2, CYBER_CYAN);
    block(scr, 234, 4, 2, 10, CYBER_CYAN);
    // 左下角 [
    block(scr, 4, 314, 10, 2, CYBER_CYAN);
    block(scr, 4, 306, 2, 10, CYBER_CYAN);
    // 右下角 ]
    block(scr, 226, 314, 10, 2, CYBER_CYAN);
    block(scr, 234, 306, 2, 10, CYBER_CYAN);

    // 科技分割标线
    block(scr, 0, 46, 240, 1, 0x142338);
    block(scr, 0, 239, 240, 2, CYBER_CYAN);
    block(scr, 90, 238, 60, 4, CYBER_BLUE);

    // 顶部科技终端铭牌
    lv_obj_t *plate = block(scr, 8, 8, 224, 32, 0x0D1626);
    lv_obj_set_style_border_color(plate, lv_color_hex(CYBER_BORDER), 0);
    lv_obj_set_style_border_width(plate, 1, 0);

    // 侧边霓虹亮条
    block(plate, 0, 0, 4, 32, CYBER_CYAN);

    // 终端主标题
    lv_obj_t *heading = ui_pixel_label(plate, title, &lv_font_montserrat_14, CYBER_CYAN);
    lv_obj_align(heading, LV_ALIGN_LEFT_MID, 12, 0);

    // 在线状态指示灯
    lv_obj_t *status = ui_pixel_label(plate, "SYS:OK", &lv_font_montserrat_14, CYBER_NEON_GREEN);
    lv_obj_align(status, LV_ALIGN_RIGHT_MID, -8, 0);

    return scr;
}

lv_obj_t *ui_pixel_panel_create(lv_obj_t *parent, int x, int y, int w, int h,
                                uint32_t color)
{
    lv_obj_t *panel = block(parent, x, y, w, h, color);
    lv_obj_set_style_border_color(panel, lv_color_hex(CYBER_BORDER), 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_pad_all(panel, 5, 0);
    return panel;
}

// 赛博大肥鱼 (DeepSeek 鲸鱼娘女仆) 像素立绘
lv_obj_t *ui_pixel_mascot_create(lv_obj_t *parent, int x, int y)
{
    // 底部左侧：技术遥测字符
    lv_obj_t *tag1 = ui_pixel_label(parent, "> DEEPSEEK", &lv_font_montserrat_14, CYBER_CYAN);
    lv_obj_set_pos(tag1, 8, 250);
    lv_obj_t *tag2 = ui_pixel_label(parent, "> BLUE WHALE", &lv_font_montserrat_14, CYBER_CYAN);
    lv_obj_set_pos(tag2, 8, 268);
    lv_obj_t *sub = ui_pixel_label(parent, "> R1 // C3-NODE", &lv_font_montserrat_14, CYBER_MUTED);
    lv_obj_set_pos(sub, 8, 286);

    // 底部右侧：动态科技音频频谱条 (Equalizer Bars)
    block(parent, 180, 280, 4, 14, CYBER_BLUE);
    block(parent, 188, 268, 4, 26, CYBER_CYAN);
    block(parent, 196, 260, 4, 34, 0x8A2BE2);
    block(parent, 204, 272, 4, 22, CYBER_CYAN);
    block(parent, 212, 264, 4, 30, CYBER_BLUE);
    block(parent, 220, 278, 4, 16, CYBER_CYAN);

    // 中央：蓝色大肥鱼女仆像素形象 (46 x 48)
    lv_obj_t *m = lv_obj_create(parent);
    lv_obj_remove_flag(m, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(m, x, y);
    lv_obj_set_size(m, 46, 48);
    lv_obj_set_style_bg_opa(m, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(m, 0, 0);
    lv_obj_set_style_pad_all(m, 0, 0);

    // 顶部女仆蕾丝发箍 (Maid Ruffle Headdress)
    block(m, 12, 0, 22, 4, 0xFFFFFF);
    block(m, 10, 2, 26, 3, 0xE2E8F0);
    block(m, 8, 1, 4, 4, CYBER_CYAN); // 蝴蝶结

    // 左右蓝鲸大耳鳍 (Whale Fin Ears)
    block(m, 2, 8, 8, 8, CYBER_BLUE);
    block(m, 4, 10, 4, 5, 0xE2E8F0);
    block(m, 36, 8, 8, 8, CYBER_BLUE);
    block(m, 38, 10, 4, 5, 0xE2E8F0);

    // 蓝色长发与刘海
    block(m, 8, 5, 30, 20, CYBER_BLUE);
    block(m, 12, 9, 22, 16, 0x2563EB); // 渐变浅蓝发丝
    // 萌系粉嫩面容
    block(m, 14, 12, 18, 12, 0xFFE8D6);

    // 发光萌系大眼睛 (左眼与右眼)
    lv_obj_t *left_eye = block(m, 16, 15, 4, 5, CYBER_CYAN);
    lv_obj_t *right_eye = block(m, 26, 15, 4, 5, CYBER_CYAN);
    block(left_eye, 1, 1, 2, 3, 0x0F172A);
    block(right_eye, 1, 1, 2, 3, 0x0F172A);

    // 腮红点缀
    block(m, 14, 21, 3, 2, 0xF472B6);
    block(m, 29, 21, 3, 2, 0xF472B6);

    // 背后大肥鱼尾巴 (Whale Tail)
    block(m, 0, 32, 10, 10, CYBER_BLUE);
    block(m, 0, 30, 4, 4, CYBER_CYAN);
    block(m, 6, 30, 4, 4, CYBER_CYAN);

    // 女仆装 (深蓝主裙 + 白色围裙)
    block(m, 11, 24, 24, 16, 0x1E293B);
    block(m, 15, 24, 16, 16, 0xFFFFFF); // 白色女仆围裙
    block(m, 21, 32, 4, 3, CYBER_BLUE); // 围裙中央小鲸鱼标

    // 蕾丝花边与裙摆
    block(m, 9, 39, 28, 4, 0xFFFFFF);

    // 黑色小皮鞋
    block(m, 14, 43, 7, 4, 0x0F172A);
    block(m, 25, 43, 7, 4, 0x0F172A);
    block(m, 15, 45, 5, 2, 0xFFFFFF);
    block(m, 26, 45, 5, 2, 0xFFFFFF);

    start_blink(left_eye);
    start_blink(right_eye);
    return m;
}

static void jump_y(void *obj, int32_t value)
{
    lv_obj_set_y((lv_obj_t *)obj, value);
}

static void blink_eye(void *obj, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, (lv_opa_t)value, 0);
}

static void start_blink(lv_obj_t *eye)
{
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, eye);
    lv_anim_set_exec_cb(&anim, blink_eye);
    lv_anim_set_values(&anim, LV_OPA_COVER, LV_OPA_20);
    lv_anim_set_duration(&anim, 80);
    lv_anim_set_playback_duration(&anim, 80);
    lv_anim_set_repeat_delay(&anim, 2200);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&anim, lv_anim_path_step);
    lv_anim_start(&anim);
}

void ui_pixel_mascot_jump(lv_obj_t *mascot)
{
    if (!mascot) return;
    int y = lv_obj_get_y(mascot);
    lv_anim_delete(mascot, jump_y);
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, mascot);
    lv_anim_set_exec_cb(&anim, jump_y);
    lv_anim_set_values(&anim, y, y - 6);
    lv_anim_set_duration(&anim, 100);
    lv_anim_set_playback_duration(&anim, 120);
    lv_anim_set_path_cb(&anim, lv_anim_path_step);
    lv_anim_start(&anim);
}

void ui_pixel_set_selected(lv_obj_t *panel, bool selected, bool enabled)
{
    if (!enabled) {
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x1F1116), 0);
        lv_obj_set_style_border_color(panel, lv_color_hex(CYBER_ALERT_RED), 0);
        lv_obj_set_style_border_width(panel, 1, 0);
    } else if (selected) {
        lv_obj_set_style_bg_color(panel, lv_color_hex(CYBER_PANEL_SEL), 0);
        lv_obj_set_style_border_color(panel, lv_color_hex(CYBER_CYAN), 0);
        lv_obj_set_style_border_width(panel, 2, 0);
    } else {
        lv_obj_set_style_bg_color(panel, lv_color_hex(CYBER_PANEL), 0);
        lv_obj_set_style_border_color(panel, lv_color_hex(CYBER_BORDER), 0);
        lv_obj_set_style_border_width(panel, 1, 0);
    }
}
