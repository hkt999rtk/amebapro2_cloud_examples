#!/usr/bin/env python3
"""Run the actual network wrapper with deterministic Wi-Fi/DHCP platform doubles."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
HEADER = r'''
#ifndef PLATFORM_STUB_H
#define PLATFORM_STUB_H
#include <stdint.h>
#include <time.h>
#define RTW_SUCCESS 0
#define RTW_SECURITY_OPEN 0
#define RTW_SECURITY_WPA2_AES_PSK 1
#define DHCP_START 0
#define DHCP_STOP 1
#define pdMS_TO_TICKS(x) (x)
#define configTICK_RATE_HZ 1000
struct rtw_network_info { struct {char val[33]; unsigned char len;} ssid; unsigned char *password; int password_len,security_type; };
typedef struct rtw_network_info rtw_network_info_t;
typedef struct { const char *wifi_security,*wifi_ssid,*wifi_password; } rtk_ameba_firmware_config_t;
extern rtk_ameba_firmware_config_t rtk_ameba_firmware_config;
int wifi_is_connected_to_ap(void);
int wifi_connect(rtw_network_info_t *, unsigned);
int wifi_disconnect(void);
unsigned char *LwIP_GetIP(int);
int LwIP_DHCP(int,int);
void vTaskDelay(unsigned);
unsigned xTaskGetTickCount(void);
void sntp_get_lasttime(time_t *,time_t *,unsigned *);
void sntp_set_max_tries(unsigned);
void sntp_init(void);
void sntp_stop(void);
#endif
'''
with tempfile.TemporaryDirectory() as tmp:
    out = Path(tmp)
    (out/'platform.h').write_text(HEADER)
    for name in ['rtk_ameba_firmware_config.h','FreeRTOS.h','task.h','wifi_conf.h','lwip_netconf.h','sntp/sntp.h']:
        p=out/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text('#include "platform.h"\n')
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-I'+str(out),'-I'+str(ROOT/'common'),str(ROOT/'common/network.c'),str(ROOT/'tests/test_network.c'),'-o',str(out/'network')],check=True)
    subprocess.run([str(out/'network')],check=True)
