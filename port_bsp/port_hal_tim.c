#include "port_hal_tim.h"

TIM_HandleTypeDef htim2;

static bool port_hal_tim_valid(const usr_port_hal_tim_instance_t *timer)
{
  return (timer != NULL) && (timer->resource != NULL) &&
         (timer->resource->instance != NULL) && (timer->handle != NULL) &&
         (timer->resource->timer_clock_hz != 0u) &&
         (timer->resource->clock_enable != NULL) &&
         (timer->resource->clock_disable != NULL);
}

static usr_status_t port_hal_tim_mode_check(
    const usr_port_hal_tim_instance_t *timer, usr_port_tim_mode_t mode)
{
  if (!port_hal_tim_valid(timer))
  {
    return USR_ERR_PARAM;
  }
  return (timer->resource->mode == mode) ? USR_OK : USR_ERR_UNSUPPORTED;
}

static usr_status_t port_hal_tim_init_common(
    usr_port_hal_tim_instance_t *timer, usr_port_tim_mode_t mode)
{
  HAL_StatusTypeDef hal_status;
  HAL_StatusTypeDef rollback_status;
  usr_status_t status = port_hal_tim_mode_check(timer, mode);

  if (status != USR_OK)
  {
    return status;
  }
  if (timer->lifecycle != USR_PORT_TIM_LIFECYCLE_RESET)
  {
    return USR_ERR_STATE;
  }

  timer->handle->Instance = timer->resource->instance;
  timer->handle->Init.Prescaler = timer->resource->prescaler;
  timer->handle->Init.CounterMode = TIM_COUNTERMODE_UP;
  timer->handle->Init.Period = timer->resource->auto_reload;
  timer->handle->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  timer->handle->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  hal_status = (mode == USR_PORT_TIM_MODE_BASIC) ?
      HAL_TIM_Base_Init(timer->handle) : HAL_TIM_PWM_Init(timer->handle);
  if (hal_status != HAL_OK)
  {
    timer->error_count++;
    rollback_status = (mode == USR_PORT_TIM_MODE_BASIC) ?
        HAL_TIM_Base_DeInit(timer->handle) : HAL_TIM_PWM_DeInit(timer->handle);
    if (rollback_status != HAL_OK)
    {
      timer->error_count++;
      timer->lifecycle = USR_PORT_TIM_LIFECYCLE_FAULT;
    }
    return USR_ERR_BUS;
  }
  timer->tick_count = 0u;
  timer->compare = timer->resource->pwm_compare;
  timer->lifecycle = USR_PORT_TIM_LIFECYCLE_READY;
  return USR_OK;
}

static usr_status_t port_hal_tim_deinit_common(
    usr_port_hal_tim_instance_t *timer, usr_port_tim_mode_t mode)
{
  HAL_StatusTypeDef hal_status;
  usr_status_t status = port_hal_tim_mode_check(timer, mode);

  if (status != USR_OK)
  {
    return status;
  }
  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_RESET)
  {
    return USR_OK;
  }
  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_STARTED)
  {
    return USR_ERR_STATE;
  }
  hal_status = (mode == USR_PORT_TIM_MODE_BASIC) ?
      HAL_TIM_Base_DeInit(timer->handle) : HAL_TIM_PWM_DeInit(timer->handle);
  if (hal_status != HAL_OK)
  {
    timer->error_count++;
    timer->lifecycle = USR_PORT_TIM_LIFECYCLE_FAULT;
    return USR_ERR_BUS;
  }
  timer->lifecycle = USR_PORT_TIM_LIFECYCLE_RESET;
  return USR_OK;
}

static usr_status_t port_hal_tim_start_common(
    usr_port_hal_tim_instance_t *timer, usr_port_tim_mode_t mode)
{
  HAL_StatusTypeDef hal_status;
  usr_status_t status = port_hal_tim_mode_check(timer, mode);

  if (status != USR_OK)
  {
    return status;
  }
  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_STARTED)
  {
    return USR_OK;
  }
  if (timer->lifecycle != USR_PORT_TIM_LIFECYCLE_READY)
  {
    return (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_FAULT) ?
        USR_ERR_STATE : USR_ERR_NOT_INIT;
  }
  hal_status = (mode == USR_PORT_TIM_MODE_BASIC) ?
      HAL_TIM_Base_Start_IT(timer->handle) :
      HAL_TIM_PWM_Start(timer->handle, timer->resource->pwm_channel);
  if (hal_status != HAL_OK)
  {
    timer->error_count++;
    return USR_ERR_BUS;
  }
  timer->lifecycle = USR_PORT_TIM_LIFECYCLE_STARTED;
  return USR_OK;
}

