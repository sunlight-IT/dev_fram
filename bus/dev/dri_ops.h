#ifndef __DRI_OPS_H__
#define __DRI_OPS_H__

#include "usr_common.h"

typedef struct device{
    const char *name;
}device;

typedef struct
{
  usr_status_t (*init)(void *ctx);
  usr_status_t (*write)(void *ctx, usr_pin_id_t pin, uint8_t level);
  usr_status_t (*read)(void *ctx, usr_pin_id_t pin, uint8_t *level);
  usr_status_t (*toggle)(void *ctx, usr_pin_id_t pin);
  usr_status_t (*set_irq_cb)(void *ctx, usr_pin_id_t pin, usr_callback_t cb, void *arg);
} dev_gpio_ops_t;

typedef struct
{
  usr_status_t (*init)(void *ctx, const void *cfg);
  usr_status_t (*deinit)(void *ctx);
  usr_status_t (*write)(void *ctx, const uint8_t *data, uint16_t len, uint32_t timeout_ms);
  usr_status_t (*read)(void *ctx, uint8_t *data, uint16_t capacity, uint16_t *read_len);
  usr_status_t (*control)(void *ctx, uint32_t cmd, void *arg);
  uint32_t     (*rx_overflow)(void *ctx);
} dev_uart_ops_t;


typedef struct
{
  usr_status_t (*init)(void *ctx, const void *cfg);
  usr_status_t (*deinit)(void *ctx);
  /* Transmit exactly tx_len bytes, then receive exactly rx_len bytes while
     the caller keeps chip select asserted. Equal non-zero lengths may use a
     full-duplex backend; unequal lengths must not access either buffer past
     its own length. */
  usr_status_t (*transmit)(void *ctx, const uint8_t *tx, uint16_t tx_len,
                           uint8_t *rx, uint16_t rx_len,
                           uint32_t timeout_ms);
} dev_spi_ops_t;

typedef struct
{
  usr_status_t (*init)(void *ctx);
  usr_status_t (*mem_write)(void *ctx, uint8_t addr7, uint16_t reg, uint8_t reg_len,
                            const uint8_t *data, uint16_t len, uint32_t timeout_ms);
  usr_status_t (*mem_read)(void *ctx, uint8_t addr7, uint16_t reg, uint8_t reg_len,
                           uint8_t *data, uint16_t len, uint32_t timeout_ms);
  usr_status_t (*transmit)(void *ctx, uint8_t addr7, const uint8_t *data, uint16_t len,uint32_t timeout_ms);
  usr_status_t (*receive)(void *ctx, uint8_t addr7, uint8_t *data, uint16_t len, uint32_t timeout_ms);
  usr_status_t (*probe)(void *ctx, uint8_t addr7, uint32_t trials,
                        uint32_t timeout_ms);                            /* device ACK check */
  usr_status_t (*recover)(void *ctx);                                    /* bus deadlock recovery */
} dev_i2c_ops_t;

typedef struct
{
  usr_status_t (*init)(void *ctx);
  usr_status_t (*deinit)(void *ctx);
  usr_status_t (*start)(void *ctx);
  usr_status_t (*stop)(void *ctx);
  uint32_t (*get_ticks)(void *ctx);
  usr_status_t (*get_status)(void *ctx, void *status);
} dev_timer_ops_t;

typedef struct
{
  usr_status_t (*init)(void *ctx);
  usr_status_t (*deinit)(void *ctx);
  usr_status_t (*start)(void *ctx);
  usr_status_t (*stop)(void *ctx);
  usr_status_t (*set_compare)(void *ctx, uint8_t channel,
                              uint32_t compare);
  usr_status_t (*get_status)(void *ctx, void *status);
} dev_pwm_ops_t;
#endif
