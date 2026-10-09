/**
  ******************************************************************************
  * @file    port_hal_uart.c
  * @brief   STM32 HAL UART adapter with shared DMA and interrupt RX buffers.
  ******************************************************************************
  */
#include "stm32wlxx_hal.h"
#include "main.h"
#include "port_hal_dma.h"
#include "port_hal_uart.h"

static UART_HandleTypeDef huart_table[UART_INDEX_MAX];

UART_HandleTypeDef *get_uart_table_handle(uint32_t index)
{
  return &huart_table[index];
}

static bool port_hal_uart_valid(const usr_port_hal_uart_instance_t *uart)
{
  const usr_port_hal_uart_resource_t *resource;

  if ((uart == NULL) || (uart->resource == NULL) || (uart->handle == NULL))
  {
    return false;
  }
  resource = uart->resource;
  if ((uart->rx_mode == USR_PORT_HAL_UART_RX_DMA_CIRCULAR) &&
      ((uart->ring.buffer == NULL) ||
       (uart->ring.mode != RING_BUF_MODE_DMA_CIRCULAR) ||
       !port_hal_dma_enabled(&resource->rx_dma)))
  {
    return false;
  }
  if ((uart->rx_mode == USR_PORT_HAL_UART_RX_INTERRUPT) &&
      ((uart->ring.buffer == NULL) ||
       (uart->ring.mode != RING_BUF_MODE_SOFTWARE)))
  {
    return false;
  }
  if ((uart->rx_mode != USR_PORT_HAL_UART_RX_DMA_CIRCULAR) &&
      port_hal_dma_present(&resource->rx_dma))
  {
    return false;
  }
  return true;
}

bool usr_port_hal_uart_dma_irq(IRQn_Type irq)
{
  usr_port_hal_uart_instance_t *uart;

  uart = usr_port_board_uart_find_dma_instance(irq);
  if (uart == NULL)
  {
    return false;
  }
  port_hal_dma_irq(&uart->resource->rx_dma);
  return true;
}

void port_hal_uart_irq(UART_HandleTypeDef *uartHandle)
{
  if (usr_port_board_uart_find_instance(uartHandle) != NULL)
  {
    HAL_UART_IRQHandler(uartHandle);
  }
}

static usr_status_t port_hal_uart_map(HAL_StatusTypeDef status)
{
  switch (status)
  {
    case HAL_OK:
      return USR_OK;
    case HAL_TIMEOUT:
      return USR_ERR_TIMEOUT;
    case HAL_BUSY:
      return USR_BUSY;
    default:
      return USR_ERR_BUS;
  }
}

static usr_status_t port_hal_uart_config_to_hal(
    usr_port_hal_uart_instance_t *uart,
    const uart_param_config_t *cfg)
{
  UART_HandleTypeDef *handle;
  uint32_t word_length;
  uint32_t parity;
  uint32_t stop_bits;
  uint32_t mode;

  if (!port_hal_uart_valid(uart) || (cfg == NULL) ||
      (cfg->baudrate == 0u))
  {
    return USR_ERR_PARAM;
  }

  if (cfg->parity == DEV_UART_PARITY_NONE)
  {
    parity = UART_PARITY_NONE;
    switch (cfg->data_bits)
    {
      case DEV_UART_DATA_BITS_7:
        word_length = UART_WORDLENGTH_7B;
        break;
      case DEV_UART_DATA_BITS_8:
        word_length = UART_WORDLENGTH_8B;
        break;
      case DEV_UART_DATA_BITS_9:
        word_length = UART_WORDLENGTH_9B;
        break;
      default:
        word_length = UART_WORDLENGTH_9B;
        break;
    }
  }
  else
  {
    if (cfg->parity == DEV_UART_PARITY_EVEN)
    {
      parity = UART_PARITY_EVEN;
    }
    else if (cfg->parity == DEV_UART_PARITY_ODD)
    {
      parity = UART_PARITY_ODD;
    }
    else
    {
      return USR_ERR_PARAM;
    }
    switch (cfg->data_bits) {
      case DEV_UART_DATA_BITS_7:
        word_length = UART_WORDLENGTH_8B;
        break;
      case DEV_UART_DATA_BITS_8:
        word_length = UART_WORDLENGTH_9B;
        break;
      case DEV_UART_DATA_BITS_9:
        return USR_ERR_UNSUPPORTED;
        break;
      default:
        return USR_ERR_PARAM;
        break;
    }
  }

  if (cfg->stop_bits == DEV_UART_STOP_BITS_1)
  {
    stop_bits = UART_STOPBITS_1;
  }
  else if (cfg->stop_bits == DEV_UART_STOP_BITS_2)
  {
    stop_bits = UART_STOPBITS_2;
  }
  else
  {
    return USR_ERR_PARAM;
  }

  
  if (cfg->direction == DEV_UART_DIRECTION_TX)
  {
    mode = UART_MODE_TX;
  }
  else if (cfg->direction == DEV_UART_DIRECTION_RX)
  {
    mode = UART_MODE_RX;
  }
  else if (cfg->direction == DEV_UART_DIRECTION_TX_RX)
  {
    mode = UART_MODE_TX_RX;
  }
  else
  {
    return USR_ERR_PARAM;
  }

  handle = uart->handle;
  handle->Instance = uart->resource->instance;
  handle->Init.BaudRate = cfg->baudrate;
  handle->Init.WordLength = word_length;
  handle->Init.StopBits = stop_bits;
  handle->Init.Parity = parity;
  handle->Init.Mode = mode;
  handle->Init.HwFlowCtl = UART_HWCONTROL_NONE;
  handle->Init.OverSampling = UART_OVERSAMPLING_16;
  handle->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  handle->Init.ClockPrescaler = UART_PRESCALER_DIV1;
  handle->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  return USR_OK;
}

