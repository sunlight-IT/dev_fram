/**
  ******************************************************************************
  * @file    port_hal_tim.c
  * @brief   STM32 timer resource, basic timer, and input-capture adapter.
  ******************************************************************************
  */
#include "port_hal_tim.h"

#include <limits.h>
#include <string.h>

TIM_HandleTypeDef htim2;

static bool port_hal_tim_config_valid(const dev_timer_config_t *config)
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
           (config->capture.filter_level <= 15u) &&
           (config->capture.timeout_us != 0u) &&
           ((config->capture.edge == DEV_TIMER_CAPTURE_EDGE_RISING) ||
            (config->capture.edge == DEV_TIMER_CAPTURE_EDGE_FALLING) ||
            (config->capture.edge == DEV_TIMER_CAPTURE_EDGE_BOTH));
  }
  return false;
}

static uint64_t port_hal_tim_now_ticks(
    const usr_port_hal_tim_instance_t *timer)
{
  uint32_t epoch_before;
  uint32_t epoch_after;
  uint32_t counter;
  bool update_pending;

  do
  {
    epoch_before = timer->update_epoch;
    counter = __HAL_TIM_GET_COUNTER(timer->handle);
    update_pending =
        (__HAL_TIM_GET_FLAG(timer->handle, TIM_FLAG_UPDATE) != RESET);
    epoch_after = timer->update_epoch;
  } while (epoch_before != epoch_after);
  return port_tim_capture_extend(epoch_after, counter, update_pending,
                                 timer->resource->counter_bits);
}

static uint32_t port_hal_tim_ticks_to_us(
    const usr_port_hal_tim_instance_t *timer, uint64_t ticks)
{
  uint64_t scaled_us;
  uint64_t whole_clock_periods;
  uint64_t remainder;
  uint64_t scale_quotient;
  uint64_t scale_remainder;
  uint64_t fractional_us;
  uint32_t result;

  if ((timer->timer_clock_hz == 0u) ||
      (timer->prescaler == UINT16_MAX))
  {
    return 0u;
  }
  scaled_us = ((uint64_t)timer->prescaler + 1u) * 1000000ull;
  whole_clock_periods = ticks / timer->timer_clock_hz;
  remainder = ticks % timer->timer_clock_hz;
  scale_quotient = scaled_us / timer->timer_clock_hz;
  scale_remainder = scaled_us % timer->timer_clock_hz;
  fractional_us = (remainder * scale_quotient) +
                  ((remainder * scale_remainder) /
                   timer->timer_clock_hz);
  result = (uint32_t)whole_clock_periods * (uint32_t)scaled_us;
  result += (uint32_t)fractional_us;
  return result;
}

static bool port_hal_tim_timeout_elapsed(
    const usr_port_hal_tim_instance_t *timer, uint64_t elapsed_ticks,
    uint32_t timeout_us)
{
  uint64_t denominator;
  uint64_t timeout_ticks;

  if ((timer->timer_clock_hz == 0u) || (timeout_us == 0u))
  {
    return false;
  }
  denominator = ((uint64_t)timer->prescaler + 1u) * 1000000ull;
  timeout_ticks = ((uint64_t)timeout_us * timer->timer_clock_hz) /
                  denominator;
  if (((uint64_t)timeout_us * timer->timer_clock_hz) % denominator != 0u)
  {
    timeout_ticks++;
  }
  return elapsed_ticks >= timeout_ticks;
}

static uint32_t port_hal_tim_capture_polarity(
    dev_timer_capture_edge_t edge)
{
  if (edge == DEV_TIMER_CAPTURE_EDGE_FALLING)
  {
    return TIM_INPUTCHANNELPOLARITY_FALLING;
  }
  if (edge == DEV_TIMER_CAPTURE_EDGE_BOTH)
  {
    return TIM_INPUTCHANNELPOLARITY_BOTHEDGE;
  }
  return TIM_INPUTCHANNELPOLARITY_RISING;
}

