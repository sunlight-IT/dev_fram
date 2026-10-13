# Public header contract snapshot

Framework baseline: `d29419be990aa562bcbacabb17ac9c78b31a0908`.

The original excerpts below remain pinned to that baseline. The final SPI DMA
section is a separately labeled fresh working-tree observation from 2026-10-12;
it supersedes the old SPI index count and ops/resource shape for that checkout.

These are public-header excerpts, not a firmware/source distribution.
They document the actual declarations including limits and legacy naming.
They do not establish behavior not specified by the implementation.
The archive has no source files; source links require the matching Git repository.

## bus/dev/dri_ops.h

Source: S045.

```c
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
```

## bus/dev/dev_gpio.h

Source: S034.

```c
#ifndef __DEV_GPIO_H__
#define __DEV_GPIO_H__

#include "dri_ops.h"



typedef struct gpio_device{
    device parent;
    void *ctx;
    const dev_gpio_ops_t *ops;
} gpio_device_t;


usr_status_t dev_gpio_write(gpio_device_t *dev,  usr_pin_id_t id, uint8_t level);
usr_status_t dev_gpio_read(gpio_device_t *dev,  usr_pin_id_t id, uint8_t *level);
usr_status_t dev_gpio_toggle(gpio_device_t *dev, usr_pin_id_t id);
usr_status_t dev_gpio_set_irq(gpio_device_t *dev, usr_pin_id_t id, usr_callback_t cb, void *arg);



#endif
```

## bus/dev/dev_i2c.h

Source: S036.

```c
#ifndef __DEV_I2C_H__
#define __DEV_I2C_H__

#include "dri_ops.h"

typedef struct i2c_device{
    device parent;
    void *ctx;
    bool in_use;
    const dev_i2c_ops_t *ops;
} i2c_device_t;

usr_status_t dev_i2c_init(i2c_device_t *i2c);

usr_status_t dev_i2c_read_reg(i2c_device_t *i2c, uint8_t addr7,
                                  uint16_t reg, uint8_t reg_len,
                                  uint8_t *data, uint16_t len,
                                  uint32_t timeout_ms);

usr_status_t dev_i2c_write_reg(i2c_device_t *i2c, uint8_t addr7,
                                   uint16_t reg, uint8_t reg_len,
                                   const uint8_t *data, uint16_t len,
                                   uint32_t timeout_ms);

usr_status_t dev_i2c_transmit(i2c_device_t *i2c, uint8_t addr7,
                                   const uint8_t *data, uint16_t len,
                                   uint32_t timeout_ms);

usr_status_t dev_i2c_receive(i2c_device_t *i2c, uint8_t addr7,
uint8_t *data, uint16_t len,
uint32_t timeout_ms);

usr_status_t dev_i2c_probe(i2c_device_t *bus, uint8_t addr7,
                               uint32_t timeout_ms);





#endif
```

## bus/dev/dev_spi.h

Source: S040.

```c
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
```

## bus/dev/dev_uart.h

Source: S044.

