/**
  ******************************************************************************
  * @file    usr_bus_gpio.c
  * @brief   Platform-independent dev backend wrapper.
  ******************************************************************************
  */
#include "dev_gpio.h"
#include "usr_check.h"

// usr_status_t dev_gpio_init(usr_bus_gpio_t *dev, usr_gpio_backend_t backend)
// {
//   if ((dev == NULL) || (backend.ops == NULL))
//   {
//     return USR_ERR_PARAM;
//   }
//   dev->backend = backend;
//   if (backend.ops->init != NULL)
//   {
//     return backend.ops->init(backend.ctx);
//   }
//   return USR_OK;
// }

usr_status_t dev_gpio_write(gpio_device_t *dev,  usr_pin_id_t id, uint8_t level)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->write == NULL))
  {
    return USR_ERR_PARAM;
  }
  
  return dev->ops->write(dev->ctx, id, level);
}

usr_status_t dev_gpio_read(gpio_device_t *dev,
                               usr_pin_id_t id, uint8_t *level)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->read == NULL) || (level == NULL))
  {
    return USR_ERR_PARAM;
  }
  return dev->ops->read(dev->ctx, id, level);
}

usr_status_t dev_gpio_toggle(gpio_device_t *dev,usr_pin_id_t id)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->toggle == NULL) )
  {
    return USR_ERR_PARAM;
  }
  return dev->ops->toggle(dev->ctx, id);
}

usr_status_t dev_gpio_set_irq(gpio_device_t *dev,
                                  usr_pin_id_t id,
                                  usr_callback_t cb, void *arg)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->set_irq_cb == NULL))
  {
    return USR_ERR_PARAM;
  }
  return dev->ops->set_irq_cb(dev->ctx, id, cb, arg);
}
