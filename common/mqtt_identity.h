#ifndef CLOUD_MQTT_IDENTITY_H
#define CLOUD_MQTT_IDENTITY_H
#include "rtk_cloud_sdk/rtk_cloud_sdk.h"
typedef struct {
    rtk_token_provider_t provider;
    const char *device_id;
    uint64_t (*unix_time)(void *);
} cloud_mqtt_identity_t;
int cloud_mqtt_credentials(void *, char *, size_t, char *, size_t,
                           char *, size_t, uint64_t *);
#endif
