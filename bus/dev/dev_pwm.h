#ifndef __DEV_PWM_H__
#define __DEV_PWM_H__

#include "dri_ops.h"

typedef enum
{
  DEV_PWM_POLARITY_ACTIVE_HIGH = 0,
  DEV_PWM_POLARITY_ACTIVE_LOW,
} dev_pwm_polarity_t;

typedef struct dev_pwm_config
{
  uint32_t frequency_hz;
  uint16_t duty_permille;
  dev_pwm_polarity_t polarity;
} dev_pwm_config_t;

typedef enum
{
  DEV_PWM_QUANTIZATION_EXACT = 0,
  DEV_PWM_QUANTIZATION_ADJUSTED,
} dev_pwm_quantization_t;

typedef struct dev_pwm_actual
{
  uint32_t frequency_hz;
  uint16_t duty_permille;
  dev_pwm_quantization_t quantization;
  uint32_t period_counts;
  uint32_t pulse_counts;
} dev_pwm_actual_t;

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
  dev_pwm_config_t config;
  const dev_pwm_ops_t *ops;
} pwm_device_t;

usr_status_t dev_pwm_init(pwm_device_t *dev, const dev_pwm_config_t *cfg);
usr_status_t dev_pwm_deinit(pwm_device_t *dev);
usr_status_t dev_pwm_start(pwm_device_t *dev);
usr_status_t dev_pwm_stop(pwm_device_t *dev);
usr_status_t dev_pwm_set_duty(pwm_device_t *dev, uint16_t duty_permille);
/* Success writes actual and failure leaves it unchanged. */
usr_status_t dev_pwm_set_frequency(pwm_device_t *dev, uint32_t frequency_hz,
                                   dev_pwm_actual_t *actual);
usr_status_t dev_pwm_get_actual(pwm_device_t *dev, dev_pwm_actual_t *actual);
usr_status_t dev_pwm_get_status(pwm_device_t *dev, dev_pwm_status_t *status);

#endif
