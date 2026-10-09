/**
  ******************************************************************************
  * @file    port_hal_pwm.c
  * @brief   TIM-backed PWM port adapter with shared-base transactions.
  ******************************************************************************
  */
#include "port_hal_pwm.h"

#include <string.h>

typedef struct
{
  uint16_t prescaler;
  uint32_t auto_reload;
  uint32_t counter_hz;
  uint32_t actual_frequency_hz;
  bool quantized;
} port_hal_pwm_base_t;

static bool port_hal_pwm_config_valid(const dev_pwm_config_t *config)
{
  return (config != NULL) && (config->frequency_hz != 0u) &&
         (config->duty_permille <= 1000u) &&
         ((config->polarity == DEV_PWM_POLARITY_ACTIVE_HIGH) ||
          (config->polarity == DEV_PWM_POLARITY_ACTIVE_LOW));
}

static bool port_hal_pwm_descriptor_valid(
    const usr_port_hal_pwm_instance_t *pwm)
{
  uint32_t channel_index;

  return (pwm != NULL) && (pwm->resource != NULL) &&
         (pwm->resource->timer != NULL) &&
         (pwm->resource->timer->resource != NULL) &&
         (pwm->resource->timer->handle != NULL) &&
         port_hal_tim_channel_index(pwm->resource->channel,
                                    &channel_index) &&
         (pwm->resource->gpio_port != NULL) &&
         (pwm->resource->gpio_clock_enable != NULL);
}

static uint32_t port_hal_pwm_claim_mask(uint32_t channel)
{
  uint32_t index;

  if (!port_hal_tim_channel_index(channel, &index))
  {
    return 0u;
  }
  return 1u << (8u + index);
}

static uint32_t port_hal_pwm_polarity(dev_pwm_polarity_t polarity)
{
  return (polarity == DEV_PWM_POLARITY_ACTIVE_LOW) ?
      TIM_OCPOLARITY_LOW : TIM_OCPOLARITY_HIGH;
}

static uint16_t port_hal_pwm_actual_duty(uint32_t auto_reload,
                                         uint32_t compare)
{
  uint64_t period_counts = (uint64_t)auto_reload + 1u;

  return (uint16_t)(((uint64_t)compare * 1000u +
                     (period_counts / 2u)) / period_counts);
}

static usr_status_t port_hal_pwm_prepare_frequency(
    const usr_port_hal_pwm_instance_t *pwm, uint32_t frequency_hz,
    port_hal_pwm_base_t *base,
    usr_port_hal_tim_pwm_channel_t *channels)
{
  usr_port_hal_tim_instance_t *timer = pwm->resource->timer;
  port_tim_calc_result_t calculation;
  uint32_t index;

  if ((base == NULL) || (channels == NULL) ||
      !port_tim_calc_frequency(timer->resource->clock_get_hz(),
                               timer->resource->counter_bits,
                               frequency_hz, &calculation))
  {
    return USR_ERR_PARAM;
  }
  base->prescaler = calculation.prescaler;
  base->auto_reload = calculation.auto_reload;
  base->counter_hz = calculation.counter_hz;
  base->actual_frequency_hz = calculation.actual_frequency_hz;
  base->quantized = calculation.quantized;
  memcpy(channels, timer->pwm_channels, sizeof(timer->pwm_channels));
  for (index = 0u; index < USR_PORT_TIM_PWM_CHANNEL_COUNT; index++)
  {
    if (channels[index].claimed)
    {
      channels[index].compare = port_tim_pwm_ccr(
          base->auto_reload, channels[index].duty_permille);
    }
  }
  return USR_OK;
}

static HAL_StatusTypeDef port_hal_pwm_commit(
    usr_port_hal_tim_instance_t *timer,
    const port_hal_pwm_base_t *base,
    const usr_port_hal_tim_pwm_channel_t *channels)
{
  uint32_t index;

  __HAL_TIM_SET_PRESCALER(timer->handle, base->prescaler);
  __HAL_TIM_SET_AUTORELOAD(timer->handle, base->auto_reload);
  timer->handle->Init.Prescaler = base->prescaler;
  timer->handle->Init.Period = base->auto_reload;
  for (index = 0u; index < USR_PORT_TIM_PWM_CHANNEL_COUNT; index++)
  {
    if (channels[index].claimed)
    {
      __HAL_TIM_SET_COMPARE(timer->handle, channels[index].channel,
                            channels[index].compare);
    }
  }
  return HAL_TIM_GenerateEvent(timer->handle, TIM_EVENTSOURCE_UPDATE);
}

