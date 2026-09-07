/* Test-only credentials: common/video.c consumes plaintext PEM material embedded
 * in this development firmware. Production private keys MUST use the PRO2 protected
 * zone, not plaintext filesystem files or raw firmware arrays. See README.md. */
#include "cloud_video.h"
#include "rtk_ameba_mmf_bridge.h"
#include "task.h"
#include <stdio.h>
static void lock_media(void *p) { (void)xSemaphoreTake(p, portMAX_DELAY); }
static void unlock_media(void *p) { (void)xSemaphoreGive(p); }
void example_run(void) {
    rtk_ameba_device_example_t device = {0};
    rtk_ameba_mmf_bridge_t media = {0};
    SemaphoreHandle_t lock = xSemaphoreCreateMutex();
    if (!lock) return;
    if (example_video_start(&device, lock, &media,
                           rtk_ameba_mmf_bridge_request_keyframe) != 0) goto done;
    if (rtk_ameba_mmf_bridge_start(&media, &device, lock,
                                 lock_media, unlock_media) != 0) goto done;
    printf("EXAMPLE_READY kind=webrtc_camera codec=H264\r\n");
    while (example_video_poll(&device) >= 0)
        vTaskDelay(pdMS_TO_TICKS(2));
done:
    rtk_ameba_mmf_bridge_stop_producer(&media);
    rtk_ameba_device_example_stop(&device);
    if (media.example && rtk_ameba_mmf_bridge_deinit(&media) != 0)
        printf("EXAMPLE_ERROR stage=frame_release\r\n");
    vSemaphoreDelete(lock);
}
