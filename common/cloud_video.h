#ifndef EXAMPLE_VIDEO_H
#define EXAMPLE_VIDEO_H
#include "FreeRTOS.h"
#include "semphr.h"
#include "rtk_ameba_device_example.h"
int example_video_start(rtk_ameba_device_example_t *, SemaphoreHandle_t,
                        void *, void (*)(void *));
int example_video_poll(rtk_ameba_device_example_t *);
#endif
