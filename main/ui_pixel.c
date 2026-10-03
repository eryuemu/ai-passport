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

// 赛博大肥鱼 DeepSeek 娘化像素立绘
lv_obj_t *ui_pixel_mascot_create(lv_obj_t *parent, int x, int y)
{
    // 底部左侧：技术遥测字符
    ui_pixel_label(parent, "> DEEPSEEK", &lv_font_montserrat_14, CYBER_CYAN);
    lv_obj_t *sub = ui_pixel_label(parent, "> R1 // C3", &lv_font_montserrat_14, CYBER_MUTED);
    lv_obj_set_pos(sub, 8, 276);
    lv_obj_t *main_tag = ui_pixel_label(parent, "> DSH_TERMINAL", &lv_font_montserrat_14, CYBER_CYAN);
    lv_obj_set_pos(main_tag, 8, 256);

    // 底部右侧：动态科技音频频谱条 (Equalizer Bars)
    block(parent, 180, 280, 4, 14, CYBER_BLUE);
    block(parent, 188, 268, 4, 26, CYBER_CYAN);
    block(parent, 196, 260, 4, 34, 0x8A2BE2);
    block(parent, 204, 272, 4, 22, CYBER_CYAN);
    block(parent, 212, 264, 4, 30, CYBER_BLUE);
    block(parent, 220, 278, 4, 16, CYBER_CYAN);

    // 中央：赛博大肥鱼 DeepSeek 娘像素形象 (44 x 48)
    lv_obj_t *m = lv_obj_create(parent);
    lv_obj_remove_flag(m, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(m, x, y);
    lv_obj_set_size(m, 44, 48);
    lv_obj_set_style_bg_opa(m, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(m, 0, 0);
    lv_obj_set_style_pad_all(m, 0, 0);

    // 顶部发光大肥鱼角/鲸鱼鳍天线
    block(m, 12, 0, 5, 7, CYBER_CYAN);
    block(m, 27, 0, 5, 7, CYBER_CYAN);
    block(m, 15, 2, 14, 3, CYBER_BLUE);

    // DeepSeek 蓝鲸兜帽发型
    block(m, 8, 6, 28, 20, CYBER_BLUE);
    // 银白刘海发丝
    block(m, 11, 10, 22, 16, 0xE2E8F0);
    // 萌系粉嫩面容
    block(m, 13, 13, 18, 12, 0xFFE8D6);

    // 左右赛博耳机/发光传感器
    block(m, 4, 12, 6, 12, 0x111927);
    block(m, 6, 14, 2, 8, CYBER_CYAN);
    block(m, 34, 12, 6, 12, 0x111927);
    block(m, 36, 14, 2, 8, CYBER_CYAN);

    // 发光萌系大眼睛 (左眼与右眼)
    lv_obj_t *left_eye = block(m, 15, 16, 4, 5, CYBER_CYAN);
    lv_obj_t *right_eye = block(m, 25, 16, 4, 5, CYBER_CYAN);
    // 深蓝瞳孔
    block(left_eye, 1, 1, 2, 3, 0x002B66);
    block(right_eye, 1, 1, 2, 3, 0x002B66);

    // 腮红点缀
    block(m, 13, 22, 3, 2, 0xFF99BB);
    block(m, 28, 22, 3, 2, 0xFF99BB);

    // DeepSeek 赛博机能卫衣 (蓝白配色)
    block(m, 10, 26, 24, 15, CYBER_BLUE);
    block(m, 17, 26, 10, 15, 0xFFFFFF); // 白色前襟胸兜

    // 卫衣发光电路纹路
    block(m, 12, 30, 4, 2, CYBER_CYAN);
    block(m, 28, 30, 4, 2, CYBER_CYAN);
    block(m, 14, 32, 2, 7, CYBER_CYAN);
    block(m, 28, 32, 2, 7, CYBER_CYAN);

    // 背后摇摆的萌萌大肥鱼尾巴 (露出左后方)
    block(m, 2, 34, 8, 8, CYBER_BLUE);
    block(m, 0, 36, 4, 4, CYBER_CYAN);

    // 赛博机能运动鞋 (踏在科技底座上)
    block(m, 12, 42, 8, 5, 0x111927);
    block(m, 24, 42, 8, 5, 0x111927);
    block(m, 13, 45, 6, 2, CYBER_CYAN);
    block(m, 25, 45, 6, 2, CYBER_CYAN);

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
