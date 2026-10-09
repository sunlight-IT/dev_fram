#ifndef __DEV_TIMER_H__
#define __DEV_TIMER_H__

#include "dri_ops.h"

typedef struct dev_timer_status
{
  bool initialized;
  bool started;
  uint32_t error_count;
} dev_timer_status_t;

typedef struct timer_device
{
  device parent;
  void *ctx;
  const dev_timer_ops_t *ops;
} timer_device_t;

usr_status_t dev_timer_init(timer_device_t *dev);
usr_status_t dev_timer_deinit(timer_device_t *dev);
usr_status_t dev_timer_start(timer_device_t *dev);
usr_status_t dev_timer_stop(timer_device_t *dev);
uint32_t dev_timer_get_ticks(timer_device_t *dev);
usr_status_t dev_timer_get_status(timer_device_t *dev,
                                  dev_timer_status_t *status);

#endif
