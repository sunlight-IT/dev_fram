/**
  ******************************************************************************
  * @file    usr_port_platform.c
  * @brief   STM32/CubeMX peripheral initialization boundary.
  ******************************************************************************
  */
#include "usr_port.h"
#include "gpio.h"
#include "usart.h"
#include "app_lorawan.h"
#include "stm32wlxx_hal.h"

usr_status_t usr_port_platform_init(void)
{
  /* Core retains legacy GPIO safe states. */
  Core_GPIO_Init();
  // MX_USART1_UART_Init();
  
  return USR_OK;
}

usr_status_t usr_port_get_system_info(usr_port_system_info_t *info)
{
  if (info == NULL)
  {
    return USR_ERR_PARAM;
  }

  info->hal_version = HAL_GetHalVersion();
  info->sys_clock_hz = HAL_RCC_GetSysClockFreq();
  return USR_OK;
}

void usr_port_system_reset(uint32_t delay_ms)
{
  if (delay_ms != 0u)
  {
    HAL_Delay(delay_ms);
  }
  NVIC_SystemReset();
}
