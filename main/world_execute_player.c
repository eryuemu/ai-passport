// main/world_execute_player.c —— 《world.execute(me);》赛博 2-bit 调色板播放器实现
#include "world_execute.h"
#include "demo.h"
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

static const char *TAG = "world_execute";

// 引用链接入 Flash 只读区的 WEXE 媒体数据
extern const uint8_t world_execute_data_bin_start[] asm("_binary_world_execute_data_bin_start");
extern const uint8_t world_execute_data_bin_end[]   asm("_binary_world_execute_data_bin_end");

// 播放状态管理
static volatile bool s_running = false;
static volatile bool s_paused = false;
static volatile bool s_restart_req = false;
static uint8_t s_volume = 80;
static volatile uint32_t s_samples_played = 0;

static TaskHandle_t s_audio_task_handle = NULL;
static TaskHandle_t s_video_task_handle = NULL;

// 调色板缓存 (RGB565 x 4)
static uint16_t s_palette[4] = { 0x0000, 0x1148, 0x3DFE, 0xFFFF };

// IMA-ADPCM 步长表与索引表
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

// 视频渲染配置：320x180 居中显示在 320x240 横屏上 (y 偏移 30..210)
#define VIDEO_W        320
#define VIDEO_H        180
#define VIDEO_Y_OFFSET 30
#define PACKED_FRAME_BYTES ((VIDEO_W * VIDEO_H) / 4) // 14,400 字节
#define SLICE_LINES    15
#define SLICE_COUNT    (VIDEO_H / SLICE_LINES)       // 12 批次

// 音频缓冲区配置 (256 字节 ADPCM = 512 采样点 = 1024 字节 PCM)
#define AUDIO_CHUNK_ADPCM_BYTES 256
#define AUDIO_CHUNK_SAMPLES     (AUDIO_CHUNK_ADPCM_BYTES * 2)

// 静态解码缓冲区 (零动态堆分配)
static uint8_t  s_decomp_buf[PACKED_FRAME_BYTES];
static uint16_t s_dma_slice_buf[VIDEO_W * SLICE_LINES];
static int16_t  s_pcm_buf[AUDIO_CHUNK_SAMPLES];

static void clear_screen_black_landscape(void) {
    esp_lcd_panel_handle_t panel = bsp_display_panel();
    if (!panel) return;

    static uint16_t black_slice[320 * 20];
    memset(black_slice, 0, sizeof(black_slice));
    for (int y = 0; y < 240; y += 20) {
        esp_lcd_panel_draw_bitmap(panel, 0, y, 320, y + 20, black_slice);
    }
}

// 绘制底部赛博进度条 HUD (Y: 215 ~ 225)
static void draw_hud_landscape(uint32_t current_frame, uint32_t total_frames) {
    esp_lcd_panel_handle_t panel = bsp_display_panel();
    if (!panel || total_frames == 0) return;

    static uint16_t hud_line[320 * 4];
    memset(hud_line, 0, sizeof(hud_line));

    int bar_width = (int)((uint64_t)current_frame * 300 / total_frames);
    if (bar_width > 300) bar_width = 300;

    // 绘制 300 像素宽的赛博蓝进度条
    for (int y = 0; y < 4; y++) {
        for (int x = 10; x < 10 + 300; x++) {
            if (x < 10 + bar_width) {
                hud_line[y * 320 + x] = s_palette[2]; // 荧光青
            } else {
                hud_line[y * 320 + x] = s_palette[1]; // 幽深暗蓝底轨
            }
        }
    }
    esp_lcd_panel_draw_bitmap(panel, 0, 222, 320, 226, hud_line);
}

