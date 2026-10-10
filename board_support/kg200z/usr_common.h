/**
  ******************************************************************************
  * @file    usr_common.h
  * @author  user
  * @brief   Common types, status codes and cross-layer interfaces for the
  *          Usr driver framework (device / driver / port 3-layer architecture).
  *
  *          Layering rule (least-knowledge):
  *            - device layer   includes only drivers/dev + usr_common
  *            - drivers layer  includes only drivers/bus + usr_common
  *            - port layer     includes drivers/bus + usr_common (HAL allowed here only)
  *            - app layer      composition root, may include everything
  ******************************************************************************
  */
#ifndef __USR_COMMON_H__
#define __USR_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "my_log.h"

/* Status codes: >= 0 success, < 0 error (project convention) */
typedef int32_t usr_status_t;

#define USR_OK                ((usr_status_t) 0)
#define USR_BUSY              ((usr_status_t) 1)
#define USR_ERR_PARAM         ((usr_status_t) -1)
#define USR_ERR_TIMEOUT       ((usr_status_t) -2)
#define USR_ERR_BUS           ((usr_status_t) -3)
#define USR_ERR_NO_DEV        ((usr_status_t) -4)
#define USR_ERR_DATA          ((usr_status_t) -5)
#define USR_ERR_NOT_INIT      ((usr_status_t) -6)
#define USR_ERR_STATE         ((usr_status_t) -7)
#define USR_ERR_UNSUPPORTED   ((usr_status_t) -8)
#define USR_ERR_NOMEM         ((usr_status_t) -9)

/* Logical pin id, resolved to a physical GPIO by the port layer. */
typedef enum
{
  USR_PIN_GNSS_PW_EN = 0,
  USR_PIN_GNSS_RST,
  USR_PIN_GNSS_STANDBY,
  USR_PIN_ICP_INT,
  USR_PIN_PRIMARY_SPI_CS,
  USR_PIN_SECONDARY_SPI_CS,
  USR_PIN_WDOG,
  USR_PIN_COUNT
} usr_pin_id_t;

typedef usr_pin_id_t usr_gpio_pin_id_t;

/* Generic callback (observer pattern, e.g. EXTI notification, ISR context) */
typedef void (*usr_callback_t)(void *arg);

/* Platform time service. The context keeps the interface reusable and testable. */
typedef struct
{
  uint32_t (*get_ms)(void);
  void     (*delay_ms)(uint32_t ms);
} usr_time_ops_t;

typedef struct
{
  const usr_time_ops_t *ops;
  void *ctx;
} usr_time_t;

typedef struct time_dev_t{
  const usr_time_ops_t *ops;
} time_dev_t;





static inline uint32_t usr_time_get_ms(const time_dev_t *time)
{
  if ((time == NULL) || (time->ops == NULL) || (time->ops->get_ms == NULL))
  {
    return 0u;
  }
  return time->ops->get_ms();
}

static inline void usr_time_delay_ms(const time_dev_t *time, uint32_t ms)
{
  if ((time != NULL) && (time->ops != NULL) && (time->ops->delay_ms != NULL))
  {
    time->ops->delay_ms(ms);
  }
}

#ifdef __cplusplus
}
#endif

#endif /* __USR_COMMON_H__ */
