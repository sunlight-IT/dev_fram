#ifndef __DEV_TIMER_H__
#define __DEV_TIMER_H__

#include "dri_ops.h"

typedef enum
{
  DEV_TIMER_MODE_BASIC = 0,
  DEV_TIMER_MODE_CAPTURE,
} dev_timer_mode_t;

typedef enum
{
  DEV_TIMER_CAPTURE_EDGE_RISING = 0,
  DEV_TIMER_CAPTURE_EDGE_FALLING,
  DEV_TIMER_CAPTURE_EDGE_BOTH,
} dev_timer_capture_edge_t;

typedef struct dev_timer_capture_config
{
  dev_timer_capture_edge_t edge;
  /* Platform-independent digital filter level: 0 (off) through 15. */
  uint8_t filter_level;
  uint32_t timeout_us;
} dev_timer_capture_config_t;

typedef struct dev_timer_config
{
  dev_timer_mode_t mode;
  uint32_t period_us;
  dev_timer_capture_config_t capture;
} dev_timer_config_t;

typedef struct dev_timer_capture_result
{
  /* Microseconds modulo UINT32_MAX + 1; unsigned subtraction handles wrap. */
  uint32_t timestamp_us;
  uint32_t period_us;
  dev_timer_capture_edge_t edge;
} dev_timer_capture_result_t;

typedef struct dev_timer_status
{
  bool initialized;
  bool started;
  bool capture_pending;
  uint32_t captured_count;
  uint32_t dropped_count;
  uint32_t error_count;
} dev_timer_status_t;

typedef struct timer_device
{
  device parent;
  void *ctx;
  dev_timer_config_t config;
  const dev_timer_ops_t *ops;
} timer_device_t;

usr_status_t dev_timer_init(timer_device_t *dev,
                            const dev_timer_config_t *cfg);
usr_status_t dev_timer_deinit(timer_device_t *dev);
usr_status_t dev_timer_start(timer_device_t *dev);
usr_status_t dev_timer_stop(timer_device_t *dev);
uint32_t dev_timer_get_ticks(timer_device_t *dev);
/*
 * A successful capture read consumes the oldest completed result.
 * Basic mode returns USR_ERR_UNSUPPORTED. An uninitialized or stopped backend
 * returns USR_ERR_NOT_INIT or USR_ERR_STATE respectively. USR_BUSY means no
 * complete sample is available yet; USR_ERR_TIMEOUT means no complete sample
 * arrived within config.capture.timeout_us. The first edge only establishes
 * the period baseline and does not produce a result. timestamp_us and
 * period_us use modulo-2^32 microseconds, so intervals must be below 2^32 us.
 * Any failure leaves the caller's result unchanged.
 */
usr_status_t dev_timer_capture_read(timer_device_t *dev,
                                    dev_timer_capture_result_t *result);
usr_status_t dev_timer_get_status(timer_device_t *dev,
                                  dev_timer_status_t *status);

#endif
