#include "port_hal_pwm.h"


TIM_HandleTypeDef htim5;

static usr_status_t port_hal_tim_init_common(
    usr_port_hal_pwm_instance_t *timer)
{
  HAL_StatusTypeDef hal_status;
  HAL_StatusTypeDef rollback_status;
  TIM_MasterConfigTypeDef master_config = {0};
  usr_status_t status;

  if (timer->lifecycle != USR_PORT_PWM_LIFECYCLE_RESET)
  {
    return USR_ERR_STATE;
  }

  timer->handle->Instance = timer->resource->instance;
  timer->handle->Init.Prescaler = timer->resource->prescaler;
  timer->handle->Init.CounterMode = TIM_COUNTERMODE_UP;
  timer->handle->Init.Period = timer->resource->auto_reload;
  timer->handle->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  timer->handle->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  hal_status = HAL_TIM_PWM_Init(timer->handle);
  if (hal_status != HAL_OK)
  {
    timer->error_count++;
    rollback_status = HAL_TIM_PWM_DeInit(timer->handle);
    if (rollback_status != HAL_OK)
    {
      timer->error_count++;
      timer->lifecycle = USR_PORT_PWM_LIFECYCLE_FAULT;
    }
    return USR_ERR_BUS;
  }

  master_config.MasterOutputTrigger = TIM_TRGO_RESET;
  master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(timer->handle, &master_config) != HAL_OK)
  {
    timer->error_count++;
    return USR_ERR_BUS;
  }
  timer->tick_count = 0u;
  timer->lifecycle = USR_PORT_PWM_LIFECYCLE_READY;
  return USR_OK;
}

static bool port_hal_pwm_valid(const usr_port_hal_pwm_instance_t *timer)
{
  return (timer != NULL) && (timer->resource != NULL) &&
         (timer->resource->instance != NULL) && (timer->handle != NULL) &&
         (timer->resource->timer_clock_hz != 0u) &&
         (timer->resource->clock_enable != NULL) &&
         (timer->resource->clock_disable != NULL);
}

static bool port_hal_tim_channel_valid(uint32_t channel)
{
  return (channel == TIM_CHANNEL_1) || (channel == TIM_CHANNEL_2) ||
         (channel == TIM_CHANNEL_3) || (channel == TIM_CHANNEL_4);
}

static bool port_hal_tim_channel_matches(uint8_t logical_channel,
                                         uint32_t hal_channel)
{
  return ((logical_channel == DEV_PWM_CHANNEL_1) &&
          (hal_channel == TIM_CHANNEL_1)) ||
         ((logical_channel == DEV_PWM_CHANNEL_2) &&
          (hal_channel == TIM_CHANNEL_2)) ||
         ((logical_channel == DEV_PWM_CHANNEL_3) &&
          (hal_channel == TIM_CHANNEL_3)) ||
         ((logical_channel == DEV_PWM_CHANNEL_4) &&
          (hal_channel == TIM_CHANNEL_4));
}

static usr_status_t port_hal_tim_pwm_resource_valid(
    const usr_port_hal_pwm_instance_t *timer)
{
  const usr_port_hal_pwm_resource_t *resource;
  uint32_t index;
  uint32_t compare_index;

  if (!port_hal_pwm_valid(timer))
  {
    return USR_ERR_PARAM;
  }
  resource = timer->resource;
  if ((resource->pwm_channel_count == 0u) ||
      (resource->pwm_channels == NULL))
  {
    return USR_ERR_PARAM;
  }
  for (index = 0u; index < resource->pwm_channel_count; index++)
  {
    const usr_port_hal_pwm_channel_t *channel =
        &resource->pwm_channels[index];

    if ((channel->logical_channel < DEV_PWM_CHANNEL_1) ||
        (channel->logical_channel > DEV_PWM_CHANNEL_4) ||
        !port_hal_tim_channel_valid(channel->hal_channel) ||
        !port_hal_tim_channel_matches(channel->logical_channel,
                                      channel->hal_channel) ||
        (channel->gpio_port == NULL) || (channel->gpio_pin == 0u) ||
        (channel->gpio_clock_enable == NULL) ||
        ((uint64_t)channel->compare >
         ((uint64_t)resource->auto_reload + 1u)))
    {
      return USR_ERR_PARAM;
    }
    for (compare_index = 0u; compare_index < index; compare_index++)
    {
      if ((resource->pwm_channels[compare_index].logical_channel ==
           channel->logical_channel) ||
          (resource->pwm_channels[compare_index].hal_channel ==
           channel->hal_channel))
      {
        return USR_ERR_PARAM;
      }
    }
  }
  return USR_OK;
}


