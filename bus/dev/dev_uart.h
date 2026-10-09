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
