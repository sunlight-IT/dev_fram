#ifndef __PORT_HAL_PWM_H__
#define __PORT_HAL_PWM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_pwm.h"
#include "port_hal_tim.h"

typedef enum
{
  USR_PORT_PWM_LIFECYCLE_RESET = 0,
  USR_PORT_PWM_LIFECYCLE_READY,
  USR_PORT_PWM_LIFECYCLE_STARTED,
  USR_PORT_PWM_LIFECYCLE_FAULT,
} usr_port_pwm_lifecycle_t;

typedef struct
{
  usr_port_hal_tim_instance_t *timer;
  uint32_t channel;
  GPIO_TypeDef *gpio_port;
  uint16_t gpio_pin;
  uint32_t gpio_alternate;
  void (*gpio_clock_enable)(void);
  dev_pwm_config_t default_config;
} usr_port_hal_pwm_resource_t;

typedef struct
{
  const usr_port_hal_pwm_resource_t *resource;
  pwm_device_t device;
  dev_pwm_config_t runtime_config;
  dev_pwm_actual_t actual;
  usr_port_pwm_lifecycle_t lifecycle;
  uint32_t compare;
  uint32_t error_count;
} usr_port_hal_pwm_instance_t;

extern const dev_pwm_ops_t port_hal_pwm_ops;

#ifdef __cplusplus
}
#endif

#endif /* __PORT_HAL_PWM_H__ */
