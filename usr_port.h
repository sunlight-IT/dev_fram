/**
  ******************************************************************************
  * @file    usr_port.h
  * @brief   Board port bundle consumed by the application composition root.
  *
  *          Bus and device layers depend only on their injected backends.
  *          Replacing the MCU or board requires replacing this port layer.
  ******************************************************************************
  */
#ifndef __USR_PORT_H__
#define __USR_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "usr_common.h"

typedef struct
{
  uint32_t hal_version;
  uint32_t sys_clock_hz;
} usr_port_system_info_t;

/* Logical board roles shared by the application and the board binding. */
// typedef enum
// {
//   USR_PIN_GNSS_PW_EN = 0,
//   USR_PIN_GNSS_RST,
//   USR_PIN_GNSS_STANDBY,
//   USR_PIN_ICP_INT,
//   USR_PIN_PRIMARY_SPI_CS,
//   USR_PIN_SECONDARY_SPI_CS,
//   USR_PIN_WDOG,
//   USR_PIN_COUNT
// } usr_pin_id_t;


usr_status_t usr_port_platform_init(void);
usr_status_t usr_port_board_init(void);
void update_instance_rx_mode(uint32_t index, uint32_t rx_mode);
usr_status_t usr_port_get_system_info(usr_port_system_info_t *info);
void usr_port_system_reset(uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif /* __USR_PORT_H__ */