static usr_status_t port_hal_tim_stop_common(
    usr_port_hal_tim_instance_t *timer, usr_port_tim_mode_t mode)
{
  HAL_StatusTypeDef hal_status;
  usr_status_t status = port_hal_tim_mode_check(timer, mode);

  if (status != USR_OK)
  {
    return status;
  }
  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_READY)
  {
    return USR_OK;
  }
  if (timer->lifecycle != USR_PORT_TIM_LIFECYCLE_STARTED)
  {
    return (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_FAULT) ?
        USR_ERR_STATE : USR_ERR_NOT_INIT;
  }
  hal_status = (mode == USR_PORT_TIM_MODE_BASIC) ?
      HAL_TIM_Base_Stop_IT(timer->handle) :
      HAL_TIM_PWM_Stop(timer->handle, timer->resource->pwm_channel);
  if (hal_status != HAL_OK)
  {
    timer->error_count++;
    return USR_ERR_BUS;
  }
  timer->lifecycle = USR_PORT_TIM_LIFECYCLE_READY;
  return USR_OK;
}

static usr_status_t port_hal_timer_init(void *ctx)
{
  return port_hal_tim_init_common(
      (usr_port_hal_tim_instance_t *)ctx, USR_PORT_TIM_MODE_BASIC);
}

static usr_status_t port_hal_timer_deinit(void *ctx)
{
  return port_hal_tim_deinit_common(
      (usr_port_hal_tim_instance_t *)ctx, USR_PORT_TIM_MODE_BASIC);
}

static usr_status_t port_hal_timer_start(void *ctx)
{
  return port_hal_tim_start_common(
      (usr_port_hal_tim_instance_t *)ctx, USR_PORT_TIM_MODE_BASIC);
}

static usr_status_t port_hal_timer_stop(void *ctx)
{
  return port_hal_tim_stop_common(
      (usr_port_hal_tim_instance_t *)ctx, USR_PORT_TIM_MODE_BASIC);
}

static uint32_t port_hal_timer_get_ticks(void *ctx)
{
  usr_port_hal_tim_instance_t *timer =
      (usr_port_hal_tim_instance_t *)ctx;

  return (port_hal_tim_mode_check(timer, USR_PORT_TIM_MODE_BASIC) == USR_OK) ?
      timer->tick_count : 0u;
}

