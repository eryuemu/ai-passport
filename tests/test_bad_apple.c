#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "bad_apple.h"
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
    printf("[Host Test] Testing Bad Apple container and decoder...\n");

    FILE *f = fopen("main/bad_apple_data.bin", "rb");
    assert(f != NULL && "Failed to open main/bad_apple_data.bin");

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    assert(file_size > 1000000 && "File size suspiciously small");

    bad_apple_header_t hdr;
    size_t n = fread(&hdr, 1, sizeof(hdr), f);
    assert(n == sizeof(hdr));

    assert(hdr.magic == BAD_APPLE_MAGIC);
    assert(hdr.version == 1);
    assert(hdr.width == 240);
    assert(hdr.height == 180);
    assert(hdr.fps == 20);
    assert(hdr.total_frames == 4383);
    assert(hdr.audio_sample_rate == 16000);
    assert(hdr.audio_channels == 1);

    printf("  Header OK: %ux%u @ %u fps, %u frames, %u Hz audio\n",
           hdr.width, hdr.height, hdr.fps, hdr.total_frames, hdr.audio_sample_rate);

    // Read index table
    uint32_t *index_table = malloc((hdr.total_frames + 1) * sizeof(uint32_t));
    assert(index_table != NULL);
    fseek(f, hdr.index_table_offset, SEEK_SET);
    n = fread(index_table, sizeof(uint32_t), hdr.total_frames + 1, f);
    assert(n == hdr.total_frames + 1);

    assert(index_table[0] == 0);
    assert(index_table[hdr.total_frames] == hdr.video_data_size);

    for (uint32_t i = 0; i < hdr.total_frames; i++) {
        assert(index_table[i+1] > index_table[i]);
    }
    printf("  Index Table OK: 0 to %u bytes\n", hdr.video_data_size);

    // Test decompressing sample frames
    uint8_t mono_buf[5400];
    uint32_t test_frames[] = {0, 100, 500, 1000, 2000, 3000, 4000, 4382};
    for (size_t i = 0; i < sizeof(test_frames)/sizeof(test_frames[0]); i++) {
        uint32_t frame_idx = test_frames[i];
        uint32_t offset = index_table[frame_idx];
        uint32_t size = index_table[frame_idx + 1] - offset;

        uint8_t *comp_buf = malloc(size);
        assert(comp_buf != NULL);
        fseek(f, hdr.video_data_offset + offset, SEEK_SET);
        n = fread(comp_buf, 1, size, f);
        assert(n == size);

        unsigned int dest_len = sizeof(mono_buf);
        int res = tinf_zlib_uncompress(mono_buf, &dest_len, comp_buf, size);
        assert(res == TINF_OK);
        assert(dest_len == sizeof(mono_buf));

        free(comp_buf);
    }
    printf("  Frame decompression OK on sample frames!\n");

    // Test decoding audio samples
    fseek(f, hdr.audio_data_offset, SEEK_SET);
    uint8_t adpcm_chunk[256];
    n = fread(adpcm_chunk, 1, sizeof(adpcm_chunk), f);
    assert(n == sizeof(adpcm_chunk));

    int16_t predicted = 0;
    int step_idx = 0;
    for (size_t i = 0; i < sizeof(adpcm_chunk); i++) {
        uint8_t b = adpcm_chunk[i];
        int16_t s1 = adpcm_decode_nibble(b & 0x0F, &predicted, &step_idx);
        int16_t s2 = adpcm_decode_nibble((b >> 4) & 0x0F, &predicted, &step_idx);
        (void)s1; (void)s2;
    }
    printf("  Audio ADPCM decode OK!\n");

    free(index_table);
    fclose(f);

    printf("[Host Test] All Bad Apple checks PASSED!\n");
    return 0;
}