```c
#ifndef __DEV_UART_H__
#define __DEV_UART_H__


#include "dri_ops.h"

typedef enum
{
  UART_INDEX_0 = 0,
  UART_INDEX_1 = 1,
  UART_INDEX_2 = 2,
  UART_INDEX_MAX = 3,
} uart_index_t;

typedef enum
{
  DEV_UART_DATA_BITS_7 = 7,
  DEV_UART_DATA_BITS_8 = 8,
  DEV_UART_DATA_BITS_9 = 9,
} uart_data_bits_t;

typedef enum
{
  DEV_UART_PARITY_NONE = 0,
  DEV_UART_PARITY_EVEN,
  DEV_UART_PARITY_ODD,
} uart_parity_t;

typedef enum
{
  DEV_UART_STOP_BITS_1 = 0,
  DEV_UART_STOP_BITS_2,
} uart_stop_bits_t;

typedef enum {
  DEV_UART_DIRECTION_TX = 0,
  DEV_UART_DIRECTION_RX,
  DEV_UART_DIRECTION_TX_RX,
} uart_direction_t;


typedef enum
{
  DEV_UART_RX_NONE = 0,
  DEV_UART_RX_DMA_CIRCULAR,
  DEV_UART_RX_INTERRUPT,
} uart_rx_mode_t;


typedef struct uart_param_config_t
{
  uint32_t baudrate;
  uart_data_bits_t data_bits;
  uart_parity_t parity;
  uart_stop_bits_t stop_bits;
  uart_direction_t direction;

  uart_rx_mode_t rx_mode;
  bool irq_enable;
} uart_param_config_t;



typedef struct uart_device
{
  device parent;
  void *ctx;
  uart_param_config_t config;
  const dev_uart_ops_t *ops;
} uart_device_t;


#define DEV_UART_CMD_SET_CONFIG     1u
#define DEV_UART_CMD_GET_CONFIG     2u
#define DEV_UART_CMD_SET_BAUDRATE   3u
#define DEV_UART_CMD_SET_PARITY     4u
#define DEV_UART_CMD_SET_STOP_BITS  5u
#define DEV_UART_CMD_SET_DATA_BITS  6u
#define DEV_UART_CMD_START_RX       7u
#define DEV_UART_CMD_STOP_RX        8u
#define DEV_UART_CMD_SET_DIRECTION  9u






usr_status_t dev_uart_init(uart_device_t *dev, const uart_param_config_t *cfg);
usr_status_t dev_uart_read(uart_device_t *dev, uint8_t *data,
                           uint16_t capacity, uint16_t *read_len);
usr_status_t dev_uart_write(uart_device_t *dev, const uint8_t *data, uint16_t len, uint32_t timeout_ms);
uint32_t dev_uart_rx_overflow(uart_device_t *dev);
usr_status_t dev_uart_deinit(uart_device_t *dev);
usr_status_t dev_uart_set_config(uart_device_t *dev, uint32_t baudrate);
usr_status_t dev_uart_control(uart_device_t *dev, uint32_t cmd, void *arg);

uart_device_t *get_uart_device(uint8_t index);

#endif
```

## bus/dev/dev_timer.h

Source: S042.

```c
#ifndef __DEV_TIMER_H__
#define __DEV_TIMER_H__

#include "dri_ops.h"

typedef enum {
  TIM_INDEX_0 = 0,
  TIM_INDEX_1 = 1,
  TIM_INDEX_2 = 2,
  TIM_INDEX_3 = 3,
  TIM_INDEX_MAX = 4,
} tim_index_t;

typedef struct dev_timer_status
{
  bool initialized;
  bool started;
  uint32_t error_count;
} dev_timer_status_t;

typedef struct timer_device
{
  device parent;
  void *ctx;
  const dev_timer_ops_t *ops;
} timer_device_t;

usr_status_t dev_timer_init(timer_device_t *dev);
usr_status_t dev_timer_deinit(timer_device_t *dev);
usr_status_t dev_timer_start(timer_device_t *dev);
usr_status_t dev_timer_stop(timer_device_t *dev);
uint32_t dev_timer_get_ticks(timer_device_t *dev);
usr_status_t dev_timer_get_status(timer_device_t *dev,
                                  dev_timer_status_t *status);

timer_device_t *get_timer_device(uint8_t index);


#endif
```

## bus/dev/dev_pwm.h

Source: S038.

```c
#ifndef __DEV_PWM_H__
#define __DEV_PWM_H__

#include "dri_ops.h"

typedef enum {
  DEV_PWM_CHANNEL_1 = 0u,
  DEV_PWM_CHANNEL_2 = 1u,
  DEV_PWM_CHANNEL_3 = 2u,
  DEV_PWM_CHANNEL_4 = 3u,
  DEV_PWM_CHANNEL_MAX = 4u,
} dev_pwm_channel_t;

typedef enum {
  PWM_INDEX_0 = 0u,
  PWM_INDEX_1 = 1u,
  PWM_INDEX_MAX = 2u,
} pwm_index_t;

typedef struct dev_pwm_status
{
  bool initialized;
  bool started;
  uint32_t error_count;
} dev_pwm_status_t;

typedef struct pwm_device
{
  device parent;
  void *ctx;
  const dev_pwm_ops_t *ops;
} pwm_device_t;

usr_status_t dev_pwm_init(pwm_device_t *dev);
usr_status_t dev_pwm_deinit(pwm_device_t *dev);
usr_status_t dev_pwm_start(pwm_device_t *dev);
usr_status_t dev_pwm_stop(pwm_device_t *dev);
usr_status_t dev_pwm_set_compare(pwm_device_t *dev, uint8_t channel,
                                 uint32_t compare);
usr_status_t dev_pwm_get_status(pwm_device_t *dev, dev_pwm_status_t *status);

pwm_device_t *get_pwm_device(uint8_t index);

#endif
```

