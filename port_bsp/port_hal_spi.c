/**
 ******************************************************************************
 * @file    port_hal_spi.c
 * @brief   Reusable STM32 HAL SPI adapter.
 ******************************************************************************
 */
#include <stdint.h>
#include <string.h>

#include "main.h"
#include "dev_spi.h"
#include "port_hal_dma.h"
#include "stm32h7xx_hal_spi.h"
#include "port_hal_spi.h"

#define PORT_HAL_SPI_RECOVERY_TIMEOUT_MS 10u

static SPI_HandleTypeDef s_spi_handles[SPI_INDEX_MAX];

SPI_HandleTypeDef *get_spi_table_handle(uint32_t index)
{
  if (index >= SPI_INDEX_MAX)
  {
    return NULL;
  }
  return &s_spi_handles[index];
}

static usr_status_t port_hal_spi_map(HAL_StatusTypeDef status)
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

static bool port_hal_spi_mode_valid(spi_transfer_mode_t mode)
{
  return (mode == DEV_SPI_TRANSFER_POLLING) ||
         (mode == DEV_SPI_TRANSFER_DMA_POLL) ||
         (mode == DEV_SPI_TRANSFER_DMA);
}

static bool port_hal_spi_dma_valid(const port_hal_dma_resource_t *dma,
                                   uint32_t direction)
{
  return port_hal_dma_enabled(dma) && (dma->init.Mode == DMA_NORMAL) &&
         (dma->init.Direction == direction);
}

static bool port_hal_spi_resource_valid(
    const usr_port_hal_spi_resource_t *resource)
{
  bool rx_dma;
  bool tx_dma;
  bool uses_irq;

  if ((resource == NULL) || (resource->instance == NULL) ||
      (resource->clock_config == NULL) || (resource->clock_enable == NULL) ||
      (resource->clock_disable == NULL) || (resource->kernel_clock_hz == NULL) ||
      !port_hal_spi_mode_valid(resource->default_config.tx_mode) ||
      !port_hal_spi_mode_valid(resource->default_config.rx_mode))
  {
    return false;
  }
  for (uint32_t index = 0u; index < 3u; index++)
  {
    if ((resource->pins[index].port == NULL) ||
        (resource->pins[index].pin == 0u) ||
        (resource->pins[index].clock_enable == NULL))
    {
      return false;
    }
  }

  rx_dma = port_hal_dma_present(&resource->rx_dma);
  tx_dma = port_hal_dma_present(&resource->tx_dma);
  if (rx_dma && !port_hal_spi_dma_valid(&resource->rx_dma,
                                        DMA_PERIPH_TO_MEMORY))
  {
    return false;
  }
  if (tx_dma && !port_hal_spi_dma_valid(&resource->tx_dma,
                                        DMA_MEMORY_TO_PERIPH))
  {
    return false;
  }
  if (rx_dma && tx_dma &&
      ((resource->rx_dma.handle == resource->tx_dma.handle) ||
       (resource->rx_dma.instance == resource->tx_dma.instance) ||
       (resource->rx_dma.irq == resource->tx_dma.irq)))
  {
    return false;
  }

  if ((resource->default_config.rx_mode == DEV_SPI_TRANSFER_DMA) && !rx_dma)
  {
    return false;
  }
  if ((resource->default_config.tx_mode == DEV_SPI_TRANSFER_DMA) && !tx_dma)
  {
    return false;
  }
  if (((resource->default_config.rx_mode == DEV_SPI_TRANSFER_DMA) ||
       (resource->default_config.tx_mode == DEV_SPI_TRANSFER_DMA)) &&
      (resource->dma_buffer_valid == NULL))
  {
    return false;
  }

  uses_irq = (resource->default_config.rx_mode != DEV_SPI_TRANSFER_POLLING) ||
             (resource->default_config.tx_mode != DEV_SPI_TRANSFER_POLLING);
  return !uses_irq || resource->enable_irq;
}

static bool port_hal_spi_config_resource_valid(
    const usr_port_hal_spi_resource_t *resource,
    const spi_device_config_t *config)
{
  if ((resource == NULL) || (config == NULL) ||
      !port_hal_spi_mode_valid(config->tx_mode) ||
      !port_hal_spi_mode_valid(config->rx_mode))
  {
    return false;
  }
  if ((config->tx_mode == DEV_SPI_TRANSFER_DMA) &&
      !port_hal_dma_enabled(&resource->tx_dma))
  {
    return false;
  }
  if ((config->rx_mode == DEV_SPI_TRANSFER_DMA) &&
      !port_hal_dma_enabled(&resource->rx_dma))
  {
    return false;
  }
  if (((config->tx_mode == DEV_SPI_TRANSFER_DMA) ||
       (config->rx_mode == DEV_SPI_TRANSFER_DMA)) &&
      (resource->dma_buffer_valid == NULL))
  {
    return false;
  }
  if (((config->tx_mode == DEV_SPI_TRANSFER_POLLING) ||
       (config->rx_mode == DEV_SPI_TRANSFER_DMA_POLL) ||
       (config->tx_mode == DEV_SPI_TRANSFER_DMA) ||
       (config->rx_mode == DEV_SPI_TRANSFER_DMA)) &&
      !resource->enable_irq)
  {
    return false;
  }
  return true;
}