bool port_hal_tim_channel_index(uint32_t channel, uint32_t *index)
{
  if (index == NULL)
  {
    return false;
  }
  switch (channel)
  {
    case TIM_CHANNEL_1:
      *index = 0u;
      return true;
    case TIM_CHANNEL_2:
      *index = 1u;
      return true;
    case TIM_CHANNEL_3:
      *index = 2u;
      return true;
    case TIM_CHANNEL_4:
      *index = 3u;
      return true;
    default:
      return false;
  }
}

static uint32_t port_hal_tim_channel_flag(uint32_t channel)
{
  static const uint32_t flags[USR_PORT_TIM_PWM_CHANNEL_COUNT] = {
    TIM_FLAG_CC1, TIM_FLAG_CC2, TIM_FLAG_CC3, TIM_FLAG_CC4,
  };
  uint32_t index;

  return port_hal_tim_channel_index(channel, &index) ? flags[index] : 0u;
}

static uint32_t port_hal_tim_channel_interrupt(uint32_t channel)
{
  static const uint32_t interrupts[USR_PORT_TIM_PWM_CHANNEL_COUNT] = {
    TIM_IT_CC1, TIM_IT_CC2, TIM_IT_CC3, TIM_IT_CC4,
  };
  uint32_t index;

  return port_hal_tim_channel_index(channel, &index) ?
      interrupts[index] : 0u;
}

static uint32_t port_hal_tim_channel_overcapture_flag(uint32_t channel)
{
  static const uint32_t flags[USR_PORT_TIM_PWM_CHANNEL_COUNT] = {
    TIM_FLAG_CC1OF, TIM_FLAG_CC2OF, TIM_FLAG_CC3OF, TIM_FLAG_CC4OF,
  };
  uint32_t index;

  return port_hal_tim_channel_index(channel, &index) ? flags[index] : 0u;
}

static void port_hal_tim_reset_capture(usr_port_hal_tim_instance_t *timer)
{
  memset(&timer->capture_ring, 0, sizeof(timer->capture_ring));
  memset(&timer->capture_state, 0, sizeof(timer->capture_state));
  timer->capture_state.require_alternating_edges =
      (timer->runtime_config.mode == DEV_TIMER_MODE_CAPTURE) &&
      (timer->runtime_config.capture.edge == DEV_TIMER_CAPTURE_EDGE_BOTH);
  timer->update_epoch = 0u;
  timer->capture_sequence = 0u;
  timer->capture_start_ticks = 0u;
  timer->capture_result_pending = false;
  timer->captured_count = 0u;
}

void port_hal_tim_reset_runtime(usr_port_hal_tim_instance_t *timer)
{
  if (timer == NULL)
  {
    return;
  }
  memset(&timer->runtime_config, 0, sizeof(timer->runtime_config));
  timer->timer_clock_hz = 0u;
  timer->counter_hz = 0u;
  timer->prescaler = 0u;
  timer->auto_reload = 0u;
  timer->actual_period_us = 0u;
  timer->actual_frequency_hz = 0u;
  timer->pwm_quantized = false;
  timer->tick_count = 0u;
  port_hal_tim_reset_capture(timer);
  timer->lifecycle = USR_PORT_TIM_LIFECYCLE_RESET;
}

static bool port_hal_tim_descriptor_valid(
    const usr_port_hal_tim_instance_t *timer)
{
  const usr_port_hal_tim_resource_t *resource;

  if ((timer == NULL) || (timer->resource == NULL) ||
      (timer->handle == NULL))
  {
    return false;
  }
  resource = timer->resource;
  return (resource->instance != NULL) &&
         ((resource->counter_bits == 16u) ||
          (resource->counter_bits == 32u)) &&
         (resource->clock_get_hz != NULL) &&
         (resource->clock_enable != NULL) &&
         (resource->clock_disable != NULL);
}

