#ifndef __DEV_PWM_H__
#define __DEV_PWM_H__

#include "dri_ops.h"

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
usr_status_t dev_pwm_set_compare(pwm_device_t *dev, uint32_t compare);
usr_status_t dev_pwm_get_status(pwm_device_t *dev, dev_pwm_status_t *status);

#endif
