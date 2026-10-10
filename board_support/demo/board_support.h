/**
  ******************************************************************************
  * @file    board_support.h
  * @brief   Demo board support header — chip-independent interface stubs.
  *          Actual projects provide a concrete implementation with real
  *          peripheral handles and HAL includes.
  ******************************************************************************
  */
#ifndef __BOARD_SUPPORT_H__
#define __BOARD_SUPPORT_H__

#include "dev_gpio.h"
#include "dev_uart.h"
#include "dev_i2c.h"
#include "dev_spi.h"
#include "dev_timer.h"
#include "dev_pwm.h"

typedef enum
{
  I2C_INDEX_0 = 0,
  I2C_INDEX_MAX = 1,
} i2c_index_t;

typedef enum
{
  TIMER_INDEX_SCHEDULER = 0,
  TIMER_INDEX_MAX = 1,
} timer_index_t;

gpio_device_t *get_gpio_device(void);
time_dev_t    *get_time_device(void);
i2c_device_t  *get_i2c_device(void);
timer_device_t *get_timer_device(void);
pwm_device_t  *get_pwm_device(uint8_t index);

#endif /* __BOARD_SUPPORT_H__ */