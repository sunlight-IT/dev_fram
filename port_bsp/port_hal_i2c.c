/**
  ******************************************************************************
  * @file    port_hal_i2c.c
  * @brief   Reusable STM32 HAL I2C adapter.
  ******************************************************************************
  */
#include <string.h>

#include "stm32wlxx_hal.h"
#include "port_hal_dma.h"
#include "port_hal_i2c.h"

static usr_status_t port_hal_i2c_map(HAL_StatusTypeDef status)
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

static bool port_hal_i2c_resource_valid(
    const usr_port_hal_i2c_resource_t *resource)
{
  return ((resource != NULL) && (resource->instance != NULL) &&
          (resource->gpio_port != NULL) && (resource->gpio_pins != 0u) &&
          (resource->clock_config != NULL) &&
          (resource->clock_enable != NULL) &&
          (resource->clock_disable != NULL) &&
          (resource->gpio_clock_enable != NULL));
}

static usr_port_hal_i2c_instance_t *port_hal_i2c_instance(void *ctx)
{
  usr_port_hal_i2c_instance_t *i2c =
      (usr_port_hal_i2c_instance_t *)ctx;

  if ((i2c == NULL) || !port_hal_i2c_resource_valid(i2c->resource))
  {
    return NULL;
  }
  return i2c;
}

static usr_status_t port_hal_i2c_init(void *ctx)
{
  usr_port_hal_i2c_instance_t *i2c = port_hal_i2c_instance(ctx);
  HAL_StatusTypeDef hal_status;

  if (i2c == NULL)
  {
    return USR_ERR_PARAM;
  }
  if (i2c->initialized != 0u)
  {
    return USR_OK;
  }

  i2c->initialized = 0u;
  if (i2c->resource->clock_config() != HAL_OK)
  {
    return USR_ERR_BUS;
  }

  (void)memset(&i2c->handle, 0, sizeof(i2c->handle));
  i2c->handle.Instance = i2c->resource->instance;
  i2c->handle.Init.Timing = i2c->resource->timing;
  i2c->handle.Init.OwnAddress1 = 0u;
  i2c->handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  i2c->handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  i2c->handle.Init.OwnAddress2 = 0u;
  i2c->handle.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  i2c->handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  i2c->handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

  hal_status = HAL_I2C_Init(&i2c->handle);
  if (hal_status != HAL_OK)
  {
    return port_hal_i2c_map(hal_status);
  }
  hal_status = HAL_I2CEx_ConfigAnalogFilter(
      &i2c->handle, I2C_ANALOGFILTER_ENABLE);
  if (hal_status != HAL_OK)
  {
    (void)HAL_I2C_DeInit(&i2c->handle);
    return port_hal_i2c_map(hal_status);
  }
  hal_status = HAL_I2CEx_ConfigDigitalFilter(&i2c->handle, 0u);
  if (hal_status != HAL_OK)
  {
    (void)HAL_I2C_DeInit(&i2c->handle);
    return port_hal_i2c_map(hal_status);
  }

  i2c->initialized = 1u;
  return USR_OK;
}

static usr_status_t port_hal_i2c_write(void *ctx, uint8_t addr7,
                                       uint16_t reg, uint8_t reg_len,
                                       const uint8_t *data, uint16_t len,
                                       uint32_t timeout_ms)
{
  usr_port_hal_i2c_instance_t *i2c = port_hal_i2c_instance(ctx);
  uint16_t address_size;

  if ((i2c == NULL) || (addr7 > 0x7fu) ||
      ((reg_len != 1u) && (reg_len != 2u)) ||
      (data == NULL) || (len == 0u))
  {
    return USR_ERR_PARAM;
  }
  if (i2c->initialized == 0u)
  {
    return USR_ERR_NOT_INIT;
  }
  address_size = (reg_len == 2u) ? I2C_MEMADD_SIZE_16BIT :
                                   I2C_MEMADD_SIZE_8BIT;
  return port_hal_i2c_map(HAL_I2C_Mem_Write(
      &i2c->handle, (uint16_t)(addr7 << 1), reg, address_size,
      (uint8_t *)data, len, timeout_ms));
}

static usr_status_t port_hal_i2c_read(void *ctx, uint8_t addr7,
                                      uint16_t reg, uint8_t reg_len,
                                      uint8_t *data, uint16_t len,
                                      uint32_t timeout_ms)
{
  usr_port_hal_i2c_instance_t *i2c = port_hal_i2c_instance(ctx);
  uint16_t address_size;

  if ((i2c == NULL) || (addr7 > 0x7fu) ||
      ((reg_len != 1u) && (reg_len != 2u)) ||
      (data == NULL) || (len == 0u))
  {
    return USR_ERR_PARAM;
  }
  if (i2c->initialized == 0u)
  {
    return USR_ERR_NOT_INIT;
  }
  address_size = (reg_len == 2u) ? I2C_MEMADD_SIZE_16BIT :
                                   I2C_MEMADD_SIZE_8BIT;
  return port_hal_i2c_map(HAL_I2C_Mem_Read(
      &i2c->handle, (uint16_t)(addr7 << 1), reg, address_size,
      data, len, timeout_ms));
}

