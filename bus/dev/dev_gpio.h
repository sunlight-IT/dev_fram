#ifndef __DEV_GPIO_H__
#define __DEV_GPIO_H__

#include "dri_ops.h"



typedef struct gpio_device{
    device parent;
    void *ctx;
    const dev_gpio_ops_t *ops;
} gpio_device_t;


usr_status_t dev_gpio_write(gpio_device_t *dev,  usr_pin_id_t id, uint8_t level);
usr_status_t dev_gpio_read(gpio_device_t *dev,  usr_pin_id_t id, uint8_t *level);
usr_status_t dev_gpio_toggle(gpio_device_t *dev, usr_pin_id_t id);
usr_status_t dev_gpio_set_irq(gpio_device_t *dev, usr_pin_id_t id, usr_callback_t cb, void *arg);



#endif