static void port_hal_pwm_restore(
    usr_port_hal_tim_instance_t *timer,
    const port_hal_pwm_base_t *old_base,
    const usr_port_hal_tim_pwm_channel_t *old_channels,
    const TIM_Base_InitTypeDef *old_init)
{
  uint32_t index;

  __HAL_TIM_SET_PRESCALER(timer->handle, old_base->prescaler);
  __HAL_TIM_SET_AUTORELOAD(timer->handle, old_base->auto_reload);
  timer->handle->Init = *old_init;
  for (index = 0u; index < USR_PORT_TIM_PWM_CHANNEL_COUNT; index++)
  {
    if (old_channels[index].claimed)
    {
      __HAL_TIM_SET_COMPARE(timer->handle, old_channels[index].channel,
                            old_channels[index].compare);
    }
  }
  timer->handle->Instance->EGR = TIM_EVENTSOURCE_UPDATE;
}

static void port_hal_pwm_fill_actual(usr_port_hal_pwm_instance_t *pwm,
                                     const port_hal_pwm_base_t *base)
{
  pwm->actual.frequency_hz = base->actual_frequency_hz;
  pwm->actual.duty_permille = port_hal_pwm_actual_duty(
      base->auto_reload, pwm->compare);
  pwm->actual.quantization =
      (base->quantized ||
       (pwm->actual.duty_permille != pwm->runtime_config.duty_permille)) ?
      DEV_PWM_QUANTIZATION_ADJUSTED : DEV_PWM_QUANTIZATION_EXACT;
  pwm->actual.period_counts = base->auto_reload + 1u;
  pwm->actual.pulse_counts = pwm->compare;
}

static void port_hal_pwm_sync_instances(
    usr_port_hal_tim_instance_t *timer,
    const port_hal_pwm_base_t *base,
    uint32_t requested_frequency_hz)
{
  uint32_t index;

  for (index = 0u; index < USR_PORT_TIM_PWM_CHANNEL_COUNT; index++)
  {
    usr_port_hal_pwm_instance_t *pwm;

    if (!timer->pwm_channels[index].claimed ||
        (timer->pwm_channels[index].owner_context == NULL))
    {
      continue;
    }
    pwm = (usr_port_hal_pwm_instance_t *)
        timer->pwm_channels[index].owner_context;
    pwm->compare = timer->pwm_channels[index].compare;
    pwm->runtime_config.frequency_hz = requested_frequency_hz;
    pwm->device.config.frequency_hz = requested_frequency_hz;
    port_hal_pwm_fill_actual(pwm, base);
  }
}

static usr_status_t port_hal_pwm_configure_channel(
    usr_port_hal_pwm_instance_t *pwm, const dev_pwm_config_t *config,
    uint32_t compare)
{
  TIM_OC_InitTypeDef channel_config = {0};

  channel_config.OCMode = TIM_OCMODE_PWM1;
  channel_config.Pulse = compare;
  channel_config.OCPolarity = port_hal_pwm_polarity(config->polarity);
  channel_config.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(pwm->resource->timer->handle,
                                &channel_config,
                                pwm->resource->channel) != HAL_OK)
  {
    return USR_ERR_BUS;
  }
  return USR_OK;
}

