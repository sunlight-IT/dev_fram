#ifndef __DEV_PWM_H__
#define __DEV_PWM_H__

#include "dri_ops.h"

typedef enum {
  DEV_PWM_CHANNEL_1 = 0u,
  DEV_PWM_CHANNEL_2 = 1u,
  DEV_PWM_CHANNEL_3 = 2u,
  DEV_PWM_CHANNEL_4 = 3u,
  DEV_PWM_CHANNEL_MAX = 4u,
} dev_pwm_channel_t;

typedef enum {
  PWM_INDEX_0 = 0u,
  PWM_INDEX_1 = 1u,
  PWM_INDEX_MAX = 2u,
} pwm_index_t;

typedef struct dev_pwm_status
{
  bool initialized;
  bool started;
  uint32_t error_count;
} dev_pwm_status_t;

typedef struct pwm_device
{
  device parent;
  void *ctx;
  const dev_pwm_ops_t *ops;
} pwm_device_t;

usr_status_t dev_pwm_init(pwm_device_t *dev);
usr_status_t dev_pwm_deinit(pwm_device_t *dev);
usr_status_t dev_pwm_start(pwm_device_t *dev);
usr_status_t dev_pwm_stop(pwm_device_t *dev);
usr_status_t dev_pwm_set_compare(pwm_device_t *dev, uint8_t channel,
                                 uint32_t compare);
usr_status_t dev_pwm_get_status(pwm_device_t *dev, dev_pwm_status_t *status);

#endif