usr_status_t port_hal_tim_claim(usr_port_hal_tim_instance_t *timer,
                                port_tim_owner_t owner,
                                uint32_t capability,
                                uint32_t claim_mask)
{
  port_tim_owner_t current_owner;

  if (!port_hal_tim_descriptor_valid(timer) || (claim_mask == 0u) ||
      ((timer->resource->capability_mask & capability) != capability))
  {
    return USR_ERR_UNSUPPORTED;
  }
  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_FAULT)
  {
    return USR_ERR_STATE;
  }
  current_owner = (timer->resource->owner != USR_PORT_TIM_OWNER_NONE) ?
      timer->resource->owner : timer->active_owner;
  if (!port_tim_claim_allowed(current_owner, owner, capability))
  {
    return USR_ERR_UNSUPPORTED;
  }
  if (timer->active_owner == USR_PORT_TIM_OWNER_NONE)
  {
    timer->active_owner = owner;
  }
  timer->claim_mask |= claim_mask;
  return USR_OK;
}

void port_hal_tim_release(usr_port_hal_tim_instance_t *timer,
                          uint32_t claim_mask)
{
  if (timer != NULL)
  {
    timer->claim_mask &= ~claim_mask;
    if (timer->claim_mask == 0u)
    {
      timer->active_owner = USR_PORT_TIM_OWNER_NONE;
    }
  }
}

static usr_status_t port_hal_tim_prepare_basic(
    usr_port_hal_tim_instance_t *timer, const dev_timer_config_t *config,
    port_tim_calc_result_t *calculation)
{
  timer->timer_clock_hz = timer->resource->clock_get_hz();
  if (!port_tim_calc_period(timer->timer_clock_hz,
                            timer->resource->counter_bits,
                            config->period_us, calculation))
  {
    return USR_ERR_PARAM;
  }
  return USR_OK;
}

static usr_status_t port_hal_tim_prepare_capture(
    usr_port_hal_tim_instance_t *timer,
    port_tim_calc_result_t *calculation)
{
  uint32_t divisor;
  uint32_t channel_index;
  uint64_t modulus;
  uint64_t period_us;

  if (!port_hal_tim_channel_index(timer->resource->capture_channel,
                                  &channel_index) ||
      ((timer->resource->channel_mask &
        (1u << channel_index)) == 0u) ||
      (timer->resource->gpio_port == NULL) ||
      (timer->resource->gpio_clock_enable == NULL))
  {
    return USR_ERR_UNSUPPORTED;
  }
  timer->timer_clock_hz = timer->resource->clock_get_hz();
  if (timer->timer_clock_hz == 0u)
  {
    return USR_ERR_PARAM;
  }
  divisor = (timer->timer_clock_hz + 999999u) / 1000000u;
  if ((divisor == 0u) || (divisor > 65536u))
  {
    return USR_ERR_UNSUPPORTED;
  }
  modulus = (timer->resource->counter_bits == 16u) ?
      0x10000ull : 0x100000000ull;
  calculation->prescaler = (uint16_t)(divisor - 1u);
  calculation->auto_reload = (uint32_t)(modulus - 1u);
  calculation->counter_hz = timer->timer_clock_hz / divisor;
  period_us = (modulus * 1000000ull) / calculation->counter_hz;
  calculation->actual_period_us = (period_us > UINT32_MAX) ?
      0u : (uint32_t)period_us;
  calculation->actual_frequency_hz = 0u;
  calculation->quantized = (calculation->counter_hz != 1000000u);
  return USR_OK;
}

