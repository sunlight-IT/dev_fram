#ifndef __PORT_HAL_GPIO_H__
#define __PORT_HAL_GPIO_H__

#include "dev_gpio.h"
#include "board_support.h"

typedef struct
{
  void *port;
  uint16_t pin;
  void (*clock_enable)(void);
  uint32_t mode;
  uint32_t pull;
  uint32_t speed;
  GPIO_PinState initial_level;
  bool write_initial;
  bool has_irq;
  IRQn_Type irq;
  uint32_t irq_priority;
  uint32_t irq_subpriority;
} usr_port_hal_gpio_pin_t;

typedef struct
{
  const usr_port_hal_gpio_pin_t *pins;
  usr_callback_t *irq_callbacks;
  void **irq_args;
  uint16_t pin_count;
} usr_port_hal_gpio_t;


void usr_port_hal_gpio_irq_dispatch(usr_port_hal_gpio_t *gpio,
                                    uint16_t physical_pin);

#endif /* __PORT_HAL_GPIO_H__ */