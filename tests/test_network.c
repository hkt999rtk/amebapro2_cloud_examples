#include "platform.h"
#include "network.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
rtk_ameba_firmware_config_t rtk_ameba_firmware_config = {"provisioned", "test", ""};
static unsigned tick, started, stopped, disconnected, time_stops, connected_calls;
static unsigned last_time_tick, time_starts;
static time_t last_time_seconds;
static int associated=1, grant=1, flap, sync_time;
static unsigned char ip[4];
int wifi_is_connected_to_ap(void){return associated ? 0 : -1;}
int wifi_connect(rtw_network_info_t *c,unsigned block){(void)c;(void)block;connected_calls++;associated=1;return 0;}
int wifi_disconnect(void){disconnected++;associated=0;return 0;}
unsigned char *LwIP_GetIP(int idx){(void)idx;return ip;}
int LwIP_DHCP(int idx,int state){(void)idx;if(state==DHCP_START){started++;memset(ip,0,4);}else stopped++;return 0;}
void vTaskDelay(unsigned ms){tick+=ms;if(flap && tick==250)associated=0;if(flap && tick==500)associated=1;if(grant && associated && tick>=1000)ip[0]=10;if(sync_time && tick==1000){last_time_seconds=1577836800;last_time_tick=tick;}}
unsigned xTaskGetTickCount(void){return tick;}
void sntp_get_lasttime(time_t *s,time_t *us,unsigned *t){*s=last_time_seconds;*us=0;*t=last_time_tick;}
void sntp_set_max_tries(unsigned n){(void)n;}
void sntp_init(void){time_starts++;}
void sntp_stop(void){time_stops++;}
int main(void){
    assert(example_network_start()==0 && started==1 && tick==1000);
    assert(example_time_start()==-1); /* App cleanup after failed time sync. */
    assert(time_starts==1 && example_unix_time(NULL)==0);
    example_network_stop();
    assert(associated && !stopped && !disconnected && time_stops==1);
    tick=0;sync_time=1;
    assert(example_time_start()==0 && time_starts==2);
    assert(example_unix_time(NULL)==1577836800ULL);
    tick+=2000;
    assert(example_unix_time(NULL)==1577836802ULL); /* Advance from SNTP sample. */
    last_time_seconds=1577836860;last_time_tick=tick;
    assert(example_unix_time(NULL)==1577836860ULL); /* New SNTP sample corrects clock. */
    sync_time=0;
    assert(example_network_start()==0 && started==1 && !connected_calls);
    memset(ip,0,4);tick=0;started=0;flap=1;
    assert(example_network_start()==0 && started==2); /* External reassociation. */
    memset(ip,0,4);tick=0;started=0;flap=0;grant=0;
    assert(example_network_start()==-1 && started==1); /* Bound retries without DHCP churn. */
    example_network_stop();assert(associated && !disconnected);
    grant=1;rtk_ameba_firmware_config.wifi_security="open";
    assert(example_network_start()==0 && connected_calls==1);
    example_network_stop();assert(stopped==1 && disconnected==1);
    puts("NETWORK_TESTS_OK");
}
