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
#include "board_support.h"





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
