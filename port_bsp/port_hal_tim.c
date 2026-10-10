#include "port_hal_tim.h"

TIM_HandleTypeDef htim2;

static TIM_HandleTypeDef htim_table[TIM_INDEX_MAX];

TIM_HandleTypeDef *get_tim_table_handle(uint32_t index) {
  if (index >= TIM_INDEX_MAX) {
    return NULL;
  }
  return &htim_table[index];
}


static bool port_hal_tim_valid(const usr_port_hal_tim_instance_t *timer)
{
  return (timer != NULL) && (timer->resource != NULL) &&
         (timer->resource->instance != NULL) && (timer->handle != NULL) &&
         (timer->resource->timer_clock_hz != 0u) &&
         (timer->resource->clock_enable != NULL) &&
         (timer->resource->clock_disable != NULL);
}

static usr_status_t port_hal_tim_init_common(
    usr_port_hal_tim_instance_t *timer)
{
  HAL_StatusTypeDef hal_status;
  HAL_StatusTypeDef rollback_status;
  TIM_MasterConfigTypeDef master_config = {0};
  usr_status_t status ;

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
  hal_status = HAL_TIM_Base_Init(timer->handle);
  if (hal_status != HAL_OK)
  {
    timer->error_count++;
    rollback_status = HAL_TIM_Base_DeInit(timer->handle);
    if (rollback_status != HAL_OK)
    {
      timer->error_count++;
      timer->lifecycle = USR_PORT_TIM_LIFECYCLE_FAULT;
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
  timer->lifecycle = USR_PORT_TIM_LIFECYCLE_READY;
  return USR_OK;
}

static usr_status_t port_hal_tim_deinit_common(
    usr_port_hal_tim_instance_t *timer)
{
  HAL_StatusTypeDef hal_status;
  usr_status_t status ;


  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_RESET)
  {
    return USR_OK;
  }
  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_STARTED)
  {
    return USR_ERR_STATE;
  }
  hal_status = HAL_TIM_Base_DeInit(timer->handle);
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
    usr_port_hal_tim_instance_t *timer)
{
  HAL_StatusTypeDef hal_status;
  usr_status_t status ;

  if (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_STARTED)
  {
    return USR_OK;
  }
  if (timer->lifecycle != USR_PORT_TIM_LIFECYCLE_READY)
  {
    return (timer->lifecycle == USR_PORT_TIM_LIFECYCLE_FAULT) ?
        USR_ERR_STATE : USR_ERR_NOT_INIT;
  }
  hal_status = 
      HAL_TIM_Base_Start_IT(timer->handle);
  if (hal_status != HAL_OK)
  {
    timer->error_count++;
    return USR_ERR_BUS;
  }
  timer->lifecycle = USR_PORT_TIM_LIFECYCLE_STARTED;
  return USR_OK;
}

static usr_status_t port_hal_tim_stop_common(
    usr_port_hal_tim_instance_t *timer)
{
  HAL_StatusTypeDef hal_status;
  usr_status_t status;

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
  hal_status = 
      HAL_TIM_Base_Stop_IT(timer->handle);

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
      (usr_port_hal_tim_instance_t *)ctx);
}

static usr_status_t port_hal_timer_deinit(void *ctx)
{
  return port_hal_tim_deinit_common(
      (usr_port_hal_tim_instance_t *)ctx);
}

static usr_status_t port_hal_timer_start(void *ctx)
{
  return port_hal_tim_start_common(
      (usr_port_hal_tim_instance_t *)ctx);
}

static usr_status_t port_hal_timer_stop(void *ctx)
{
  return port_hal_tim_stop_common(
      (usr_port_hal_tim_instance_t *)ctx);
}

static uint32_t port_hal_timer_get_ticks(void *ctx)
{
  usr_port_hal_tim_instance_t *timer =
      (usr_port_hal_tim_instance_t *)ctx;

  return timer->tick_count;
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
  if (timer->resource == NULL)
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
                                  bool enable_update_irq) {

  if (handle->Instance == SYSTEM_TIMER)
  {
    return;
  }
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


void port_hal_tim_irq(usr_port_hal_tim_instance_t *timer)
{

  
}

void TIM2_IRQHandler(void) {

  if (usr_port_board_tim_find_instance(&htim_table[TIM_INDEX_0]) != NULL)
  {
    HAL_TIM_IRQHandler(&htim_table[TIM_INDEX_0]);
  }
  // port_hal_tim_irq(usr_port_board_tim_find_instance(&htim2));
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  usr_port_hal_tim_instance_t *timer = port_hal_tim_from_handle(htim);
  
  if (htim->Instance == TIM2)
  {
    timer->tick_count++;
  }
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