static usr_status_t port_hal_tim_apply_capture(
    usr_port_hal_tim_instance_t *timer,
    const dev_timer_config_t *config)
{
  TIM_IC_InitTypeDef capture = {0};

  if (HAL_TIM_IC_Init(timer->handle) != HAL_OK)
  {
    return USR_ERR_BUS;
  }
  capture.ICPolarity = port_hal_tim_capture_polarity(config->capture.edge);
  capture.ICSelection = TIM_ICSELECTION_DIRECTTI;
  capture.ICPrescaler = TIM_ICPSC_DIV1;
  capture.ICFilter = config->capture.filter_level;
  if (HAL_TIM_IC_ConfigChannel(timer->handle, &capture,
                               timer->resource->capture_channel) != HAL_OK)
  {
    return USR_ERR_BUS;
  }
  return USR_OK;
}

static usr_status_t port_hal_tim_init(void *ctx, const void *cfg)
{
  usr_port_hal_tim_instance_t *timer =
      (usr_port_hal_tim_instance_t *)ctx;
  const dev_timer_config_t *config;
  port_tim_calc_result_t calculation;
  port_tim_owner_t owner;
  uint32_t capability;
  usr_status_t status;

  if (!port_hal_tim_descriptor_valid(timer))
  {
    return USR_ERR_PARAM;
  }
  if (timer->lifecycle != USR_PORT_TIM_LIFECYCLE_RESET)
  {
    return USR_ERR_STATE;
  }
  config = (cfg == NULL) ? &timer->resource->default_config :
                           (const dev_timer_config_t *)cfg;
  if (!port_hal_tim_config_valid(config))
  {
    return USR_ERR_PARAM;
  }
  if (((config->mode == DEV_TIMER_MODE_BASIC) &&
       !timer->resource->has_update_irq) ||
      ((config->mode == DEV_TIMER_MODE_CAPTURE) &&
       (!timer->resource->has_update_irq ||
        !timer->resource->has_capture_irq)))
  {
    return USR_ERR_UNSUPPORTED;
  }
  if (config->mode == DEV_TIMER_MODE_BASIC)
  {
    owner = (timer->resource->reserved_role ==
             USR_PORT_TIM_ROLE_SCHEDULER_TICK) ?
        USR_PORT_TIM_OWNER_SCHEDULER : USR_PORT_TIM_OWNER_BASIC;
    capability = PORT_TIM_CAP_BASIC;
    status = port_hal_tim_prepare_basic(timer, config, &calculation);
  }
  else
  {
    owner = USR_PORT_TIM_OWNER_CAPTURE;
    capability = PORT_TIM_CAP_CAPTURE;
    status = port_hal_tim_prepare_capture(timer, &calculation);
  }
  if (status != USR_OK)
  {
    port_hal_tim_reset_runtime(timer);
    return status;
  }
  status = port_hal_tim_claim(timer, owner, capability, capability);
  if (status != USR_OK)
  {
    port_hal_tim_reset_runtime(timer);
    return status;
  }

  timer->runtime_config = *config;

  timer->handle->Instance = timer->resource->instance;
  timer->handle->Init.Prescaler = calculation.prescaler;
  timer->handle->Init.CounterMode = TIM_COUNTERMODE_UP;
  timer->handle->Init.Period = calculation.auto_reload;
  timer->handle->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  timer->handle->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (config->mode == DEV_TIMER_MODE_BASIC)
  {
    status = (HAL_TIM_Base_Init(timer->handle) == HAL_OK) ?
        USR_OK : USR_ERR_BUS;
  }
  else
  {
    status = port_hal_tim_apply_capture(timer, config);
  }
  if (status != USR_OK)
  {
    HAL_StatusTypeDef rollback_status;

    rollback_status = (config->mode == DEV_TIMER_MODE_BASIC) ?
        HAL_TIM_Base_DeInit(timer->handle) :
        HAL_TIM_IC_DeInit(timer->handle);
    timer->error_count++;
    if (rollback_status != HAL_OK)
    {
      timer->error_count++;
      timer->lifecycle = USR_PORT_TIM_LIFECYCLE_FAULT;
      return status;
    }
    port_hal_tim_release(timer, capability);
    port_hal_tim_reset_runtime(timer);
    return status;
  }

  timer->device.config = *config;
  timer->counter_hz = calculation.counter_hz;
  timer->prescaler = calculation.prescaler;
  timer->auto_reload = calculation.auto_reload;
  timer->actual_period_us = calculation.actual_period_us;
  timer->tick_count = 0u;
  port_hal_tim_reset_capture(timer);
  timer->lifecycle = USR_PORT_TIM_LIFECYCLE_READY;
  return USR_OK;
}

