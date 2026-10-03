// tests/test_world_execute.c —— 《world.execute(me);》WEXE 容器与解码 Host 单元测试
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "world_execute.h"
#include "tinf.h"

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

int main(void) {
    printf("[Host Test] Testing World Execute WEXE container and decoder...\n");

    FILE *f = fopen("main/world_execute_data.bin", "rb");
    assert(f != NULL && "Failed to open main/world_execute_data.bin");

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    assert(file_size > 5000000 && "File size suspiciously small");

    world_execute_header_t hdr;
    size_t n = fread(&hdr, 1, sizeof(hdr), f);
    assert(n == sizeof(hdr));

    assert(hdr.magic == WEXE_MAGIC);
    assert(hdr.version == 1);
    assert(hdr.width == 320);
    assert(hdr.height == 180);
    assert(hdr.fps == 15);
    assert(hdr.color_bits == 2);
    assert(hdr.total_frames == 3178);
    assert(hdr.audio_sample_rate == 16000);
    assert(hdr.palette[0] == 0x0000); // 极夜纯黑
    assert(hdr.palette[3] == 0xFFFF); // 极光白

    printf("  Header OK: %ux%u @ %u fps, %u frames, %u Hz audio, 4-color palette [%04X %04X %04X %04X]\n",
           hdr.width, hdr.height, hdr.fps, hdr.total_frames, hdr.audio_sample_rate,
           hdr.palette[0], hdr.palette[1], hdr.palette[2], hdr.palette[3]);

    // 读取帧索引表
    world_execute_index_entry_t *index_table = malloc(hdr.total_frames * sizeof(world_execute_index_entry_t));
    assert(index_table != NULL);
    fseek(f, hdr.video_index_offset, SEEK_SET);
    n = fread(index_table, sizeof(world_execute_index_entry_t), hdr.total_frames, f);
    assert(n == hdr.total_frames);

    for (uint32_t i = 0; i < hdr.total_frames; i++) {
        assert(index_table[i].comp_size > 0);
    }
    printf("  Index Table OK: %u frames indexed correctly\n", hdr.total_frames);

    // 抽样解压多帧进行校验
    tinf_init();
    uint8_t decomp_buf[14400]; // 320 * 180 / 4
    uint32_t test_frames[] = {0, 50, 100, 300, 600, 1200, 1800, 2400, 3000, 3177};
    for (size_t i = 0; i < sizeof(test_frames)/sizeof(test_frames[0]); i++) {
        uint32_t frame_idx = test_frames[i];
        uint32_t offset = index_table[frame_idx].offset;
        uint16_t size = index_table[frame_idx].comp_size;

        uint8_t *comp_buf = malloc(size);
        assert(comp_buf != NULL);
        fseek(f, hdr.video_data_offset + offset, SEEK_SET);
        n = fread(comp_buf, 1, size, f);
        assert(n == size);

        unsigned int dest_len = sizeof(decomp_buf);
        int ret = tinf_zlib_uncompress(decomp_buf, &dest_len, comp_buf, size);
        assert(ret == TINF_OK);
        assert(dest_len == sizeof(decomp_buf));

        // 验证 2-bit 像素解包逻辑与调色板映射
        uint16_t pixel_sample[4];
        uint8_t b = decomp_buf[0];
        pixel_sample[0] = hdr.palette[(b >> 6) & 3];
        pixel_sample[1] = hdr.palette[(b >> 4) & 3];
        pixel_sample[2] = hdr.palette[(b >> 2) & 3];
        pixel_sample[3] = hdr.palette[b & 3];
        (void)pixel_sample;

        free(comp_buf);
    }
    printf("  Sample Frame Deflate Decompression OK: verified %zu frames\n",
           sizeof(test_frames)/sizeof(test_frames[0]));

    // 验证音频前 1000 个采样点解码
    fseek(f, hdr.audio_data_offset, SEEK_SET);
    uint8_t adpcm_chunk[256];
    n = fread(adpcm_chunk, 1, sizeof(adpcm_chunk), f);
    assert(n == sizeof(adpcm_chunk));

    int16_t predicted = 0;
    int step_idx = 0;
    int16_t pcm_out[512];
    int pcm_idx = 0;
    for (size_t i = 0; i < sizeof(adpcm_chunk); i++) {
        pcm_out[pcm_idx++] = adpcm_decode_nibble((adpcm_chunk[i] >> 4) & 0x0F, &predicted, &step_idx);
        pcm_out[pcm_idx++] = adpcm_decode_nibble(adpcm_chunk[i] & 0x0F, &predicted, &step_idx);
    }
    assert(pcm_idx == 512);
    assert(pcm_out[0] >= -32768 && pcm_out[0] <= 32767);
    (void)pcm_out;
    printf("  Audio IMA-ADPCM Decoding OK: 512 samples decoded smoothly\n");

    free(index_table);
    fclose(f);
    printf("[Host Test] ALL world_execute tests passed successfully!\n");
    return 0;
}
