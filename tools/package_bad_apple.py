#!/usr/bin/env python3
"""
FoloToy AI Passport - Bad Apple Asset Packager
Transcodes Bad Apple MP4 into an embedded 1-bit frame sequence and 16kHz IMA-ADPCM audio.
"""

import os
import sys
import struct
import zlib
import subprocess
import argparse

STEP_TABLE = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
]
INDEX_TABLE = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8]

def encode_ima_adpcm(pcm_samples):
    predicted = 0
    step_idx = 0
    out_bytes = bytearray()
    
    for i in range(0, len(pcm_samples), 2):
        s1 = pcm_samples[i]
        s2 = pcm_samples[i+1] if i+1 < len(pcm_samples) else s1
        
        def encode_sample(sample):
            nonlocal predicted, step_idx
            step = STEP_TABLE[step_idx]
            diff = sample - predicted
            nibble = 0
            if diff < 0:
                nibble = 8
                diff = -diff
            mask = 4
            tempstep = step
            for _ in range(3):
                if diff >= tempstep:
                    nibble |= mask
                    diff -= tempstep
                tempstep >>= 1
                mask >>= 1
            
            diff = step >> 3
            if nibble & 4: diff += step
            if nibble & 2: diff += step >> 1
            if nibble & 1: diff += step >> 2
            if nibble & 8: predicted -= diff
            else: predicted += diff
            predicted = max(-32768, min(32767, predicted))
            
            step_idx += INDEX_TABLE[nibble & 0x07]
            step_idx = max(0, min(88, step_idx))
            return nibble
        
        n1 = encode_sample(s1)
        n2 = encode_sample(s2)
        out_bytes.append((n2 << 4) | (n1 & 0x0F))
    return out_bytes

def package(input_mp4, output_bin, width=240, height=180, fps=20, sample_rate=16000):
    print(f"[1/4] Extracting monochrome video frames ({width}x{height} @ {fps} fps)...")
    cmd_v = [
        'ffmpeg', '-y', '-i', input_mp4,
        '-vf', f'fps={fps},scale={width}:{height},format=monob',
        '-f', 'rawvideo', '-'
    ]
    proc_v = subprocess.Popen(cmd_v, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    raw_video, _ = proc_v.communicate()
    
    frame_bytes = (width * height) // 8
    total_frames = len(raw_video) // frame_bytes
    print(f"      Extracted {total_frames} frames ({len(raw_video) / 1024 / 1024:.2f} MB raw)")
    
    print(f"[2/4] Compressing frames individually with zlib (Deflate)...")
    offsets = [0]
    comp_frames_data = bytearray()
    curr_offset = 0
    for i in range(total_frames):
        f_raw = raw_video[i * frame_bytes : (i + 1) * frame_bytes]
        comp = zlib.compress(f_raw, level=9)
        comp_frames_data.extend(comp)
        curr_offset += len(comp)
        offsets.append(curr_offset)
    
    print(f"      Compressed video payload: {len(comp_frames_data) / 1024 / 1024:.2f} MB")
    
    print(f"[3/4] Extracting and encoding audio ({sample_rate} Hz mono IMA-ADPCM)...")
    cmd_a = [
        'ffmpeg', '-y', '-i', input_mp4,
        '-vn', '-ar', str(sample_rate), '-ac', '1', '-f', 's16le', '-'
    ]
    proc_a = subprocess.Popen(cmd_a, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    raw_pcm, _ = proc_a.communicate()
    
    num_samples = len(raw_pcm) // 2
    pcm_samples = struct.unpack(f'<{num_samples}h', raw_pcm)
    adpcm_data = encode_ima_adpcm(pcm_samples)
    print(f"      Encoded {num_samples} audio samples ({len(adpcm_data) / 1024 / 1024:.2f} MB ADPCM)")
    
    print(f"[4/4] Assembling final container binary...")
    HEADER_SIZE = 64
    index_table_bytes = struct.pack(f'<{len(offsets)}I', *offsets)
    index_table_offset = HEADER_SIZE
    video_data_offset = index_table_offset + len(index_table_bytes)
    audio_data_offset = video_data_offset + len(comp_frames_data)
    
    header = struct.pack(
        '<IIHHHHIIIIIIIII12s',
        0x41444142,         # magic: 'BADA'
        1,                  # version: 1
        width,              # width: 240
        height,             # height: 180
        fps,                # fps: 20
        0,                  # reserved16
        total_frames,       # total_frames
        sample_rate,        # audio_sample_rate
        1,                  # audio_channels
        num_samples,        # audio_total_samples
        len(adpcm_data),    # audio_data_size
        len(comp_frames_data), # video_data_size
        index_table_offset, # index_table_offset
        video_data_offset,  # video_data_offset
        audio_data_offset,  # audio_data_offset
        b'\x00' * 12        # reserved
    )
    assert len(header) == HEADER_SIZE, f"Header size mismatch: {len(header)} != {HEADER_SIZE}"
    
    os.makedirs(os.path.dirname(os.path.abspath(output_bin)), exist_ok=True)
    with open(output_bin, 'wb') as f:
        f.write(header)
        f.write(index_table_bytes)
        f.write(comp_frames_data)
        f.write(adpcm_data)
    
    total_size = os.path.getsize(output_bin)
    print(f"SUCCESS! Output: {output_bin}")
    print(f"Total package size: {total_size} bytes ({total_size / 1024 / 1024:.2f} MB)")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Package Bad Apple MP4 for FoloToy AI Passport")
    parser.add_argument("input", help="Path to input MP4 file")
    parser.add_argument("-o", "--output", default="main/bad_apple_data.bin", help="Output bin path")
    parser.add_argument("--fps", type=int, default=20, help="Frame rate (default: 20)")
    args = parser.parse_args()
    
    package(args.input, args.output, fps=args.fps)