static usr_status_t port_hal_pwm_init(void *ctx, const void *cfg)
{
  usr_port_hal_pwm_instance_t *pwm =
      (usr_port_hal_pwm_instance_t *)ctx;
  usr_port_hal_tim_instance_t *timer;
  const dev_pwm_config_t *config;
  port_tim_calc_result_t calculation;
  port_hal_pwm_base_t base;
  GPIO_InitTypeDef gpio = {0};
  uint32_t channel_index;
  uint32_t channel_bit;
  uint32_t compare;
  bool timer_initialized = false;
  usr_status_t status;

  if (!port_hal_pwm_descriptor_valid(pwm))
  {
    return USR_ERR_PARAM;
  }
  if (pwm->lifecycle != USR_PORT_PWM_LIFECYCLE_RESET)
  {
    return USR_ERR_STATE;
  }
  config = (cfg == NULL) ? &pwm->resource->default_config :
                           (const dev_pwm_config_t *)cfg;
  if (!port_hal_pwm_config_valid(config))
  {
    return USR_ERR_PARAM;
  }
  timer = pwm->resource->timer;
  if (!port_hal_tim_channel_index(pwm->resource->channel, &channel_index))
  {
    return USR_ERR_UNSUPPORTED;
  }
  channel_bit = 1u << channel_index;
  if (((timer->resource->channel_mask & channel_bit) == 0u) ||
      timer->pwm_channels[channel_index].claimed)
  {
    return USR_ERR_UNSUPPORTED;
  }
  status = port_hal_tim_claim(timer, USR_PORT_TIM_OWNER_PWM,
                              PORT_TIM_CAP_PWM,
                              port_hal_pwm_claim_mask(
                                  pwm->resource->channel));
  if (status != USR_OK)
  {
    return status;
  }
  if (!port_tim_calc_frequency(timer->resource->clock_get_hz(),
                               timer->resource->counter_bits,
                               config->frequency_hz, &calculation))
  {
    port_hal_tim_release(timer,
                         port_hal_pwm_claim_mask(pwm->resource->channel));
    return USR_ERR_PARAM;
  }
  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_RESET)
  {
    timer->handle->Instance = timer->resource->instance;
    timer->handle->Init.Prescaler = calculation.prescaler;
    timer->handle->Init.CounterMode = TIM_COUNTERMODE_UP;
    timer->handle->Init.Period = calculation.auto_reload;
    timer->handle->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer->handle->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(timer->handle) != HAL_OK)
    {
      HAL_StatusTypeDef rollback_status =
          HAL_TIM_PWM_DeInit(timer->handle);

      pwm->error_count++;
      if (rollback_status != HAL_OK)
      {
        pwm->error_count++;
        pwm->lifecycle = USR_PORT_PWM_LIFECYCLE_FAULT;
        timer->lifecycle = USR_PORT_TIM_LIFECYCLE_FAULT;
        return USR_ERR_BUS;
      }
      port_hal_tim_release(timer,
                           port_hal_pwm_claim_mask(pwm->resource->channel));
      port_hal_tim_reset_runtime(timer);
      return USR_ERR_BUS;
    }
    timer->timer_clock_hz = timer->resource->clock_get_hz();
    timer->counter_hz = calculation.counter_hz;
    timer->prescaler = calculation.prescaler;
    timer->auto_reload = calculation.auto_reload;
    timer->actual_frequency_hz = calculation.actual_frequency_hz;
    timer->pwm_quantized = calculation.quantized;
    timer->lifecycle = USR_PORT_TIM_LIFECYCLE_READY;
    timer_initialized = true;
  }
  else if ((timer->prescaler != calculation.prescaler) ||
           (timer->auto_reload != calculation.auto_reload) ||
           (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_STARTED))
  {
    port_hal_tim_release(timer,
                         port_hal_pwm_claim_mask(pwm->resource->channel));
    return USR_ERR_STATE;
  }

  compare = port_tim_pwm_ccr(calculation.auto_reload,
                             config->duty_permille);
  pwm->resource->gpio_clock_enable();
  gpio.Pin = pwm->resource->gpio_pin;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Alternate = pwm->resource->gpio_alternate;
  HAL_GPIO_Init(pwm->resource->gpio_port, &gpio);
  status = port_hal_pwm_configure_channel(pwm, config, compare);
  if (status != USR_OK)
  {
    HAL_GPIO_DeInit(pwm->resource->gpio_port, pwm->resource->gpio_pin);
    if (timer_initialized)
    {
      if (HAL_TIM_PWM_DeInit(timer->handle) != HAL_OK)
      {
        pwm->error_count++;
        pwm->lifecycle = USR_PORT_PWM_LIFECYCLE_FAULT;
        timer->lifecycle = USR_PORT_TIM_LIFECYCLE_FAULT;
        pwm->error_count++;
        return status;
      }
    }
    port_hal_tim_release(timer,
                         port_hal_pwm_claim_mask(pwm->resource->channel));
    if (timer_initialized)
    {
      port_hal_tim_reset_runtime(timer);
    }
    pwm->error_count++;
    return status;
  }

  timer->pwm_channels[channel_index].claimed = true;
  timer->pwm_channels[channel_index].started = false;
  timer->pwm_channels[channel_index].channel = pwm->resource->channel;
  timer->pwm_channels[channel_index].duty_permille = config->duty_permille;
  timer->pwm_channels[channel_index].compare = compare;
  timer->pwm_channels[channel_index].polarity =
      port_hal_pwm_polarity(config->polarity);
  timer->pwm_channels[channel_index].owner_context = pwm;
  pwm->runtime_config = *config;
  pwm->device.config = *config;
  pwm->compare = compare;
  base.prescaler = calculation.prescaler;
  base.auto_reload = calculation.auto_reload;
  base.counter_hz = calculation.counter_hz;
  base.actual_frequency_hz = calculation.actual_frequency_hz;
  base.quantized = calculation.quantized;
  port_hal_pwm_fill_actual(pwm, &base);
  pwm->lifecycle = USR_PORT_PWM_LIFECYCLE_READY;
  return USR_OK;
}