static usr_port_hal_spi_instance_t *port_hal_spi_instance(void *ctx)
{
  usr_port_hal_spi_instance_t *spi = (usr_port_hal_spi_instance_t *)ctx;

  if ((spi == NULL) || (spi->handle == NULL) || (spi->device == NULL) ||
      !port_hal_spi_resource_valid(spi->resource))
  {
    return NULL;
  }
  return spi;
}

static uint32_t port_hal_spi_irq_save(void)
{
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  __DMB();
  return primask;
}

static void port_hal_spi_irq_restore(uint32_t primask)
{
  __DMB();
  __set_PRIMASK(primask);
}


static usr_status_t port_hal_spi_select_prescaler(uint32_t kernel_clock_hz,
                                                  uint32_t max_speed_hz,
                                                  uint32_t *prescaler)
{
  static const struct
  {
    uint16_t divisor;
    uint32_t hal_value;
  } options[] = {
      {2u, SPI_BAUDRATEPRESCALER_2},     {4u, SPI_BAUDRATEPRESCALER_4},
      {8u, SPI_BAUDRATEPRESCALER_8},     {16u, SPI_BAUDRATEPRESCALER_16},
      {32u, SPI_BAUDRATEPRESCALER_32},   {64u, SPI_BAUDRATEPRESCALER_64},
      {128u, SPI_BAUDRATEPRESCALER_128}, {256u, SPI_BAUDRATEPRESCALER_256},
  };

  if ((kernel_clock_hz == 0u) || (max_speed_hz == 0u) ||
      (prescaler == NULL))
  {
    return USR_ERR_PARAM;
  }
  for (uint32_t index = 0u;
       index < (sizeof(options) / sizeof(options[0])); index++)
  {
    if ((uint64_t)kernel_clock_hz <=
        ((uint64_t)max_speed_hz * options[index].divisor))
    {
      *prescaler = options[index].hal_value;
      return USR_OK;
    }
  }
  return USR_ERR_UNSUPPORTED;
}

static usr_status_t port_hal_spi_config_to_hal(
    usr_port_hal_spi_instance_t *spi, const spi_device_config_t *config)
{
  uint32_t prescaler;
  uint32_t polarity;
  uint32_t phase;
  uint32_t data_size;
  uint32_t first_bit;
  usr_status_t status;

  if ((spi == NULL) || (config == NULL) ||
      !port_hal_spi_config_resource_valid(spi->resource, config))
  {
    return USR_ERR_PARAM;
  }
  status = port_hal_spi_select_prescaler(spi->resource->kernel_clock_hz(),
                                         config->max_speed_hz, &prescaler);
  if (status != USR_OK)
  {
    return status;
  }

  switch (config->mode)
  {
    case DEV_SPI_MODE_0:
      polarity = SPI_POLARITY_LOW;
      phase = SPI_PHASE_1EDGE;
      break;
    case DEV_SPI_MODE_1:
      polarity = SPI_POLARITY_LOW;
      phase = SPI_PHASE_2EDGE;
      break;
    case DEV_SPI_MODE_2:
      polarity = SPI_POLARITY_HIGH;
      phase = SPI_PHASE_1EDGE;
      break;
    case DEV_SPI_MODE_3:
      polarity = SPI_POLARITY_HIGH;
      phase = SPI_PHASE_2EDGE;
      break;
    default:
      return USR_ERR_PARAM;
  }

  if (config->data_width == DEV_SPI_DATA_BITS_8)
  {
    data_size = SPI_DATASIZE_8BIT;
  }
  else if (config->data_width == DEV_SPI_DATA_BITS_16)
  {
    data_size = SPI_DATASIZE_16BIT;
  }
  else
  {
    return USR_ERR_PARAM;
  }

  if (config->bit_order == DEV_SPI_BIT_ORDER_MSB_FIRST)
  {
    first_bit = SPI_FIRSTBIT_MSB;
  }
  else if (config->bit_order == DEV_SPI_BIT_ORDER_LSB_FIRST)
  {
    first_bit = SPI_FIRSTBIT_LSB;
  }
  else
  {
    return USR_ERR_PARAM;
  }

  (void)memset(&spi->handle->Init, 0, sizeof(spi->handle->Init));
  spi->handle->Instance = spi->resource->instance;
  spi->handle->Init.Mode = SPI_MODE_MASTER;
  spi->handle->Init.Direction = SPI_DIRECTION_2LINES;
  spi->handle->Init.DataSize = data_size;
  spi->handle->Init.CLKPolarity = polarity;
  spi->handle->Init.CLKPhase = phase;
  spi->handle->Init.NSS = SPI_NSS_SOFT;
  spi->handle->Init.BaudRatePrescaler = prescaler;
  spi->handle->Init.FirstBit = first_bit;
  spi->handle->Init.TIMode = SPI_TIMODE_DISABLE;
  spi->handle->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  spi->handle->Init.CRCPolynomial = 7u;
  spi->handle->Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  spi->handle->Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
#if defined(STM32H743xx)
  spi->handle->Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  spi->handle->Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  spi->handle->Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  spi->handle->Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
#endif
  return USR_OK;
}

