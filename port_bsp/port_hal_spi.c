/**
  ******************************************************************************
  * @file    usr_port_hal_spi.c
  * @brief   Reusable STM32 HAL SPI adapter.
  ******************************************************************************
  */
#include <string.h>
#include <stdint.h>

#include "main.h"
#include "dev_spi.h"
#include "port_hal_dma.h"
#include "port_hal_spi.h"

SPI_HandleTypeDef s_spi_handles[SPI_INDEX_MAX];

SPI_HandleTypeDef* get_spi_table_handle(uint32_t index) {
  if (index >= SPI_INDEX_MAX) {
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

static bool port_hal_spi_resource_valid(
    const usr_port_hal_spi_resource_t *resource)
{
  return ((resource != NULL) && (resource->instance != NULL) &&
          (resource->gpio_port != NULL) && (resource->gpio_pins != 0u) &&
          (resource->clock_config != NULL) &&
          (resource->clock_enable != NULL) &&
          (resource->clock_disable != NULL) &&
          (resource->gpio_clock_enable != NULL));
}

static usr_port_hal_spi_instance_t *port_hal_spi_instance(void *ctx)
{
  usr_port_hal_spi_instance_t *spi =
      (usr_port_hal_spi_instance_t *)ctx;

  if ((spi == NULL) || !port_hal_spi_resource_valid(spi->resource))
  {
    return NULL;
  }
  return spi;
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
    {2u, SPI_BAUDRATEPRESCALER_2},
    {4u, SPI_BAUDRATEPRESCALER_4},
    {8u, SPI_BAUDRATEPRESCALER_8},
    {16u, SPI_BAUDRATEPRESCALER_16},
    {32u, SPI_BAUDRATEPRESCALER_32},
    {64u, SPI_BAUDRATEPRESCALER_64},
    {128u, SPI_BAUDRATEPRESCALER_128},
    {256u, SPI_BAUDRATEPRESCALER_256},
  };
  uint32_t index;

  if ((kernel_clock_hz == 0u) || (max_speed_hz == 0u) ||
      (prescaler == NULL))
  {
    return USR_ERR_PARAM;
  }
  for (index = 0u; index < (sizeof(options) / sizeof(options[0])); index++)
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
    usr_port_hal_spi_instance_t *spi,
    const spi_device_config_t *config)
{
  uint32_t prescaler;
  uint32_t polarity;
  uint32_t phase;
  uint32_t data_size;
  uint32_t first_bit;
  usr_status_t status;

  if ((spi == NULL) || (config == NULL) ||
      (spi->resource->kernel_clock_hz == NULL))
  {
    return USR_ERR_PARAM;
  }
  status = port_hal_spi_select_prescaler(
      spi->resource->kernel_clock_hz(), config->max_speed_hz, &prescaler);
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

  // (void)memset(&spi->handle, 0, sizeof(spi->handle));
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
  return USR_OK;
}

bool usr_port_hal_spi_dma_irq(IRQn_Type irq)
{
  usr_port_hal_spi_instance_t *spi;

  spi = usr_port_board_spi_find_dma_instance(irq);
  if (spi == NULL)
  {
    return false;
  }
  port_hal_dma_irq(&spi->resource->rx_dma);
  return true;
}

void usr_port_hal_spi_irq(SPI_HandleTypeDef *handle)
{
  if (usr_port_board_spi_find_instance(handle) == NULL)
  {
    return;
  }
  HAL_SPI_IRQHandler(handle);
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
  config = (cfg == NULL) ? &spi->resource->default_config :
                           (const spi_device_config_t *)cfg;
  if (spi->initialized != 0u)
  {
    return USR_ERR_STATE;
  }

  status = port_hal_spi_config_to_hal(spi, config);
  if (status != USR_OK)
  {
    return status;
  }
  hal_status = HAL_SPI_Init(spi->handle);
  if (hal_status != HAL_OK)
  {
    (void)HAL_SPI_DeInit(spi->handle);
    return port_hal_spi_map(hal_status);
  }

  spi->runtime_config = *config;
  spi->device->config = *config;
  spi->initialized = 1u;
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
  if (spi->initialized == 0u)
  {
    return USR_OK;
  }

  status = port_hal_spi_map(HAL_SPI_DeInit(spi->handle));
  if (status == USR_OK)
  {
    spi->initialized = 0u;
  }
  return status;
}


static usr_status_t port_hal_spi_transmit(void *ctx, const uint8_t *tx,
                                              uint16_t tx_len, uint8_t *rx,
                                              uint16_t rx_len,
                                              uint32_t timeout_ms)
{
  usr_port_hal_spi_instance_t *spi = port_hal_spi_instance(ctx);
  uint16_t tx_frames;
  uint16_t rx_frames;
  HAL_StatusTypeDef status;

  if ((spi == NULL) ||
      ((tx == NULL) && (tx_len != 0u)) ||
      ((rx == NULL) && (rx_len != 0u)) ||
      ((tx_len == 0u) && (rx_len == 0u)))
  {
    return USR_ERR_PARAM;
  }
  if (spi->initialized == 0u)
  {
    return USR_ERR_NOT_INIT;
  }
  tx_frames = tx_len;
  rx_frames = rx_len;
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
  if ((tx_frames != 0u) && (rx_frames != 0u) &&
      (tx_frames == rx_frames))
  {
    status = HAL_SPI_TransmitReceive(spi->handle, (uint8_t *)tx, rx,
                                     tx_frames, timeout_ms);
  }
  else if (tx_frames != 0u)
  {
    status = HAL_SPI_Transmit(spi->handle, (uint8_t *)tx, tx_frames,
                              timeout_ms);
    if ((status == HAL_OK) && (rx_frames != 0u))
    {
      status = HAL_SPI_Receive(spi->handle, rx, rx_frames, timeout_ms);
    }
  }
  else
  {
    status = HAL_SPI_Receive(spi->handle, rx, rx_frames, timeout_ms);
  }
  return port_hal_spi_map(status);
}

bool usr_port_hal_spi_msp_init(SPI_HandleTypeDef *hspi)
{
  GPIO_InitTypeDef gpio_init = {0};
  usr_port_hal_spi_instance_t *spi;
  const usr_port_hal_spi_resource_t *resource;

  spi = usr_port_board_spi_find_instance(hspi);
  if (spi == NULL)
  {
    return false;
  }

  resource = spi->resource;
  if (!port_hal_spi_resource_valid(resource))
  {
    Error_Handler();
    return true;
  }

  if (resource->clock_config() != HAL_OK)
  {
    Error_Handler();
    return true;
  }
  resource->gpio_clock_enable();
  resource->clock_enable();

  gpio_init.Pin = resource->gpio_pins;
  gpio_init.Mode = GPIO_MODE_AF_PP;
  gpio_init.Pull = GPIO_NOPULL;
  gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
  gpio_init.Alternate = resource->gpio_alternate;
  HAL_GPIO_Init(resource->gpio_port, &gpio_init);

  if (port_hal_dma_enabled(&resource->rx_dma))
  {
    if (port_hal_dma_init(&resource->rx_dma) != HAL_OK)
    {
      Error_Handler();
      return true;
    }
    __HAL_LINKDMA(hspi, hdmarx, *resource->rx_dma.handle);
    port_hal_dma_irq_enable(&resource->rx_dma);
  }

  if (resource->enable_irq)
  {
    HAL_NVIC_SetPriority(resource->irq, resource->irq_priority, 0u);
    HAL_NVIC_EnableIRQ(resource->irq);
  }
  return true;
}

bool usr_port_hal_spi_msp_deinit(SPI_HandleTypeDef *hspi)
{
  usr_port_hal_spi_instance_t *spi;
  const usr_port_hal_spi_resource_t *resource;

  spi = usr_port_board_spi_find_instance(hspi);
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
  }
  if (port_hal_dma_present(&resource->rx_dma))
  {
    port_hal_dma_irq_disable(&resource->rx_dma);
    (void)port_hal_dma_deinit(&resource->rx_dma);
    hspi->hdmarx = NULL;
  }
  HAL_GPIO_DeInit(resource->gpio_port, resource->gpio_pins);
  resource->clock_disable();
  return true;
}

const dev_spi_ops_t port_hal_spi_ops =
{
  .init = port_hal_spi_init,
  .deinit = port_hal_spi_deinit,
  .transmit = port_hal_spi_transmit,
};
