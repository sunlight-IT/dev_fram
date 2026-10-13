#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "dev_spi.h"

static uint32_t s_task = 1u;
static uint32_t s_owner;
static uint32_t s_writes;
static uint8_t s_cs = 1u;
static usr_status_t s_gpio_status;
static usr_status_t s_transfer_status;
static spi_device_config_t s_init_config;

static usr_status_t init(void *ctx, const void *cfg)
{
  (void)ctx;
  assert(cfg != NULL);
  s_init_config = *(const spi_device_config_t *)cfg;
  return USR_OK;
}

static usr_status_t claim(void *ctx)
{
  (void)ctx;
  if (s_owner != 0u)
  {
    return USR_BUSY;
  }
  s_owner = s_task;
  return USR_OK;
}
static bool is_owner(void *ctx)
{
  (void)ctx;
  return s_owner == s_task;
}
static usr_status_t release(void *ctx)
{
  (void)ctx;
  if (!is_owner(ctx))
  {
    return USR_ERR_STATE;
  }
  s_owner = 0u;
  return USR_OK;
}
static usr_status_t write_pin(void *ctx, usr_pin_id_t pin, uint8_t level)
{
  (void)ctx;
  (void)pin;
  s_writes++;
  if (s_gpio_status == USR_OK)
  {
    s_cs = level;
  }
  return s_gpio_status;
}
static usr_status_t transfer(void *ctx, const uint8_t *tx, uint16_t tx_len, uint8_t *rx,
                             uint16_t rx_len, uint32_t timeout)
{
  (void)ctx;
  (void)tx;
  (void)tx_len;
  (void)rx;
  (void)rx_len;
  (void)timeout;
  assert(s_cs == 0u && s_owner == s_task);
  return s_transfer_status;
}
static const dev_spi_ops_t s_ops = {
    .init = init,
    .transmit = transfer,
    .claim = claim,
    .release = release,
    .is_owner = is_owner,
};
static const dev_gpio_ops_t s_gpio_ops = {.write = write_pin};

int main(void)
{
  uint8_t byte = 0x5au;
  gpio_device_t gpio = {.ops = &s_gpio_ops};
  spi_device_t spi = {.ops = &s_ops, .gpio = &gpio};
  spi_device_config_t config = {
      .max_speed_hz = 1000000u,
      .mode = DEV_SPI_MODE_0,
      .bit_order = DEV_SPI_BIT_ORDER_MSB_FIRST,
      .data_width = DEV_SPI_DATA_BITS_8,
      .tx_mode = DEV_SPI_TRANSFER_POLLING,
      .rx_mode = DEV_SPI_TRANSFER_DMA,
  };

  assert(dev_spi_init(&spi, &config) == USR_OK);
  assert(s_init_config.tx_mode == DEV_SPI_TRANSFER_POLLING);
  assert(s_init_config.rx_mode == DEV_SPI_TRANSFER_DMA);
  config.tx_mode = (spi_transfer_mode_t)3;
  assert(dev_spi_init(&spi, &config) == USR_ERR_PARAM);
  config.tx_mode = DEV_SPI_TRANSFER_INTERRUPT;
  config.rx_mode = (spi_transfer_mode_t)3;
  assert(dev_spi_init(&spi, &config) == USR_ERR_PARAM);
  assert(dev_spi_transact(&spi, 0, &byte, 1u, NULL, 0u, 10u) == USR_OK);
  assert(s_cs == 1u && s_owner == 0u && !spi.in_use);
  assert(dev_spi_begin(&spi, 0) == USR_OK);
  s_task = 2u;
  assert(dev_spi_begin(&spi, 0) == USR_BUSY);
  assert(dev_spi_frame(&spi, &byte, 1u, NULL, 0u, 10u) == USR_ERR_STATE);
  assert(dev_spi_end(&spi, 0) == USR_ERR_STATE);
  assert(s_cs == 0u);
  s_task = 1u;
  assert(dev_spi_frame(&spi, &byte, 1u, NULL, 0u, 10u) == USR_OK);
  s_gpio_status = USR_ERR_BUS;
  assert(dev_spi_end(&spi, 0) == USR_ERR_BUS);
  assert(s_owner == 1u && spi.in_use);
  s_gpio_status = USR_OK;
  assert(dev_spi_end(&spi, 0) == USR_OK);
  s_gpio_status = USR_ERR_BUS;
  assert(dev_spi_begin(&spi, 0) == USR_ERR_BUS);
  assert(s_owner == 0u);
  s_gpio_status = USR_OK;
  s_transfer_status = USR_ERR_TIMEOUT;
  assert(dev_spi_transact(&spi, 0, &byte, 1u, NULL, 0u, 1u) == USR_ERR_TIMEOUT);
  assert(s_cs == 1u && s_owner == 0u);
  s_writes = 0u;
  assert(dev_spi_transact(&spi, 0, NULL, 1u, NULL, 0u, 1u) == USR_ERR_PARAM);
  assert(s_writes == 0u);
  puts("dev_spi_test: PASS (ownership, CS, multi-frame, failures)");
  return 0;
}