static usr_status_t port_hal_tim_deinit(void *ctx)
{
  usr_port_hal_tim_instance_t *timer =
      (usr_port_hal_tim_instance_t *)ctx;
  HAL_StatusTypeDef hal_status;
  uint32_t capability;

  if (!port_hal_tim_descriptor_valid(timer))
  {
    return USR_ERR_PARAM;
  }
  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_RESET)
  {
    return USR_OK;
  }
  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_STARTED)
  {
    return USR_ERR_STATE;
  }
  capability = (timer->runtime_config.mode == DEV_TIMER_MODE_BASIC) ?
      PORT_TIM_CAP_BASIC : PORT_TIM_CAP_CAPTURE;
  hal_status = (timer->runtime_config.mode == DEV_TIMER_MODE_BASIC) ?
      HAL_TIM_Base_DeInit(timer->handle) : HAL_TIM_IC_DeInit(timer->handle);
  if (hal_status != HAL_OK)
  {
    timer->error_count++;
    return USR_ERR_BUS;
  }
  port_hal_tim_release(timer, capability);
  port_hal_tim_reset_runtime(timer);
  return USR_OK;
}

static usr_status_t port_hal_tim_start(void *ctx)
{
  usr_port_hal_tim_instance_t *timer =
      (usr_port_hal_tim_instance_t *)ctx;
  HAL_StatusTypeDef hal_status;

  if (!port_hal_tim_descriptor_valid(timer))
  {
    return USR_ERR_PARAM;
  }
  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_STARTED)
  {
    return USR_OK;
  }
  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_FAULT)
  {
    return USR_ERR_STATE;
  }
  if (timer->lifecycle != USR_PORT_TIM_LIFECYCLE_READY)
  {
    return USR_ERR_NOT_INIT;
  }
  if (timer->runtime_config.mode == DEV_TIMER_MODE_BASIC)
  {
    hal_status = HAL_TIM_Base_Start_IT(timer->handle);
  }
  else
  {
    port_hal_tim_reset_capture(timer);
    hal_status = HAL_TIM_Base_Start_IT(timer->handle);
    if ((hal_status == HAL_OK) &&
        (HAL_TIM_IC_Start_IT(timer->handle,
                             timer->resource->capture_channel) != HAL_OK))
    {
      if (HAL_TIM_Base_Stop_IT(timer->handle) != HAL_OK)
      {
        timer->error_count++;
        timer->lifecycle = USR_PORT_TIM_LIFECYCLE_FAULT;
      }
      hal_status = HAL_ERROR;
    }
  }
  if (hal_status != HAL_OK)
  {
    timer->error_count++;
    return USR_ERR_BUS;
  }
  timer->lifecycle = USR_PORT_TIM_LIFECYCLE_STARTED;
  timer->capture_start_ticks = port_hal_tim_now_ticks(timer);
  return USR_OK;
}

