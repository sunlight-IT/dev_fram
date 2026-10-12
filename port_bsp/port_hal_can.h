#ifndef __PORT_HAL_CAN_H__
#define __PORT_HAL_CAN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "dev_can.h"
#include "board_support.h"

typedef HAL_StatusTypeDef (*usr_port_hal_can_clock_config_fn_t)(void);
typedef void (*usr_port_hal_can_clock_gate_fn_t)(void);

typedef enum
{
  USR_PORT_CAN_LIFECYCLE_RESET = 0,
  USR_PORT_CAN_LIFECYCLE_READY,
  USR_PORT_CAN_LIFECYCLE_STARTED,
  USR_PORT_CAN_LIFECYCLE_FAULT,
} usr_port_can_lifecycle_t;

typedef struct
{
  FDCAN_GlobalTypeDef *instance;
  GPIO_TypeDef *rx_gpio_port;
  uint16_t rx_gpio_pin;
  GPIO_TypeDef *tx_gpio_port;
  uint16_t tx_gpio_pin;
  uint32_t gpio_alternate;
  IRQn_Type irq;
  uint32_t irq_priority;
  usr_port_hal_can_clock_config_fn_t clock_config;
  usr_port_hal_can_clock_gate_fn_t clock_enable;
  usr_port_hal_can_clock_gate_fn_t clock_disable;
  usr_port_hal_can_clock_gate_fn_t rx_gpio_clock_enable;
  usr_port_hal_can_clock_gate_fn_t tx_gpio_clock_enable;
  uint32_t mode;
  bool transmit_pause;
  bool protocol_exception;
  uint32_t message_ram_offset;
  uint32_t message_ram_word_capacity;
  uint32_t std_filter_count;
  uint32_t ext_filter_count;
  uint32_t rx_fifo0_element_count;
  uint32_t rx_fifo0_element_size;
  uint32_t rx_fifo1_element_count;
  uint32_t rx_fifo1_element_size;
  uint32_t rx_buffer_count;
  uint32_t rx_buffer_size;
  uint32_t tx_event_count;
  uint32_t tx_buffer_count;
  uint32_t tx_fifo_queue_count;
  uint32_t tx_fifo_queue_mode;
  uint32_t tx_element_size;
  uint32_t filter_index;
  uint32_t filter_config;
  uint32_t rx_location;
  uint32_t nonmatching_std;
  uint32_t nonmatching_ext;
  dev_can_frame_t *rx_queue_storage;
  uint16_t rx_queue_capacity;
  dev_can_config_t default_config;
} usr_port_hal_can_resource_t;

typedef struct
{
  const usr_port_hal_can_resource_t *resource;
  FDCAN_HandleTypeDef *handle;
  can_device_t *device;
  dev_can_config_t runtime_config;
  usr_port_can_lifecycle_t lifecycle;
  volatile uint16_t rx_read_pos;
  volatile uint16_t rx_write_pos;
  volatile uint32_t tx_frames;
  volatile uint32_t rx_frames;
  volatile uint32_t error_count;
  volatile uint32_t rx_fifo_lost;
  volatile uint32_t rx_queue_overflow;
} usr_port_hal_can_instance_t;

extern const dev_can_ops_t port_hal_can_ops;

usr_port_hal_can_instance_t *usr_port_board_can_find_instance(
    const FDCAN_HandleTypeDef *handle);
bool usr_port_hal_can_msp_init(FDCAN_HandleTypeDef *handle);
bool usr_port_hal_can_msp_deinit(FDCAN_HandleTypeDef *handle);
void port_hal_can_irq(FDCAN_HandleTypeDef *canHandle);
FDCAN_HandleTypeDef *get_can_table_handle(uint8_t index);

#ifdef __cplusplus
}
#endif

#endif /* __PORT_HAL_CAN_H__ */