bool usr_port_hal_spi_dma_irq(IRQn_Type irq)
{
  usr_port_hal_spi_instance_t *spi =
      usr_port_board_spi_find_dma_instance(irq);

  if (spi == NULL)
  {
    return false;
  }
  if ((spi->rx_dma_ready != 0u) && (spi->rx_dma.irq == irq))
  {
    port_hal_dma_irq(&spi->rx_dma);
    return true;
  }
  if ((spi->tx_dma_ready != 0u) && (spi->tx_dma.irq == irq))
  {
    port_hal_dma_irq(&spi->tx_dma);
    return true;
  }
  return false;
}

void usr_port_hal_spi_irq(SPI_HandleTypeDef *handle)
{
  usr_port_hal_spi_instance_t *spi =
      usr_port_board_spi_find_instance(handle);

  if (spi != NULL)
  {
    HAL_SPI_IRQHandler(handle);
    if (handle->ErrorCode != HAL_SPI_ERROR_NONE)
    {
      uint32_t primask = port_hal_spi_irq_save();
      if (spi->transfer_state == USR_PORT_SPI_ACTIVE)
      {
        spi->transfer_result = USR_ERR_BUS;
        spi->error_count++;
        spi->recovery_started_ms = HAL_GetTick();
        spi->recovery_started = 1u;
        __DMB();
        spi->transfer_state = USR_PORT_SPI_ABORTING;
      }
      port_hal_spi_irq_restore(primask);
    }
  }
}





static void port_hal_spi_configure_dma_alignment(
    usr_port_hal_spi_instance_t *spi, const spi_device_config_t *config)
{
  uint32_t memory_alignment = (config->data_width == DEV_SPI_DATA_BITS_16)
                                  ? DMA_MDATAALIGN_HALFWORD
                                  : DMA_MDATAALIGN_BYTE;
  uint32_t peripheral_alignment =
      (config->data_width == DEV_SPI_DATA_BITS_16)
          ? DMA_PDATAALIGN_HALFWORD
          : DMA_PDATAALIGN_BYTE;

  spi->rx_dma = spi->resource->rx_dma;
  spi->tx_dma = spi->resource->tx_dma;
  if (config->rx_mode == DEV_SPI_TRANSFER_DMA)
  {
    spi->rx_dma.init.MemDataAlignment = memory_alignment;
    spi->rx_dma.init.PeriphDataAlignment = peripheral_alignment;
  }
  else
  {
    (void)memset(&spi->rx_dma, 0, sizeof(spi->rx_dma));
  }
  if (config->tx_mode == DEV_SPI_TRANSFER_DMA)
  {
    spi->tx_dma.init.MemDataAlignment = memory_alignment;
    spi->tx_dma.init.PeriphDataAlignment = peripheral_alignment;
  }
  else
  {
    (void)memset(&spi->tx_dma, 0, sizeof(spi->tx_dma));
  }
}

static usr_status_t port_hal_spi_init(void *ctx, const void *cfg)
{
  usr_port_hal_spi_instance_t *spi = port_hal_spi_instance(ctx);
  const spi_device_config_t *config;
  usr_status_t status;
  HAL_StatusTypeDef hal_status;

  if (spi == NULL)
  {
    return USR_ERR_PARAM;
  }
  config = (cfg == NULL) ? &spi->resource->default_config
                         : (const spi_device_config_t *)cfg;
  if (!port_hal_spi_config_resource_valid(spi->resource, config))
  {
    return USR_ERR_PARAM;
  }


  status = port_hal_spi_map(spi->resource->clock_config());
  if (status == USR_OK)
  {
    status = port_hal_spi_config_to_hal(spi, config);
  }


  spi->runtime_config = *config;
  port_hal_spi_configure_dma_alignment(spi, config);
  hal_status = HAL_SPI_Init(spi->handle);
  if ((hal_status != HAL_OK))
  {
    status = port_hal_spi_map(HAL_SPI_DeInit(spi->handle));
    if ((status != USR_OK) ||
        (spi->rx_dma_ready != 0u) || (spi->tx_dma_ready != 0u))
    {
      spi->faulted = 1u;
      spi->transfer_state = USR_PORT_SPI_FAULTED;
    }
    return (hal_status != HAL_OK) ? port_hal_spi_map(hal_status)
                                  : USR_ERR_BUS;
  }

  spi->device->config = *config;
  spi->initialized = 1u;
  spi->faulted = 0u;
  spi->transfer_state = USR_PORT_SPI_IDLE;
  spi->transfer_result = USR_OK;
  spi->recovery_started = 0u;
  return USR_OK;
}