static usr_status_t port_hal_tim_stop(void *ctx)
{
  usr_port_hal_tim_instance_t *timer =
      (usr_port_hal_tim_instance_t *)ctx;
  HAL_StatusTypeDef hal_status;

  if (!port_hal_tim_descriptor_valid(timer))
  {
    return USR_ERR_PARAM;
  }
  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_RESET)
  {
    return USR_ERR_NOT_INIT;
  }
  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_FAULT)
  {
    return USR_ERR_STATE;
  }
  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_READY)
  {
    return USR_OK;
  }
  if (timer->runtime_config.mode == DEV_TIMER_MODE_BASIC)
  {
    hal_status = HAL_TIM_Base_Stop_IT(timer->handle);
  }
  else
  {
    hal_status = HAL_TIM_IC_Stop_IT(timer->handle,
                                    timer->resource->capture_channel);
    if ((hal_status == HAL_OK) &&
        (HAL_TIM_Base_Stop_IT(timer->handle) != HAL_OK))
    {
      if (HAL_TIM_IC_Start_IT(timer->handle,
                             timer->resource->capture_channel) != HAL_OK)
      {
        timer->error_count++;
        timer->lifecycle = USR_PORT_TIM_LIFECYCLE_FAULT;
      }
      hal_status = HAL_ERROR;
    }
  }
  if (hal_status != HAL_OK)
  {
    timer->error_count++;
    return USR_ERR_BUS;
  }
  timer->lifecycle = USR_PORT_TIM_LIFECYCLE_READY;
  return USR_OK;
}

static uint32_t port_hal_tim_get_ticks(void *ctx)
{
  const usr_port_hal_tim_instance_t *timer =
      (const usr_port_hal_tim_instance_t *)ctx;

  return (timer != NULL) ? timer->tick_count : 0u;
}

static void port_hal_tim_process_capture(
    usr_port_hal_tim_instance_t *timer)
{
  port_tim_capture_raw_event_t event;
  port_tim_capture_sample_t sample;
  uint32_t primask;

  while (true)
  {
    primask = __get_PRIMASK();
    __disable_irq();
    if (!port_tim_capture_ring_pop(&timer->capture_ring, &event))
    {
      if (primask == 0u)
      {
        __enable_irq();
      }
      break;
    }
    if (primask == 0u)
    {
      __enable_irq();
    }
    if (port_tim_capture_process_event(
            &event, timer->resource->counter_bits, &timer->capture_state,
            &sample, &timer->error_count))
    {
      timer->capture_result.timestamp_us =
          port_hal_tim_ticks_to_us(timer, sample.timestamp_ticks);
      timer->capture_result.period_us =
          port_hal_tim_ticks_to_us(timer, sample.period_ticks);
      timer->capture_result.edge = (dev_timer_capture_edge_t)sample.edge;
      timer->capture_result_pending = true;
      timer->captured_count++;
      timer->capture_start_ticks = sample.timestamp_ticks;
      return;
    }
    timer->capture_start_ticks = timer->capture_state.latest_event_ticks;
  }
}

