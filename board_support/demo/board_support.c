/**
  ******************************************************************************
  * @file    board_support.c
  * @brief   Demo board support — weak / no-op implementations.
  *          Returns NULL for every device so the framework links without
  *          a concrete board.  Replace this file in your main project with
  *          a real board_support.c (e.g. kg200z/ or H743/).
  ******************************************************************************
  */
#include "usr_port.h"
#include "board_support.h"

gpio_device_t *get_gpio_device(void)
{
  return (gpio_device_t *)0;
}

time_dev_t *get_time_device(void)
{
  return (time_dev_t *)0;
}

i2c_device_t *get_i2c_device(void)
{
  return (i2c_device_t *)0;
}

timer_device_t *get_timer_device(void)
{
  return (timer_device_t *)0;
}

pwm_device_t *get_pwm_device(uint8_t index)
{
  (void)index;
  return (pwm_device_t *)0;
}

usr_status_t usr_port_board_init(void)
{
  return USR_OK;
}

void update_instance_rx_mode(uint32_t index, uint32_t rx_mode)
{
  (void)index;
  (void)rx_mode;
}