static usr_status_t port_hal_spi_deinit(void *ctx)
{
  usr_port_hal_spi_instance_t *spi = port_hal_spi_instance(ctx);
  usr_status_t status;

  if (spi == NULL)
  {
    return USR_ERR_PARAM;
  }

  if ((spi->initialized == 0u) && (spi->faulted == 0u))
  {
    return USR_OK;
  }
  if (spi->device->in_use || spi->device->cs_active ||
      (spi->transfer_state == USR_PORT_SPI_ACTIVE) ||
      (spi->transfer_state == USR_PORT_SPI_ABORTING))
  {
    return USR_BUSY;
  }


  status = port_hal_spi_map(HAL_SPI_DeInit(spi->handle));

  if (status == USR_OK)
  {
    spi->initialized = 0u;
    spi->faulted = 0u;
    spi->transfer_state = USR_PORT_SPI_IDLE;
  }
  else
  {
    spi->faulted = 1u;
    spi->transfer_state = USR_PORT_SPI_FAULTED;
  }

  return status;
}

bool usr_port_hal_spi_complete(SPI_HandleTypeDef *handle, usr_status_t result)
{
  usr_port_hal_spi_instance_t *spi =
      usr_port_board_spi_find_instance(handle);
  uint32_t primask;

  if (spi == NULL)
  {
    return false;
  }
  primask = port_hal_spi_irq_save();
  if (spi->transfer_state == USR_PORT_SPI_ACTIVE)
  {
    spi->transfer_result = result;
    if (result == USR_OK)
    {
      spi->completed_count++;
      __DMB();
      spi->transfer_state = USR_PORT_SPI_COMPLETED;
    }
    else
    {
      spi->error_count++;
      spi->recovery_started_ms = HAL_GetTick();
      spi->recovery_started = 1u;
      __DMB();
      spi->transfer_state = USR_PORT_SPI_ABORTING;
    }
  }
  port_hal_spi_irq_restore(primask);
  return true;
}

static bool port_hal_spi_dma_stopped(
    const port_hal_dma_resource_t *resource, uint8_t ready)
{
  if (ready == 0u)
  {
    return true;
  }
  if ((resource == NULL) || (resource->handle == NULL) ||
      (resource->handle->Instance == NULL))
  {
    return false;
  }
  if ((resource->handle->State != HAL_DMA_STATE_READY) &&
      (resource->handle->State != HAL_DMA_STATE_RESET))
  {
    return false;
  }
#if defined(STM32H743xx)
  if ((((DMA_Stream_TypeDef *)resource->handle->Instance)->CR & DMA_SxCR_EN) !=
      0u)
  {
    return false;
  }
#endif
  return true;
}

static void port_hal_spi_abort_dma_if_busy(
    const port_hal_dma_resource_t *resource, uint8_t ready)
{
  if ((ready != 0u) && (resource != NULL) &&
      (resource->handle != NULL) &&
      (resource->handle->State == HAL_DMA_STATE_BUSY))
  {
    (void)HAL_DMA_Abort(resource->handle);
  }
}

