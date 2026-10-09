#ifndef __PORT_HAL_SPI_H__
#define __PORT_HAL_SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_spi.h"
#include "port_hal_dma.h"

typedef HAL_StatusTypeDef (*usr_port_hal_spi_clock_config_fn_t)(void);
typedef void (*usr_port_hal_spi_clock_gate_fn_t)(void);
typedef uint32_t (*usr_port_hal_spi_kernel_clock_fn_t)(void);


typedef struct
{
  SPI_TypeDef *instance;
  GPIO_TypeDef *gpio_port;
  uint16_t gpio_pins;
  uint32_t gpio_alternate;
  port_hal_dma_resource_t rx_dma;
  IRQn_Type irq;
  uint32_t irq_priority;
  bool enable_irq;
  usr_port_hal_spi_clock_config_fn_t clock_config;
  usr_port_hal_spi_clock_gate_fn_t clock_enable;
  usr_port_hal_spi_clock_gate_fn_t clock_disable;
  usr_port_hal_spi_clock_gate_fn_t gpio_clock_enable;
  usr_port_hal_spi_kernel_clock_fn_t kernel_clock_hz;
  spi_device_config_t default_config;
} usr_port_hal_spi_resource_t;

typedef struct
{
  const usr_port_hal_spi_resource_t *resource;
  SPI_HandleTypeDef* handle;
  spi_device_t* device;
  spi_device_config_t runtime_config;
  uint8_t initialized;
} usr_port_hal_spi_instance_t;

extern const dev_spi_ops_t port_hal_spi_ops;


SPI_HandleTypeDef* get_spi_table_handle(uint32_t index);
usr_port_hal_spi_instance_t *usr_port_board_spi_find_instance(
    const SPI_HandleTypeDef *handle);
usr_port_hal_spi_instance_t *usr_port_board_spi_find_dma_instance(
    IRQn_Type irq);
bool usr_port_hal_spi_msp_init(SPI_HandleTypeDef *hspi);
bool usr_port_hal_spi_msp_deinit(SPI_HandleTypeDef *hspi);
bool usr_port_hal_spi_dma_irq(IRQn_Type irq);
void usr_port_hal_spi_irq(SPI_HandleTypeDef *hspi);

#ifdef __cplusplus
}
#endif

#endif /* __PORT_HAL_SPI_H__ */
