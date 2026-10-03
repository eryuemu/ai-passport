// main/demo_world_execute.c —— 《world.execute(me);》演示接入
#include "demo.h"
#include "world_execute.h"
#include "esp_log.h"

static const char *TAG = "demo_world_execute";

void demo_world_execute_enter(void) {
    ESP_LOGI(TAG, "进入 world.execute(me); 演示");
}

void demo_world_execute_exit(void) {
    ESP_LOGI(TAG, "退出 world.execute(me); 演示");
    world_execute_player_stop();
}

esp_err_t demo_world_execute_start(void) {
    return world_execute_player_start();
}

esp_err_t demo_world_execute_stop(void) {
    world_execute_player_stop();
    return ESP_OK;
}

void demo_world_execute_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (ev == BSP_BTN_PRESS) {
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
    } else if (ev == BSP_BTN_LONG) {
        if (btn == BSP_BTN_OK) {
            world_execute_player_restart();
        }
    }
}