static usr_status_t port_hal_spi_abort(usr_port_hal_spi_instance_t *spi)
{
  uint32_t primask;
  uint32_t started_ms;
  HAL_StatusTypeDef hal_status;

  primask = port_hal_spi_irq_save();
  if (spi->recovery_started == 0u)
  {
    spi->recovery_started_ms = HAL_GetTick();
    spi->recovery_started = 1u;
  }
  started_ms = spi->recovery_started_ms;
  spi->transfer_state = USR_PORT_SPI_ABORTING;
  port_hal_spi_irq_restore(primask);

  hal_status = HAL_SPI_Abort(spi->handle);
  port_hal_spi_abort_dma_if_busy(&spi->rx_dma, spi->rx_dma_ready);
  port_hal_spi_abort_dma_if_busy(&spi->tx_dma, spi->tx_dma_ready);
  if ((HAL_GetTick() - started_ms) >= PORT_HAL_SPI_RECOVERY_TIMEOUT_MS)
  {
    hal_status = HAL_ERROR;
  }

  while ((spi->handle->State != HAL_SPI_STATE_READY) ||
         !port_hal_spi_dma_stopped(&spi->rx_dma, spi->rx_dma_ready) ||
         !port_hal_spi_dma_stopped(&spi->tx_dma, spi->tx_dma_ready))
  {
    if ((HAL_GetTick() - started_ms) >= PORT_HAL_SPI_RECOVERY_TIMEOUT_MS)
    {
      hal_status = HAL_ERROR;
      break;
    }
  }

  if (spi->resource->enable_irq)
  {
    HAL_NVIC_DisableIRQ(spi->resource->irq);
  }
  if (spi->rx_dma_ready != 0u)
  {
    port_hal_dma_irq_disable(&spi->rx_dma);
  }
  if (spi->tx_dma_ready != 0u)
  {
    port_hal_dma_irq_disable(&spi->tx_dma);
  }
  if (spi->resource->enable_irq)
  {
    HAL_NVIC_ClearPendingIRQ(spi->resource->irq);
  }
  if (spi->rx_dma_ready != 0u)
  {
    HAL_NVIC_ClearPendingIRQ(spi->rx_dma.irq);
  }
  if (spi->tx_dma_ready != 0u)
  {
    HAL_NVIC_ClearPendingIRQ(spi->tx_dma.irq);
  }

  if ((hal_status != HAL_OK) ||
      (spi->handle->State != HAL_SPI_STATE_READY) ||
      !port_hal_spi_dma_stopped(&spi->rx_dma, spi->rx_dma_ready) ||
      !port_hal_spi_dma_stopped(&spi->tx_dma, spi->tx_dma_ready))
  {
    spi->faulted = 1u;
    spi->transfer_state = USR_PORT_SPI_FAULTED;
    spi->recovery_started = 0u;
    return USR_ERR_BUS;
  }

  if (spi->rx_dma_ready != 0u)
  {
    port_hal_dma_irq_enable(&spi->rx_dma);
  }
  if (spi->tx_dma_ready != 0u)
  {
    port_hal_dma_irq_enable(&spi->tx_dma);
  }
  if (spi->resource->enable_irq)
  {
    HAL_NVIC_EnableIRQ(spi->resource->irq);
  }
  spi->transfer_state = USR_PORT_SPI_IDLE;
  spi->recovery_started = 0u;
  return USR_OK;
}

static uint32_t port_hal_spi_remaining(uint32_t started_ms,
                                       uint32_t timeout_ms)
{
  uint32_t elapsed = HAL_GetTick() - started_ms;
  return (elapsed >= timeout_ms) ? 0u : timeout_ms - elapsed;
}

static HAL_StatusTypeDef port_hal_spi_async_start(
    usr_port_hal_spi_instance_t *spi, spi_transfer_mode_t mode,
    const uint8_t *tx, uint8_t *rx, uint16_t frames)
{
  if (mode == DEV_SPI_TRANSFER_POLLING)
  {
    if ((tx != NULL) && (rx != NULL))
    {
      return HAL_SPI_TransmitReceive_IT(spi->handle, tx, rx, frames);
    }
    if (tx != NULL)
    {
      return HAL_SPI_Transmit_IT(spi->handle, tx, frames);
    }
    return HAL_SPI_Receive_IT(spi->handle, rx, frames);
  }

  if ((tx != NULL) && (rx != NULL))
  {
    return HAL_SPI_TransmitReceive_DMA(spi->handle, tx, rx, frames);
  }
  if (tx != NULL)
  {
    return HAL_SPI_Transmit_DMA(spi->handle, tx, frames);
  }
  return HAL_SPI_Receive_DMA(spi->handle, rx, frames);
}



