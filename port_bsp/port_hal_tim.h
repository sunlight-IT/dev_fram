#ifndef __PORT_HAL_TIM_H__
#define __PORT_HAL_TIM_H__

#include "dev_timer.h"
#include "dev_pwm.h"
#include "board_support.h"

typedef enum
{
  USR_PORT_TIM_MODE_BASIC = 0,
  USR_PORT_TIM_MODE_PWM,
} usr_port_tim_mode_t;

typedef enum
{
  USR_PORT_TIM_LIFECYCLE_RESET = 0,
  USR_PORT_TIM_LIFECYCLE_READY,
  USR_PORT_TIM_LIFECYCLE_STARTED,
  USR_PORT_TIM_LIFECYCLE_FAULT,
} usr_port_tim_lifecycle_t;

typedef struct usr_port_hal_tim_resource
{
  TIM_TypeDef *instance;
  usr_port_tim_mode_t mode;
  uint32_t timer_clock_hz;
  uint16_t prescaler;
  uint32_t auto_reload;
  void (*clock_enable)(void);
  void (*clock_disable)(void);
  bool has_update_irq;
  IRQn_Type update_irq;
  uint32_t irq_priority;
  uint32_t pwm_channel;
  uint32_t pwm_compare;
  uint32_t pwm_polarity;
  GPIO_TypeDef *pwm_gpio_port;
  uint16_t pwm_gpio_pin;
  uint32_t pwm_gpio_alternate;
  void (*pwm_gpio_clock_enable)(void);
} usr_port_hal_tim_resource_t;

typedef struct usr_port_hal_tim_instance
{
  const usr_port_hal_tim_resource_t *resource;
  TIM_HandleTypeDef *handle;
  timer_device_t device;
  pwm_device_t pwm_device;
  usr_port_tim_lifecycle_t lifecycle;
  volatile uint32_t tick_count;
  uint32_t compare;
  uint32_t error_count;
} usr_port_hal_tim_instance_t;

extern TIM_HandleTypeDef htim2;
extern const dev_timer_ops_t port_hal_tim_ops;
extern const dev_pwm_ops_t port_hal_pwm_ops;

void port_hal_tim_irq(usr_port_hal_tim_instance_t *timer);
usr_port_hal_tim_instance_t *usr_port_board_tim_find_instance(
    const TIM_HandleTypeDef *handle);

#endif
