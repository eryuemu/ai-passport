#pragma once

#include "lvgl.h"

// 赛博朋克深空与霓虹调色板 (DeepSeek 赛博大肥鱼科技风)
#define CYBER_VOID        0x080D1A // 虚空深蓝黑背景
#define CYBER_PANEL       0x121A2A // 哑光科技面板灰蓝
#define CYBER_PANEL_SEL   0x152D4A // 选中态电光青暗底
#define CYBER_CYAN        0x00F0FF // 电光霓虹青 (主高亮色)
#define CYBER_BLUE        0x0066FF // DeepSeek 标志钴蓝
#define CYBER_WHITE       0xF1F5F9 // 终端字符亮白
#define CYBER_MUTED       0x64748B // 次要参数暗灰
#define CYBER_BORDER      0x1E3A5F // 科技边框暗青
#define CYBER_NEON_GREEN  0x00FF66 // 状态在线/正常指示
#define CYBER_ALERT_RED   0xFF0055 // 故障/告警霓虹红

// 向后兼容旧颜色宏名
#define UI_SKY         CYBER_VOID
#define UI_SKY_DARK    0x060A14
#define UI_INK         CYBER_WHITE
#define UI_PAPER       CYBER_PANEL
#define UI_GRASS       0x0E1726
#define UI_GRASS_DARK  0x070B14
#define UI_YELLOW      CYBER_CYAN
#define UI_ORANGE      0xFF9900
#define UI_RED         CYBER_ALERT_RED
#define UI_MUTED       CYBER_MUTED

lv_obj_t *ui_pixel_screen_create(const char *title);
lv_obj_t *ui_pixel_panel_create(lv_obj_t *parent, int x, int y, int w, int h,
                                uint32_t color);
lv_obj_t *ui_pixel_label(lv_obj_t *parent, const char *text,
                         const lv_font_t *font, uint32_t color);
lv_obj_t *ui_pixel_mascot_create(lv_obj_t *parent, int x, int y);
void ui_pixel_mascot_jump(lv_obj_t *mascot);
void ui_pixel_set_selected(lv_obj_t *panel, bool selected, bool enabled);