static usr_status_t port_hal_timer_get_status(void *ctx, void *status)
{
  const usr_port_hal_tim_instance_t *timer =
      (const usr_port_hal_tim_instance_t *)ctx;
  dev_timer_status_t *result = (dev_timer_status_t *)status;

  if ((timer == NULL) || (result == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (timer->resource == NULL || timer->resource->mode != USR_PORT_TIM_MODE_BASIC)
  {
    return USR_ERR_UNSUPPORTED;
  }
  result->initialized = (timer->lifecycle != USR_PORT_TIM_LIFECYCLE_RESET);
  result->started = (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_STARTED);
  result->error_count = timer->error_count;
  return USR_OK;
}

static usr_status_t port_hal_pwm_init(void *ctx)
{
  usr_port_hal_tim_instance_t *timer =
      (usr_port_hal_tim_instance_t *)ctx;
  TIM_OC_InitTypeDef channel_config = {0};
  GPIO_InitTypeDef gpio = {0};
  usr_status_t status;

  status = port_hal_tim_mode_check(timer, USR_PORT_TIM_MODE_PWM);
  if (status != USR_OK)
  {
    return status;
  }
  if ((timer->resource->pwm_gpio_port == NULL) ||
      (timer->resource->pwm_gpio_pin == 0u) ||
      (timer->resource->pwm_gpio_clock_enable == NULL) ||
      ((timer->resource->pwm_channel != TIM_CHANNEL_1) &&
       (timer->resource->pwm_channel != TIM_CHANNEL_2) &&
       (timer->resource->pwm_channel != TIM_CHANNEL_3) &&
       (timer->resource->pwm_channel != TIM_CHANNEL_4)) ||
      ((uint64_t)timer->resource->pwm_compare >
       ((uint64_t)timer->resource->auto_reload + 1u)))
  {
    return USR_ERR_PARAM;
  }
  status = port_hal_tim_init_common(timer, USR_PORT_TIM_MODE_PWM);
  if (status != USR_OK)
  {
    return status;
  }

  timer->resource->pwm_gpio_clock_enable();
  gpio.Pin = timer->resource->pwm_gpio_pin;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Alternate = timer->resource->pwm_gpio_alternate;
  HAL_GPIO_Init(timer->resource->pwm_gpio_port, &gpio);
  channel_config.OCMode = TIM_OCMODE_PWM1;
  channel_config.Pulse = timer->resource->pwm_compare;
  channel_config.OCPolarity = timer->resource->pwm_polarity;
  channel_config.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(timer->handle, &channel_config,
                                timer->resource->pwm_channel) != HAL_OK)
  {
    HAL_GPIO_DeInit(timer->resource->pwm_gpio_port,
                    timer->resource->pwm_gpio_pin);
    if (HAL_TIM_PWM_DeInit(timer->handle) != HAL_OK)
    {
      timer->error_count++;
      timer->lifecycle = USR_PORT_TIM_LIFECYCLE_FAULT;
    }
    else
    {
      timer->lifecycle = USR_PORT_TIM_LIFECYCLE_RESET;
    }
    timer->error_count++;
    return USR_ERR_BUS;
  }
  timer->compare = timer->resource->pwm_compare;
  return USR_OK;
}

static usr_status_t port_hal_pwm_deinit(void *ctx)
{
  usr_port_hal_tim_instance_t *timer =
      (usr_port_hal_tim_instance_t *)ctx;
  usr_status_t status = port_hal_tim_deinit_common(
      timer, USR_PORT_TIM_MODE_PWM);

  if ((status == USR_OK) && (timer->resource->pwm_gpio_port != NULL))
  {
    HAL_GPIO_DeInit(timer->resource->pwm_gpio_port,
                    timer->resource->pwm_gpio_pin);
  }
  return status;
}

static usr_status_t port_hal_pwm_start(void *ctx)
{
  return port_hal_tim_start_common(
      (usr_port_hal_tim_instance_t *)ctx, USR_PORT_TIM_MODE_PWM);
}

static usr_status_t port_hal_pwm_stop(void *ctx)
{
  return port_hal_tim_stop_common(
      (usr_port_hal_tim_instance_t *)ctx, USR_PORT_TIM_MODE_PWM);
}

static usr_status_t port_hal_pwm_set_compare(void *ctx, uint32_t compare)
{
  usr_port_hal_tim_instance_t *timer =
      (usr_port_hal_tim_instance_t *)ctx;
  usr_status_t status = port_hal_tim_mode_check(timer, USR_PORT_TIM_MODE_PWM);

  if (status != USR_OK)
  {
    return status;
  }
  if ((timer->lifecycle == USR_PORT_TIM_LIFECYCLE_RESET) ||
      (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_FAULT))
  {
    return (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_FAULT) ?
        USR_ERR_STATE : USR_ERR_NOT_INIT;
  }
  if ((uint64_t)compare > ((uint64_t)timer->resource->auto_reload + 1u))
  {
    return USR_ERR_PARAM;
  }
  __HAL_TIM_SET_COMPARE(timer->handle, timer->resource->pwm_channel, compare);
  timer->compare = compare;
  return USR_OK;
}

static usr_status_t port_hal_pwm_get_status(void *ctx, void *status)
{
  const usr_port_hal_tim_instance_t *timer =
      (const usr_port_hal_tim_instance_t *)ctx;
  dev_pwm_status_t *result = (dev_pwm_status_t *)status;

  if ((timer == NULL) || (result == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (timer->resource == NULL || timer->resource->mode != USR_PORT_TIM_MODE_PWM)
  {
    return USR_ERR_UNSUPPORTED;
  }
  result->initialized = (timer->lifecycle != USR_PORT_TIM_LIFECYCLE_RESET);
  result->started = (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_STARTED);
  result->error_count = timer->error_count;
  return USR_OK;
}

static usr_port_hal_tim_instance_t *port_hal_tim_from_handle(
    TIM_HandleTypeDef *handle)
{
  return usr_port_board_tim_find_instance(handle);
}

static void port_hal_tim_msp_init(TIM_HandleTypeDef *handle,
                                  bool enable_update_irq)
{
  usr_port_hal_tim_instance_t *timer = port_hal_tim_from_handle(handle);

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

static void port_hal_tim_msp_deinit(TIM_HandleTypeDef *handle,
                                    bool disable_update_irq)
{
  usr_port_hal_tim_instance_t *timer = port_hal_tim_from_handle(handle);

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

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *handle)
{
  port_hal_tim_msp_init(handle, true);
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *handle)
{
  port_hal_tim_msp_deinit(handle, true);
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *handle)
{
  port_hal_tim_msp_init(handle, false);
}

void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef *handle)
{
  port_hal_tim_msp_deinit(handle, false);
}

void port_hal_tim_irq(usr_port_hal_tim_instance_t *timer)
{
  if ((timer != NULL) && (timer->handle != NULL) &&
      (timer->resource->mode == USR_PORT_TIM_MODE_BASIC) &&
      (__HAL_TIM_GET_FLAG(timer->handle, TIM_FLAG_UPDATE) != RESET) &&
      (__HAL_TIM_GET_IT_SOURCE(timer->handle, TIM_IT_UPDATE) != RESET))
  {
    __HAL_TIM_CLEAR_IT(timer->handle, TIM_IT_UPDATE);
    timer->tick_count++;
  }
}

void TIM2_IRQHandler(void)
{
  port_hal_tim_irq(usr_port_board_tim_find_instance(&htim2));
}

const dev_timer_ops_t port_hal_tim_ops =
{
  .init = port_hal_timer_init,
  .deinit = port_hal_timer_deinit,
  .start = port_hal_timer_start,
  .stop = port_hal_timer_stop,
  .get_ticks = port_hal_timer_get_ticks,
  .get_status = port_hal_timer_get_status,
};

const dev_pwm_ops_t port_hal_pwm_ops =
{
  .init = port_hal_pwm_init,
  .deinit = port_hal_pwm_deinit,
  .start = port_hal_pwm_start,
  .stop = port_hal_pwm_stop,
  .set_compare = port_hal_pwm_set_compare,
  .get_status = port_hal_pwm_get_status,
};
