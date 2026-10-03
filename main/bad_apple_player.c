// main/bad_apple_player.c —— FoloToy AI Passport Bad Apple!! 有声满帧播放器
#include "bad_apple.h"
#include "bsp_display.h"
#include "bsp_audio.h"
#include "bsp_button.h"
#include "tinf.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "bad_apple";

// 引用通过 CMake EMBED_FILES 链接进只读 Flash 区域的媒体数据
extern const uint8_t bad_apple_data_bin_start[] asm("_binary_bad_apple_data_bin_start");
extern const uint8_t bad_apple_data_bin_end[]   asm("_binary_bad_apple_data_bin_end");

// 播放状态管理
static volatile bool s_running = false;
static volatile bool s_paused = false;
static volatile bool s_restart_req = false;
static uint8_t s_volume = 80;
static volatile uint32_t s_samples_played = 0;

static TaskHandle_t s_audio_task_handle = NULL;
static TaskHandle_t s_video_task_handle = NULL;

// IMA-ADPCM 解码常数表
static const int16_t STEP_TABLE[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static const int8_t INDEX_TABLE[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

static inline int16_t adpcm_decode_nibble(uint8_t nibble, int16_t *predicted, int *step_idx) {
    int step = STEP_TABLE[*step_idx];
    *step_idx += INDEX_TABLE[nibble & 0x0F];
    if (*step_idx < 0) *step_idx = 0;
    else if (*step_idx > 88) *step_idx = 88;

    int diff = step >> 3;
    if (nibble & 4) diff += step;
    if (nibble & 2) diff += step >> 1;
    if (nibble & 1) diff += step >> 2;

    int pred = *predicted;
    if (nibble & 8) pred -= diff;
    else pred += diff;

    if (pred > 32767) pred = 32767;
    else if (pred < -32768) pred = -32768;

    *predicted = (int16_t)pred;
    return (int16_t)pred;
}

// 渲染缓冲区配置 (240x180 视频居中显示在 240x320 竖屏上，y 坐标 70..250)
#define VIDEO_W 240
#define VIDEO_H 180
#define VIDEO_Y_OFFSET 70
#define MONO_FRAME_BYTES ((VIDEO_W * VIDEO_H) / 8) // 5400 字节
#define SLICE_LINES 15 // 每次刷 15 行，240 * 15 * 2 = 7200 字节 DMA 缓冲
#define SLICE_COUNT (VIDEO_H / SLICE_LINES) // 12 批次

// 音频块大小 (256 字节 ADPCM = 512 采样点 = 1024 字节 PCM)
#define AUDIO_CHUNK_ADPCM_BYTES 256
#define AUDIO_CHUNK_SAMPLES (AUDIO_CHUNK_ADPCM_BYTES * 2)

static void clear_screen_black(void) {
    esp_lcd_panel_handle_t panel = bsp_display_panel();
    if (!panel) return;

    // 分块刷纯黑背景
    static uint16_t black_slice[240 * 20];
    memset(black_slice, 0, sizeof(black_slice));
    for (int y = 0; y < 320; y += 20) {
        esp_lcd_panel_draw_bitmap(panel, 0, y, 240, y + 20, black_slice);
    }
}

// 音频播放任务 (主时间基准)
static void audio_task(void *arg) {
    const bad_apple_header_t *hdr = (const bad_apple_header_t *)bad_apple_data_bin_start;
    const uint8_t *adpcm_stream = bad_apple_data_bin_start + hdr->audio_data_offset;
    uint32_t total_adpcm_bytes = hdr->audio_data_size;

    int16_t *pcm_buf = malloc(AUDIO_CHUNK_SAMPLES * sizeof(int16_t));
    if (!pcm_buf) {
        ESP_LOGE(TAG, "无法分配 PCM 缓冲区");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "音频任务启动: %lu Hz, %lu 字节 ADPCM", hdr->audio_sample_rate, total_adpcm_bytes);
    bsp_audio_set_format(hdr->audio_sample_rate, 16, hdr->audio_channels);
    bsp_audio_set_volume(s_volume);

    while (s_running) {
        int16_t predicted = 0;
        int step_idx = 0;
        uint32_t adpcm_pos = 0;
        s_samples_played = 0;

        while (adpcm_pos < total_adpcm_bytes && s_running) {
            if (s_restart_req) {
                s_restart_req = false;
                break;
            }

            if (s_paused) {
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }

            uint32_t chunk_len = total_adpcm_bytes - adpcm_pos;
            if (chunk_len > AUDIO_CHUNK_ADPCM_BYTES) chunk_len = AUDIO_CHUNK_ADPCM_BYTES;

            int pcm_idx = 0;
            for (uint32_t i = 0; i < chunk_len; i++) {
                uint8_t b = adpcm_stream[adpcm_pos + i];
                pcm_buf[pcm_idx++] = adpcm_decode_nibble(b & 0x0F, &predicted, &step_idx);
                pcm_buf[pcm_idx++] = adpcm_decode_nibble((b >> 4) & 0x0F, &predicted, &step_idx);
            }

            // 写入 I2S 硬件 FIFO（阻塞式，精确保持音频时序）
            size_t bytes_to_write = (size_t)pcm_idx * sizeof(int16_t);
            bsp_audio_write(pcm_buf, bytes_to_write);

            adpcm_pos += chunk_len;
            s_samples_played += (chunk_len * 2);
        }

        if (!s_running) break;
        // 播完后短暂停顿，准备循环播放
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    free(pcm_buf);
    ESP_LOGI(TAG, "音频任务退出");
    vTaskDelete(NULL);
}

// 视频渲染任务
static void video_task(void *arg) {
    const bad_apple_header_t *hdr = (const bad_apple_header_t *)bad_apple_data_bin_start;
    esp_lcd_panel_handle_t panel = bsp_display_panel();
    if (!panel) {
        ESP_LOGE(TAG, "未获取到底层 LCD 面板句柄");
        vTaskDelete(NULL);
        return;
    }

    const uint32_t *index_table = (const uint32_t *)(bad_apple_data_bin_start + hdr->index_table_offset);
    const uint8_t *video_payload = bad_apple_data_bin_start + hdr->video_data_offset;
    uint32_t total_frames = hdr->total_frames;
    uint32_t fps = hdr->fps ? hdr->fps : 20;

    // 单帧二值点阵缓冲 (5.4 KB)
    uint8_t *mono_buf = malloc(MONO_FRAME_BYTES);
    // RGB565 行切片 DMA 缓冲 (240 * 15 * 2 = 7200 字节)
    uint16_t *rgb_slice = malloc(VIDEO_W * SLICE_LINES * sizeof(uint16_t));

    if (!mono_buf || !rgb_slice) {
        ESP_LOGE(TAG, "无法分配视频渲染缓冲区");
        if (mono_buf) free(mono_buf);
        if (rgb_slice) free(rgb_slice);
        vTaskDelete(NULL);
        return;
    }

    clear_screen_black();
    bsp_display_backlight(100);

    ESP_LOGI(TAG, "视频任务启动: %u 帧 @ %lu fps", (unsigned)total_frames, fps);

    uint32_t last_drawn_frame = UINT32_MAX;
    static uint16_t progress_bar[VIDEO_W];

    while (s_running) {
        if (s_paused) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // 以音频播放采样点为基准计算目标画面帧
        uint32_t samples = s_samples_played;
        uint32_t current_time_ms = (uint32_t)(((uint64_t)samples * 1000) / hdr->audio_sample_rate);
        uint32_t target_frame = (current_time_ms * fps) / 1000;

        if (target_frame >= total_frames) {
            target_frame = total_frames - 1;
        }

        // 若当前帧已画过，休眠 5ms 避免无意义忙等
        if (target_frame == last_drawn_frame) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        // 获取帧压缩数据并解压
        uint32_t offset = index_table[target_frame];
        uint32_t next_offset = index_table[target_frame + 1];
        uint32_t comp_size = next_offset - offset;
        const void *comp_ptr = video_payload + offset;

        unsigned int dest_len = MONO_FRAME_BYTES;
        int res = tinf_zlib_uncompress(mono_buf, &dest_len, comp_ptr, comp_size);
        if (res != TINF_OK || dest_len != MONO_FRAME_BYTES) {
            ESP_LOGW(TAG, "第 %lu 帧解压异常: %d (长度: %u)", target_frame, res, dest_len);
        } else {
            // 切片转为 RGB565 并推屏
            for (int s = 0; s < SLICE_COUNT; s++) {
                int start_y = s * SLICE_LINES;
                int mono_offset = (start_y * VIDEO_W) / 8;

                int out_idx = 0;
                for (int l = 0; l < SLICE_LINES; l++) {
                    for (int b = 0; b < (VIDEO_W / 8); b++) {
                        uint8_t byte_val = mono_buf[mono_offset++];
                        rgb_slice[out_idx++] = (byte_val & 0x80) ? 0xFFFF : 0x0000;
                        rgb_slice[out_idx++] = (byte_val & 0x40) ? 0xFFFF : 0x0000;
                        rgb_slice[out_idx++] = (byte_val & 0x20) ? 0xFFFF : 0x0000;
                        rgb_slice[out_idx++] = (byte_val & 0x10) ? 0xFFFF : 0x0000;
                        rgb_slice[out_idx++] = (byte_val & 0x08) ? 0xFFFF : 0x0000;
                        rgb_slice[out_idx++] = (byte_val & 0x04) ? 0xFFFF : 0x0000;
                        rgb_slice[out_idx++] = (byte_val & 0x02) ? 0xFFFF : 0x0000;
                        rgb_slice[out_idx++] = (byte_val & 0x01) ? 0xFFFF : 0x0000;
                    }
                }

                int screen_y1 = VIDEO_Y_OFFSET + start_y;
                int screen_y2 = screen_y1 + SLICE_LINES;
                esp_lcd_panel_draw_bitmap(panel, 0, screen_y1, VIDEO_W, screen_y2, rgb_slice);
            }

            // 更新顶部极简进度条 (y = 66..67)
            int progress_px = (int)((target_frame * VIDEO_W) / total_frames);
            if (progress_px > VIDEO_W) progress_px = VIDEO_W;
            for (int x = 0; x < VIDEO_W; x++) {
                progress_bar[x] = (x < progress_px) ? 0xFFFF : 0x31A6; // 白色已播，暗灰未播
            }
            esp_lcd_panel_draw_bitmap(panel, 0, 66, VIDEO_W, 67, progress_bar);
        }

        last_drawn_frame = target_frame;
    }

    free(mono_buf);
    free(rgb_slice);
    ESP_LOGI(TAG, "视频任务退出");
    vTaskDelete(NULL);
}

esp_err_t bad_apple_player_start(void) {
    if (s_running) return ESP_OK;

    const bad_apple_header_t *hdr = (const bad_apple_header_t *)bad_apple_data_bin_start;
    if (hdr->magic != BAD_APPLE_MAGIC) {
        ESP_LOGE(TAG, "无效的 Bad Apple 媒体格式 (magic: 0x%08lX, expected 0x%08X)",
                 hdr->magic, BAD_APPLE_MAGIC);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "找到有效 Bad Apple 数据包! 画面: %ux%u @ %u fps, 帧数: %lu",
             hdr->width, hdr->height, hdr->fps, hdr->total_frames);

    esp_err_t err = bsp_display_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bsp_display_init 失败: %s", esp_err_to_name(err));
        return err;
    }

    err = bsp_audio_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bsp_audio_init 失败: %s", esp_err_to_name(err));
        return err;
    }

    s_running = true;
    s_paused = false;
    s_restart_req = false;

    // 创建音视频双任务
    xTaskCreatePinnedToCore(audio_task, "ba_audio", 4096, NULL, 5, &s_audio_task_handle, 0);
    xTaskCreatePinnedToCore(video_task, "ba_video", 6144, NULL, 4, &s_video_task_handle, 0);

    return ESP_OK;
}

void bad_apple_player_stop(void) {
    if (!s_running) return;
    s_running = false;
    vTaskDelay(pdMS_TO_TICKS(100));
}

void bad_apple_player_on_button(uint8_t btn, uint8_t event) {
    if (event == BSP_BTN_LONG && btn == BSP_BTN_OK) {
        ESP_LOGI(TAG, "长按 OK：重新从头开始播放");
        s_restart_req = true;
        return;
    }

    if (event != BSP_BTN_CLICK) return;

    if (btn == BSP_BTN_OK) {
        s_paused = !s_paused;
        ESP_LOGI(TAG, "按键 OK：%s", s_paused ? "暂停" : "继续");
    } else if (btn == BSP_BTN_UP) {
        if (s_volume <= 90) s_volume += 10;
        else s_volume = 100;
        bsp_audio_set_volume(s_volume);
        ESP_LOGI(TAG, "按键 UP：音量调大至 %u%%", s_volume);
    } else if (btn == BSP_BTN_DOWN) {
        if (s_volume >= 10) s_volume -= 10;
        else s_volume = 0;
        bsp_audio_set_volume(s_volume);
        ESP_LOGI(TAG, "按键 DOWN：音量调小至 %u%%", s_volume);
    }
}

bool bad_apple_player_is_playing(void) {
    return s_running && !s_paused;
}
