#ifndef EXAMPLE_PLAYBACK_H
#define EXAMPLE_PLAYBACK_H
#include <stdint.h>
/* All fixture frames are IDR: every frame satisfies a PLI without rewinding time. */
typedef struct { uint32_t index; uint32_t timestamp; } playback_t;
static inline void playback_advance(playback_t *p, uint32_t count) {
    p->index = (p->index + 1U) % count;
    p->timestamp += 6000U; /* H.264 90 kHz / 15 fps; natural RTP wrap. */
}
#endif