static usr_status_t port_hal_pwm_deinit(void *ctx)
{
  usr_port_hal_pwm_instance_t *pwm =
      (usr_port_hal_pwm_instance_t *)ctx;
  usr_port_hal_tim_instance_t *timer;
  uint32_t index;
  uint32_t channel_index;
  bool another_channel = false;

  if (!port_hal_pwm_descriptor_valid(pwm))
  {
    return USR_ERR_PARAM;
  }
  if (pwm->lifecycle == USR_PORT_PWM_LIFECYCLE_RESET)
  {
    return USR_OK;
  }
  if (pwm->lifecycle == USR_PORT_PWM_LIFECYCLE_STARTED)
  {
    return USR_ERR_STATE;
  }
  timer = pwm->resource->timer;
  if (!port_hal_tim_channel_index(pwm->resource->channel, &channel_index))
  {
    return USR_ERR_UNSUPPORTED;
  }
  for (index = 0u; index < USR_PORT_TIM_PWM_CHANNEL_COUNT; index++)
  {
    if ((index != channel_index) && timer->pwm_channels[index].claimed)
    {
      another_channel = true;
    }
  }
  if (!another_channel && (HAL_TIM_PWM_DeInit(timer->handle) != HAL_OK))
  {
    pwm->error_count++;
    pwm->lifecycle = USR_PORT_PWM_LIFECYCLE_FAULT;
    timer->lifecycle = USR_PORT_TIM_LIFECYCLE_FAULT;
    return USR_ERR_BUS;
  }
  HAL_GPIO_DeInit(pwm->resource->gpio_port, pwm->resource->gpio_pin);
  memset(&timer->pwm_channels[channel_index], 0,
         sizeof(timer->pwm_channels[channel_index]));
  port_hal_tim_release(timer,
                       port_hal_pwm_claim_mask(pwm->resource->channel));
  if (!another_channel)
  {
    port_hal_tim_reset_runtime(timer);
  }
  pwm->lifecycle = USR_PORT_PWM_LIFECYCLE_RESET;
  return USR_OK;
}

static usr_status_t port_hal_pwm_start(void *ctx)
{
  usr_port_hal_pwm_instance_t *pwm =
      (usr_port_hal_pwm_instance_t *)ctx;
  usr_port_hal_tim_instance_t *timer;
  uint32_t index;

  if (!port_hal_pwm_descriptor_valid(pwm))
  {
    return USR_ERR_PARAM;
  }
  if (pwm->lifecycle == USR_PORT_PWM_LIFECYCLE_STARTED)
  {
    return USR_OK;
  }
  if (pwm->lifecycle == USR_PORT_PWM_LIFECYCLE_FAULT)
  {
    return USR_ERR_STATE;
  }
  if (pwm->lifecycle != USR_PORT_PWM_LIFECYCLE_READY)
  {
    return USR_ERR_NOT_INIT;
  }
  timer = pwm->resource->timer;
  if (!port_hal_tim_channel_index(pwm->resource->channel, &index))
  {
    return USR_ERR_UNSUPPORTED;
  }
  if (HAL_TIM_PWM_Start(timer->handle, pwm->resource->channel) != HAL_OK)
  {
    pwm->error_count++;
    return USR_ERR_BUS;
  }
  timer->pwm_channels[index].started = true;
  timer->lifecycle = USR_PORT_TIM_LIFECYCLE_STARTED;
  pwm->lifecycle = USR_PORT_PWM_LIFECYCLE_STARTED;
  return USR_OK;
}

