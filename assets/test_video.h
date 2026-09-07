/* Generated synthetic fixture; regenerate with tools/generate_video.py. */
#ifndef TEST_VIDEO_H
#define TEST_VIDEO_H
#include <stdint.h>
#include <stddef.h>
#define TEST_FRAME_COUNT 30U
typedef struct { size_t offset; size_t size; } test_frame_t;
extern const uint8_t test_video[];
extern const test_frame_t test_frames[TEST_FRAME_COUNT];
#endif
