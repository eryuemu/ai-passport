// main/demo_world_execute.c —— 《world.execute(me);》演示接入
#include "demo.h"
#include "world_execute.h"
#include "lvgl.h"
#include "esp_log.h"

static const char *TAG = "demo_world_execute";
static lv_obj_t *s_wexe_scr = NULL;

void demo_world_execute_enter(void) {
    ESP_LOGI(TAG, "进入 world.execute(me); 演示");
    // 创建一个合法的纯黑 LVGL 屏幕，保证 LVGL 调度器始终拥有 active screen
    s_wexe_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_wexe_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_wexe_scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_pad_all(s_wexe_scr, 0, 0);
    lv_screen_load(s_wexe_scr);
}

void demo_world_execute_exit(void) {
    ESP_LOGI(TAG, "退出 world.execute(me); 演示");
    world_execute_player_stop();
    if (s_wexe_scr) {
        lv_obj_delete(s_wexe_scr);
        s_wexe_scr = NULL;
    }
}

esp_err_t demo_world_execute_start(void) {
    return world_execute_player_start();
}

esp_err_t demo_world_execute_stop(void) {
    world_execute_player_stop();
    return ESP_OK;
}

void demo_world_execute_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    // 播放若已结束，任意按键立即触发返回主菜单
    if (!world_execute_player_is_running()) {
        ESP_LOGI(TAG, "播放已完成，按键触发返回菜单");
        demo_request_exit();
        return;
    }

    if (ev == BSP_BTN_PRESS || ev == BSP_BTN_CLICK) {
        if (btn == BSP_BTN_OK) {
            world_execute_player_toggle_pause();
        } else if (btn == BSP_BTN_UP) {
            uint8_t vol = world_execute_player_get_volume();
            if (vol <= 90) vol += 10; else vol = 100;
            world_execute_player_set_volume(vol);
        } else if (btn == BSP_BTN_DOWN) {
            uint8_t vol = world_execute_player_get_volume();
            if (vol >= 10) vol -= 10; else vol = 0;
            world_execute_player_set_volume(vol);
        }
    } else if (ev == BSP_BTN_LONG || ev == BSP_BTN_DOUBLE) {
        // 长按或双击 OK 键立即退出到主菜单
        if (btn == BSP_BTN_OK) {
            ESP_LOGI(TAG, "按键触发退出到主菜单");
            demo_request_exit();
        }
    }
}
