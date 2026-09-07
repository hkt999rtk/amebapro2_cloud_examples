#include "cloud_video.h"
#include "network.h"
#include "rtk_ameba_firmware_config.h"
#include <string.h>
#include <stdio.h>
#include "task.h"
static void lock_queue(void *p) { (void)xSemaphoreTake(p, portMAX_DELAY); }
static void unlock_queue(void *p) { (void)xSemaphoreGive(p); }
int example_video_start(rtk_ameba_device_example_t *device, SemaphoreHandle_t lock,
                        void *media, void (*keyframe)(void *)) {
    rtk_ameba_device_example_config_t config;
    const rtk_ameba_firmware_config_t *local = &rtk_ameba_firmware_config;
    memset(&config, 0, sizeof(config));
    config.device_id = local->device_id;
    config.token_url = local->token_url;
    config.cloud_base_url = local->cloud_base_url;
    config.mqtt_broker_host = local->mqtt_broker_host;
    config.mqtt_broker_port = local->mqtt_broker_port;
    config.mqtt_command_topic = local->mqtt_command_topic;
    config.mqtt_presence_topic = local->mqtt_presence_topic;
    config.mqtt_tenant_topic_prefix = local->mqtt_tenant_topic_prefix;
    /* DEVELOPMENT ONLY: these PEM buffers come from plaintext host files and are
     * embedded in the firmware solely to simplify testing. Do not retain plaintext
     * private key/certificate files in a production filesystem or ship raw key arrays.
     * Production private keys MUST reside in the PRO2 protected zone; replace this
     * path with the product's protected-zone key operations and certificate provisioning.
     * Protected-zone integration is NOT implemented by this example. */
    config.device_certificate_chain_pem = local->device_certificate_chain_pem;
    config.device_certificate_chain_pem_length =
        local->device_certificate_chain_pem_length;
    config.device_private_key_pem = local->device_private_key_pem;
    config.device_private_key_pem_length = local->device_private_key_pem_length;
    config.https_server_ca_pem = local->https_server_ca_pem;
    config.https_server_ca_pem_length = local->https_server_ca_pem_length;
    config.mqtt_server_ca_pem = local->mqtt_server_ca_pem;
    config.mqtt_server_ca_pem_length = local->mqtt_server_ca_pem_length;
    config.unix_time = example_unix_time;
    config.video_context = media;
    config.request_keyframe = keyframe;
    config.queue_lock_context = lock;
    config.queue_lock = lock_queue;
    config.queue_unlock = unlock_queue;
    config.frame_queue_capacity = 3;
    config.force_relay = local->force_relay;
    return rtk_ameba_device_example_start(device, &config);
}

int example_video_poll(rtk_ameba_device_example_t *device) {
    static TickType_t last_report;
    TickType_t now = xTaskGetTickCount();
    if (!example_network_ready()) return -1;
    if ((now - last_report) >= pdMS_TO_TICKS(30000U)) {
        rtk_ameba_device_stats_t stats;
        rtk_ameba_mmf_stats_t queue;
        rtk_ameba_device_example_get_stats(device, &stats, &queue);
        printf("EXAMPLE_STATS sent=%llu dropped=%llu queue=%u heap=%u heap_min=%u\r\n",
               (unsigned long long)stats.frames_sent,
               (unsigned long long)(stats.frames_dropped + queue.frames_dropped_queue),
               (unsigned int)queue.current_depth,
               (unsigned int)xPortGetFreeHeapSize(),
               (unsigned int)xPortGetMinimumEverFreeHeapSize());
        last_report = now;
    }
    return rtk_ameba_device_example_poll(device);
}
