#include "network.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
void example_run(void);
static void run(void *unused) {
    (void)unused;
    for (;;) {
        if (example_network_start() == 0 && example_time_start() == 0) {
            printf("EXAMPLE_NETWORK_READY\r\n");
            example_run();
        } else printf("EXAMPLE_ERROR stage=network_or_time\r\n");
        example_network_stop();
        printf("EXAMPLE_RESTART delay_ms=5000\r\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
void app_example(void) {
    if (xTaskCreate(run, "cloud_example", 16U * 1024U, NULL,
                    tskIDLE_PRIORITY + 2, NULL) != pdPASS)
        printf("EXAMPLE_ERROR stage=task_create\r\n");
}