static usr_status_t port_hal_spi_async_phase(
    usr_port_hal_spi_instance_t *spi, spi_transfer_mode_t mode,
    const uint8_t *tx, uint8_t *rx, uint16_t frames, uint32_t started_ms,
    uint32_t timeout_ms)
{
  HAL_StatusTypeDef hal_status;
  usr_status_t result;
  uint32_t primask;

  if (port_hal_spi_remaining(started_ms, timeout_ms) == 0u)
  {
    return USR_ERR_TIMEOUT;
  }

  primask = port_hal_spi_irq_save();
  if (spi->resource->enable_irq)
  {
    HAL_NVIC_ClearPendingIRQ(spi->resource->irq);
  }
  if ((mode == DEV_SPI_TRANSFER_DMA) && (rx != NULL) &&
      (spi->rx_dma_ready != 0u))
  {
    HAL_NVIC_ClearPendingIRQ(spi->rx_dma.irq);
  }
  if ((mode == DEV_SPI_TRANSFER_DMA) && (tx != NULL) &&
      (spi->tx_dma_ready != 0u))
  {
    HAL_NVIC_ClearPendingIRQ(spi->tx_dma.irq);
  }
  spi->transfer_result = USR_BUSY;
  spi->recovery_started = 0u;

  hal_status = port_hal_spi_async_start(spi, mode, tx, rx, frames);
  if (hal_status != HAL_OK)
  {
    spi->transfer_state = USR_PORT_SPI_IDLE;
    port_hal_spi_irq_restore(primask);
    result = port_hal_spi_map(hal_status);
    if ((hal_status != HAL_BUSY) && (port_hal_spi_abort(spi) != USR_OK))
    {
      result = USR_ERR_BUS;
    }
    return result;
  }
  __DMB();
  spi->transfer_state = USR_PORT_SPI_ACTIVE;
  port_hal_spi_irq_restore(primask);

  for (;;)
  {
    usr_port_hal_spi_transfer_state_t state;

    primask = port_hal_spi_irq_save();
    state = spi->transfer_state;
    if (state != USR_PORT_SPI_ACTIVE)
    {
      __DMB();
      result = spi->transfer_result;
      port_hal_spi_irq_restore(primask);
      break;
    }
    if (port_hal_spi_remaining(started_ms, timeout_ms) == 0u)
    {
      spi->timeout_count++;
      spi->transfer_result = USR_ERR_TIMEOUT;
      spi->recovery_started_ms = HAL_GetTick();
      spi->recovery_started = 1u;
      __DMB();
      spi->transfer_state = USR_PORT_SPI_ABORTING;
      port_hal_spi_irq_restore(primask);
      result = USR_ERR_TIMEOUT;
      break;
    }
    port_hal_spi_irq_restore(primask);
  }

  if ((result != USR_OK) && (port_hal_spi_abort(spi) != USR_OK))
  {
    return USR_ERR_BUS;
  }
  spi->transfer_state = USR_PORT_SPI_IDLE;
  return result;
}

static usr_status_t port_hal_spi_polling_phase(
    usr_port_hal_spi_instance_t *spi, const uint8_t *tx, uint8_t *rx,
    uint16_t frames, uint32_t started_ms, uint32_t timeout_ms)
{
  uint32_t remaining = port_hal_spi_remaining(started_ms, timeout_ms);
  HAL_StatusTypeDef status;
  usr_status_t result;
  uint32_t primask;

  if (remaining == 0u)
  {
    return USR_ERR_TIMEOUT;
  }
  if ((tx != NULL) && (rx != NULL))
  {
    status = HAL_SPI_TransmitReceive(spi->handle, (uint8_t *)tx, rx, frames,
                                     remaining);
  }
  else if (tx != NULL)
  {
    status = HAL_SPI_Transmit(spi->handle, (uint8_t *)tx, frames,
                              timeout_ms);
    if ((status == HAL_OK) && (rx != 0u))
    {
      status = HAL_SPI_Receive(spi->handle, rx, frames, timeout_ms);
    }
  
  }
  else
  {
    status = HAL_SPI_Receive(spi->handle, rx, frames, remaining);
  }
  result = port_hal_spi_map(status);
  if ((status == HAL_OK) || (status == HAL_BUSY))
  {
    return result;
  }
//待定审查
  primask = port_hal_spi_irq_save();
  spi->transfer_result = result;
  if (status == HAL_TIMEOUT)
  {
    spi->timeout_count++;
  }
  else
  {
    spi->error_count++;
  }
  spi->recovery_started_ms = HAL_GetTick();
  spi->recovery_started = 1u;
  __DMB();
  spi->transfer_state = USR_PORT_SPI_ABORTING;
  port_hal_spi_irq_restore(primask);

  return (port_hal_spi_abort(spi) == USR_OK) ? result : USR_ERR_BUS;
  //待定审查
}

static bool port_hal_spi_dma_buffer_valid(
    const usr_port_hal_spi_instance_t *spi, const uint8_t *tx, uint16_t tx_len,
    uint8_t *rx, uint16_t rx_len)
{
  if ((spi->resource->dma_buffer_valid == NULL) ||
      ((tx != NULL) &&
       !spi->resource->dma_buffer_valid(tx, tx_len, false)) ||
      ((rx != NULL) &&
       !spi->resource->dma_buffer_valid(rx, rx_len, true)))
  {
    return false;
  }
  return true;
}


