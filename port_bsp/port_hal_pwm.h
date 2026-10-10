#ifndef __PORT_HAL_PWM_H__
#define __PORT_HAL_PWM_H__


#include "dev_pwm.h"
#include "board_support.h"

typedef enum {
  USR_PORT_PWM_LIFECYCLE_RESET = 0,
  USR_PORT_PWM_LIFECYCLE_READY,
  USR_PORT_PWM_LIFECYCLE_STARTED,
  USR_PORT_PWM_LIFECYCLE_FAULT,
} usr_port_pwm_lifecycle_t;

typedef struct usr_port_hal_pwm_channel
{
  uint8_t logical_channel;
  uint32_t hal_channel;
  uint32_t compare;
  uint32_t polarity;
  
  GPIO_TypeDef *gpio_port;
  uint16_t gpio_pin;
  uint32_t gpio_alternate;
  void (*gpio_clock_enable)(void);
} usr_port_hal_pwm_channel_t;

typedef struct usr_port_hal_pwm_resource
{
  TIM_TypeDef *instance;
  uint32_t timer_clock_hz;
  uint16_t prescaler;
  uint32_t auto_reload;
  void (*clock_enable)(void);
  void (*clock_disable)(void);

  
  bool has_update_irq;
  IRQn_Type update_irq;
  uint32_t irq_priority;

  uint8_t pwm_channel_count;
  const usr_port_hal_pwm_channel_t *pwm_channels;
} usr_port_hal_pwm_resource_t;

typedef struct usr_port_hal_pwm_instance
{
  const usr_port_hal_pwm_resource_t *resource;
  TIM_HandleTypeDef *handle;
  pwm_device_t device;
  usr_port_pwm_lifecycle_t lifecycle;
  volatile uint32_t tick_count;
  uint32_t compare;
  uint32_t error_count;
} usr_port_hal_pwm_instance_t;

extern TIM_HandleTypeDef htim5;
extern const dev_pwm_ops_t port_hal_pwm_ops;

usr_port_hal_pwm_instance_t *usr_port_board_pwm_find_instance(
                    const TIM_HandleTypeDef *handle);

#endif
