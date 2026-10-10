#ifndef __DEV_TIMER_H__
#define __DEV_TIMER_H__

#include "dri_ops.h"

typedef enum {
  TIM_INDEX_0 = 0,
  TIM_INDEX_1 = 1,
  TIM_INDEX_2 = 2,
  TIM_INDEX_3 = 3,
  TIM_INDEX_MAX = 4,
} tim_index_t;

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

timer_device_t *get_timer_device(uint8_t index);


#endif
