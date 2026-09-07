/* Test-only credentials: common/video.c consumes plaintext PEM material embedded
 * in this development firmware. Production private keys MUST use the PRO2 protected
 * zone, not plaintext filesystem files or raw firmware arrays. See README.md. */
#include "cloud_video.h"
#include "playback.h"
#include "test_video.h"
#include "task.h"
#include <stdio.h>
static void keyframe(void *context) { (void)context; /* Every frame is IDR. */ }
static void release_frame(void *owner, const uint8_t *data) { (void)owner; (void)data; }
void example_run(void) {
    rtk_ameba_device_example_t device = {0};
    SemaphoreHandle_t lock = xSemaphoreCreateMutex();
    playback_t playback = {0, 0};
    TickType_t due = xTaskGetTickCount();
    unsigned int phase = 0;
    if (!lock) return;
    if (example_video_start(&device, lock, NULL, keyframe) != 0) goto done;
    printf("EXAMPLE_READY kind=webrtc_test_video codec=H264\r\n");
    for (;;) {
        if (example_video_poll(&device) < 0) break;
        TickType_t now = xTaskGetTickCount();
        if ((int32_t)(now - due) >= 0) {
            const test_frame_t *frame = &test_frames[playback.index];
            if (rtk_ameba_device_example_on_h264(&device,
                    test_video + frame->offset, frame->size, playback.timestamp,
                    NULL, release_frame) < 0) break;
            playback_advance(&playback, TEST_FRAME_COUNT);
            /* Rational pacing avoids rounding every frame to 66 ms. */
            phase += configTICK_RATE_HZ;
            due += phase / 15U;
            phase %= 15U;
            /* A blocked network must not cause a burst of old frames. */
            if ((int32_t)(now - due) > (int32_t)configTICK_RATE_HZ) due = now;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
done:
    rtk_ameba_device_example_stop(&device);
    vSemaphoreDelete(lock);
}
