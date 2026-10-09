#ifndef __DEV_SPI_H__
#define __DEV_SPI_H__

#include "dri_ops.h"
#include "dev_gpio.h"

typedef enum
{
  DEV_SPI_MODE_0 = 0,
  DEV_SPI_MODE_1,
  DEV_SPI_MODE_2,
  DEV_SPI_MODE_3,
} spi_mode_t;

typedef enum
{
  DEV_SPI_BIT_ORDER_MSB_FIRST = 0,
  DEV_SPI_BIT_ORDER_LSB_FIRST,
} spi_bit_order_t;

typedef enum
{
  DEV_SPI_DATA_BITS_8 = 8,
  DEV_SPI_DATA_BITS_16 = 16,
} spi_data_width_t;

typedef enum
{
  SPI_INDEX_0 = 0,
  SPI_INDEX_1 = 1,
  SPI_INDEX_MAX = 2,
} spi_index_t;

typedef struct spi_device_config
{
  uint32_t max_speed_hz;
  spi_mode_t mode;
  spi_bit_order_t bit_order;
  spi_data_width_t data_width;
} spi_device_config_t;



typedef struct spi_device
{
  device parent;
  void *ctx;
  spi_device_config_t config;
  bool in_use;
  bool cs_active;
  usr_pin_id_t active_cs_pin;
  gpio_device_t *gpio;
  const dev_spi_ops_t *ops;
} spi_device_t;

spi_device_t *get_spi_device(uint8_t index);

usr_status_t dev_spi_init(spi_device_t *dev, const spi_device_config_t *cfg);
usr_status_t dev_spi_deinit(spi_device_t *dev);
usr_status_t dev_spi_transact(spi_device_t *dev,
                                  usr_pin_id_t cs_pin,
                                  const uint8_t *tx, uint16_t tx_len,
                                  uint8_t *rx, uint16_t rx_len,
                                  uint32_t timeout_ms);
/* Convenience transaction: assert CS, execute one frame, then release CS. */

/* Multi-frame transaction: CS remains asserted until dev_spi_end(). */

usr_status_t dev_spi_frame(spi_device_t *dev, const uint8_t *tx, uint16_t tx_len,
                           uint8_t *rx, uint16_t rx_len, uint32_t timeout_ms);
usr_status_t dev_spi_begin(spi_device_t *dev, usr_pin_id_t cs_pin);
usr_status_t dev_spi_end(spi_device_t *dev, usr_pin_id_t cs_pin);




#endif