// 音频播放任务 (主时钟基准)
static void audio_task(void *arg) {
    const world_execute_header_t *hdr = (const world_execute_header_t *)world_execute_data_bin_start;
    const uint8_t *adpcm_stream = world_execute_data_bin_start + hdr->audio_data_offset;
    uint32_t total_adpcm_bytes = hdr->audio_data_size;

    ESP_LOGI(TAG, "音频任务启动: 采样率 %lu Hz, 总音频 %lu 字节 (%.1f 秒)",
             (unsigned long)hdr->audio_sample_rate,
             (unsigned long)total_adpcm_bytes,
             (double)total_adpcm_bytes / (hdr->audio_sample_rate / 2));

    bsp_audio_set_volume(s_volume);

    while (s_running) {
        if (s_restart_req) {
            s_restart_req = false;
            s_samples_played = 0;
        }

        int16_t predicted = 0;
        int step_idx = 0;
        uint32_t adpcm_offset = 0;

        while (s_running && adpcm_offset < total_adpcm_bytes) {
            if (s_restart_req) break;

            if (s_paused) {
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }

            uint32_t chunk_bytes = AUDIO_CHUNK_ADPCM_BYTES;
            if (adpcm_offset + chunk_bytes > total_adpcm_bytes) {
                chunk_bytes = total_adpcm_bytes - adpcm_offset;
            }

            const uint8_t *src = adpcm_stream + adpcm_offset;
            int sample_count = 0;

            for (uint32_t i = 0; i < chunk_bytes; i++) {
                uint8_t byte_val = src[i];
                s_pcm_buf[sample_count++] = adpcm_decode_nibble((byte_val >> 4) & 0x0F, &predicted, &step_idx);
                s_pcm_buf[sample_count++] = adpcm_decode_nibble(byte_val & 0x0F, &predicted, &step_idx);
            }

            size_t bytes_to_write = (size_t)sample_count * sizeof(int16_t);
            esp_err_t ret = bsp_audio_write(s_pcm_buf, bytes_to_write);
            if (ret != ESP_OK) {
                vTaskDelay(pdMS_TO_TICKS(10));
            } else {
                s_samples_played += sample_count;
            }

            adpcm_offset += chunk_bytes;
        }

        if (!s_restart_req && s_running) {
            ESP_LOGI(TAG, "全曲播放完成，自然结束");
            s_running = false;
            demo_request_exit();
            break;
        }
    }

    ESP_LOGI(TAG, "音频任务退出");
    vTaskDelete(NULL);
}

