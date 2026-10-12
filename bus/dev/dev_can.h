#ifndef __DEV_CAN_H__
#define __DEV_CAN_H__

#include "dri_ops.h"

typedef enum
{
  CAN_INDEX_0 = 0,
  CAN_INDEX_MAX = 1,
} can_index_t;

typedef enum
{
  DEV_CAN_FORMAT_CLASSIC = 0,
  DEV_CAN_FORMAT_FD,
} dev_can_format_t;

typedef enum
{
  DEV_CAN_ID_STANDARD = 0,
  DEV_CAN_ID_EXTENDED,
} dev_can_id_type_t;

typedef enum
{
  DEV_CAN_FRAME_DATA = 0,
  DEV_CAN_FRAME_REMOTE,
} dev_can_frame_type_t;

typedef enum
{
  DEV_CAN_DIRECTION_TX = 0,
  DEV_CAN_DIRECTION_RX,
  DEV_CAN_DIRECTION_TX_RX,
} dev_can_direction_t;

typedef enum
{
  DEV_CAN_RX_POLLING = 0,
  DEV_CAN_RX_INTERRUPT,
} dev_can_rx_mode_t;

typedef struct
{
  uint16_t prescaler;
  uint16_t sync_jump_width;
  uint16_t time_seg1;
  uint16_t time_seg2;
} dev_can_bit_timing_t;

typedef struct
{
  bool enabled;
  dev_can_id_type_t id_type;
  uint32_t identifier;
  uint32_t mask;
  bool accept_remote;
} dev_can_filter_t;

typedef struct
{
  dev_can_format_t format;
  dev_can_direction_t direction;
  dev_can_rx_mode_t rx_mode;
  bool auto_retransmission;
  dev_can_bit_timing_t nominal;
  dev_can_bit_timing_t data;
  dev_can_filter_t filter;
} dev_can_config_t;

typedef struct
{
  uint32_t id;
  dev_can_id_type_t id_type;
  dev_can_frame_type_t frame_type;
  dev_can_format_t format;
  bool bit_rate_switch;
  uint8_t data_len;
  uint8_t data[64];
} dev_can_frame_t;

typedef struct
{
  bool initialized;
  bool started;
  bool error_warning;
  bool error_passive;
  bool bus_off;
  uint32_t tx_frames;
  uint32_t rx_frames;
  uint32_t error_count;
  uint32_t rx_fifo_lost;
  uint32_t rx_queue_overflow;
} dev_can_status_t;

typedef struct can_device
{
  device parent;
  void *ctx;
  dev_can_config_t config;
  bool initialized;
  const dev_can_ops_t *ops;
} can_device_t;

can_device_t *get_can_device(uint8_t index);
usr_status_t dev_can_init(can_device_t *dev,
                          const dev_can_config_t *config);
usr_status_t dev_can_deinit(can_device_t *dev);
usr_status_t dev_can_send(can_device_t *dev,
                          const dev_can_frame_t *frame,
                          uint32_t timeout_ms);
usr_status_t dev_can_recv(can_device_t *dev, dev_can_frame_t *frame);
usr_status_t dev_can_get_status(can_device_t *dev, dev_can_status_t *status);

usr_status_t dev_can_echo_test(can_device_t *dev);
#endif /* __DEV_CAN_H__ */
