// main/world_execute.h —— 《world.execute(me);》赛博 2-bit 调色板播放器接口
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WEXE_MAGIC   0x57455845 // 'WEXE'
#define WEXE_VERSION 1

// 二进制文件头部结构 (48 字节定长)
typedef struct {
    uint32_t magic;               // 'WEXE' = 0x57455845
    uint16_t version;             // 1
    uint16_t width;               // 320
    uint16_t height;              // 180
    uint16_t fps;                 // 15
    uint16_t color_bits;          // 2 (4 色调色板)
    uint16_t palette[4];          // RGB565 x 4
    uint32_t total_frames;        // 3178
    uint32_t audio_sample_rate;   // 16000
    uint32_t audio_data_offset;   // 音频起始偏移
    uint32_t audio_data_size;     // 音频总字节数
    uint32_t video_index_offset;  // 视频索引表偏移
    uint32_t video_data_offset;   // 视频帧数据起始偏移
    uint16_t reserved;            // 对齐填充
} __attribute__((packed)) world_execute_header_t;

// 帧索引表项 (6 字节定长)
typedef struct {
    uint32_t offset;              // 相对 video_data_offset 的偏移
    uint16_t comp_size;           // Deflate 压缩字节数
} __attribute__((packed)) world_execute_index_entry_t;

/**
 * @brief 启动 《world.execute(me);》 播放器
 * @return esp_err_t ESP_OK 成功启动；ESP_ERR_INVALID_STATE 正在运行中
 */
esp_err_t world_execute_player_start(void);

/**
 * @brief 停止播放器并释放资源
 * @return esp_err_t ESP_OK 成功停止
 */
esp_err_t world_execute_player_stop(void);

/**
 * @brief 查询播放器是否正在运行
 */
bool world_execute_player_is_running(void);

/**
 * @brief 切换播放/暂停状态
 */
void world_execute_player_toggle_pause(void);

/**
 * @brief 从头重新播放
 */
void world_execute_player_restart(void);

/**
 * @brief 设置播放音量 (0~100)
 */
void world_execute_player_set_volume(uint8_t volume);

/**
 * @brief 获取当前音量
 */
uint8_t world_execute_player_get_volume(void);

#ifdef __cplusplus
}
#endif
