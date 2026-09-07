#include "mqtt_identity.h"
#include <assert.h>
#include <string.h>
static int refreshes, fail;
static uint64_t now(void *p) { (void)p; return 1900000000ULL; }
static int refresh(void *p) { (void)p; ++refreshes; return fail ? -1 : 0; }
static int token(void *p, char *out, size_t cap, uint64_t *expiry) {
    const char *jwt = "e30.eyJzY29wZSI6ImRldmljZSIsInN1YmplY3RfaWQiOiJkZXZpY2UtZml4dHVyZS0xIiwiZXhwIjoyMDAwMDAwMDAwLCJicmFuZF9jbG91ZF9pZCI6ImJyYW5kLWZpeHR1cmUiLCJtcXR0X2NsaWVudF9pZCI6Im1xdHQtZml4dHVyZSJ9.signature";
    (void)p; assert(cap > strlen(jwt)); strcpy(out,jwt); *expiry=2000000000ULL; return 0;
}
int main(void) {
    cloud_mqtt_identity_t identity = {0};
    identity.device_id="device-fixture-1"; identity.unix_time=now;
    identity.provider.refresh=refresh; identity.provider.get_access_token=token;
    char client[64], user[64], password[8192]; uint64_t expiry;
    for (int connection=1; connection<=2; ++connection) {
        assert(cloud_mqtt_credentials(&identity,client,sizeof(client),user,sizeof(user),password,sizeof(password),&expiry)==0);
        assert(refreshes==connection && expiry==2000000000ULL);
        assert(strcmp(client,"mqtt-fixture")==0 && strcmp(user,"brand-fixture")==0);
    }
    fail=1;
    memset(password,'x',sizeof(password));
    assert(cloud_mqtt_credentials(&identity,client,sizeof(client),user,sizeof(user),password,sizeof(password),&expiry)<0);
    assert(!expiry && !client[0] && !user[0]);
    for (unsigned i=0; i<sizeof(password); ++i) assert(password[i]==0);
    return 0;
}
