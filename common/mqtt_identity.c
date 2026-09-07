/* Mirrors the external RTK service's JWT-to-broker mapping for MQTT-only use. */
#include "mqtt_identity.h"
#include "rtk_token_jwt.h"
#define RTK_AMEBA_MQTT_TOKEN_CAPACITY 8192U
static void clear_secret(void *value, size_t size) {
    volatile unsigned char *cursor = (volatile unsigned char *)value;
    while (size-- != 0U) *cursor++ = 0U;
}

int cloud_mqtt_credentials(
    void *context,
    char *client_id, size_t client_id_capacity,
    char *username, size_t username_capacity,
    char *password, size_t password_capacity,
    uint64_t *expires_at) {
    cloud_mqtt_identity_t *state = (cloud_mqtt_identity_t *)context;
    rtk_token_provider_t *provider;
    uint64_t provider_expiry = 0U;
    uint64_t claim_expiry = 0U;
    uint64_t now;
    int result = -1;
    if (state == NULL || client_id == NULL || username == NULL ||
        password == NULL || expires_at == NULL ||
        password_capacity < RTK_AMEBA_MQTT_TOKEN_CAPACITY) return -1;
    provider = &state->provider;
    if (provider == NULL || provider->get_access_token == NULL ||
        provider->refresh == NULL) return -1;
    now = state->unix_time(NULL);
    /* The generic provider contract does not promise an internal refresh
     * margin. Force invalidation before every broker CONNECT so a reconnect
     * can never reuse a cached short-lived JWT. */
    if (provider->refresh(provider->context) != 0 ||
        provider->get_access_token(provider->context, password,
            password_capacity, &provider_expiry) != 0 ||
        rtk_token_jwt_extract_mqtt(password, state->device_id, now,
            username, username_capacity, client_id, client_id_capacity,
            &claim_expiry) != 0 || provider_expiry != claim_expiry)
        goto cleanup;
    *expires_at = claim_expiry;
    result = 0;
cleanup:
    if (result != 0) {
        clear_secret(client_id, client_id_capacity);
        clear_secret(username, username_capacity);
        clear_secret(password, password_capacity);
        *expires_at = 0U;
    }
    return result;
}