static usr_status_t port_hal_i2c_transmit(void *ctx, uint8_t addr7,
                                          const uint8_t *data, uint16_t len,
                                          uint32_t timeout_ms)
{
  usr_port_hal_i2c_instance_t *i2c = port_hal_i2c_instance(ctx);

  if ((i2c == NULL) || (addr7 > 0x7fu) ||
      (data == NULL) || (len == 0u))
  {
    return USR_ERR_PARAM;
  }
  if (i2c->initialized == 0u)
  {
    return USR_ERR_NOT_INIT;
  }
  return port_hal_i2c_map(HAL_I2C_Master_Transmit(
      &i2c->handle, (uint16_t)(addr7 << 1), (uint8_t *)data,
      len, timeout_ms));
}

static usr_status_t port_hal_i2c_receive(void *ctx, uint8_t addr7,
                                         uint8_t *data, uint16_t len,
                                         uint32_t timeout_ms)
{
  usr_port_hal_i2c_instance_t *i2c = port_hal_i2c_instance(ctx);

  if ((i2c == NULL) || (addr7 > 0x7fu) ||
      (data == NULL) || (len == 0u))
  {
    return USR_ERR_PARAM;
  }
  if (i2c->initialized == 0u)
  {
    return USR_ERR_NOT_INIT;
  }
  return port_hal_i2c_map(HAL_I2C_Master_Receive(
      &i2c->handle, (uint16_t)(addr7 << 1), data, len, timeout_ms));
}

static usr_status_t port_hal_i2c_probe(void *ctx, uint8_t addr7,
                                       uint32_t trials,
                                       uint32_t timeout_ms)
{
  usr_port_hal_i2c_instance_t *i2c = port_hal_i2c_instance(ctx);
  HAL_StatusTypeDef status;

  if ((i2c == NULL) || (trials == 0u) || (addr7 > 0x7fu))
  {
    return USR_ERR_PARAM;
  }
  if (i2c->initialized == 0u)
  {
    return USR_ERR_NOT_INIT;
  }
  status = HAL_I2C_IsDeviceReady(&i2c->handle, (uint16_t)(addr7 << 1),
                                 trials, timeout_ms);
  if (status == HAL_ERROR)
  {
    return USR_ERR_NO_DEV;
  }
  return port_hal_i2c_map(status);
}

static usr_status_t port_hal_i2c_recover(void *ctx)
{
  usr_port_hal_i2c_instance_t *i2c = port_hal_i2c_instance(ctx);
  usr_status_t status;

  if (i2c == NULL)
  {
    return USR_ERR_PARAM;
  }
  if (i2c->initialized == 0u)
  {
    return USR_ERR_NOT_INIT;
  }

  i2c->initialized = 0u;
  status = port_hal_i2c_map(HAL_I2C_DeInit(&i2c->handle));
  if (status != USR_OK)
  {
    return status;
  }
  return port_hal_i2c_init(i2c);
}

bool usr_port_hal_i2c_dma_irq(IRQn_Type irq)
{
  usr_port_hal_i2c_instance_t *i2c;

  i2c = usr_port_board_i2c_find_dma_instance(irq);
  if (i2c == NULL)
  {
    return false;
  }
  port_hal_dma_irq(&i2c->resource->rx_dma);
  return true;
}

bool usr_port_hal_i2c_msp_init(I2C_HandleTypeDef *handle)
{
  GPIO_InitTypeDef gpio_init = {0};
  usr_port_hal_i2c_instance_t *i2c;
  const usr_port_hal_i2c_resource_t *resource;

  i2c = usr_port_board_i2c_find_instance(handle);
  if (i2c == NULL)
  {
    return false;
  }
  resource = i2c->resource;
  if (!port_hal_i2c_resource_valid(resource))
  {
    return true;
  }

  resource->gpio_clock_enable();
  resource->clock_enable();

  gpio_init.Pin = resource->gpio_pins;
  gpio_init.Mode = GPIO_MODE_AF_OD;
  gpio_init.Pull = GPIO_NOPULL;
  gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
  gpio_init.Alternate = resource->gpio_alternate;
  HAL_GPIO_Init(resource->gpio_port, &gpio_init);

  if (port_hal_dma_present(&resource->rx_dma))
  {
    if (port_hal_dma_init(&resource->rx_dma) != HAL_OK)
    {
      return true;
    }
    __HAL_LINKDMA(handle, hdmarx, *resource->rx_dma.handle);
    port_hal_dma_irq_enable(&resource->rx_dma);
  }
  return true;
}

bool usr_port_hal_i2c_msp_deinit(I2C_HandleTypeDef *handle)
{
  usr_port_hal_i2c_instance_t *i2c;
  const usr_port_hal_i2c_resource_t *resource;

  i2c = usr_port_board_i2c_find_instance(handle);
  if (i2c == NULL)
  {
    return false;
  }
  resource = i2c->resource;
  if (!port_hal_i2c_resource_valid(resource))
  {
    return true;
  }

  if (port_hal_dma_present(&resource->rx_dma))
  {
    port_hal_dma_irq_disable(&resource->rx_dma);
    (void)port_hal_dma_deinit(&resource->rx_dma);
    handle->hdmarx = NULL;
  }
  HAL_GPIO_DeInit(resource->gpio_port, resource->gpio_pins);
  resource->clock_disable();
  return true;
}

const dev_i2c_ops_t port_hal_i2c_ops =
{
  port_hal_i2c_init,
  port_hal_i2c_write,
  port_hal_i2c_read,
  port_hal_i2c_transmit,
  port_hal_i2c_receive,
  port_hal_i2c_probe,
  port_hal_i2c_recover,
};