// 视频渲染任务 (从属时钟刷新)
static void video_task(void *arg) {
    esp_lcd_panel_handle_t panel = bsp_display_panel();
    if (!panel) {
        ESP_LOGE(TAG, "无法获取显示屏句柄");
        s_running = false;
        vTaskDelete(NULL);
        return;
    }

    const world_execute_header_t *hdr = (const world_execute_header_t *)world_execute_data_bin_start;
    if (hdr->magic != WEXE_MAGIC) {
        ESP_LOGE(TAG, "WEXE 魔数不匹配: 0x%08lX (期望 0x%08X)",
                 (unsigned long)hdr->magic, WEXE_MAGIC);
        s_running = false;
        vTaskDelete(NULL);
        return;
    }

    // 读取固化调色板
    s_palette[0] = hdr->palette[0];
    s_palette[1] = hdr->palette[1];
    s_palette[2] = hdr->palette[2];
    s_palette[3] = hdr->palette[3];

    // 配置横屏 (Landscape: 320 x 240)
    esp_lcd_panel_swap_xy(panel, true);
    esp_lcd_panel_mirror(panel, false, true);

    clear_screen_black_landscape();

    const world_execute_index_entry_t *index_table =
        (const world_execute_index_entry_t *)(world_execute_data_bin_start + hdr->video_index_offset);
    const uint8_t *video_stream = world_execute_data_bin_start + hdr->video_data_offset;

    uint32_t total_frames = hdr->total_frames;
    uint32_t fps = hdr->fps;
    uint32_t sample_rate = hdr->audio_sample_rate;
    uint32_t current_frame = 0;
    uint32_t last_offset = 0xFFFFFFFF;

    tinf_init();

    ESP_LOGI(TAG, "视频任务启动: %ux%u @ %u FPS, 4 色调色板, 共 %lu 帧",
             hdr->width, hdr->height, fps, (unsigned long)total_frames);

    while (s_running && current_frame < total_frames) {
        if (s_restart_req) {
            current_frame = 0;
            last_offset = 0xFFFFFFFF;
            clear_screen_black_landscape();
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (s_paused) {
            vTaskDelay(pdMS_TO_TICKS(30));
            continue;
        }

        // 以音频采样数作为绝对时间源
        uint32_t audio_frame = (uint32_t)(((uint64_t)s_samples_played * fps) / sample_rate);

        if (current_frame <= audio_frame) {
            // 追赶或正常刷新
            const world_execute_index_entry_t *entry = &index_table[current_frame];
            uint32_t frame_offset = entry->offset;
            uint16_t comp_size = entry->comp_size;

            // 若与上一帧偏移不同，则解压并渲染
            if (frame_offset != last_offset && comp_size > 0) {
                unsigned int dest_len = sizeof(s_decomp_buf);
                int ret = tinf_zlib_uncompress(s_decomp_buf, &dest_len, video_stream + frame_offset, comp_size);
                if (ret != TINF_OK) {
                    ESP_LOGW(TAG, "帧 %lu 解压警告 (%d)", (unsigned long)current_frame, ret);
                } else {
                    // 分片推送到屏幕 (每次 15 行，共 12 批)
                    for (int s = 0; s < SLICE_COUNT; s++) {
                        int y_start = s * SLICE_LINES;
                        for (int r = 0; r < SLICE_LINES; r++) {
                            int line = y_start + r;
                            const uint8_t *src_line = &s_decomp_buf[line * (VIDEO_W / 4)];
                            uint16_t *dst_line = &s_dma_slice_buf[r * VIDEO_W];

                            // 2-bit 调色板查表解压: 4 像素/字节
                            for (int x = 0; x < VIDEO_W; x += 4) {
                                uint8_t b = src_line[x / 4];
                                dst_line[x + 0] = s_palette[(b >> 6) & 3];
                                dst_line[x + 1] = s_palette[(b >> 4) & 3];
                                dst_line[x + 2] = s_palette[(b >> 2) & 3];
                                dst_line[x + 3] = s_palette[b & 3];
                            }
                        }

                        int screen_y1 = VIDEO_Y_OFFSET + y_start;
                        int screen_y2 = screen_y1 + SLICE_LINES;
                        esp_lcd_panel_draw_bitmap(panel, 0, screen_y1, VIDEO_W, screen_y2, s_dma_slice_buf);
                    }
                }
                last_offset = frame_offset;
            }

            // 每 15 帧 (1 秒) 刷新一次底部进度条
            if (current_frame % 15 == 0) {
                draw_hud_landscape(current_frame, total_frames);
            }

            current_frame++;
            vTaskDelay(pdMS_TO_TICKS(1));
        } else {
            // 视频渲染快于音频进度，等待 5ms 让出 CPU
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }

    // 播放结束，恢复竖屏配置
    clear_screen_black_landscape();
    esp_lcd_panel_swap_xy(panel, false);
    esp_lcd_panel_mirror(panel, false, false);

    ESP_LOGI(TAG, "视频任务退出");
    if (s_running) {
        s_running = false;
        demo_request_exit();
    }
    vTaskDelete(NULL);
}

esp_err_t world_execute_player_start(void) {
    if (s_running) {
        return ESP_ERR_INVALID_STATE;
    }

    const world_execute_header_t *hdr = (const world_execute_header_t *)world_execute_data_bin_start;
    if (hdr->magic != WEXE_MAGIC) {
        ESP_LOGE(TAG, "WEXE 魔数不匹配: 0x%08lX (期望 0x%08X)",
                 (unsigned long)hdr->magic, WEXE_MAGIC);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = bsp_audio_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bsp_audio_init 失败: %s", esp_err_to_name(err));
        return err;
    }

    err = bsp_audio_set_format(hdr->audio_sample_rate, 16, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bsp_audio_set_format 失败: %s", esp_err_to_name(err));
        return err;
    }

    bsp_audio_set_volume(s_volume);

    s_running = true;
    s_paused = false;
    s_restart_req = false;
    s_samples_played = 0;

    // 音频任务优先级高于视频任务，保证音频时钟不卡顿；视频任务赋予 8192 栈以防解压溢出
    BaseType_t ret_a = xTaskCreatePinnedToCore(audio_task, "wexe_audio", 4096, NULL, 5, &s_audio_task_handle, 0);
    BaseType_t ret_v = xTaskCreatePinnedToCore(video_task, "wexe_video", 8192, NULL, 4, &s_video_task_handle, 0);

    if (ret_a != pdPASS || ret_v != pdPASS) {
        ESP_LOGE(TAG, "创建播放任务失败");
        world_execute_player_stop();
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t world_execute_player_stop(void) {
    if (s_running) {
        s_running = false;
        s_paused = false;
        vTaskDelay(pdMS_TO_TICKS(150));
    }

    s_audio_task_handle = NULL;
    s_video_task_handle = NULL;

    esp_lcd_panel_handle_t panel = bsp_display_panel();
    if (panel) {
        esp_lcd_panel_swap_xy(panel, false);
        esp_lcd_panel_mirror(panel, false, false);
    }

    return ESP_OK;
}

bool world_execute_player_is_running(void) {
    return s_running;
}

void world_execute_player_toggle_pause(void) {
    s_paused = !s_paused;
    ESP_LOGI(TAG, "播放状态切换: %s", s_paused ? "暂停" : "继续");
}

void world_execute_player_restart(void) {
    s_restart_req = true;
    s_paused = false;
    ESP_LOGI(TAG, "重头播放请求");
}

void world_execute_player_set_volume(uint8_t volume) {
    if (volume > 100) volume = 100;
    s_volume = volume;
    bsp_audio_set_volume(s_volume);
    ESP_LOGI(TAG, "音量调节: %u%%", s_volume);
}

uint8_t world_execute_player_get_volume(void) {
    return s_volume;
}
