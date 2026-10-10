/**
  ******************************************************************************
  * @file    dev_timer.c
  * @brief   Platform-independent timer device wrapper (same style as dev_gpio).
  ******************************************************************************
  */
#include "dev_timer.h"

static timer_device_t s_devices[TIM_INDEX_MAX];

timer_device_t *get_timer_device(uint8_t index)
{
  if (index >= TIM_INDEX_MAX)
  {
    return NULL;
  }
  return &s_devices[index];
}

usr_status_t dev_timer_init(timer_device_t *dev)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->init == NULL))
  {
    return USR_ERR_PARAM;
  }
  return dev->ops->init(dev->ctx);
}

usr_status_t dev_timer_deinit(timer_device_t *dev)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->deinit == NULL))
  {
    return USR_ERR_PARAM;
  }
  return dev->ops->deinit(dev->ctx);
}

usr_status_t dev_timer_start(timer_device_t *dev)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->start == NULL))
  {
    return USR_ERR_PARAM;
  }
  return dev->ops->start(dev->ctx);
}

usr_status_t dev_timer_stop(timer_device_t *dev)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->stop == NULL))
  {
    return USR_ERR_PARAM;
  }
  return dev->ops->stop(dev->ctx);
}

uint32_t dev_timer_get_ticks(timer_device_t *dev)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->get_ticks == NULL))
  {
    return 0u;
  }
  return dev->ops->get_ticks(dev->ctx);
}

usr_status_t dev_timer_get_status(timer_device_t *dev,
                                  dev_timer_status_t *status)
{
  dev_timer_status_t status_snapshot;
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