static usr_status_t port_hal_pwm_stop(void *ctx)
{
  usr_port_hal_pwm_instance_t *pwm =
      (usr_port_hal_pwm_instance_t *)ctx;
  usr_port_hal_tim_instance_t *timer;
  uint32_t index;
  bool another_started = false;

  if (!port_hal_pwm_descriptor_valid(pwm))
  {
    return USR_ERR_PARAM;
  }
  if (pwm->lifecycle == USR_PORT_PWM_LIFECYCLE_RESET)
  {
    return USR_ERR_NOT_INIT;
  }
  if (pwm->lifecycle == USR_PORT_PWM_LIFECYCLE_FAULT)
  {
    return USR_ERR_STATE;
  }
  if (pwm->lifecycle == USR_PORT_PWM_LIFECYCLE_READY)
  {
    return USR_OK;
  }
  timer = pwm->resource->timer;
  if (!port_hal_tim_channel_index(pwm->resource->channel, &index))
  {
    return USR_ERR_UNSUPPORTED;
  }
  if (HAL_TIM_PWM_Stop(timer->handle, pwm->resource->channel) != HAL_OK)
  {
    pwm->error_count++;
    return USR_ERR_BUS;
  }
  timer->pwm_channels[index].started = false;
  for (index = 0u; index < USR_PORT_TIM_PWM_CHANNEL_COUNT; index++)
  {
    another_started = another_started || timer->pwm_channels[index].started;
  }
  if (!another_started)
  {
    timer->lifecycle = USR_PORT_TIM_LIFECYCLE_READY;
  }
  pwm->lifecycle = USR_PORT_PWM_LIFECYCLE_READY;
  return USR_OK;
}

static usr_status_t port_hal_pwm_set_duty(void *ctx,
                                          uint16_t duty_permille)
{
  usr_port_hal_pwm_instance_t *pwm =
      (usr_port_hal_pwm_instance_t *)ctx;
  usr_port_hal_tim_instance_t *timer;
  uint32_t index;
  uint32_t compare;
  port_hal_pwm_base_t base;

  if (!port_hal_pwm_descriptor_valid(pwm) || (duty_permille > 1000u))
  {
    return USR_ERR_PARAM;
  }
  if (pwm->lifecycle == USR_PORT_PWM_LIFECYCLE_RESET)
  {
    return USR_ERR_NOT_INIT;
  }
  if (pwm->lifecycle == USR_PORT_PWM_LIFECYCLE_FAULT)
  {
    return USR_ERR_STATE;
  }
  timer = pwm->resource->timer;
  if (!port_hal_tim_channel_index(pwm->resource->channel, &index))
  {
    return USR_ERR_UNSUPPORTED;
  }
  compare = port_tim_pwm_ccr(timer->auto_reload, duty_permille);
  __HAL_TIM_SET_COMPARE(timer->handle, pwm->resource->channel, compare);
  timer->pwm_channels[index].duty_permille = duty_permille;
  timer->pwm_channels[index].compare = compare;
  pwm->runtime_config.duty_permille = duty_permille;
  pwm->device.config.duty_permille = duty_permille;
  pwm->compare = compare;
  base.prescaler = timer->prescaler;
  base.auto_reload = timer->auto_reload;
  base.counter_hz = timer->counter_hz;
  base.actual_frequency_hz = timer->actual_frequency_hz;
  base.quantized = timer->pwm_quantized;
  port_hal_pwm_fill_actual(pwm, &base);
  return USR_OK;
}

