// main/demo_cover.h —— 赛博朋克大肥鱼 DeepSeek 娘化形象封面展示
#pragma once

#include "esp_err.h"
#include "bsp_button.h"

void demo_cover_enter(void);
void demo_cover_exit(void);
void demo_cover_key(bsp_btn_t btn, bsp_btn_ev_t ev);
esp_err_t demo_cover_start(void);
esp_err_t demo_cover_stop(void);
