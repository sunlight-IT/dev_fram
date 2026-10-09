/**
  ******************************************************************************
  * @file    dev_spi.c
  * @brief   Platform-independent SPI transactions and software chip select.
  ******************************************************************************
  */
#include "dev_spi.h"

static spi_device_t s_devices[SPI_INDEX_MAX];

spi_device_t *get_spi_device(uint8_t index)
{
  if (index >= SPI_INDEX_MAX)
  {
    return NULL;
  }
  return &s_devices[index];
}

static bool dev_spi_config_valid(const spi_device_config_t *config)
{
  return (config != NULL) && (config->max_speed_hz != 0u) &&
         ((config->mode == DEV_SPI_MODE_0) ||
          (config->mode == DEV_SPI_MODE_1) ||
          (config->mode == DEV_SPI_MODE_2) ||
          (config->mode == DEV_SPI_MODE_3)) &&
         ((config->bit_order == DEV_SPI_BIT_ORDER_MSB_FIRST) ||
          (config->bit_order == DEV_SPI_BIT_ORDER_LSB_FIRST)) &&
         ((config->data_width == DEV_SPI_DATA_BITS_8) ||
          (config->data_width == DEV_SPI_DATA_BITS_16));
}

static bool dev_spi_lengths_valid(const uint8_t *tx, uint16_t tx_len,
                                  const uint8_t *rx, uint16_t rx_len)
{
  return !(((tx == NULL) && (tx_len != 0u)) ||
           ((rx == NULL) && (rx_len != 0u)) ||
           ((tx_len == 0u) && (rx_len == 0u)));
}

usr_status_t dev_spi_init(spi_device_t *dev, const spi_device_config_t *cfg)
{
  spi_device_config_t candidate;
  usr_status_t status;

  if ((dev == NULL) || (dev->ops == NULL) || (dev->ops->init == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (cfg != NULL)
  {
    if (!dev_spi_config_valid(cfg))
    {
      return USR_ERR_PARAM;
    }
    candidate = *cfg;
    status = dev->ops->init(dev->ctx, &candidate);
    if (status == USR_OK)
    {
      dev->config = candidate;
    }
    return status;
  }
  return dev->ops->init(dev->ctx, NULL);
}

usr_status_t dev_spi_deinit(spi_device_t *dev)
{
  if ((dev == NULL) || (dev->ops == NULL) || (dev->ops->deinit == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (dev->in_use || dev->cs_active)
  {
    return USR_BUSY;
  }
  return dev->ops->deinit(dev->ctx);
}

usr_status_t dev_spi_transact(spi_device_t *dev,
                              usr_pin_id_t cs_pin,
                              const uint8_t *tx, uint16_t tx_len,
                              uint8_t *rx, uint16_t rx_len,
                              uint32_t timeout_ms)
{
  usr_status_t status;
  usr_status_t release_status;

  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->transmit == NULL) ||
      !dev_spi_lengths_valid(tx, tx_len, rx, rx_len))
  {
    return USR_ERR_PARAM;
  }

  status = dev_spi_begin(dev, cs_pin);
  if (status != USR_OK)
  {
    return status;
  }

  status = dev_spi_frame(dev, tx, tx_len, rx, rx_len, timeout_ms);
  release_status = dev_spi_end(dev, cs_pin);
  if ((status == USR_OK) && (release_status != USR_OK))
  {
    status = release_status;
  }
  return status;
}

usr_status_t dev_spi_begin(spi_device_t *dev, usr_pin_id_t cs_pin)
{
  usr_status_t status;

  if ((dev == NULL) || (dev->ops == NULL) || (dev->gpio == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (dev->in_use || dev->cs_active)
  {
    return USR_BUSY;
  }

  status = dev_gpio_write(dev->gpio, cs_pin, 0u);
  if (status != USR_OK)
  {
    return status;
  }

  dev->active_cs_pin = cs_pin;
  dev->cs_active = true;
  dev->in_use = true;
  return USR_OK;
}

usr_status_t dev_spi_frame(spi_device_t *dev,
                           const uint8_t *tx, uint16_t tx_len,
                           uint8_t *rx, uint16_t rx_len,
                           uint32_t timeout_ms)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->transmit == NULL) ||
      !dev_spi_lengths_valid(tx, tx_len, rx, rx_len))
  {
    return USR_ERR_PARAM;
  }
  if (!dev->in_use || !dev->cs_active)
  {
    return USR_ERR_STATE;
  }
  return dev->ops->transmit(dev->ctx, tx, tx_len, rx, rx_len, timeout_ms);
}

usr_status_t dev_spi_end(spi_device_t *dev, usr_pin_id_t cs_pin)
{
  usr_status_t status;

  if ((dev == NULL) || (dev->gpio == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (!dev->in_use || !dev->cs_active ||
      (dev->active_cs_pin != cs_pin))
  {
    return USR_ERR_STATE;
  }

  status = dev_gpio_write(dev->gpio, cs_pin, 1u);
  if (status == USR_OK)
  {
    dev->cs_active = false;
    dev->in_use = false;
  }
  return status;
}