## component/ring_buf.h

Source: S048.

```c
#ifndef RING_BUF_H
#define RING_BUF_H

#include <stdint.h>

typedef enum
{
  RING_BUF_MODE_SOFTWARE = 0,
  RING_BUF_MODE_DMA_CIRCULAR,
} ring_buf_mode_t;

typedef struct
{
  uint8_t *buffer;
  uint16_t capacity;
  ring_buf_mode_t mode;
  volatile uint16_t read_pos;
  volatile uint16_t write_pos;
  volatile uint32_t read_epoch;
  volatile uint32_t write_epoch;
  volatile uint32_t overflow;
} ring_buf_t;

void ring_buf_reset(ring_buf_t *ring);
uint16_t ring_buf_write(ring_buf_t *ring, const uint8_t *data,
                        uint16_t length);
uint16_t ring_buf_dma_commit(ring_buf_t *ring, uint16_t length);
uint16_t ring_buf_read(ring_buf_t *ring, uint8_t *data, uint16_t capacity);
uint16_t ring_buf_available(ring_buf_t *ring);
uint32_t ring_buf_get_overflow(ring_buf_t *ring);

#endif /* RING_BUF_H */
```

## SPI Bare-Metal Transfer Modes - Fresh Working-Tree Addendum (2026-10-13)

This section supersedes the 2026-10-12 RTOS synchronization addendum for the current
dirty checkout. S039/S040/S045/S052/S053/S060/S061 remain baseline references, not
hashes for these uncommitted declarations. Selected current declarations follow.

```c
typedef enum
{
  DEV_SPI_TRANSFER_POLLING = 0,
  DEV_SPI_TRANSFER_INTERRUPT,
  DEV_SPI_TRANSFER_DMA,
} spi_transfer_mode_t;

typedef struct spi_device_config
{
  uint32_t max_speed_hz;
  spi_mode_t mode;
  spi_bit_order_t bit_order;
  spi_data_width_t data_width;
  spi_transfer_mode_t tx_mode;
  spi_transfer_mode_t rx_mode;
} spi_device_config_t;

typedef enum
{
  SPI_INDEX_0 = 0,
  SPI_INDEX_1 = 1,
  SPI_INDEX_2 = 2,
  SPI_INDEX_MAX = 3,
} spi_index_t;

typedef struct
{
  GPIO_TypeDef *port;
  uint16_t pin;
  uint32_t alternate;
  usr_port_hal_spi_clock_gate_fn_t clock_enable;
} usr_port_hal_spi_pin_t;

typedef bool (*usr_port_hal_spi_dma_buffer_valid_fn_t)(const void *buffer,
                                                       uint16_t length,
                                                       bool receive);

typedef enum
{
  USR_PORT_SPI_IDLE = 0,
  USR_PORT_SPI_ACTIVE,
  USR_PORT_SPI_COMPLETED,
  USR_PORT_SPI_ERROR,
  USR_PORT_SPI_ABORTING,
  USR_PORT_SPI_FAULTED,
} usr_port_hal_spi_transfer_state_t;

bool usr_port_hal_spi_complete(SPI_HandleTypeDef *hspi, usr_status_t result);
void port_hal_dma_controller_clock_enable_dma1(void);
void port_hal_dma_controller_clock_enable_dma2(void);
```

The resource retains `pins[3]` in SCK/MISO/MOSI order and independent
`rx_dma/tx_dma`, and replaces host sync ops with `dma_buffer_valid`. Only a
direction configured for DMA needs a valid resource. The instance retains a
bare-metal non-recursive claim, per-direction DMA readiness, transfer state,
result and counters; no FreeRTOS type or synchronization callback is present.

`dev_spi_init/deinit/transact/begin/frame/end` signatures remain unchanged.
`tx_len/rx_len` are bytes; 16-bit mode requires even lengths and aligned
buffers. Single-direction and non-equal two-stage transfers use each configured
mode. Equal-length DMA+DMA and IT+IT use their full-duplex HAL APIs; all mixed
combinations use blocking full duplex. Rebuild custom initializers because the
config and resource layouts are not binary compatible.

See [current architecture](current-architecture.md),
[host integration](host-integration.md) and
[SPI DMA evidence](../completions/2026-10-12-spi-dma.md) for implementation
semantics, H743 cache/buffer limits and verification scope.
