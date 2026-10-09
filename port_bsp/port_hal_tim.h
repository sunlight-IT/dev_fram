#ifndef __PORT_HAL_TIM_H__
#define __PORT_HAL_TIM_H__

#include "stm32wlxx_hal.h"
#include "dev_timer.h"
#include "port_tim_semantic.h"

#define USR_PORT_TIM_PWM_CHANNEL_COUNT 4u

typedef enum
{
  USR_PORT_TIM_ROLE_GENERAL = 0,
  USR_PORT_TIM_ROLE_SCHEDULER_TICK,
} usr_port_tim_role_t;

typedef enum
{
  USR_PORT_TIM_LIFECYCLE_RESET = 0,
  USR_PORT_TIM_LIFECYCLE_READY,
  USR_PORT_TIM_LIFECYCLE_STARTED,
  USR_PORT_TIM_LIFECYCLE_FAULT,
} usr_port_tim_lifecycle_t;

typedef struct
{
  bool claimed;
  bool started;
  uint32_t channel;
  uint16_t duty_permille;
  uint32_t compare;
  uint32_t polarity;
  void *owner_context;
} usr_port_hal_tim_pwm_channel_t;

typedef struct usr_port_hal_tim_resource
{
  TIM_TypeDef *instance;
  uint8_t counter_bits;
  uint32_t capability_mask;
  port_tim_owner_t owner;
  usr_port_tim_role_t reserved_role;
  uint8_t conflict_domain;
  uint32_t (*clock_get_hz)(void);
  void (*clock_enable)(void);
  void (*clock_disable)(void);
  bool has_update_irq;
  bool has_capture_irq;
  IRQn_Type update_irq;
  IRQn_Type capture_irq;
  uint32_t irq_priority;
  uint32_t irq_subpriority;
  uint32_t channel_mask;
  uint32_t capture_channel;
  GPIO_TypeDef *gpio_port;
  uint16_t gpio_pin;
  uint32_t gpio_alternate;
  void (*gpio_clock_enable)(void);
  dev_timer_config_t default_config;
} usr_port_hal_tim_resource_t;

typedef struct usr_port_hal_tim_instance
{
  const usr_port_hal_tim_resource_t *resource;
  TIM_HandleTypeDef *handle;
  timer_device_t device;
  dev_timer_config_t runtime_config;
  uint32_t timer_clock_hz;
  uint32_t counter_hz;
  uint16_t prescaler;
  uint32_t auto_reload;
  uint32_t actual_period_us;
  uint32_t actual_frequency_hz;
  bool pwm_quantized;
  usr_port_tim_lifecycle_t lifecycle;
  port_tim_owner_t active_owner;
  uint32_t claim_mask;
  port_tim_capture_ring_t capture_ring;
  uint32_t error_count;
  volatile uint32_t update_epoch;
  volatile uint32_t tick_count;
  volatile uint32_t capture_sequence;
  port_tim_capture_state_t capture_state;
  uint64_t capture_start_ticks;
  bool capture_result_pending;
  dev_timer_capture_result_t capture_result;
  uint32_t captured_count;
  usr_port_hal_tim_pwm_channel_t
      pwm_channels[USR_PORT_TIM_PWM_CHANNEL_COUNT];
} usr_port_hal_tim_instance_t;

extern TIM_HandleTypeDef htim2;
extern const dev_timer_ops_t port_hal_tim_ops;

usr_status_t port_hal_tim_claim(usr_port_hal_tim_instance_t *timer,
                                port_tim_owner_t owner,
                                uint32_t capability,
                                uint32_t claim_mask);
void port_hal_tim_release(usr_port_hal_tim_instance_t *timer,
                          uint32_t claim_mask);
void port_hal_tim_reset_runtime(usr_port_hal_tim_instance_t *timer);
void port_hal_tim_irq(usr_port_hal_tim_instance_t *timer);
bool port_hal_tim_channel_index(uint32_t channel, uint32_t *index);
usr_port_hal_tim_instance_t *usr_port_board_tim_find_instance(
    const TIM_HandleTypeDef *handle);

#endif