static usr_status_t port_hal_pwm_set_frequency(void *ctx,
                                               uint32_t frequency_hz,
                                               void *actual)
{
  usr_port_hal_pwm_instance_t *pwm =
      (usr_port_hal_pwm_instance_t *)ctx;
  dev_pwm_actual_t *actual_result = (dev_pwm_actual_t *)actual;
  usr_port_hal_tim_instance_t *timer;
  port_hal_pwm_base_t base;
  port_hal_pwm_base_t old_base;
  usr_port_hal_tim_pwm_channel_t
      channels[USR_PORT_TIM_PWM_CHANNEL_COUNT];
  usr_port_hal_tim_pwm_channel_t
      old_channels[USR_PORT_TIM_PWM_CHANNEL_COUNT];
  TIM_Base_InitTypeDef old_init;
  HAL_StatusTypeDef hal_status;
  uint32_t primask;
  usr_status_t status;

  if (!port_hal_pwm_descriptor_valid(pwm) || (actual_result == NULL) ||
      (frequency_hz == 0u))
  {
    return USR_ERR_PARAM;
  }
  if (pwm->lifecycle == USR_PORT_PWM_LIFECYCLE_RESET)
  {
    return USR_ERR_NOT_INIT;
  }
  if (pwm->lifecycle == USR_PORT_PWM_LIFECYCLE_FAULT)
  {
    return USR_ERR_STATE;
  }
  timer = pwm->resource->timer;
  status = port_hal_pwm_prepare_frequency(pwm, frequency_hz,
                                          &base, channels);
  if (status != USR_OK)
  {
    return status;
  }
  old_base.prescaler = timer->prescaler;
  old_base.auto_reload = timer->auto_reload;
  old_base.counter_hz = timer->counter_hz;
  old_base.actual_frequency_hz = timer->actual_frequency_hz;
  old_base.quantized = timer->pwm_quantized;
  memcpy(old_channels, timer->pwm_channels, sizeof(old_channels));
  old_init = timer->handle->Init;
  primask = __get_PRIMASK();
  __disable_irq();
  hal_status = port_hal_pwm_commit(timer, &base, channels);
  if (hal_status != HAL_OK)
  {
    port_hal_pwm_restore(timer, &old_base, old_channels, &old_init);
  }
  if (primask == 0u)
  {
    __enable_irq();
  }
  if (hal_status != HAL_OK)
  {
    pwm->error_count++;
    return USR_ERR_BUS;
  }
  memcpy(timer->pwm_channels, channels, sizeof(timer->pwm_channels));
  timer->prescaler = base.prescaler;
  timer->auto_reload = base.auto_reload;
  timer->counter_hz = base.counter_hz;
  timer->actual_frequency_hz = base.actual_frequency_hz;
  timer->pwm_quantized = base.quantized;
  port_hal_pwm_sync_instances(timer, &base, frequency_hz);
  *actual_result = pwm->actual;
  return USR_OK;
}

static usr_status_t port_hal_pwm_get_actual(void *ctx, void *actual)
{
  const usr_port_hal_pwm_instance_t *pwm =
      (const usr_port_hal_pwm_instance_t *)ctx;

  if ((pwm == NULL) || (actual == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (pwm->lifecycle == USR_PORT_PWM_LIFECYCLE_RESET)
  {
    return USR_ERR_NOT_INIT;
  }
  if (pwm->lifecycle == USR_PORT_PWM_LIFECYCLE_FAULT)
  {
    return USR_ERR_STATE;
  }
  *(dev_pwm_actual_t *)actual = pwm->actual;
  return USR_OK;
}

static usr_status_t port_hal_pwm_get_status(void *ctx, void *status)
{
  const usr_port_hal_pwm_instance_t *pwm =
      (const usr_port_hal_pwm_instance_t *)ctx;
  dev_pwm_status_t *pwm_status = (dev_pwm_status_t *)status;

  if ((pwm == NULL) || (pwm_status == NULL))
  {
    return USR_ERR_PARAM;
  }
  pwm_status->initialized =
      (pwm->lifecycle != USR_PORT_PWM_LIFECYCLE_RESET);
  pwm_status->started =
      (pwm->lifecycle == USR_PORT_PWM_LIFECYCLE_STARTED);
  pwm_status->error_count = pwm->error_count;
  return USR_OK;
}

const dev_pwm_ops_t port_hal_pwm_ops =
{
  .init = port_hal_pwm_init,
  .deinit = port_hal_pwm_deinit,
  .start = port_hal_pwm_start,
  .stop = port_hal_pwm_stop,
  .set_duty = port_hal_pwm_set_duty,
  .set_frequency = port_hal_pwm_set_frequency,
  .get_actual = port_hal_pwm_get_actual,
  .get_status = port_hal_pwm_get_status,
};