static const usr_port_hal_pwm_channel_t *port_hal_pwm_find_channel(
    const usr_port_hal_pwm_instance_t *timer, uint8_t logical_channel)
{
  uint32_t index;

  if ((timer == NULL) || (timer->resource == NULL) ||
      (timer->resource->pwm_channels == NULL))
  {
    return NULL;
  }
  for (index = 0u; index < timer->resource->pwm_channel_count; index++)
  {
    if (timer->resource->pwm_channels[index].logical_channel ==
        logical_channel)
    {
      return &timer->resource->pwm_channels[index];
    }
  }
  return NULL;
}
static usr_status_t port_hal_pwm_init(void *ctx)
{
  usr_port_hal_pwm_instance_t *timer =
      (usr_port_hal_pwm_instance_t *)ctx;
  TIM_OC_InitTypeDef channel_config = {0};
  GPIO_InitTypeDef gpio = {0};
  usr_status_t status;
  uint32_t index;

  status = port_hal_tim_pwm_resource_valid(timer);
  if (status != USR_OK)
  {
    return status;
  }
  status = port_hal_tim_init_common(timer);
  if (status != USR_OK)
  {
    return status;
  }

   for (index = 0u; index < timer->resource->pwm_channel_count; index++)
  {
    const usr_port_hal_pwm_channel_t *channel =
        &timer->resource->pwm_channels[index];

    channel_config.OCMode = TIM_OCMODE_PWM1;
    channel_config.Pulse = channel->compare;
    channel_config.OCPolarity = channel->polarity;
    channel_config.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(timer->handle, &channel_config,
                                  channel->hal_channel) != HAL_OK)
    {
      uint32_t rollback_index;

      for (rollback_index = 0u;
           rollback_index < timer->resource->pwm_channel_count;
           rollback_index++)
      {
        const usr_port_hal_pwm_channel_t *rollback_channel =
            &timer->resource->pwm_channels[rollback_index];
        HAL_GPIO_DeInit(rollback_channel->gpio_port,
                        rollback_channel->gpio_pin);
      }
      if (HAL_TIM_PWM_DeInit(timer->handle) != HAL_OK)
      {
        timer->error_count++;
        timer->lifecycle = USR_PORT_PWM_LIFECYCLE_FAULT;
      }
      else
      {
        timer->lifecycle = USR_PORT_PWM_LIFECYCLE_RESET;
      }
      timer->error_count++;
      return USR_ERR_BUS;
    }
  }

  for (index = 0u; index < timer->resource->pwm_channel_count; index++)
  {
    const usr_port_hal_pwm_channel_t *channel =
        &timer->resource->pwm_channels[index];

    channel->gpio_clock_enable();
    gpio.Pin = channel->gpio_pin;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = channel->gpio_alternate;
    HAL_GPIO_Init(channel->gpio_port, &gpio);
  }


  
  return USR_OK;
}

static usr_status_t port_hal_tim_deinit_common(
    usr_port_hal_pwm_instance_t *timer)
{
  HAL_StatusTypeDef hal_status;
  usr_status_t status;

  if (timer->lifecycle == USR_PORT_PWM_LIFECYCLE_RESET)
  {
    return USR_OK;
  }
  if (timer->lifecycle == USR_PORT_PWM_LIFECYCLE_STARTED)
  {
    return USR_ERR_STATE;
  }
  hal_status = HAL_TIM_PWM_DeInit(timer->handle);
  if (hal_status != HAL_OK)
  {
    timer->error_count++;
    timer->lifecycle = USR_PORT_PWM_LIFECYCLE_FAULT;
    return USR_ERR_BUS;
  }
  timer->lifecycle = USR_PORT_PWM_LIFECYCLE_RESET;
  return USR_OK;
}

