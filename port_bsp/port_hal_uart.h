#ifndef __PORT_HAL_UART_H__
#define __PORT_HAL_UART_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_uart.h"
#include "port_hal_dma.h"
#include "ring_buf.h"
#include "usr_port_hal.h"

#define UART1_CONFIG_DEFAULT                                                \
  {.baudrate = 9600u,                                                       \
   .data_bits = DEV_UART_DATA_BITS_8,                                       \
   .parity = DEV_UART_PARITY_NONE,                                         \
   .stop_bits = DEV_UART_STOP_BITS_1,                                      \
   .direction = DEV_UART_DIRECTION_TX_RX,                                      \
   .rx_mode = DEV_UART_RX_INTERRUPT,                                    \
   .irq_enable = true,                                                        \                  
}

#define UART2_CONFIG_DEFAULT                                                   \
  {                                                                            \
    .baudrate = 115200u,                                                       \
    .data_bits = DEV_UART_DATA_BITS_8,                    \
    .parity = DEV_UART_PARITY_NONE, \
    .stop_bits = DEV_UART_STOP_BITS_1,                         \
    .direction = DEV_UART_DIRECTION_TX_RX,                                      \
    .rx_mode = DEV_UART_RX_INTERRUPT,                                    \
    .irq_enable = true,                                                        \                  
}

#define UART3_CONFIG_DEFAULT                                                \
  {.baudrate = 9600u,                                                       \
   .data_bits = DEV_UART_DATA_BITS_8,                                       \
   .parity = DEV_UART_PARITY_NONE,                                         \
   .stop_bits = DEV_UART_STOP_BITS_1,                                      \
   .direction = DEV_UART_DIRECTION_TX_RX,                                      \
   .rx_mode = DEV_UART_RX_INTERRUPT,                                    \
   .irq_enable = true,                                                        \                  
}

typedef enum
{
  USR_PORT_HAL_UART_RX_NONE = 0,
  USR_PORT_HAL_UART_RX_DMA_CIRCULAR,
  USR_PORT_HAL_UART_RX_INTERRUPT,
} usr_port_hal_uart_rx_mode_t;

typedef HAL_StatusTypeDef (*usr_port_hal_uart_clock_config_fn_t)(void);
typedef void (*usr_port_hal_uart_clock_gate_fn_t)(void);

typedef struct
{
  USART_TypeDef *instance;
  GPIO_TypeDef *gpio_port;
  uint16_t gpio_pins;
  uint32_t gpio_alternate;
  IRQn_Type irq;
  uint32_t irq_priority;
  port_hal_dma_resource_t rx_dma;
  usr_port_hal_uart_clock_config_fn_t clock_config;
  usr_port_hal_uart_clock_gate_fn_t clock_enable;
  usr_port_hal_uart_clock_gate_fn_t clock_disable;
  usr_port_hal_uart_clock_gate_fn_t gpio_clock_enable;
  uart_param_config_t default_config;
} usr_port_hal_uart_resource_t;

typedef struct
{
  const usr_port_hal_uart_resource_t *resource;
  UART_HandleTypeDef *handle;
  
  ring_buf_t ring;
  bool irq_enable;
  uart_rx_mode_t rx_mode;
  uint8_t it_byte;
  uint8_t enabled;
  uint8_t initialized;
  uint8_t faulted;
  uint8_t restart_pending;
  uart_param_config_t runtime_config;
  uint32_t error_count;
  uint32_t dma_missed_cycles;

  uart_device_t* device;
} usr_port_hal_uart_instance_t;

extern const dev_uart_ops_t port_hal_uart_ops;

UART_HandleTypeDef *get_uart_table_handle(uint32_t index);

usr_port_hal_uart_instance_t *usr_port_board_uart_find_instance(
    const UART_HandleTypeDef *handle);
usr_port_hal_uart_instance_t *
usr_port_board_uart_find_dma_instance(IRQn_Type irq);
bool usr_port_hal_uart_msp_init(UART_HandleTypeDef *uartHandle);
bool usr_port_hal_uart_msp_deinit(UART_HandleTypeDef *uartHandle);
bool usr_port_hal_uart_dma_irq(IRQn_Type irq);
void port_hal_uart_irq(UART_HandleTypeDef *uartHandle);
bool usr_port_hal_uart_rx_half_cplt(UART_HandleTypeDef *uartHandle);
bool usr_port_hal_uart_rx_cplt(UART_HandleTypeDef *uartHandle);
bool usr_port_hal_uart_error(UART_HandleTypeDef *uartHandle);

#ifdef __cplusplus
}
#endif

#endif /* __PORT_HAL_UART_H__ */