static usr_status_t
port_hal_uart_apply_config(usr_port_hal_uart_instance_t *uart,
                           const uart_param_config_t *cfg) {
  
  return port_hal_uart_config_to_hal(uart, cfg);
}

static bool port_hal_uart_direction_has_rx(uart_direction_t direction)
{
  return (direction == DEV_UART_DIRECTION_RX) ||
         (direction == DEV_UART_DIRECTION_TX_RX);
}

static bool port_hal_uart_direction_has_tx(uart_direction_t direction)
{
  return (direction == DEV_UART_DIRECTION_TX) ||
         (direction == DEV_UART_DIRECTION_TX_RX);
}

static usr_status_t port_hal_uart_start_rx(
    usr_port_hal_uart_instance_t *uart,
    const uart_param_config_t *config,
    bool reset_ring);

static usr_status_t port_hal_uart_restart(
    usr_port_hal_uart_instance_t *uart,
    const uart_param_config_t *config,
    bool restart_rx)
{
  usr_status_t status;

  status = port_hal_uart_map(HAL_UART_DeInit(uart->handle));
  if (status != USR_OK)
  {
    return status;
  }
  status = port_hal_uart_map(HAL_UART_Init(uart->handle));
  if (status != USR_OK)
  {
    return status;
  }
  if (restart_rx)
  {
    return port_hal_uart_start_rx(uart, config, true);
  }
  return USR_OK;
}

static usr_status_t port_hal_uart_cleanup(
    usr_port_hal_uart_instance_t *uart)
{
  usr_status_t status;

  status = port_hal_uart_map(HAL_UART_DeInit(uart->handle));
  uart->enabled = 0u;
  uart->restart_pending = 0u;
  uart->initialized = 0u;
  if (status != USR_OK)
  {
    uart->faulted = 1u;
    uart->error_count++;
    return status;
  }
  uart->faulted = 0u;
  return USR_OK;
}