static usr_status_t port_hal_spi_transmit(void *ctx, const uint8_t *tx,
                                          uint16_t tx_len, uint8_t *rx,
                                          uint16_t rx_len,
                                          uint32_t timeout_ms)
{
  usr_port_hal_spi_instance_t *spi = port_hal_spi_instance(ctx);
  uint16_t tx_frames;
  uint16_t rx_frames;
  spi_transfer_mode_t rx_mode;
  spi_transfer_mode_t tx_mode;
  uint32_t started_ms;
  usr_status_t result;

  if ((spi == NULL) || ((tx == NULL) && (tx_len != 0u)) ||
      ((rx == NULL) && (rx_len != 0u)) ||
      ((tx_len == 0u) && (rx_len == 0u)))
  {
    return USR_ERR_PARAM;
  }
  if (spi->initialized == 0u)
  {
    return USR_ERR_NOT_INIT;
  }
  if (spi->faulted != 0u)
  {
    return USR_ERR_STATE;
  }


  tx_frames = tx_len;
  rx_frames = rx_len;
  rx_mode = spi->runtime_config.rx_mode;
  tx_mode = spi->runtime_config.tx_mode;

  if (spi->runtime_config.data_width == DEV_SPI_DATA_BITS_16)
  {
    if (((tx_len & 1u) != 0u) || ((rx_len & 1u) != 0u) ||
        ((tx_len != 0u) && (((uintptr_t)tx & 1u) != 0u)) ||
        ((rx_len != 0u) && (((uintptr_t)rx & 1u) != 0u)))
    {
      return USR_ERR_PARAM;
    }
    tx_frames = (uint16_t)(tx_len / 2u);
    rx_frames = (uint16_t)(rx_len / 2u);
  }

  started_ms = HAL_GetTick();
  // if ((tx_frames != 0u) && (tx_frames == rx_frames))
  // {
  //   spi_transfer_mode_t mode;
  //   if ((spi->runtime_config.tx_mode == DEV_SPI_TRANSFER_DMA) &&
  //       (spi->runtime_config.rx_mode == DEV_SPI_TRANSFER_DMA))
  //   {
  //     mode = DEV_SPI_TRANSFER_DMA;
  //   }
  //   else if ((spi->runtime_config.tx_mode == DEV_SPI_TRANSFER_POLLING) &&
  //            (spi->runtime_config.rx_mode == DEV_SPI_TRANSFER_DMA))
  //   {
  //     mode = DEV_SPI_TRANSFER_DMA_POLL;
  //   }
  //   else
  //   {
  //     mode = DEV_SPI_TRANSFER_POLLING;
  //   }
    // return port_hal_spi_phase(spi, mode, tx, tx_len, rx, rx_len, tx_frames,
    //                           started_ms, timeout_ms);
  // }

  // if (tx_frames != 0u)
  // {
  //   result = port_hal_spi_phase(spi, spi->runtime_config.tx_mode, tx, tx_len,
  //                               NULL, 0u, tx_frames, started_ms, timeout_ms);
  //   if ((result != USR_OK) || (rx_frames == 0u))
  //   {
  //     return result;
  //   }
  // }
  // return port_hal_spi_phase(spi, spi->runtime_config.rx_mode, NULL, 0u, rx,
  //                           rx_len, rx_frames, started_ms, timeout_ms);


  HAL_StatusTypeDef status;
  if ((tx != NULL) && (rx != NULL) && (tx_frames == rx_frames)) {//发送指令与数据等长
      if (rx_mode != DEV_SPI_TRANSFER_DMA && tx_mode != DEV_SPI_TRANSFER_DMA) {
        return port_hal_spi_map(HAL_SPI_TransmitReceive(spi->handle, tx, rx, tx_frames,timeout_ms));
      }
      else if (rx_mode == DEV_SPI_TRANSFER_DMA && tx_mode == DEV_SPI_TRANSFER_DMA) {
        return port_hal_spi_map(HAL_SPI_TransmitReceive_DMA(spi->handle, (uint8_t *)tx, (uint8_t *)rx, tx_frames));
      } else if (rx_mode == DEV_SPI_TRANSFER_DMA &&
                 tx_mode == DEV_SPI_TRANSFER_POLLING) {
        status = HAL_SPI_Transmit(spi->handle, (uint8_t *)tx, tx_frames,timeout_ms);
        if ((status != HAL_OK)) {
          return port_hal_spi_map(status);
        }
        return port_hal_spi_map(HAL_SPI_Receive_DMA(spi->handle, (uint8_t *)rx, rx_frames));
      }
  } else if (tx != NULL && rx != NULL && tx_frames != rx_frames) { // 发送指令与数据不等长
     if (rx_mode != DEV_SPI_TRANSFER_DMA && tx_mode != DEV_SPI_TRANSFER_DMA) {
       status = HAL_SPI_Transmit(spi->handle, tx, tx_frames, timeout_ms);
       if ((status != HAL_OK)) {
         return port_hal_spi_map(status);
       }
       return port_hal_spi_map(HAL_SPI_Receive(spi->handle, rx, rx_frames, timeout_ms));
       return HAL_SPI_Receive(spi->handle, rx, rx_frames, timeout_ms);
      }else if (rx_mode == DEV_SPI_TRANSFER_DMA && tx_mode == DEV_SPI_TRANSFER_DMA) {
        status = HAL_SPI_Transmit_DMA(spi->handle, tx, tx_frames);
        if ((status != HAL_OK)) {
          return port_hal_spi_map(status);
        }
       return port_hal_spi_map(HAL_SPI_Receive_DMA(spi->handle, rx, rx_frames));
      }else if (rx_mode == DEV_SPI_TRANSFER_DMA && tx_mode == DEV_SPI_TRANSFER_POLLING) {
        status = HAL_SPI_Transmit(spi->handle, (uint8_t *)tx, tx_frames,timeout_ms);
        if ((status != HAL_OK)) {
          return port_hal_spi_map(status);
        }
        return port_hal_spi_map(HAL_SPI_Receive_DMA(spi->handle, (uint8_t *)rx, rx_frames));
      }
  }
}

