/**
  ******************************************************************************
  * @file    dev_pwm.c
  * @brief   Platform-independent PWM device wrapper.
  ******************************************************************************
  */
#include "dev_pwm.h"

static bool dev_pwm_config_valid(const dev_pwm_config_t *config)
{
  return (config != NULL) && (config->frequency_hz != 0u) &&
         (config->duty_permille <= 1000u) &&
         ((config->polarity == DEV_PWM_POLARITY_ACTIVE_HIGH) ||
          (config->polarity == DEV_PWM_POLARITY_ACTIVE_LOW));
}

static bool dev_pwm_actual_valid(const dev_pwm_actual_t *actual)
{
  return (actual != NULL) && (actual->frequency_hz != 0u) &&
         (actual->duty_permille <= 1000u) &&
         ((actual->quantization == DEV_PWM_QUANTIZATION_EXACT) ||
          (actual->quantization == DEV_PWM_QUANTIZATION_ADJUSTED));
}

usr_status_t dev_pwm_init(pwm_device_t *dev, const dev_pwm_config_t *cfg)
{
  dev_pwm_config_t candidate;
  usr_status_t status;

  if ((dev == NULL) || (dev->ops == NULL) || (dev->ops->init == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (cfg != NULL)
  {
    if (!dev_pwm_config_valid(cfg))
    {
      return USR_ERR_PARAM;
    }
    candidate = *cfg;
    status = dev->ops->init(dev->ctx, &candidate);
    if (status == USR_OK)
    {
      dev->config = candidate;
    }
    return status;
  }
  return dev->ops->init(dev->ctx, NULL);
}

usr_status_t dev_pwm_deinit(pwm_device_t *dev)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->deinit == NULL))
  {
    return USR_ERR_PARAM;
  }
  return dev->ops->deinit(dev->ctx);
}

usr_status_t dev_pwm_start(pwm_device_t *dev)
{
  if ((dev == NULL) || (dev->ops == NULL) || (dev->ops->start == NULL))
  {
    return USR_ERR_PARAM;
  }
  return dev->ops->start(dev->ctx);
}

usr_status_t dev_pwm_stop(pwm_device_t *dev)
{
  if ((dev == NULL) || (dev->ops == NULL) || (dev->ops->stop == NULL))
  {
    return USR_ERR_PARAM;
  }
  return dev->ops->stop(dev->ctx);
}

usr_status_t dev_pwm_set_duty(pwm_device_t *dev, uint16_t duty_permille)
{
  usr_status_t status;

  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->set_duty == NULL) || (duty_permille > 1000u))
  {
    return USR_ERR_PARAM;
  }
  status = dev->ops->set_duty(dev->ctx, duty_permille);
  if (status == USR_OK)
  {
    dev->config.duty_permille = duty_permille;
  }
  return status;
}

usr_status_t dev_pwm_set_frequency(pwm_device_t *dev, uint32_t frequency_hz,
                                   dev_pwm_actual_t *actual)
{
  dev_pwm_actual_t actual_snapshot;
  usr_status_t status;

  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->set_frequency == NULL) || (frequency_hz == 0u) ||
      (actual == NULL))
  {
    return USR_ERR_PARAM;
  }
  status = dev->ops->set_frequency(dev->ctx, frequency_hz,
                                   &actual_snapshot);
  if (status == USR_OK)
  {
    if (!dev_pwm_actual_valid(&actual_snapshot))
    {
      return USR_ERR_DATA;
    }
    dev->config.frequency_hz = frequency_hz;
    *actual = actual_snapshot;
  }
  return status;
}

usr_status_t dev_pwm_get_actual(pwm_device_t *dev, dev_pwm_actual_t *actual)
{
  dev_pwm_actual_t actual_snapshot;
  usr_status_t status;

  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->get_actual == NULL) || (actual == NULL))
  {
    return USR_ERR_PARAM;
  }
  status = dev->ops->get_actual(dev->ctx, &actual_snapshot);
  if (status == USR_OK)
  {
    if (!dev_pwm_actual_valid(&actual_snapshot))
    {
      return USR_ERR_DATA;
    }
    *actual = actual_snapshot;
  }
  return status;
}

usr_status_t dev_pwm_get_status(pwm_device_t *dev, dev_pwm_status_t *status)
{
  dev_pwm_status_t status_snapshot;
  usr_status_t result;

  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->get_status == NULL) || (status == NULL))
  {
    return USR_ERR_PARAM;
  }
  result = dev->ops->get_status(dev->ctx, &status_snapshot);
  if (result == USR_OK)
  {
    *status = status_snapshot;
  }
  return result;
}
