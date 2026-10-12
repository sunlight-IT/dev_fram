/**
  ******************************************************************************
  * @file    dev_can.c
  * @brief   Platform-independent CAN and CAN FD frame interface.
  ******************************************************************************
  */
#include "dev_can.h"

static can_device_t s_devices[CAN_INDEX_MAX];

can_device_t *get_can_device(uint8_t index)
{
  if (index >= CAN_INDEX_MAX)
  {
    return NULL;
  }
  return &s_devices[index];
}

static bool dev_can_id_valid(dev_can_id_type_t id_type, uint32_t id)
{
  if (id_type == DEV_CAN_ID_STANDARD)
  {
    return id <= 0x7ffu;
  }
  if (id_type == DEV_CAN_ID_EXTENDED)
  {
    return id <= 0x1fffffffu;
  }
  return false;
}

static bool dev_can_fd_length_valid(uint8_t length)
{
  return (length <= 8u) || (length == 12u) || (length == 16u) ||
         (length == 20u) || (length == 24u) || (length == 32u) ||
         (length == 48u) || (length == 64u);
}


static bool dev_can_data_timing_valid(const dev_can_bit_timing_t *timing)
{
  return (timing->prescaler >= 1u) && (timing->prescaler <= 32u) &&
         (timing->sync_jump_width >= 1u) &&
         (timing->sync_jump_width <= 16u) &&
         (timing->time_seg1 >= 1u) && (timing->time_seg1 <= 32u) &&
         (timing->time_seg2 >= 1u) && (timing->time_seg2 <= 16u) &&
         (timing->sync_jump_width <= timing->time_seg2);
}

static bool dev_can_config_valid(const dev_can_config_t *config)
{
  if ((config == NULL) ||
      ((config->format != DEV_CAN_FORMAT_CLASSIC) &&
       (config->format != DEV_CAN_FORMAT_FD)) ||
      ((config->direction != DEV_CAN_DIRECTION_TX) &&
       (config->direction != DEV_CAN_DIRECTION_RX) &&
       (config->direction != DEV_CAN_DIRECTION_TX_RX)) ||
      ((config->rx_mode != DEV_CAN_RX_POLLING) &&
       (config->rx_mode != DEV_CAN_RX_INTERRUPT)))
  {
    return false;
  }
  if ((config->format == DEV_CAN_FORMAT_FD) &&
      !dev_can_data_timing_valid(&config->data))
  {
    return false;
  }
  if (config->filter.enabled &&
      (!dev_can_id_valid(config->filter.id_type,
                         config->filter.identifier) ||
       !dev_can_id_valid(config->filter.id_type, config->filter.mask)))
  {
    return false;
  }
  return true;
}

static bool dev_can_direction_has_tx(dev_can_direction_t direction)
{
  return (direction == DEV_CAN_DIRECTION_TX) ||
         (direction == DEV_CAN_DIRECTION_TX_RX);
}

static bool dev_can_direction_has_rx(dev_can_direction_t direction)
{
  return (direction == DEV_CAN_DIRECTION_RX) ||
         (direction == DEV_CAN_DIRECTION_TX_RX);
}

static bool dev_can_frame_valid(const can_device_t *dev,
                                const dev_can_frame_t *frame)
{
  if ((frame == NULL) || !dev_can_id_valid(frame->id_type, frame->id) ||
      ((frame->frame_type != DEV_CAN_FRAME_DATA) &&
       (frame->frame_type != DEV_CAN_FRAME_REMOTE)) ||
      ((frame->format != DEV_CAN_FORMAT_CLASSIC) &&
       (frame->format != DEV_CAN_FORMAT_FD)))
  {
    return false;
  }
  if ((frame->format == DEV_CAN_FORMAT_CLASSIC) &&
      ((frame->data_len > 8u) || frame->bit_rate_switch))
  {
    return false;
  }
  if ((frame->format == DEV_CAN_FORMAT_FD) &&
      (!dev_can_fd_length_valid(frame->data_len) ||
       (frame->frame_type == DEV_CAN_FRAME_REMOTE) ||
       (dev->config.format != DEV_CAN_FORMAT_FD)))
  {
    return false;
  }
  return true;
}

usr_status_t dev_can_init(can_device_t *dev,
                          const dev_can_config_t *config)
{
  const dev_can_config_t *candidate;
  usr_status_t status;

  if ((dev == NULL) || (dev->ops == NULL) || (dev->ops->init == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (dev->initialized)
  {
    return USR_ERR_STATE;
  }
  candidate = (config == NULL) ? &dev->config : config;
  if (!dev_can_config_valid(candidate))
  {
    return USR_ERR_PARAM;
  }
  status = dev->ops->init(dev->ctx, candidate);
  if (status == USR_OK)
  {
    dev->config = *candidate;
    dev->initialized = true;
  }
  return status;
}

usr_status_t dev_can_deinit(can_device_t *dev)
{
  usr_status_t status;

  if ((dev == NULL) || (dev->ops == NULL) || (dev->ops->deinit == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (!dev->initialized)
  {
    return USR_OK;
  }
  status = dev->ops->deinit(dev->ctx);
  if (status == USR_OK)
  {
    dev->initialized = false;
  }
  return status;
}

usr_status_t dev_can_send(can_device_t *dev,
                          const dev_can_frame_t *frame,
                          uint32_t timeout_ms)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->send == NULL) || (frame == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (!dev->initialized)
  {
    return USR_ERR_NOT_INIT;
  }
  if (!dev_can_direction_has_tx(dev->config.direction))
  {
    return USR_ERR_UNSUPPORTED;
  }
  if (!dev_can_frame_valid(dev, frame))
  {
    return USR_ERR_PARAM;
  }
  return dev->ops->send(dev->ctx, frame, timeout_ms);
}

usr_status_t dev_can_recv(can_device_t *dev, dev_can_frame_t *frame)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->recv == NULL) || (frame == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (!dev->initialized)
  {
    return USR_ERR_NOT_INIT;
  }
  if (!dev_can_direction_has_rx(dev->config.direction))
  {
    return USR_ERR_UNSUPPORTED;
  }
  return dev->ops->recv(dev->ctx, frame);
}

usr_status_t dev_can_get_status(can_device_t *dev,
                                dev_can_status_t *status)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->get_status == NULL) || (status == NULL))
  {
    return USR_ERR_PARAM;
  }
  return dev->ops->get_status(dev->ctx, status);
}


usr_status_t dev_can_echo_test(can_device_t *dev) {
  if ((dev == NULL)) {
    return USR_ERR_PARAM;
  }
  dev_can_frame_t frame;
  if (dev_can_recv(dev, &frame) == USR_OK)
  {
    dev_can_send(dev, &frame, 0u);
  }
  return USR_OK;
}
