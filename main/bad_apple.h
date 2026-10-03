#pragma once

#include <stdint.h>
#include <stdbool.h>
#ifdef ESP_PLATFORM
#include "esp_err.h"
#else
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_ARG -2
#endif

#define BAD_APPLE_MAGIC 0x41444142 // 'BADA' in little-endian

typedef struct __attribute__((packed)) {
    uint32_t magic;              // 'BADA'
    uint32_t version;            // 1
    uint16_t width;              // 240
    uint16_t height;             // 180
    uint16_t fps;                // 20
    uint16_t reserved16;
    uint32_t total_frames;       // 4383
    uint32_t audio_sample_rate;  // 16000
    uint32_t audio_channels;     // 1
    uint32_t audio_total_samples;// 3507142
    uint32_t audio_data_size;    // 1753571 bytes
    uint32_t video_data_size;    // 2810848 bytes
    uint32_t index_table_offset; // 64
    uint32_t video_data_offset;
    uint32_t audio_data_offset;
    uint8_t  reserved[12];
} bad_apple_header_t;

// Start playback
esp_err_t bad_apple_player_start(void);

// Stop playback and release resources
void bad_apple_player_stop(void);

// Handle physical button inputs
void bad_apple_player_on_button(uint8_t btn, uint8_t event);

// Check if currently playing
bool bad_apple_player_is_playing(void);
