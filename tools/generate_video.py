#!/usr/bin/env python3
"""Generate synthetic, silent 320x240 H.264, 15 fps, 2 seconds, all IDR."""
from pathlib import Path
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as tmp:
    video = Path(tmp) / 'video.h264'
    subprocess.run(['ffmpeg', '-hide_banner', '-loglevel', 'error', '-f', 'lavfi',
        '-i', 'testsrc2=size=320x240:rate=15', '-t', '2', '-an', '-c:v', 'libx264',
        '-profile:v', 'baseline', '-level', '3.1', '-pix_fmt', 'yuv420p',
        '-threads', '1', '-g', '1', '-bf', '0', '-crf', '32',
        '-x264-params', 'aud=1:repeat-headers=1:scenecut=0', '-f', 'h264', str(video)], check=True)
    data = video.read_bytes()
starts = [m.start() for m in re.finditer(b'\x00\x00\x00\x01\x09', data)]
assert len(starts) == 30 and starts[0] == 0
ends = starts[1:] + [len(data)]
(root / 'assets/test_video.h').write_text('''/* Generated synthetic fixture; regenerate with tools/generate_video.py. */
#ifndef TEST_VIDEO_H
#define TEST_VIDEO_H
#include <stdint.h>
#include <stddef.h>
#define TEST_FRAME_COUNT 30U
typedef struct { size_t offset; size_t size; } test_frame_t;
extern const uint8_t test_video[];
extern const test_frame_t test_frames[TEST_FRAME_COUNT];
#endif
''')
lines = ['#include "test_video.h"', 'const uint8_t test_video[] = {']
lines += ['    ' + ','.join(f'0x{x:02x}' for x in data[i:i+16]) + ',' for i in range(0,len(data),16)]
lines += ['};', 'const test_frame_t test_frames[TEST_FRAME_COUNT] = {']
lines += [f'    {{{a}U, {b-a}U}},' for a,b in zip(starts,ends)]
lines += ['};', '']
(root/'assets/test_video.c').write_text('\n'.join(lines))
print(f'Generated {len(starts)} all-IDR frames, {len(data)} bytes')