static usr_status_t port_hal_pwm_deinit(void *ctx)
{
  usr_port_hal_pwm_instance_t *timer = (usr_port_hal_pwm_instance_t *)ctx;
   uint32_t index;
 for (index = 0u; index < timer->resource->pwm_channel_count; index++)
  {
    const usr_port_hal_pwm_channel_t *channel =
        &timer->resource->pwm_channels[index];
    HAL_GPIO_DeInit(channel->gpio_port, channel->gpio_pin);
  }
  if (HAL_TIM_PWM_DeInit(timer->handle) != HAL_OK)
  {
    timer->error_count++;
    timer->lifecycle = USR_PORT_PWM_LIFECYCLE_FAULT;
    return USR_ERR_BUS;
  }
  timer->lifecycle = USR_PORT_PWM_LIFECYCLE_RESET;
  return USR_OK;
}

static usr_status_t port_hal_pwm_start(void *ctx)
{
  HAL_StatusTypeDef hal_status;
  usr_port_hal_pwm_instance_t *timer =
      (usr_port_hal_pwm_instance_t *)ctx;
  usr_status_t status;
  uint32_t index;

  if (timer->lifecycle == USR_PORT_PWM_LIFECYCLE_STARTED)
  {
    return USR_OK;
  }
  if (timer->lifecycle != USR_PORT_PWM_LIFECYCLE_READY)
  {
    return (timer->lifecycle == USR_PORT_PWM_LIFECYCLE_FAULT) ?
        USR_ERR_STATE : USR_ERR_NOT_INIT;
  }
  for (index = 0u; index < timer->resource->pwm_channel_count; index++)
  {
    const usr_port_hal_pwm_channel_t *channel =
        &timer->resource->pwm_channels[index];

    if (HAL_TIM_PWM_Start(timer->handle, channel->hal_channel) != HAL_OK)
    {
      uint32_t rollback_index = index;
      bool rollback_failed = false;

      timer->error_count++;
      while (rollback_index > 0u)
      {
        const usr_port_hal_pwm_channel_t *rollback_channel;
        rollback_index--;
        rollback_channel = &timer->resource->pwm_channels[rollback_index];
        if (HAL_TIM_PWM_Stop(timer->handle,
                             rollback_channel->hal_channel) != HAL_OK)
        {
          rollback_failed = true;
          timer->error_count++;
        }
      }
      if (rollback_failed)
      {
        timer->lifecycle = USR_PORT_PWM_LIFECYCLE_FAULT;
      }
      return USR_ERR_BUS;
    }
  }
  timer->lifecycle = USR_PORT_PWM_LIFECYCLE_STARTED;
  return USR_OK;
}

static usr_status_t port_hal_pwm_stop(void *ctx)
{
   HAL_StatusTypeDef hal_status;
  usr_port_hal_pwm_instance_t *timer = (usr_port_hal_pwm_instance_t *)ctx;
  usr_status_t status;
  uint32_t index;
  bool stop_failed = false;

  if (status != USR_OK)
  {
    return status;
  }
  if (timer->lifecycle == USR_PORT_PWM_LIFECYCLE_READY)
  {
    return USR_OK;
  }
  if (timer->lifecycle != USR_PORT_PWM_LIFECYCLE_STARTED)
  {
    return (timer->lifecycle == USR_PORT_PWM_LIFECYCLE_FAULT) ?
        USR_ERR_STATE : USR_ERR_NOT_INIT;
  }
  for (index = 0u; index < timer->resource->pwm_channel_count; index++)
  {
    const usr_port_hal_pwm_channel_t *channel =
        &timer->resource->pwm_channels[index];

    if (HAL_TIM_PWM_Stop(timer->handle, channel->hal_channel) != HAL_OK)
    {
      stop_failed = true;
      timer->error_count++;
    }
  }
  if (stop_failed)
  {
    timer->lifecycle = USR_PORT_PWM_LIFECYCLE_FAULT;
    return USR_ERR_BUS;
  }
  timer->lifecycle = USR_PORT_PWM_LIFECYCLE_READY;
  return USR_OK;
}

