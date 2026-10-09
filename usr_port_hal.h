/**
  ******************************************************************************
  * @file    usr_port_hal.h
  * @brief   Reusable STM32 HAL adapter contexts and shared operation tables.
  ******************************************************************************
  */
#ifndef __USR_PORT_HAL_H__
#define __USR_PORT_HAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "usr_common.h"
#include "stm32wlxx_hal.h"


typedef struct
{
  void *port;
  uint16_t pin;
  void (*clock_enable)(void);
  uint32_t mode;
  uint32_t pull;
  uint32_t speed;
  GPIO_PinState initial_level;
  bool write_initial;
  bool has_irq;
  IRQn_Type irq;
  uint32_t irq_priority;
  uint32_t irq_subpriority;
} usr_port_hal_gpio_pin_t;

typedef struct
{
  const usr_port_hal_gpio_pin_t *pins;
  usr_callback_t *irq_callbacks;
  void **irq_args;
  uint16_t pin_count;
} usr_port_hal_gpio_t;


void usr_port_hal_gpio_irq_dispatch(usr_port_hal_gpio_t *gpio,
                                    uint16_t physical_pin);
void usr_port_hal_gpio_irq(IRQn_Type irq);
bool usr_port_hal_uart_msp_init(UART_HandleTypeDef *uartHandle);
bool usr_port_hal_uart_msp_deinit(UART_HandleTypeDef *uartHandle);
bool usr_port_hal_uart_dma_irq(IRQn_Type irq);
bool usr_port_hal_uart_rx_half_cplt(UART_HandleTypeDef *uartHandle);
bool usr_port_hal_uart_rx_cplt(UART_HandleTypeDef *uartHandle);
bool usr_port_hal_uart_error(UART_HandleTypeDef *uartHandle);
void port_hal_uart_irq(UART_HandleTypeDef *uartHandle);

#ifdef __cplusplus
}
#endif

#endif /* __USR_PORT_HAL_H__ */
