/* Adapted from the RTK SDK firmware example; protocol implementations remain external. */
#include "network.h"
#include "rtk_ameba_firmware_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "wifi_conf.h"
#include "lwip_netconf.h"
#include "sntp/sntp.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#define RTK_EXAMPLE_WIFI_ATTEMPTS 3U
#define RTK_EXAMPLE_DHCP_TIMEOUT_MS 30000U
#define RTK_EXAMPLE_SNTP_TIMEOUT_MS 30000U
#define RTK_EXAMPLE_VALID_EPOCH 1577836800ULL
static int has_ipv4_address(void) {
    const unsigned char *address = LwIP_GetIP(0);
    return address != NULL &&
        (address[0] != 0U || address[1] != 0U ||
         address[2] != 0U || address[3] != 0U);
}

int example_network_start(void) {
    rtw_network_info_t connection;
    unsigned int attempt;
    unsigned int elapsed;
    const rtk_ameba_firmware_config_t *config = &rtk_ameba_firmware_config;

    if (strcmp(config->wifi_security, "provisioned") == 0) {
        int dhcp_started = 0;
        for (attempt = 1U; attempt <= RTK_EXAMPLE_WIFI_ATTEMPTS; ++attempt) {
            for (elapsed = 0U; elapsed < RTK_EXAMPLE_DHCP_TIMEOUT_MS;
                 elapsed += 250U) {
                if (wifi_is_connected_to_ap() == RTW_SUCCESS) {
                    /* Do not restart DHCP while waiting for a lease. */
                    if (!has_ipv4_address() && !dhcp_started) {
                        dhcp_started = 1;
                        (void)LwIP_DHCP(0, DHCP_START);
                    }
                    if (has_ipv4_address()) {
                        printf("RTK_AMEBA_EXAMPLE_STAGE network=ready mode=provisioned attempt=%u\r\n",
                               attempt);
                        return 0;
                    }
                } else {
                    dhcp_started = 0;
                }
                vTaskDelay(pdMS_TO_TICKS(250U));
            }
            printf("RTK_AMEBA_EXAMPLE_RETRY stage=network mode=provisioned attempt=%u\r\n",
                   attempt);
        }
        return -1;
    }
    if (strlen(config->wifi_ssid) >= sizeof(connection.ssid.val) ||
        strlen(config->wifi_password) > 64U)
        return -1;
    for (attempt = 1U; attempt <= RTK_EXAMPLE_WIFI_ATTEMPTS; ++attempt) {
        memset(&connection, 0, sizeof(connection));
        memcpy(connection.ssid.val, config->wifi_ssid,
               strlen(config->wifi_ssid));
        connection.ssid.len = (unsigned char)strlen(config->wifi_ssid);
        connection.password = (unsigned char *)config->wifi_password;
        connection.password_len = (int)strlen(config->wifi_password);
        connection.security_type = strcmp(config->wifi_security, "open") == 0
            ? RTW_SECURITY_OPEN : RTW_SECURITY_WPA2_AES_PSK;
        if (wifi_connect(&connection, 1U) == RTW_SUCCESS) {
            (void)LwIP_DHCP(0, DHCP_START);
            for (elapsed = 0U; elapsed < RTK_EXAMPLE_DHCP_TIMEOUT_MS;
                 elapsed += 250U) {
                if (has_ipv4_address()) {
                    printf("RTK_AMEBA_EXAMPLE_STAGE network=ready attempt=%u\r\n",
                           attempt);
                    return 0;
                }
                vTaskDelay(pdMS_TO_TICKS(250U));
            }
        }
        printf("RTK_AMEBA_EXAMPLE_RETRY stage=network attempt=%u\r\n",
               attempt);
        (void)LwIP_DHCP(0, DHCP_STOP);
        (void)wifi_disconnect();
        vTaskDelay(pdMS_TO_TICKS(1000U * attempt));
    }
    return -1;
}

uint64_t example_unix_time(void *context) {
    unsigned int update_tick = 0U;
    uint64_t seconds;
    (void)context;
#if defined(CONFIG_SYSTEM_TIME64) && CONFIG_SYSTEM_TIME64
    long long update_seconds = 0;
    long long update_microseconds = 0;
#else
    time_t update_seconds = 0;
    time_t update_microseconds = 0;
#endif
    sntp_get_lasttime(&update_seconds, &update_microseconds, &update_tick);
    if (update_tick == 0U || update_seconds < 0) return 0U;
    seconds = (uint64_t)update_seconds;
    seconds += (uint64_t)(xTaskGetTickCount() - update_tick) /
               (uint64_t)configTICK_RATE_HZ;
    return seconds;
}

int example_time_start(void) {
    unsigned int elapsed;
    sntp_set_max_tries(5U);
    sntp_init();
    for (elapsed = 0U; elapsed < RTK_EXAMPLE_SNTP_TIMEOUT_MS;
         elapsed += 250U) {
        if (example_unix_time(NULL) >= RTK_EXAMPLE_VALID_EPOCH) {
            printf("RTK_AMEBA_EXAMPLE_STAGE time=ready\r\n");
            return 0;
        }
        vTaskDelay(pdMS_TO_TICKS(250U));
    }
    return -1;
}


void example_network_stop(void) {
    sntp_stop();
    /* Provisioning owns the association: keep it available for application retries. */
    if (strcmp(rtk_ameba_firmware_config.wifi_security, "provisioned") == 0)
        return;
    (void)LwIP_DHCP(0, DHCP_STOP);
    (void)wifi_disconnect();
}

int example_network_ready(void) {
    return wifi_is_connected_to_ap() == RTW_SUCCESS && has_ipv4_address();
}