bool usr_port_hal_spi_msp_init(SPI_HandleTypeDef *handle)
{
  GPIO_InitTypeDef gpio_init = {0};
  usr_port_hal_spi_instance_t *spi =
      usr_port_board_spi_find_instance(handle);
  const usr_port_hal_spi_resource_t *resource;

  if (spi == NULL)
  {
    return false;
  }
  resource = spi->resource;
  if (!port_hal_spi_resource_valid(resource))
  {
    return true;
  }

  resource->clock_enable();
  gpio_init.Mode = GPIO_MODE_AF_PP;
  gpio_init.Pull = GPIO_NOPULL;
  gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
  for (uint32_t index = 0u; index < 3u; index++)
  {
    resource->pins[index].clock_enable();
    gpio_init.Pin = resource->pins[index].pin;
    gpio_init.Alternate = resource->pins[index].alternate;
    HAL_GPIO_Init(resource->pins[index].port, &gpio_init);
  }
  spi->gpio_ready = 1u;

  if (port_hal_dma_enabled(&spi->rx_dma))
  {
    if (port_hal_dma_init(&spi->rx_dma) != HAL_OK)
    {
      return true;
    }
    spi->rx_dma_ready = 1u;
    __HAL_LINKDMA(handle, hdmarx, *spi->rx_dma.handle);
    port_hal_dma_irq_enable(&spi->rx_dma);
  }
  if (port_hal_dma_enabled(&spi->tx_dma))
  {
    if (port_hal_dma_init(&spi->tx_dma) != HAL_OK)
    {
      return true;
    }
    spi->tx_dma_ready = 1u;
    __HAL_LINKDMA(handle, hdmatx, *spi->tx_dma.handle);
    port_hal_dma_irq_enable(&spi->tx_dma);
  }

  if (resource->enable_irq)
  {
    HAL_NVIC_SetPriority(resource->irq, resource->irq_priority, 0u);
    HAL_NVIC_ClearPendingIRQ(resource->irq);
    HAL_NVIC_EnableIRQ(resource->irq);
  }
  return true;
}

bool usr_port_hal_spi_msp_deinit(SPI_HandleTypeDef *handle)
{
  usr_port_hal_spi_instance_t *spi =
      usr_port_board_spi_find_instance(handle);
  const usr_port_hal_spi_resource_t *resource;

  if (spi == NULL)
  {
    return false;
  }
  resource = spi->resource;
  if (!port_hal_spi_resource_valid(resource))
  {
    return true;
  }

  if (resource->enable_irq)
  {
    HAL_NVIC_DisableIRQ(resource->irq);
    HAL_NVIC_ClearPendingIRQ(resource->irq);
  }
  if (spi->tx_dma_ready != 0u)
  {
    port_hal_dma_irq_disable(&spi->tx_dma);
    if (port_hal_dma_deinit(&spi->tx_dma) != HAL_OK)
    {

    }
    else
    {
      spi->tx_dma_ready = 0u;
      handle->hdmatx = NULL;
    }
  }
  if (spi->rx_dma_ready != 0u)
  {
    port_hal_dma_irq_disable(&spi->rx_dma);
    if (port_hal_dma_deinit(&spi->rx_dma) != HAL_OK)
    {
     
    }
    else
    {
      spi->rx_dma_ready = 0u;
      handle->hdmarx = NULL;
    }
  }
  if (spi->gpio_ready != 0u)
  {
    for (uint32_t index = 0u; index < 3u; index++)
    {
      HAL_GPIO_DeInit(resource->pins[index].port,
                      resource->pins[index].pin);
    }
    spi->gpio_ready = 0u;
  }
  resource->clock_disable();
  return true;
}

const dev_spi_ops_t port_hal_spi_ops = {
    .init = port_hal_spi_init,
    .deinit = port_hal_spi_deinit,
    .transmit = port_hal_spi_transmit,
};
