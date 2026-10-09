/**
  ******************************************************************************
  * @file    dev_pwm.c
  * @brief   Platform-independent PWM device wrapper.
  ******************************************************************************
  */
#include "dev_pwm.h"

usr_status_t dev_pwm_init(pwm_device_t *dev)
{
  if ((dev == NULL) || (dev->ops == NULL) || (dev->ops->init == NULL))
  {
    return USR_ERR_PARAM;
  }
  return dev->ops->init(dev->ctx);
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

usr_status_t dev_pwm_set_compare(pwm_device_t *dev, uint32_t compare)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->set_compare == NULL))
  {
    return USR_ERR_PARAM;
  }
  return dev->ops->set_compare(dev->ctx, compare);
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
