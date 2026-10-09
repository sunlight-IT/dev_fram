/**
  ******************************************************************************
  * @file    dev_timer.c
  * @brief   Platform-independent timer device wrapper (same style as dev_gpio).
  ******************************************************************************
  */
#include "dev_timer.h"

static bool dev_timer_capture_config_valid(
    const dev_timer_capture_config_t *capture)
{
  return (capture != NULL) &&
         ((capture->edge == DEV_TIMER_CAPTURE_EDGE_RISING) ||
          (capture->edge == DEV_TIMER_CAPTURE_EDGE_FALLING) ||
          (capture->edge == DEV_TIMER_CAPTURE_EDGE_BOTH)) &&
         (capture->filter_level <= 15u) && (capture->timeout_us != 0u);
}

static bool dev_timer_config_valid(const dev_timer_config_t *config)
{
  if (config == NULL)
  {
    return false;
  }
  if (config->mode == DEV_TIMER_MODE_BASIC)
  {
    return (config->period_us != 0u) &&
           (config->capture.filter_level == 0u) &&
           (config->capture.timeout_us == 0u);
  }
  if (config->mode == DEV_TIMER_MODE_CAPTURE)
  {
    return (config->period_us == 0u) &&
           dev_timer_capture_config_valid(&config->capture);
  }
  return false;
}

usr_status_t dev_timer_init(timer_device_t *dev,
                            const dev_timer_config_t *cfg)
{
  dev_timer_config_t candidate;
  usr_status_t status;

  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->init == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (cfg != NULL)
  {
    if (!dev_timer_config_valid(cfg))
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

usr_status_t dev_timer_capture_read(timer_device_t *dev,
                                    dev_timer_capture_result_t *result)
{
  dev_timer_capture_result_t result_snapshot;
  dev_timer_status_t device_status;
  usr_status_t status;

  if ((dev == NULL) || (dev->ops == NULL) ||
      (result == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (dev->config.mode != DEV_TIMER_MODE_CAPTURE)
  {
    return USR_ERR_UNSUPPORTED;
  }
  if (dev->ops->capture_read == NULL)
  {
    return USR_ERR_UNSUPPORTED;
  }
  status = dev_timer_get_status(dev, &device_status);
  if (status != USR_OK)
  {
    return status;
  }
  if (!device_status.initialized)
  {
    return USR_ERR_NOT_INIT;
  }
  if (!device_status.started)
  {
    return USR_ERR_STATE;
  }
  status = dev->ops->capture_read(dev->ctx, &result_snapshot);
  if (status == USR_OK)
  {
    *result = result_snapshot;
  }
  return status;
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