static usr_status_t port_hal_uart_start_rx(
    usr_port_hal_uart_instance_t *uart,
    const uart_param_config_t *config,
    bool reset_ring)
{
  HAL_StatusTypeDef status;

  if (!port_hal_uart_valid(uart) || (config == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (!port_hal_uart_direction_has_rx(config->direction))
  {
    return USR_ERR_UNSUPPORTED;
  }
  if (uart->rx_mode == USR_PORT_HAL_UART_RX_NONE)
  {
    return USR_OK;
  }
  if (uart->enabled != 0u)
  {
    return USR_OK;
  }

  if (reset_ring)
  {
    ring_buf_reset(&uart->ring);
  }
  uart->restart_pending = 0u;
  uart->enabled = 1u;
  if (uart->rx_mode == USR_PORT_HAL_UART_RX_DMA_CIRCULAR)
  {
    status = HAL_UART_Receive_DMA(uart->handle, uart->ring.buffer,
                                  uart->ring.capacity);
  }
  else
  {
    status = HAL_UART_Receive_IT(uart->handle, &uart->it_byte, 1u);
  }
  if (status != HAL_OK)
  {
    uart->enabled = 0u;
    return port_hal_uart_map(status);
  }
  return USR_OK;
}

static usr_status_t port_hal_uart_stop_rx(
    usr_port_hal_uart_instance_t *uart)
{
  HAL_StatusTypeDef status = HAL_OK;
  usr_status_t stop_status;


  if (uart->enabled == 0u)
  {
    uart->restart_pending = 0u;
    return USR_OK;
  }

  if (uart->rx_mode == USR_PORT_HAL_UART_RX_DMA_CIRCULAR)
  {
    status = HAL_UART_DMAStop(uart->handle);
  }
  else if (uart->rx_mode == USR_PORT_HAL_UART_RX_INTERRUPT)
  {
    status = HAL_UART_AbortReceive(uart->handle);
  }

  
  if (status != HAL_OK)
  {
    stop_status = port_hal_uart_map(status);
    uart->enabled = 0u;
    uart->restart_pending = 0u;
    if ((HAL_UART_AbortReceive(uart->handle) != HAL_OK) ||
        (port_hal_uart_start_rx(uart, &uart->runtime_config, false) != USR_OK))
    {
      uart->enabled = 0u;
      uart->restart_pending = 0u;
      uart->faulted = 1u;
      uart->error_count++;
    }
    return stop_status;
  }
  uart->enabled = 0u;
  uart->restart_pending = 0u;
  return USR_OK;
}

static usr_status_t port_hal_uart_recover_rx_if_needed(
    usr_port_hal_uart_instance_t *uart)
{
  usr_status_t status;

  if (uart->restart_pending == 0u)
  {
    return USR_OK;
  }

  if (uart->rx_mode == USR_PORT_HAL_UART_RX_DMA_CIRCULAR)
  {
    (void)HAL_UART_DMAStop(uart->handle);
  }
  else if (uart->rx_mode == USR_PORT_HAL_UART_RX_INTERRUPT)
  {
    (void)HAL_UART_AbortReceive(uart->handle);
  }
  uart->enabled = 0u;
  uart->restart_pending = 0u;
  status = port_hal_uart_start_rx(uart, &uart->runtime_config, false);
  if (status != USR_OK)
  {
    uart->restart_pending = 1u;
  }
  return status;
}

static usr_status_t port_hal_uart_init(void *ctx, const void *cfg)
{
  usr_port_hal_uart_instance_t *uart = (usr_port_hal_uart_instance_t *)ctx;
  const uart_param_config_t *config;
  usr_status_t status;

  config = (cfg == NULL) ? &uart->resource->default_config
                         : (const uart_param_config_t *)cfg;
  
  uart->rx_mode = config->rx_mode; // Update the rx_mode in the instance
  uart->irq_enable = config->irq_enable; // Update the irq_enable in the instance
  if (config->rx_mode == DEV_UART_RX_DMA_CIRCULAR) {
    uart->ring.mode = RING_BUF_MODE_DMA_CIRCULAR;
  } else {
    uart->ring.mode = RING_BUF_MODE_SOFTWARE;
  }

  if (!port_hal_uart_valid(uart))
  {
    return USR_ERR_PARAM;
  }

  if (uart->faulted != 0u || uart->initialized != 0u)
  {
    return USR_ERR_STATE;
  }

  status = port_hal_uart_apply_config(uart, config);
  if (status != USR_OK)
  {
    return status;
  }
  status = port_hal_uart_map(HAL_UART_Init(uart->handle));
  if (status != USR_OK)
  {
    (void)port_hal_uart_cleanup(uart);
    return status;
  }
  if (port_hal_uart_direction_has_rx(config->direction))
  {
    status = port_hal_uart_start_rx(uart, config, true);
    if (status != USR_OK)
    {
      (void)port_hal_uart_cleanup(uart);
      return status;
    }
  }
  uart->runtime_config = *config;
  uart->device->config = *config;
  uart->initialized = 1u;
  return USR_OK;
}

static usr_status_t port_hal_uart_deinit(void *ctx)
{
  usr_port_hal_uart_instance_t *uart =
      (usr_port_hal_uart_instance_t *)ctx;
  usr_status_t status;

  if (!port_hal_uart_valid(uart))
  {
    return USR_ERR_PARAM;
  }

  if (uart->faulted != 0u)
  {
    return port_hal_uart_cleanup(uart);
  }

  if (uart->initialized == 0u)
  {
    return USR_OK;
  }

  status = port_hal_uart_stop_rx(uart);
  if (status != USR_OK)
  {
    return status;
  }
  return port_hal_uart_cleanup(uart);
}

static void port_hal_uart_rollback_config(
    usr_port_hal_uart_instance_t *uart,
    const uart_param_config_t *rollback_config,
    uint8_t old_initialized,
    bool restart_rx)
{
  if ((uart == NULL) || (rollback_config == NULL))
  {
    return;
  }
  (void)port_hal_uart_stop_rx(uart);
  if (HAL_UART_DeInit(uart->handle) != HAL_OK)
  {
    uart->enabled = 0u;
    uart->restart_pending = 0u;
    uart->initialized = 0u;
    uart->faulted = 1u;
    uart->error_count++;
    return;
  }
  if (old_initialized == 0u)
  {
    uart->initialized = 0u;
    uart->faulted = 0u;
    return;
  }
  if ((port_hal_uart_apply_config(uart, rollback_config) != USR_OK) ||
      (HAL_UART_Init(uart->handle) != HAL_OK))
  {
    uart->error_count++;
    (void)port_hal_uart_cleanup(uart);
    return;
  }
  uart->runtime_config = *rollback_config;
  uart->device->config = *rollback_config;
  uart->initialized = 1u;
  uart->faulted = 0u;
  if (restart_rx &&
      (port_hal_uart_start_rx(uart, rollback_config, false) != USR_OK))
  {
    uart->restart_pending = 1u;
    uart->error_count++;
  }
}

static usr_status_t port_hal_uart_write(void *ctx, const uint8_t *data,
                                        uint16_t len, uint32_t timeout_ms)
{
  usr_port_hal_uart_instance_t *uart =
      (usr_port_hal_uart_instance_t *)ctx;

  if (!port_hal_uart_valid(uart) || (data == NULL) || (len == 0u))
  {
    return USR_ERR_PARAM;
  }
  if (uart->faulted != 0u)
  {
    return USR_ERR_STATE;
  }
  if (uart->initialized == 0u)
  {
    return USR_ERR_NOT_INIT;
  }

  return port_hal_uart_map(
      HAL_UART_Transmit(uart->handle, (uint8_t *)data, len, timeout_ms));
}

static usr_status_t port_hal_uart_read(void *ctx, uint8_t *data,
                                       uint16_t capacity, uint16_t *read_len)
{
  usr_port_hal_uart_instance_t *uart =
      (usr_port_hal_uart_instance_t *)ctx;

  if (!port_hal_uart_valid(uart) || (data == NULL) || (capacity == 0u) ||
      (read_len == NULL))
  {
    return USR_ERR_PARAM;
  }
  *read_len = 0u;
  if (uart->faulted != 0u)
  {
    return USR_ERR_STATE;
  }
  if (uart->initialized == 0u)
  {
    return USR_ERR_NOT_INIT;
  }

  if (uart->rx_mode == USR_PORT_HAL_UART_RX_NONE)
  {
    return USR_ERR_UNSUPPORTED;
  }

  if (port_hal_uart_recover_rx_if_needed(uart) != USR_OK)
  {
    *read_len = 0u;
    return USR_ERR_BUS;
  }

  *read_len = ring_buf_read(&uart->ring, data, capacity);
  return USR_OK;
}

static usr_status_t port_hal_uart_control(void *ctx, uint32_t cmd, void *arg)
{
  usr_port_hal_uart_instance_t *uart =
      (usr_port_hal_uart_instance_t *)ctx;
  uart_param_config_t old_config;
  const uart_param_config_t *candidate;
  uint8_t old_initialized;
  bool old_rx_requested;
  usr_status_t status;

  if (!port_hal_uart_valid(uart))
  {
    return USR_ERR_PARAM;
  }
  if (uart->faulted != 0u)
  {
    return USR_ERR_STATE;
  }
  switch (cmd)
  {
    case DEV_UART_CMD_SET_CONFIG:
      if (arg == NULL)
      {
        return USR_ERR_PARAM;
      }
      if (uart->initialized == 0u)
      {
        return USR_ERR_NOT_INIT;
      }

      candidate = (const uart_param_config_t *)arg;
      old_config = uart->runtime_config;
      old_initialized = uart->initialized;
      old_rx_requested = (uart->enabled != 0u) || (uart->restart_pending != 0u);
      
      status = port_hal_uart_stop_rx(uart);
      if (status != USR_OK)
      {
        return status;
      }
      status = port_hal_uart_apply_config(uart, candidate);
      if (status != USR_OK)
      {
        port_hal_uart_rollback_config(uart, &old_config, old_initialized,
                                      old_rx_requested);
        return status;
      }
      status = port_hal_uart_restart(
          uart, candidate,
          old_rx_requested &&
              port_hal_uart_direction_has_rx(candidate->direction));
      if (status != USR_OK)
      {
        port_hal_uart_rollback_config(uart, &old_config, old_initialized,
                                      old_rx_requested);
        return status;
      }
      uart->runtime_config = *candidate;
      uart->device->config = *candidate;
      return USR_OK;

    case DEV_UART_CMD_START_RX:
      if (uart->initialized == 0u)
      {
        return USR_ERR_NOT_INIT;
      }
      return port_hal_uart_start_rx(uart, &uart->runtime_config, true);
    case DEV_UART_CMD_STOP_RX:
      if (uart->initialized == 0u)
      {
        return USR_ERR_NOT_INIT;
      }
      return port_hal_uart_stop_rx(uart);
    case DEV_UART_CMD_GET_CONFIG:
      if (arg == NULL)
      {
        return USR_ERR_PARAM;
      }
      *(uart_param_config_t *)arg = uart->runtime_config;
      return (uart->initialized != 0u) ? USR_OK : USR_ERR_NOT_INIT;
    default:
      return USR_ERR_UNSUPPORTED;
  }
}

static uint32_t port_hal_uart_rx_overflow(void *ctx)
{
  usr_port_hal_uart_instance_t *uart =
      (usr_port_hal_uart_instance_t *)ctx;

  return (!port_hal_uart_valid(uart) ||
          (uart->rx_mode == USR_PORT_HAL_UART_RX_NONE)) ? 0u :
      ring_buf_get_overflow(&uart->ring);
}

bool usr_port_hal_uart_rx_half_cplt(UART_HandleTypeDef *uartHandle)
{
  usr_port_hal_uart_instance_t *uart =
      usr_port_board_uart_find_instance(uartHandle);
  uint16_t remaining;
  uint16_t half;

  if ((uart == NULL) || (uart->rx_mode != USR_PORT_HAL_UART_RX_DMA_CIRCULAR) ||
      (uart->enabled == 0u) || (uart->ring.buffer == NULL) ||
      (uart->ring.capacity < 2u) ||
      (uartHandle->hdmarx == NULL))
  {
    return (uart != NULL) &&
           (uart->rx_mode == USR_PORT_HAL_UART_RX_DMA_CIRCULAR);
  }
  remaining = (uint16_t)__HAL_DMA_GET_COUNTER(uartHandle->hdmarx);
  half = (uint16_t)(uart->ring.capacity / 2u);
  if (remaining > half)
  {
    uart->dma_missed_cycles++;
    return true;
  }
  (void)ring_buf_dma_commit(&uart->ring, half);
  return true;
}

bool usr_port_hal_uart_rx_cplt(UART_HandleTypeDef *uartHandle)
{
  usr_port_hal_uart_instance_t *uart =
      usr_port_board_uart_find_instance(uartHandle);

  if (uart == NULL)
  {
    return false;
  }
  if (uart->rx_mode == USR_PORT_HAL_UART_RX_NONE)
  {
    return true;
  }
  if ((uart->rx_mode == USR_PORT_HAL_UART_RX_DMA_CIRCULAR) &&
      (uart->enabled != 0u) && (uart->ring.buffer != NULL) &&
      (uart->ring.capacity >= 2u) &&
      (uartHandle->hdmarx != NULL))
  {
    uint16_t remaining = (uint16_t)__HAL_DMA_GET_COUNTER(uartHandle->hdmarx);
    uint16_t half = (uint16_t)(uart->ring.capacity / 2u);

    if (remaining < half)
    {
      uart->dma_missed_cycles++;
      return true;
    }
    (void)ring_buf_dma_commit(&uart->ring, half);
    return true;
  }
  if (uart->rx_mode == USR_PORT_HAL_UART_RX_INTERRUPT)
  {
    if (uart->enabled != 0u)
    {
      (void)ring_buf_write(&uart->ring, &uart->it_byte, 1u);
      if (HAL_UART_Receive_IT(uart->handle, &uart->it_byte, 1u) != HAL_OK)
      {
        uart->restart_pending = 1u;
      }
    }
    return true;
  }
  return false;
}

bool usr_port_hal_uart_error(UART_HandleTypeDef *uartHandle)
{
  usr_port_hal_uart_instance_t *uart =
      usr_port_board_uart_find_instance(uartHandle);

  if (uart == NULL)
  {
    return false;
  }
  uart->error_count++;
  if ((uart->initialized != 0u) &&
      port_hal_uart_direction_has_rx(uart->runtime_config.direction) &&
      ((uart->enabled != 0u) || (uart->restart_pending != 0u)))
  {
    uart->restart_pending = 1u;
    uart->enabled = 0u;
  }
  return true;
}

bool usr_port_hal_uart_msp_init(UART_HandleTypeDef *uartHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  const usr_port_hal_uart_resource_t *resource;
  usr_port_hal_uart_instance_t *uart;

  uart = usr_port_board_uart_find_instance(uartHandle);
  if (uart == NULL)
  {
    return false;
  }
  resource = uart->resource;
  if (!port_hal_uart_valid(uart))
  {
    Error_Handler();
    return true;
  }
  if (resource->clock_config() != HAL_OK)
  {
    Error_Handler();
    return true;
  }
  resource->clock_enable();
  resource->gpio_clock_enable();
  GPIO_InitStruct.Pin = resource->gpio_pins;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = resource->gpio_alternate;
  HAL_GPIO_Init(resource->gpio_port, &GPIO_InitStruct);

  if (uart->rx_mode == USR_PORT_HAL_UART_RX_DMA_CIRCULAR)
  {
    if (port_hal_dma_init(&resource->rx_dma) != HAL_OK)
    {
      Error_Handler();
      return true;
    }
    __HAL_LINKDMA(uartHandle, hdmarx, *resource->rx_dma.handle);
    port_hal_dma_irq_enable(&resource->rx_dma);
  }
  if (uart->irq_enable)
  {
    HAL_NVIC_SetPriority(resource->irq, resource->irq_priority, 0u);
    HAL_NVIC_EnableIRQ(resource->irq);
  }
  return true;
}

bool usr_port_hal_uart_msp_deinit(UART_HandleTypeDef *uartHandle)
{
  const usr_port_hal_uart_resource_t *resource;
  usr_port_hal_uart_instance_t *uart;

  uart = usr_port_board_uart_find_instance(uartHandle);
  if (uart == NULL)
  {
    return false;
  }
  resource = uart->resource;
  if (!port_hal_uart_valid(uart))
  {
    return true;
  }
  if (uart->irq_enable)
  {
    HAL_NVIC_DisableIRQ(resource->irq);
    HAL_NVIC_ClearPendingIRQ(resource->irq);
  }
  if (uart->rx_mode == USR_PORT_HAL_UART_RX_DMA_CIRCULAR)
  {
    port_hal_dma_irq_disable(&resource->rx_dma);
    (void)port_hal_dma_deinit(&resource->rx_dma);
    uartHandle->hdmarx = NULL;
  }
  HAL_GPIO_DeInit(resource->gpio_port, resource->gpio_pins);
  resource->clock_disable();
  return true;
}

const dev_uart_ops_t port_hal_uart_ops =
{
  .init = port_hal_uart_init,
  .deinit = port_hal_uart_deinit,
  .write = port_hal_uart_write,
  .read = port_hal_uart_read,
  .control = port_hal_uart_control,
  .rx_overflow = port_hal_uart_rx_overflow,
};