static usr_status_t port_hal_tim_capture_read(void *ctx, void *result)
{
  usr_port_hal_tim_instance_t *timer =
      (usr_port_hal_tim_instance_t *)ctx;
  dev_timer_capture_result_t *capture_result =
      (dev_timer_capture_result_t *)result;
  uint64_t elapsed_ticks;

  if ((timer == NULL) || (capture_result == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (timer->runtime_config.mode != DEV_TIMER_MODE_CAPTURE)
  {
    return USR_ERR_UNSUPPORTED;
  }
  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_RESET)
  {
    return USR_ERR_NOT_INIT;
  }
  if (timer->lifecycle != USR_PORT_TIM_LIFECYCLE_STARTED)
  {
    return USR_ERR_STATE;
  }
  port_hal_tim_process_capture(timer);
  if (timer->capture_result_pending)
  {
    *capture_result = timer->capture_result;
    timer->capture_result_pending = false;
    return USR_OK;
  }
  elapsed_ticks = port_hal_tim_now_ticks(timer) - timer->capture_start_ticks;
  if (port_hal_tim_timeout_elapsed(
          timer, elapsed_ticks, timer->runtime_config.capture.timeout_us))
  {
    timer->capture_state.baseline_valid = false;
    timer->capture_start_ticks = port_hal_tim_now_ticks(timer);
    return USR_ERR_TIMEOUT;
  }
  return USR_BUSY;
}

static usr_status_t port_hal_tim_get_status(void *ctx, void *status)
{
  const usr_port_hal_tim_instance_t *timer =
      (const usr_port_hal_tim_instance_t *)ctx;
  dev_timer_status_t *timer_status = (dev_timer_status_t *)status;

  if ((timer == NULL) || (timer_status == NULL))
  {
    return USR_ERR_PARAM;
  }
  timer_status->initialized =
      (timer->lifecycle != USR_PORT_TIM_LIFECYCLE_RESET);
  timer_status->started =
      (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_STARTED);
  timer_status->capture_pending = timer->capture_result_pending ||
                                  (timer->capture_ring.count != 0u);
  timer_status->captured_count = timer->captured_count;
  timer_status->dropped_count = timer->capture_ring.dropped_count;
  timer_status->error_count = timer->error_count;
  return USR_OK;
}

static void port_hal_tim_msp_init(TIM_HandleTypeDef *handle,
                                  bool configure_gpio,
                                  bool enable_update_irq,
                                  bool enable_capture_irq)
{
  usr_port_hal_tim_instance_t *timer =
      usr_port_board_tim_find_instance(handle);
  GPIO_InitTypeDef gpio = {0};

  if (timer == NULL)
  {
    return;
  }
  timer->resource->clock_enable();
  if (configure_gpio && (timer->resource->gpio_port != NULL) &&
      (timer->resource->gpio_clock_enable != NULL))
  {
    timer->resource->gpio_clock_enable();
    gpio.Pin = timer->resource->gpio_pin;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = timer->resource->gpio_alternate;
    HAL_GPIO_Init(timer->resource->gpio_port, &gpio);
  }
  if (enable_update_irq && timer->resource->has_update_irq)
  {
    HAL_NVIC_SetPriority(timer->resource->update_irq,
                         timer->resource->irq_priority,
                         timer->resource->irq_subpriority);
    HAL_NVIC_EnableIRQ(timer->resource->update_irq);
  }
  if (enable_capture_irq && timer->resource->has_capture_irq &&
      (!enable_update_irq ||
       (timer->resource->capture_irq != timer->resource->update_irq)))
  {
    HAL_NVIC_SetPriority(timer->resource->capture_irq,
                         timer->resource->irq_priority,
                         timer->resource->irq_subpriority);
    HAL_NVIC_EnableIRQ(timer->resource->capture_irq);
  }
}

static void port_hal_tim_msp_deinit(TIM_HandleTypeDef *handle,
                                    bool deinit_gpio,
                                    bool disable_update_irq,
                                    bool disable_capture_irq)
{
  usr_port_hal_tim_instance_t *timer =
      usr_port_board_tim_find_instance(handle);

  if (timer == NULL)
  {
    return;
  }
  if (disable_update_irq && timer->resource->has_update_irq)
  {
    HAL_NVIC_DisableIRQ(timer->resource->update_irq);
    HAL_NVIC_ClearPendingIRQ(timer->resource->update_irq);
  }
  if (disable_capture_irq && timer->resource->has_capture_irq &&
      (!disable_update_irq ||
       (timer->resource->capture_irq != timer->resource->update_irq)))
  {
    HAL_NVIC_DisableIRQ(timer->resource->capture_irq);
    HAL_NVIC_ClearPendingIRQ(timer->resource->capture_irq);
  }
  if (deinit_gpio && (timer->resource->gpio_port != NULL))
  {
    HAL_GPIO_DeInit(timer->resource->gpio_port, timer->resource->gpio_pin);
  }
  timer->resource->clock_disable();
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *handle)
{
  port_hal_tim_msp_init(handle, false, true, false);
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *handle)
{
  port_hal_tim_msp_deinit(handle, false, true, false);
}

void HAL_TIM_IC_MspInit(TIM_HandleTypeDef *handle)
{
  port_hal_tim_msp_init(handle, true, true, true);
}

void HAL_TIM_IC_MspDeInit(TIM_HandleTypeDef *handle)
{
  port_hal_tim_msp_deinit(handle, true, true, true);
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *handle)
{
  port_hal_tim_msp_init(handle, false, false, false);
}

void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef *handle)
{
  port_hal_tim_msp_deinit(handle, false, false, false);
}

