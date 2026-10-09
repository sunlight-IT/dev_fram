#ifndef __PORT_HAL_I2C_H__
#define __PORT_HAL_I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32wlxx_hal.h"
#include "dev_i2c.h"
#include "port_hal_dma.h"

typedef HAL_StatusTypeDef (*usr_port_hal_i2c_clock_config_fn_t)(void);
typedef void (*usr_port_hal_i2c_clock_gate_fn_t)(void);

typedef struct
{
  I2C_TypeDef *instance;
  GPIO_TypeDef *gpio_port;
  uint16_t gpio_pins;
  uint32_t gpio_alternate;
  uint32_t timing;
  port_hal_dma_resource_t rx_dma;
  usr_port_hal_i2c_clock_config_fn_t clock_config;
  usr_port_hal_i2c_clock_gate_fn_t clock_enable;
  usr_port_hal_i2c_clock_gate_fn_t clock_disable;
  usr_port_hal_i2c_clock_gate_fn_t gpio_clock_enable;
} usr_port_hal_i2c_resource_t;

typedef struct
{
  const usr_port_hal_i2c_resource_t *resource;
  I2C_HandleTypeDef handle;
  i2c_device_t device;
  uint8_t initialized;
} usr_port_hal_i2c_instance_t;

extern const dev_i2c_ops_t port_hal_i2c_ops;

usr_port_hal_i2c_instance_t *usr_port_board_i2c_find_instance(
    const I2C_HandleTypeDef *handle);
usr_port_hal_i2c_instance_t *usr_port_board_i2c_find_dma_instance(
    IRQn_Type irq);
bool usr_port_hal_i2c_msp_init(I2C_HandleTypeDef *handle);
bool usr_port_hal_i2c_msp_deinit(I2C_HandleTypeDef *handle);
bool usr_port_hal_i2c_dma_irq(IRQn_Type irq);

#ifdef __cplusplus
}
#endif

#endif /* __PORT_HAL_I2C_H__ */
