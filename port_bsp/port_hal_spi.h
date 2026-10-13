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
typedef bool (*usr_port_hal_spi_dma_buffer_valid_fn_t)(const void *buffer,
                                                       uint16_t length,
                                                       bool receive);

typedef struct
{
  GPIO_TypeDef *port;
  uint16_t pin;
  uint32_t alternate;
  usr_port_hal_spi_clock_gate_fn_t clock_enable;
} usr_port_hal_spi_pin_t;

typedef enum
{
  USR_PORT_SPI_IDLE = 0,
  USR_PORT_SPI_ACTIVE,
  USR_PORT_SPI_COMPLETED,
  USR_PORT_SPI_ERROR,
  USR_PORT_SPI_ABORTING,
  USR_PORT_SPI_FAULTED,
} usr_port_hal_spi_transfer_state_t;

typedef struct
{
  SPI_TypeDef *instance;
  usr_port_hal_spi_pin_t pins[3]; /* SCK, MISO, MOSI. */
  port_hal_dma_resource_t rx_dma;
  port_hal_dma_resource_t tx_dma;
  IRQn_Type irq;
  uint32_t irq_priority;
  bool enable_irq;
  usr_port_hal_spi_clock_config_fn_t clock_config;
  usr_port_hal_spi_clock_gate_fn_t clock_enable;
  usr_port_hal_spi_clock_gate_fn_t clock_disable;
  usr_port_hal_spi_kernel_clock_fn_t kernel_clock_hz;
  usr_port_hal_spi_dma_buffer_valid_fn_t dma_buffer_valid;
  spi_device_config_t default_config;
} usr_port_hal_spi_resource_t;

typedef struct
{
  const usr_port_hal_spi_resource_t *resource;
  SPI_HandleTypeDef* handle;
  spi_device_t* device;
  spi_device_config_t runtime_config;

  uint8_t initialized;
  uint8_t faulted;
  uint8_t gpio_ready;
  uint8_t rx_dma_ready;
  uint8_t tx_dma_ready;

  port_hal_dma_resource_t rx_dma;
  port_hal_dma_resource_t tx_dma;

  volatile usr_port_hal_spi_transfer_state_t transfer_state;
  volatile usr_status_t transfer_result;
  volatile uint32_t recovery_started_ms;
  volatile uint8_t recovery_started;
  volatile uint32_t completed_count;
  volatile uint32_t timeout_count;
  volatile uint32_t error_count;
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
bool usr_port_hal_spi_complete(SPI_HandleTypeDef *hspi, usr_status_t result);

#ifdef __cplusplus
}
#endif

#endif /* __PORT_HAL_SPI_H__ */
