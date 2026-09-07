#include "network.h"
#include "rtk_ameba_firmware_config.h"
#include "rtk_ameba_credential.h"
#include "rtk_ameba_https.h"
#include "rtk_ameba_mqtt_transport.h"
#include "rtk_token_jwt.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include "mqtt_identity.h"

static int receive_message(void *context, const char *json, size_t size) {
    (void)context; (void)json;
    printf("EXAMPLE_MQTT_RECEIVED bytes=%u\r\n", (unsigned int)size);
    return 0;
}
static void diagnostic(void *context, const char *code) {
    (void)context;
    printf("EXAMPLE_MQTT event=%s\r\n", code);
}
void example_run(void) {
    const rtk_ameba_firmware_config_t *local = &rtk_ameba_firmware_config;
    cloud_mqtt_identity_t state = {0};
    state.device_id = local->device_id;
    state.unix_time = example_unix_time;
    rtk_ameba_mqtt_transport_t *mqtt = NULL;
    rtk_ameba_pem_credential_config_t credential = {0};
    rtk_ameba_https_config_t https = {0};
    rtk_ameba_mqtt_transport_config_t config = {0};
    static const unsigned char presence[] = "{\"kind\":\"legacy_status\",\"metadata\":{}}";
    https.connect_timeout_ms = 10000U;
    https.io_timeout_ms = 15000U;
    https.max_header_bytes = 8192U;
    /* DEVELOPMENT ONLY: these PEM buffers come from plaintext host files and are
     * embedded in the firmware solely to simplify testing. Do not retain plaintext
     * private key/certificate files in a production filesystem or ship raw key arrays.
     * Production private keys MUST reside in the PRO2 protected zone; replace this
     * path with the product's protected-zone key operations and certificate provisioning.
     * Protected-zone integration is NOT implemented by this example. */
    credential.device_certificate_chain_pem = local->device_certificate_chain_pem;
    credential.device_certificate_chain_pem_length = local->device_certificate_chain_pem_length;
    credential.device_private_key_pem = local->device_private_key_pem;
    credential.device_private_key_pem_length = local->device_private_key_pem_length;
    credential.server_ca_pem = local->https_server_ca_pem;
    credential.server_ca_pem_length = local->https_server_ca_pem_length;
    credential.token_url = local->token_url;
    credential.device_id = local->device_id;
    credential.token_expiry_seconds = 300U;
    credential.unix_time = example_unix_time;
    credential.https_context = &https;
    if (rtk_ameba_p256_token_provider_create(&credential, &state.provider) != RTK_STATUS_OK)
        goto done;
    config.broker_host = local->mqtt_broker_host;
    config.broker_port = local->mqtt_broker_port;
    config.command_topic = local->mqtt_command_topic;
    config.presence_topic = local->mqtt_presence_topic;
    config.presence_payload = presence;
    config.presence_payload_size = sizeof(presence) - 1;
    config.tenant_topic_prefix = local->mqtt_tenant_topic_prefix;
    config.max_event_bytes = 64U * 1024U;
    config.keep_alive_seconds = 30U;
    config.io_timeout_ms = 15000U;
    config.reconnect_initial_delay_ms = 500U;
    config.reconnect_max_delay_ms = 30000U;
    config.use_tls = 1;
    config.server_ca_pem = local->mqtt_server_ca_pem;
    config.server_ca_pem_length = local->mqtt_server_ca_pem_length;
    config.get_credentials = cloud_mqtt_credentials;
    config.credential_context = &state;
    config.unix_time = example_unix_time;
    config.diagnostic = diagnostic;
    if (rtk_ameba_mqtt_transport_create(&config, &mqtt) != 0 ||
        rtk_ameba_mqtt_transport_start(mqtt) != 0) goto done;
    printf("EXAMPLE_READY kind=mqtt publish=presence\r\n");
    while (example_network_ready() && rtk_ameba_mqtt_transport_poll_event(mqtt, receive_message, NULL) >= 0)
        vTaskDelay(pdMS_TO_TICKS(2));
done:
    rtk_ameba_mqtt_transport_destroy(mqtt);
    if (state.provider.destroy) state.provider.destroy(state.provider.context);
}
