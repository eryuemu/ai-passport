// main/demo_bad_apple.c —— Bad Apple!! 影绘有声播放演示接入
#include "demo.h"
#include "bad_apple.h"
#include "esp_log.h"

static const char *TAG = "demo_bad_apple";

void demo_bad_apple_enter(void) {
    ESP_LOGI(TAG, "进入 Bad Apple 演示");
}

void demo_bad_apple_exit(void) {
    ESP_LOGI(TAG, "退出 Bad Apple 演示");
    bad_apple_player_stop();
}

esp_err_t demo_bad_apple_start(void) {
    return bad_apple_player_start();
}

esp_err_t demo_bad_apple_stop(void) {
    bad_apple_player_stop();
    return ESP_OK;
}

void demo_bad_apple_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    bad_apple_player_on_button((uint8_t)btn, (uint8_t)ev);
}