void port_hal_tim_irq(usr_port_hal_tim_instance_t *timer)
{
  uint32_t channel;
  uint32_t channel_index;
  uint32_t channel_flag;
  uint32_t channel_interrupt;
  uint32_t overcapture_flag;
  port_tim_capture_raw_event_t event;

  if ((timer == NULL) || (timer->handle == NULL))
  {
    return;
  }
  channel = timer->resource->capture_channel;
  channel_flag = port_hal_tim_channel_flag(channel);
  channel_interrupt = port_hal_tim_channel_interrupt(channel);
  overcapture_flag = port_hal_tim_channel_overcapture_flag(channel);
  if ((timer->runtime_config.mode == DEV_TIMER_MODE_CAPTURE) &&
      port_hal_tim_channel_index(channel, &channel_index) &&
      (channel_flag != 0u) &&
      (__HAL_TIM_GET_FLAG(timer->handle, channel_flag) != RESET) &&
      (__HAL_TIM_GET_IT_SOURCE(timer->handle, channel_interrupt) != RESET))
  {
    event.sequence = ++timer->capture_sequence;
    event.raw_counter = HAL_TIM_ReadCapturedValue(timer->handle, channel);
    event.update_epoch = timer->update_epoch;
    event.channel = (uint8_t)channel_index;
    if (timer->runtime_config.capture.edge == DEV_TIMER_CAPTURE_EDGE_BOTH)
    {
      event.edge = ((timer->resource->gpio_port->IDR &
                     timer->resource->gpio_pin) != 0u) ?
          DEV_TIMER_CAPTURE_EDGE_RISING : DEV_TIMER_CAPTURE_EDGE_FALLING;
    }
    else
    {
      event.edge = (uint8_t)timer->runtime_config.capture.edge;
    }
    event.overcapture =
        (__HAL_TIM_GET_FLAG(timer->handle, overcapture_flag) != RESET);
    event.update_pending =
        (__HAL_TIM_GET_FLAG(timer->handle, TIM_FLAG_UPDATE) != RESET);
    __HAL_TIM_CLEAR_IT(timer->handle, channel_interrupt);
    __HAL_TIM_CLEAR_FLAG(timer->handle, overcapture_flag);
    (void)port_tim_capture_ring_push_isr(&timer->capture_ring, &event);
  }
  if ((__HAL_TIM_GET_FLAG(timer->handle, TIM_FLAG_UPDATE) != RESET) &&
      (__HAL_TIM_GET_IT_SOURCE(timer->handle, TIM_IT_UPDATE) != RESET))
  {
    __HAL_TIM_CLEAR_IT(timer->handle, TIM_IT_UPDATE);
    timer->update_epoch++;
    if (timer->runtime_config.mode == DEV_TIMER_MODE_BASIC)
    {
      timer->tick_count++;
    }
  }
}

void TIM2_IRQHandler(void)
{
  usr_port_hal_tim_instance_t *timer =
      usr_port_board_tim_find_instance(&htim2);

  port_hal_tim_irq(timer);
}

const dev_timer_ops_t port_hal_tim_ops =
{
  .init = port_hal_tim_init,
  .deinit = port_hal_tim_deinit,
  .start = port_hal_tim_start,
  .stop = port_hal_tim_stop,
  .get_ticks = port_hal_tim_get_ticks,
  .capture_read = port_hal_tim_capture_read,
  .get_status = port_hal_tim_get_status,
};
