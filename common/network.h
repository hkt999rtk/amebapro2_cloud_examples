#ifndef EXAMPLE_NETWORK_H
#define EXAMPLE_NETWORK_H
#include <stdint.h>
int example_network_start(void);
int example_network_ready(void);
int example_time_start(void);
uint64_t example_unix_time(void *context);
void example_network_stop(void);
#endif
