/**
  ******************************************************************************
  * @file    usr_port_hal_gpio.c
  * @brief   Reusable descriptor-driven STM32 HAL GPIO adapter.
  ******************************************************************************
  */

#include "dev_gpio.h"
#include "port_hal_gpio.h"

static usr_port_hal_gpio_t *s_irq_gpio;

static bool port_hal_gpio_valid(usr_port_hal_gpio_t *gpio)
{
  return ((gpio != NULL) && (gpio->pins != NULL) &&
          (gpio->irq_callbacks != NULL) && (gpio->irq_args != NULL) &&
          (gpio->pin_count > 0u));
}

static bool port_hal_gpio_descriptor_valid(
    const usr_port_hal_gpio_pin_t *descriptor)
{
  if ((descriptor == NULL) || (descriptor->port == NULL) ||
      (descriptor->pin == 0u) || (descriptor->clock_enable == NULL))
  {
    return false;
  }
  if (descriptor->has_irq &&
      (((int32_t)descriptor->irq < 0) ||
       (descriptor->irq_priority >= (1u << __NVIC_PRIO_BITS))))
  {
    return false;
  }
  return true;
}

static usr_status_t port_hal_gpio_init(void *ctx)
{
  usr_port_hal_gpio_t *gpio = (usr_port_hal_gpio_t *)ctx;
  GPIO_InitTypeDef gpio_init = {0};

  if (!port_hal_gpio_valid(gpio))
  {
    return USR_ERR_PARAM;
  }

  for (uint16_t i = 0u; i < gpio->pin_count; i++)
  {
    if (!port_hal_gpio_descriptor_valid(&gpio->pins[i]))
    {
      return USR_ERR_PARAM;
    }
  }

  for (uint16_t i = 0u; i < gpio->pin_count; i++)
  {
    const usr_port_hal_gpio_pin_t *descriptor = &gpio->pins[i];

    gpio->irq_callbacks[i] = NULL;
    gpio->irq_args[i] = NULL;
    descriptor->clock_enable();
    if (descriptor->write_initial)
    {
      HAL_GPIO_WritePin((GPIO_TypeDef *)descriptor->port,
                        descriptor->pin,
                        descriptor->initial_level);
    }

    gpio_init.Pin = descriptor->pin;
    gpio_init.Mode = descriptor->mode;
    gpio_init.Pull = descriptor->pull;
    gpio_init.Speed = descriptor->speed;
    gpio_init.Alternate = 0u;
    HAL_GPIO_Init((GPIO_TypeDef *)descriptor->port, &gpio_init);
  }

  s_irq_gpio = gpio;
  for (uint16_t i = 0u; i < gpio->pin_count; i++)
  {
    const usr_port_hal_gpio_pin_t *descriptor = &gpio->pins[i];

    if (descriptor->has_irq)
    {
      HAL_NVIC_SetPriority(descriptor->irq,
                           descriptor->irq_priority,
                           descriptor->irq_subpriority);
      HAL_NVIC_EnableIRQ(descriptor->irq);
    }
  }
  return USR_OK;
}

static usr_status_t port_hal_gpio_write(void *ctx,
                                        usr_pin_id_t id,
                                        uint8_t level)
{
  usr_port_hal_gpio_t *gpio = (usr_port_hal_gpio_t *)ctx;

  if (!port_hal_gpio_valid(gpio) || (id >= gpio->pin_count))
  {
    return USR_ERR_PARAM;
  }

  GPIO_TypeDef *port = (GPIO_TypeDef *)gpio->pins[id].port;
  uint16_t pin = gpio->pins[id].pin;
  HAL_GPIO_WritePin(port, pin,(level != 0u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  return USR_OK;
}

static usr_status_t port_hal_gpio_read(void *ctx,
                                       usr_pin_id_t id,
                                       uint8_t *level)
{
  usr_port_hal_gpio_t *gpio = (usr_port_hal_gpio_t *)ctx;

  if (!port_hal_gpio_valid(gpio) || (id >= gpio->pin_count) ||
      (level == NULL))
  {
    return USR_ERR_PARAM;
  }

  GPIO_TypeDef *port = (GPIO_TypeDef *)gpio->pins[id].port;
  uint16_t pin = gpio->pins[id].pin;
  *level = (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) ? 1u : 0u;
  return USR_OK;
}

static usr_status_t port_hal_gpio_toggle(void *ctx, usr_pin_id_t id)
{
  usr_port_hal_gpio_t *gpio = (usr_port_hal_gpio_t *)ctx;

  if (!port_hal_gpio_valid(gpio) || (id >= gpio->pin_count))
  {
    return USR_ERR_PARAM;
  }
  GPIO_TypeDef *port = (GPIO_TypeDef *)gpio->pins[id].port;
  uint16_t pin = gpio->pins[id].pin;
  HAL_GPIO_TogglePin(port, pin);
  return USR_OK;
}

static usr_status_t port_hal_gpio_set_irq(void *ctx,
                                          usr_pin_id_t id,
                                          usr_callback_t cb, void *arg)
{
  usr_port_hal_gpio_t *gpio = (usr_port_hal_gpio_t *)ctx;
  const usr_port_hal_gpio_pin_t *descriptor;

  if (!port_hal_gpio_valid(gpio) || (id >= gpio->pin_count))
  {
    return USR_ERR_PARAM;
  }
  descriptor = &gpio->pins[id];
  if (!descriptor->has_irq)
  {
    return USR_ERR_UNSUPPORTED;
  }

  HAL_NVIC_DisableIRQ(descriptor->irq);
  gpio->irq_args[id] = arg;
  gpio->irq_callbacks[id] = cb;
  HAL_NVIC_EnableIRQ(descriptor->irq);
  return USR_OK;
}

void usr_port_hal_gpio_irq(IRQn_Type irq)
{
  if (!port_hal_gpio_valid(s_irq_gpio))
  {
    return;
  }

  for (uint16_t i = 0u; i < s_irq_gpio->pin_count; i++)
  {
    const usr_port_hal_gpio_pin_t *descriptor = &s_irq_gpio->pins[i];

    if (descriptor->has_irq && (descriptor->irq == irq))
    {
      HAL_GPIO_EXTI_IRQHandler(descriptor->pin);
    }
  }
}

void usr_port_hal_gpio_irq_dispatch(usr_port_hal_gpio_t *gpio,
                                    uint16_t physical_pin)
{
  if (!port_hal_gpio_valid(gpio))
  {
    return;
  }
  for (uint16_t i = 0u; i < gpio->pin_count; i++)
  {
    if ((gpio->pins[i].pin == physical_pin) &&
        (gpio->irq_callbacks[i] != NULL))
    {
      gpio->irq_callbacks[i](gpio->irq_args[i]);
    }
  }
}

const dev_gpio_ops_t port_hal_gpio_ops =
{
  port_hal_gpio_init,
  port_hal_gpio_write,
  port_hal_gpio_read,
  port_hal_gpio_toggle,
  port_hal_gpio_set_irq,
};