static usr_status_t port_hal_pwm_set_compare(void *ctx, uint8_t logical_channel,
                                             uint32_t compare)
{
  usr_port_hal_pwm_instance_t *timer =
      (usr_port_hal_pwm_instance_t *)ctx;
  usr_status_t status;
  const usr_port_hal_pwm_channel_t *channel;

  if (status != USR_OK)
  {
    return status;
  }
  if ((timer->lifecycle == USR_PORT_PWM_LIFECYCLE_RESET) ||
      (timer->lifecycle == USR_PORT_PWM_LIFECYCLE_FAULT))
  {
    return (timer->lifecycle == USR_PORT_PWM_LIFECYCLE_FAULT) ?
        USR_ERR_STATE : USR_ERR_NOT_INIT;
  }
  channel = port_hal_pwm_find_channel(timer, logical_channel);
  if ((channel == NULL) ||
      ((uint64_t)compare > ((uint64_t)timer->resource->auto_reload + 1u)))
  {
    return USR_ERR_PARAM;
  }
  __HAL_TIM_SET_COMPARE(timer->handle, channel->hal_channel, compare);
  timer->compare = compare;
  return USR_OK;
}

static usr_status_t port_hal_pwm_get_status(void *ctx, void *status)
{
  const usr_port_hal_pwm_instance_t *timer =
      (const usr_port_hal_pwm_instance_t *)ctx;
  dev_pwm_status_t *result = (dev_pwm_status_t *)status;

  if ((timer == NULL) || (result == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (timer->resource == NULL)
  {
    return USR_ERR_UNSUPPORTED;
  }
  result->initialized = (timer->lifecycle != USR_PORT_PWM_LIFECYCLE_RESET);
  result->started = (timer->lifecycle == USR_PORT_PWM_LIFECYCLE_STARTED);
  result->error_count = timer->error_count;
  return USR_OK;
}

static void port_hal_pwm_msp_init(TIM_HandleTypeDef *handle,
                                  bool enable_update_irq)
{
  usr_port_hal_pwm_instance_t *timer = usr_port_board_pwm_find_instance(handle);

  if (timer == NULL)
  {
    return;
  }
  timer->resource->clock_enable();
  if (enable_update_irq && timer->resource->has_update_irq)
  {
    HAL_NVIC_SetPriority(timer->resource->update_irq,
                         timer->resource->irq_priority, 0u);
    HAL_NVIC_EnableIRQ(timer->resource->update_irq);
  }
}

static void port_hal_pwm_msp_deinit(TIM_HandleTypeDef *handle,
                                    bool disable_update_irq)
{
  usr_port_hal_pwm_instance_t *timer = usr_port_board_pwm_find_instance(handle);

  if (timer == NULL)
  {
    return;
  }
  if (disable_update_irq && timer->resource->has_update_irq)
  {
    HAL_NVIC_DisableIRQ(timer->resource->update_irq);
    HAL_NVIC_ClearPendingIRQ(timer->resource->update_irq);
  }
  timer->resource->clock_disable();
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *handle)
{
    port_hal_pwm_msp_init(handle, false);
}

void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef *handle)
{
  port_hal_pwm_msp_deinit(handle, false);
}

const dev_pwm_ops_t port_hal_pwm_ops =
{
  .init = port_hal_pwm_init,
  .deinit = port_hal_pwm_deinit,
  .start = port_hal_pwm_start,
  .stop = port_hal_pwm_stop,
  .set_compare = port_hal_pwm_set_compare,
  .get_status = port_hal_pwm_get_status,
};