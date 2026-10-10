#ifndef __BOARD_SUPPORT_H__
#define __BOARD_SUPPORT_H__


#include "dev_gpio.h"
#include "dev_uart.h"
#include "dev_i2c.h"
#include "dev_spi.h"
#include "dev_timer.h"
#include "dev_pwm.h"

#include "stm32h7xx_hal.h"



typedef enum
{
  I2C_INDEX_0 = 0,
  I2C_INDEX_MAX = 1,
} i2c_index_t;

gpio_device_t *get_gpio_device(void);
uart_device_t *get_uart_device(uint8_t index);
time_dev_t *get_time_device(void);
spi_device_t *get_spi_device(uint8_t index);
i2c_device_t *get_i2c_device(void);
timer_device_t *get_timer_device(void);
pwm_device_t *get_pwm_device(uint8_t index);

timer_device_t *get_tim_device(uint8_t index);

#endif
