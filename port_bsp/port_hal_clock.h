#ifndef __PORT_HAL_CLOCK_H__
#define __PORT_HAL_CLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "board_support.h"

#define PORT_HAL_JOIN_IMPL(left, right) left##right
#define PORT_HAL_JOIN(left, right) PORT_HAL_JOIN_IMPL(left, right)

#define PORT_HAL_DEFINE_CLOCK_FUNCTIONS(                                      \
    prefix, peripheral_clock, kernel_field, kernel_source, peripheral_prefix,   \
    peripheral_number, gpio_prefix, gpio_suffix)                                \
  static HAL_StatusTypeDef PORT_HAL_JOIN(prefix, _clock_config)(void)         \
  {                                                                            \
    RCC_PeriphCLKInitTypeDef config = {0};                                     \
    config.PeriphClockSelection = peripheral_clock;                            \
    config.kernel_field = kernel_source;                                       \
    return HAL_RCCEx_PeriphCLKConfig(&config);                                 \
  }                                                                            \
  static void PORT_HAL_JOIN(prefix, _clock_enable)(void)                      \
  {                                                                            \
    __HAL_RCC_##peripheral_prefix##peripheral_number##_CLK_ENABLE();           \
  }                                                                            \
  static void PORT_HAL_JOIN(prefix, _clock_disable)(void)                     \
  {                                                                            \
    __HAL_RCC_##peripheral_prefix##peripheral_number##_CLK_DISABLE();          \
  }                                                                            \
  static void PORT_HAL_JOIN(prefix, _gpio_clock_enable)(void)                 \
  {                                                                            \
    __HAL_RCC_##gpio_prefix##gpio_suffix##_CLK_ENABLE();                       \
  }

#define PORT_HAL_DEFINE_CLOCK_FUNCTIONS_NO_KERNEL(                             \
    prefix, peripheral_prefix, peripheral_number, gpio_prefix, gpio_suffix)     \
  static HAL_StatusTypeDef PORT_HAL_JOIN(prefix, _clock_config)(void)         \
  {                                                                            \
    return HAL_OK;                                                             \
  }                                                                            \
  static void PORT_HAL_JOIN(prefix, _clock_enable)(void)                      \
  {                                                                            \
    __HAL_RCC_##peripheral_prefix##peripheral_number##_CLK_ENABLE();           \
  }                                                                            \
  static void PORT_HAL_JOIN(prefix, _clock_disable)(void)                     \
  {                                                                            \
    __HAL_RCC_##peripheral_prefix##peripheral_number##_CLK_DISABLE();          \
  }                                                                            \
  static void PORT_HAL_JOIN(prefix, _gpio_clock_enable)(void)                 \
  {                                                                            \
    __HAL_RCC_##gpio_prefix##gpio_suffix##_CLK_ENABLE();                       \
  }

#define PORT_HAL_DEFINE_GPIO_CLOCK_ENABLE_FUNCTION(prefix, gpio_prefix,        \
                                                   gpio_suffix)                \
  static void PORT_HAL_JOIN(prefix, _clock_enable)(void) {                     \
    __HAL_RCC_##gpio_prefix##gpio_suffix##_CLK_ENABLE();                       \
  }

#define PORT_HAL_DEFINE_PERIPHERAL_CLOCK_ENABLE_FUNCTION(                      \
    prefix, peripheral_prefix, peripheral_number)                              \
  static void PORT_HAL_JOIN(prefix, _clock_enable)(void) {                     \
    __HAL_RCC_##peripheral_prefix##peripheral_number##_CLK_ENABLE();           \
  }

#define PORT_HAL_DEFINE_PERIPHERAL_CLOCK_DISABLE_FUNCTION(\
    prefix, peripheral_prefix, peripheral_number)    \
static void PORT_HAL_JOIN(prefix, _clock_disable)(void)                      \
  {                                                                            \
    __HAL_RCC_##peripheral_prefix##peripheral_number##_CLK_DISABLE();          \
  }      
#ifdef __cplusplus
}
#endif

#endif /* __PORT_HAL_CLOCK_H__ */
