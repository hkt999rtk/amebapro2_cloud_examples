#include "playback.h"
#include "test_video.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(int argc, char **argv) {
    playback_t p = {0, 0};
    for (unsigned int i = 0; i < TEST_FRAME_COUNT * 5; ++i) {
        assert(p.index == i % TEST_FRAME_COUNT);
        assert(p.timestamp == i * 6000U);
        playback_advance(&p, TEST_FRAME_COUNT);
    }
    p.timestamp = UINT32_MAX - 2999U;
    playback_advance(&p, TEST_FRAME_COUNT);
    assert(p.timestamp == 3000U);
    assert(argc == 2);
    FILE *out = fopen(argv[1], "wb");
    assert(out);
    /* Write three complete loops: decode verifies loop boundary independently. */
    for (unsigned int loop = 0; loop < 3; ++loop)
        for (unsigned int i = 0; i < TEST_FRAME_COUNT; ++i)
            assert(fwrite(test_video + test_frames[i].offset, 1,
                          test_frames[i].size, out) == test_frames[i].size);
    assert(fclose(out) == 0);
    return 0;
}
