/**
  ******************************************************************************
  * @file    port_hal_can.c
  * @brief   Reusable STM32 HAL FDCAN adapter.
  ******************************************************************************
  */
#include <string.h>

#include "main.h"
#include "port_hal_can.h"

#define PORT_HAL_CAN_ERROR_NOTIFICATIONS                                       \
  (FDCAN_IT_ERROR_WARNING | FDCAN_IT_ERROR_PASSIVE | FDCAN_IT_BUS_OFF)


static  FDCAN_HandleTypeDef s_can_handle[CAN_INDEX_MAX];
FDCAN_HandleTypeDef *get_can_table_handle(uint8_t index) {
  if (index >= CAN_INDEX_MAX) {
    return NULL;
  }
  return &s_can_handle[index];
}

static usr_status_t port_hal_can_map(HAL_StatusTypeDef status)
{
  switch (status)
  {
    case HAL_OK:
      return USR_OK;
    case HAL_TIMEOUT:
      return USR_ERR_TIMEOUT;
    case HAL_BUSY:
      return USR_BUSY;
    default:
      return USR_ERR_BUS;
  }
}

static bool port_hal_can_mode_valid(uint32_t mode)
{
  return (mode == FDCAN_MODE_NORMAL) ||
         (mode == FDCAN_MODE_RESTRICTED_OPERATION) ||
         (mode == FDCAN_MODE_BUS_MONITORING) ||
         (mode == FDCAN_MODE_INTERNAL_LOOPBACK) ||
         (mode == FDCAN_MODE_EXTERNAL_LOOPBACK);
}

static bool port_hal_can_element_size_valid(uint32_t size)
{
  return (size == FDCAN_DATA_BYTES_8) ||
         (size == FDCAN_DATA_BYTES_12) ||
         (size == FDCAN_DATA_BYTES_16) ||
         (size == FDCAN_DATA_BYTES_20) ||
         (size == FDCAN_DATA_BYTES_24) ||
         (size == FDCAN_DATA_BYTES_32) ||
         (size == FDCAN_DATA_BYTES_48) ||
         (size == FDCAN_DATA_BYTES_64);
}

static uint8_t port_hal_can_element_capacity(uint32_t size)
{
  switch (size)
  {
    case FDCAN_DATA_BYTES_8: return 8u;
    case FDCAN_DATA_BYTES_12: return 12u;
    case FDCAN_DATA_BYTES_16: return 16u;
    case FDCAN_DATA_BYTES_20: return 20u;
    case FDCAN_DATA_BYTES_24: return 24u;
    case FDCAN_DATA_BYTES_32: return 32u;
    case FDCAN_DATA_BYTES_48: return 48u;
    case FDCAN_DATA_BYTES_64: return 64u;
    default: return 0u;
  }
}

static uint32_t port_hal_can_rx_element_size(
    const usr_port_hal_can_resource_t *resource)
{
  if (resource->rx_location == FDCAN_RX_FIFO0)
  {
    return resource->rx_fifo0_element_size;
  }
  if (resource->rx_location == FDCAN_RX_FIFO1)
  {
    return resource->rx_fifo1_element_size;
  }
  return resource->rx_buffer_size;
}

static bool port_hal_can_rx_route_valid(
    const usr_port_hal_can_resource_t *resource)
{
  if (resource->rx_location == FDCAN_RX_FIFO0)
  {
    return (resource->rx_fifo0_element_count != 0u) &&
           ((resource->filter_config == FDCAN_FILTER_TO_RXFIFO0) ||
            (resource->filter_config == FDCAN_FILTER_TO_RXFIFO0_HP));
  }
  if (resource->rx_location == FDCAN_RX_FIFO1)
  {
    return (resource->rx_fifo1_element_count != 0u) &&
           ((resource->filter_config == FDCAN_FILTER_TO_RXFIFO1) ||
            (resource->filter_config == FDCAN_FILTER_TO_RXFIFO1_HP));
  }
  return (resource->rx_location <= FDCAN_RX_BUFFER63) &&
         (resource->rx_location < resource->rx_buffer_count) &&
         (resource->filter_config == FDCAN_FILTER_TO_RXBUFFER);
}

static bool port_hal_can_message_ram_valid(
    const usr_port_hal_can_resource_t *resource)
{
  uint64_t words;

  words = (uint64_t)resource->message_ram_offset +
          resource->std_filter_count +
          ((uint64_t)resource->ext_filter_count * 2u) +
          ((uint64_t)resource->rx_fifo0_element_count *
           resource->rx_fifo0_element_size) +
          ((uint64_t)resource->rx_fifo1_element_count *
           resource->rx_fifo1_element_size) +
          ((uint64_t)resource->rx_buffer_count * resource->rx_buffer_size) +
          ((uint64_t)resource->tx_event_count * 2u) +
          ((uint64_t)(resource->tx_buffer_count +
                      resource->tx_fifo_queue_count) *
           resource->tx_element_size);
  return (resource->message_ram_word_capacity != 0u) &&
         (resource->message_ram_word_capacity <= 2560u) &&
         (resource->message_ram_offset <=
          resource->message_ram_word_capacity) &&
         (words <= resource->message_ram_word_capacity);
}

static bool port_hal_can_resource_valid(
    const usr_port_hal_can_resource_t *resource)
{
  return (resource != NULL) && (resource->instance != NULL) &&
         (resource->rx_gpio_port != NULL) && (resource->rx_gpio_pin != 0u) &&
         (resource->tx_gpio_port != NULL) && (resource->tx_gpio_pin != 0u) &&
         (resource->clock_config != NULL) &&
         (resource->clock_enable != NULL) &&
         (resource->clock_disable != NULL) &&
         (resource->rx_gpio_clock_enable != NULL) &&
         (resource->tx_gpio_clock_enable != NULL) &&
         port_hal_can_mode_valid(resource->mode) &&
         (resource->std_filter_count <= 128u) &&
         (resource->ext_filter_count <= 64u) &&
         (resource->rx_fifo0_element_count <= 64u) &&
         (resource->rx_fifo1_element_count <= 64u) &&
         (resource->rx_buffer_count <= 64u) &&
         (resource->tx_event_count <= 32u) &&
         (resource->tx_buffer_count <= 32u) &&
         (resource->tx_fifo_queue_count != 0u) &&
         ((resource->tx_buffer_count + resource->tx_fifo_queue_count) <= 32u) &&
         ((resource->tx_fifo_queue_mode == FDCAN_TX_FIFO_OPERATION) ||
          (resource->tx_fifo_queue_mode == FDCAN_TX_QUEUE_OPERATION)) &&
         (resource->filter_index < 128u) &&
         ((resource->nonmatching_std == FDCAN_ACCEPT_IN_RX_FIFO0) ||
          (resource->nonmatching_std == FDCAN_ACCEPT_IN_RX_FIFO1) ||
          (resource->nonmatching_std == FDCAN_REJECT)) &&
         ((resource->nonmatching_ext == FDCAN_ACCEPT_IN_RX_FIFO0) ||
          (resource->nonmatching_ext == FDCAN_ACCEPT_IN_RX_FIFO1) ||
          (resource->nonmatching_ext == FDCAN_REJECT)) &&
         port_hal_can_rx_route_valid(resource) &&
         port_hal_can_message_ram_valid(resource) &&
         (resource->rx_queue_storage != NULL) &&
         (resource->rx_queue_capacity >= 2u);
}

static uint32_t port_hal_can_rx_notifications(
    const usr_port_hal_can_resource_t *resource)
{
  if (resource->rx_location == FDCAN_RX_FIFO0)
  {
    return FDCAN_IT_RX_FIFO0_NEW_MESSAGE |
           FDCAN_IT_RX_FIFO0_MESSAGE_LOST;
  }
  if (resource->rx_location == FDCAN_RX_FIFO1)
  {
    return FDCAN_IT_RX_FIFO1_NEW_MESSAGE |
           FDCAN_IT_RX_FIFO1_MESSAGE_LOST;
  }
  return FDCAN_IT_RX_BUFFER_NEW_MESSAGE;
}

static uint32_t port_hal_can_notifications(
    const usr_port_hal_can_resource_t *resource)
{
  return port_hal_can_rx_notifications(resource) |
         PORT_HAL_CAN_ERROR_NOTIFICATIONS;
}


static usr_port_hal_can_instance_t *port_hal_can_instance(void *ctx)
{
  usr_port_hal_can_instance_t *can =
      (usr_port_hal_can_instance_t *)ctx;

  if ((can == NULL) || (can->handle == NULL) || (can->device == NULL) ||
      !port_hal_can_resource_valid(can->resource))
  {
    return NULL;
  }
  return can;
}

static uint32_t port_hal_can_dlc_from_length(uint8_t length)
{
  static const uint32_t dlc[] = {
    FDCAN_DLC_BYTES_0, FDCAN_DLC_BYTES_1, FDCAN_DLC_BYTES_2,
    FDCAN_DLC_BYTES_3, FDCAN_DLC_BYTES_4, FDCAN_DLC_BYTES_5,
    FDCAN_DLC_BYTES_6, FDCAN_DLC_BYTES_7, FDCAN_DLC_BYTES_8,
  };

  if (length <= 8u)
  {
    return dlc[length];
  }
  switch (length)
  {
    case 12u: return FDCAN_DLC_BYTES_12;
    case 16u: return FDCAN_DLC_BYTES_16;
    case 20u: return FDCAN_DLC_BYTES_20;
    case 24u: return FDCAN_DLC_BYTES_24;
    case 32u: return FDCAN_DLC_BYTES_32;
    case 48u: return FDCAN_DLC_BYTES_48;
    case 64u: return FDCAN_DLC_BYTES_64;
    default: return UINT32_MAX;
  }
}

static uint8_t port_hal_can_length_from_dlc(uint32_t dlc)
{
  static const uint8_t lengths[] = {
    0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u,
    8u, 12u, 16u, 20u, 24u, 32u, 48u, 64u,
  };

  return (dlc < (sizeof(lengths) / sizeof(lengths[0]))) ?
         lengths[dlc] : UINT8_MAX;
}

static void port_hal_can_queue_reset(usr_port_hal_can_instance_t *can)
{
  can->rx_read_pos = 0u;
  can->rx_write_pos = 0u;
}

static void port_hal_can_record_error(usr_port_hal_can_instance_t *can)
{
  uint32_t primask = __get_PRIMASK();

  __disable_irq();
  can->error_count++;
  if (primask == 0u)
  {
    __enable_irq();
  }
}

static bool port_hal_can_queue_push(usr_port_hal_can_instance_t *can,
                                    const dev_can_frame_t *frame)
{
  uint16_t read_pos = can->rx_read_pos;
  uint16_t write_pos = can->rx_write_pos;

  if ((uint16_t)(write_pos - read_pos) >=
      can->resource->rx_queue_capacity)
  {
    can->rx_queue_overflow++;
    return false;
  }
  can->resource->rx_queue_storage[
      write_pos % can->resource->rx_queue_capacity] = *frame;
  __DMB();
  can->rx_write_pos = (uint16_t)(write_pos + 1u);
  return true;
}

static bool port_hal_can_queue_pop(usr_port_hal_can_instance_t *can,
                                   dev_can_frame_t *frame)
{
  uint16_t read_pos = can->rx_read_pos;

  if (read_pos == can->rx_write_pos)
  {
    return false;
  }
  __DMB();
  *frame = can->resource->rx_queue_storage[
      read_pos % can->resource->rx_queue_capacity];
  can->rx_read_pos = (uint16_t)(read_pos + 1u);
  return true;
}

static usr_status_t port_hal_can_header_to_frame(
    const FDCAN_RxHeaderTypeDef *header, dev_can_frame_t *frame)
{
  uint8_t length;

  length = port_hal_can_length_from_dlc(header->DataLength);
  if (length == UINT8_MAX)
  {
    return USR_ERR_DATA;
  }
  frame->id = header->Identifier;
  frame->id_type = (header->IdType == FDCAN_STANDARD_ID) ?
                   DEV_CAN_ID_STANDARD : DEV_CAN_ID_EXTENDED;
  frame->frame_type = (header->RxFrameType == FDCAN_DATA_FRAME) ?
                      DEV_CAN_FRAME_DATA : DEV_CAN_FRAME_REMOTE;
  frame->format = (header->FDFormat == FDCAN_CLASSIC_CAN) ?
                  DEV_CAN_FORMAT_CLASSIC : DEV_CAN_FORMAT_FD;
  frame->bit_rate_switch = (header->BitRateSwitch == FDCAN_BRS_ON);
  frame->data_len = length;
  return USR_OK;
}

static usr_status_t port_hal_can_read_message(
    usr_port_hal_can_instance_t *can, dev_can_frame_t *frame)
{
  FDCAN_RxHeaderTypeDef header = {0};
  HAL_StatusTypeDef hal_status;
  usr_status_t status;

  (void)memset(frame, 0, sizeof(*frame));
  hal_status = HAL_FDCAN_GetRxMessage(can->handle,
                                      can->resource->rx_location,
                                      &header, frame->data);
  if (hal_status != HAL_OK)
  {
    port_hal_can_record_error(can);
    return port_hal_can_map(hal_status);
  }
  status = port_hal_can_header_to_frame(&header, frame);
  if (status != USR_OK)
  {
    port_hal_can_record_error(can);
    return status;
  }
  can->rx_frames++;
  return USR_OK;
}

static void port_hal_can_configure_handle(
    usr_port_hal_can_instance_t *can, const dev_can_config_t *config)
{
  FDCAN_InitTypeDef *init = &can->handle->Init;

  can->handle->Instance = can->resource->instance;
  init->FrameFormat = (config->format == DEV_CAN_FORMAT_FD) ?
                      FDCAN_FRAME_FD_BRS : FDCAN_FRAME_CLASSIC;
  init->Mode = can->resource->mode;
  init->AutoRetransmission = config->auto_retransmission ? ENABLE : DISABLE;
  init->TransmitPause = can->resource->transmit_pause ? ENABLE : DISABLE;
  init->ProtocolException = can->resource->protocol_exception ? ENABLE : DISABLE;
  init->NominalPrescaler = config->nominal.prescaler;
  init->NominalSyncJumpWidth = config->nominal.sync_jump_width;
  init->NominalTimeSeg1 = config->nominal.time_seg1;
  init->NominalTimeSeg2 = config->nominal.time_seg2;
  if (config->format == DEV_CAN_FORMAT_FD)
  {
    init->DataPrescaler = config->data.prescaler;
    init->DataSyncJumpWidth = config->data.sync_jump_width;
    init->DataTimeSeg1 = config->data.time_seg1;
    init->DataTimeSeg2 = config->data.time_seg2;
  }
  else
  {
    init->DataPrescaler = 1u;
    init->DataSyncJumpWidth = 1u;
    init->DataTimeSeg1 = 1u;
    init->DataTimeSeg2 = 1u;
  }
  init->MessageRAMOffset = can->resource->message_ram_offset;
  init->StdFiltersNbr = can->resource->std_filter_count;
  init->ExtFiltersNbr = can->resource->ext_filter_count;
  init->RxFifo0ElmtsNbr = can->resource->rx_fifo0_element_count;
  init->RxFifo0ElmtSize = can->resource->rx_fifo0_element_size;
  init->RxFifo1ElmtsNbr = can->resource->rx_fifo1_element_count;
  init->RxFifo1ElmtSize = can->resource->rx_fifo1_element_size;
  init->RxBuffersNbr = can->resource->rx_buffer_count;
  init->RxBufferSize = can->resource->rx_buffer_size;
  init->TxEventsNbr = can->resource->tx_event_count;
  init->TxBuffersNbr = can->resource->tx_buffer_count;
  init->TxFifoQueueElmtsNbr = can->resource->tx_fifo_queue_count;
  init->TxFifoQueueMode = can->resource->tx_fifo_queue_mode;
  init->TxElmtSize = can->resource->tx_element_size;
}

static usr_status_t port_hal_can_configure_filter(
    usr_port_hal_can_instance_t *can, const dev_can_filter_t *filter)
{
  FDCAN_FilterTypeDef hal_filter = {0};
  uint32_t remote_policy = filter->accept_remote ?
                           FDCAN_FILTER_REMOTE : FDCAN_REJECT_REMOTE;

  if (HAL_FDCAN_ConfigGlobalFilter(can->handle,
                                   can->resource->nonmatching_std,
                                   can->resource->nonmatching_ext,
                                   remote_policy, remote_policy) != HAL_OK)
  {
    return USR_ERR_BUS;
  }
  if (!filter->enabled)
  {
    return USR_OK;
  }
  if (((filter->id_type == DEV_CAN_ID_STANDARD) &&
       ((can->resource->std_filter_count == 0u) ||
        (can->resource->filter_index >=
         can->resource->std_filter_count))) ||
      ((filter->id_type == DEV_CAN_ID_EXTENDED) &&
       ((can->resource->ext_filter_count == 0u) ||
        (can->resource->filter_index >=
         can->resource->ext_filter_count))))
  {
    return USR_ERR_UNSUPPORTED;
  }
  if (can->resource->filter_config == FDCAN_FILTER_TO_RXBUFFER)
  {
    uint32_t exact_mask = (filter->id_type == DEV_CAN_ID_STANDARD) ?
                          0x7ffu : 0x1fffffffu;

    if (filter->mask != exact_mask)
    {
      return USR_ERR_UNSUPPORTED;
    }
  }
  hal_filter.IdType = (filter->id_type == DEV_CAN_ID_STANDARD) ?
                      FDCAN_STANDARD_ID : FDCAN_EXTENDED_ID;
  hal_filter.FilterIndex = can->resource->filter_index;
  hal_filter.FilterType = FDCAN_FILTER_MASK;
  hal_filter.FilterConfig = can->resource->filter_config;
  hal_filter.FilterID1 = filter->identifier;
  hal_filter.FilterID2 = filter->mask;
  hal_filter.RxBufferIndex = can->resource->rx_location;
  return port_hal_can_map(HAL_FDCAN_ConfigFilter(can->handle, &hal_filter));
}

static usr_status_t port_hal_can_init(void *ctx, const void *cfg)
{
  usr_port_hal_can_instance_t *can = port_hal_can_instance(ctx);
  const dev_can_config_t *config = (const dev_can_config_t *)cfg;
  usr_status_t status;

  if ((can == NULL) || (config == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (can->lifecycle != USR_PORT_CAN_LIFECYCLE_RESET)
  {
    return USR_ERR_STATE;
  }

  port_hal_can_configure_handle(can, config);
  if (HAL_FDCAN_Init(can->handle) != HAL_OK)
  {
    can->lifecycle = USR_PORT_CAN_LIFECYCLE_FAULT;
    port_hal_can_record_error(can);
    (void)HAL_FDCAN_DeInit(can->handle);
    can->lifecycle = USR_PORT_CAN_LIFECYCLE_RESET;
    return USR_ERR_BUS;
  }
  can->lifecycle = USR_PORT_CAN_LIFECYCLE_READY;
  status = port_hal_can_configure_filter(can, &config->filter);
  if ((status != USR_OK) || (HAL_FDCAN_Start(can->handle) != HAL_OK))
  {
    port_hal_can_record_error(can);
    (void)HAL_FDCAN_DeInit(can->handle);
    can->lifecycle = USR_PORT_CAN_LIFECYCLE_RESET;
    return (status != USR_OK) ? status : USR_ERR_BUS;
  }
  can->lifecycle = USR_PORT_CAN_LIFECYCLE_STARTED;
  can->runtime_config = *config;
  port_hal_can_queue_reset(can);
  {
    uint32_t notifications = PORT_HAL_CAN_ERROR_NOTIFICATIONS;

    if (((config->direction == DEV_CAN_DIRECTION_RX) ||
         (config->direction == DEV_CAN_DIRECTION_TX_RX)) &&
        (config->rx_mode == DEV_CAN_RX_INTERRUPT))
    {
      notifications |= port_hal_can_rx_notifications(can->resource);
    }
    if (HAL_FDCAN_ActivateNotification(can->handle,
                                       notifications, 0u) != HAL_OK)
    {
      port_hal_can_record_error(can);
      (void)HAL_FDCAN_Stop(can->handle);
      (void)HAL_FDCAN_DeInit(can->handle);
      can->lifecycle = USR_PORT_CAN_LIFECYCLE_RESET;
      return USR_ERR_BUS;
    }
  }
  return USR_OK;
}

static usr_status_t port_hal_can_deinit(void *ctx)
{
  usr_port_hal_can_instance_t *can = port_hal_can_instance(ctx);
  usr_status_t status = USR_OK;

  if (can == NULL)
  {
    return USR_ERR_PARAM;
  }
  if (can->lifecycle == USR_PORT_CAN_LIFECYCLE_RESET)
  {
    return USR_OK;
  }
  (void)HAL_FDCAN_DeactivateNotification(can->handle,
                                         port_hal_can_notifications(can->resource));
  if ((can->lifecycle == USR_PORT_CAN_LIFECYCLE_STARTED) &&
      (HAL_FDCAN_Stop(can->handle) != HAL_OK))
  {
    status = USR_ERR_BUS;
  }
  if (HAL_FDCAN_DeInit(can->handle) != HAL_OK)
  {
    status = USR_ERR_BUS;
  }
  port_hal_can_queue_reset(can);
  can->lifecycle = (status == USR_OK) ?
                   USR_PORT_CAN_LIFECYCLE_RESET :
                   USR_PORT_CAN_LIFECYCLE_FAULT;
  return status;
}

static usr_status_t port_hal_can_send(void *ctx, const void *arg,
                                      uint32_t timeout_ms)
{
  usr_port_hal_can_instance_t *can = port_hal_can_instance(ctx);
  const dev_can_frame_t *frame = (const dev_can_frame_t *)arg;
  FDCAN_TxHeaderTypeDef header = {0};
  uint8_t payload[64] = {0};
  uint32_t start_tick;
  uint32_t dlc;
  HAL_StatusTypeDef hal_status;

  if ((can == NULL) || (frame == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (can->lifecycle != USR_PORT_CAN_LIFECYCLE_STARTED)
  {
    return USR_ERR_NOT_INIT;
  }
  if (frame->data_len >
      port_hal_can_element_capacity(can->resource->tx_element_size))
  {
    return USR_ERR_UNSUPPORTED;
  }
  start_tick = HAL_GetTick();
  while (HAL_FDCAN_GetTxFifoFreeLevel(can->handle) == 0u)
  {
    if (timeout_ms == 0u)
    {
      return USR_BUSY;
    }
    if ((HAL_GetTick() - start_tick) >= timeout_ms)
    {
      return USR_ERR_TIMEOUT;
    }
  }
  
  dlc = port_hal_can_dlc_from_length(frame->data_len);
  if (dlc == UINT32_MAX)
  {
    return USR_ERR_PARAM;
  }
  header.Identifier = frame->id;
  header.IdType = (frame->id_type == DEV_CAN_ID_STANDARD) ?
                  FDCAN_STANDARD_ID : FDCAN_EXTENDED_ID;
  header.TxFrameType = (frame->frame_type == DEV_CAN_FRAME_DATA) ?
                       FDCAN_DATA_FRAME : FDCAN_REMOTE_FRAME;
  header.DataLength = dlc;
  header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  header.BitRateSwitch = frame->bit_rate_switch ? FDCAN_BRS_ON : FDCAN_BRS_OFF;
  header.FDFormat = (frame->format == DEV_CAN_FORMAT_FD) ?
                    FDCAN_FD_CAN : FDCAN_CLASSIC_CAN;
  header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  (void)memcpy(payload, frame->data, frame->data_len);
  hal_status = HAL_FDCAN_AddMessageToTxFifoQ(can->handle, &header,
                                              payload);
  if (hal_status == HAL_OK)
  {
    can->tx_frames++;
    return USR_OK;
  }
  port_hal_can_record_error(can);
  return ((HAL_FDCAN_GetError(can->handle) & HAL_FDCAN_ERROR_FIFO_FULL) != 0u) ?
         USR_BUSY : port_hal_can_map(hal_status);
}

static bool port_hal_can_message_available(
    usr_port_hal_can_instance_t *can)
{
  if ((can->resource->rx_location == FDCAN_RX_FIFO0) ||
      (can->resource->rx_location == FDCAN_RX_FIFO1))
  {
    return HAL_FDCAN_GetRxFifoFillLevel(
               can->handle, can->resource->rx_location) != 0u;
  }
  return HAL_FDCAN_IsRxBufferMessageAvailable(
             can->handle, can->resource->rx_location) != 0u;
}

static usr_status_t port_hal_can_recv(void *ctx, void *arg)
{
  usr_port_hal_can_instance_t *can = port_hal_can_instance(ctx);
  dev_can_frame_t *frame = (dev_can_frame_t *)arg;

  if ((can == NULL) || (frame == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (can->lifecycle != USR_PORT_CAN_LIFECYCLE_STARTED)
  {
    return USR_ERR_NOT_INIT;
  }
  if (can->runtime_config.rx_mode == DEV_CAN_RX_INTERRUPT)
  {
    return port_hal_can_queue_pop(can, frame) ? USR_OK : USR_BUSY;
  }
  if (!port_hal_can_message_available(can))
  {
    return USR_BUSY;
  }
  return port_hal_can_read_message(can, frame);
}

static void port_hal_can_drain_rx(usr_port_hal_can_instance_t *can)
{
  dev_can_frame_t frame;

  while (port_hal_can_message_available(can))
  {
    if (port_hal_can_read_message(can, &frame) != USR_OK)
    {
      break;
    }
    (void)port_hal_can_queue_push(can, &frame);
  }
}

static usr_status_t port_hal_can_get_status(void *ctx, void *arg)
{
  usr_port_hal_can_instance_t *can = port_hal_can_instance(ctx);
  dev_can_status_t *status = (dev_can_status_t *)arg;
  FDCAN_ProtocolStatusTypeDef protocol = {0};
  uint32_t primask;

  if ((can == NULL) || (status == NULL))
  {
    return USR_ERR_PARAM;
  }
  if ((can->lifecycle == USR_PORT_CAN_LIFECYCLE_STARTED) &&
      (HAL_FDCAN_GetProtocolStatus(can->handle, &protocol) != HAL_OK))
  {
    return USR_ERR_BUS;
  }
  primask = __get_PRIMASK();
  __disable_irq();
  status->initialized = can->lifecycle != USR_PORT_CAN_LIFECYCLE_RESET;
  status->started = can->lifecycle == USR_PORT_CAN_LIFECYCLE_STARTED;
  status->error_warning = protocol.Warning != 0u;
  status->error_passive = protocol.ErrorPassive != 0u;
  status->bus_off = protocol.BusOff != 0u;
  status->tx_frames = can->tx_frames;
  status->rx_frames = can->rx_frames;
  status->error_count = can->error_count;
  status->rx_fifo_lost = can->rx_fifo_lost;
  status->rx_queue_overflow = can->rx_queue_overflow;
  if (primask == 0u)
  {
    __enable_irq();
  }
  return USR_OK;
}

bool usr_port_hal_can_msp_init(FDCAN_HandleTypeDef *handle)
{
  usr_port_hal_can_instance_t *can =
      usr_port_board_can_find_instance(handle);
  const usr_port_hal_can_resource_t *resource;
  GPIO_InitTypeDef gpio = {0};

  if (can == NULL)
  {
    return false;
  }
  resource = can->resource;
  if (!port_hal_can_resource_valid(resource))
  {
    Error_Handler();
    return true;
  }
  if (resource->clock_config() != HAL_OK)
  {
    Error_Handler();
    return true;
  }
  resource->clock_enable();
  resource->rx_gpio_clock_enable();
  resource->tx_gpio_clock_enable();
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Alternate = resource->gpio_alternate;
  gpio.Pin = resource->rx_gpio_pin;
  HAL_GPIO_Init(resource->rx_gpio_port, &gpio);
  gpio.Pin = resource->tx_gpio_pin;
  HAL_GPIO_Init(resource->tx_gpio_port, &gpio);
  HAL_NVIC_SetPriority(resource->irq, resource->irq_priority, 0u);
  HAL_NVIC_EnableIRQ(resource->irq);
  return true;
}

bool usr_port_hal_can_msp_deinit(FDCAN_HandleTypeDef *handle)
{
  usr_port_hal_can_instance_t *can =
      usr_port_board_can_find_instance(handle);
  const usr_port_hal_can_resource_t *resource;

  if (can == NULL)
  {
    return false;
  }
  resource = can->resource;
  HAL_NVIC_DisableIRQ(resource->irq);
  HAL_NVIC_ClearPendingIRQ(resource->irq);
  HAL_GPIO_DeInit(resource->rx_gpio_port, resource->rx_gpio_pin);
  HAL_GPIO_DeInit(resource->tx_gpio_port, resource->tx_gpio_pin);
  resource->clock_disable();
  return true;
}

void port_hal_can_irq(FDCAN_HandleTypeDef *canHandle)
{
  if (usr_port_board_can_find_instance(canHandle) != NULL)
  {
    HAL_FDCAN_IRQHandler(canHandle);
  }
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *handle,
                               uint32_t interrupts)
{
  usr_port_hal_can_instance_t *can =
      usr_port_board_can_find_instance(handle);

  if ((can == NULL) || (can->resource->rx_location != FDCAN_RX_FIFO0))
  {
    return;
  }
  if ((interrupts & FDCAN_IT_RX_FIFO0_MESSAGE_LOST) != 0u)
  {
    can->rx_fifo_lost++;
  }
  if ((interrupts & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0u)
  {
    return;
  }
  port_hal_can_drain_rx(can);
}

void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *handle,
                               uint32_t interrupts)
{
  usr_port_hal_can_instance_t *can =
      usr_port_board_can_find_instance(handle);

  if ((can == NULL) || (can->resource->rx_location != FDCAN_RX_FIFO1))
  {
    return;
  }
  if ((interrupts & FDCAN_IT_RX_FIFO1_MESSAGE_LOST) != 0u)
  {
    can->rx_fifo_lost++;
  }
  if ((interrupts & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) == 0u)
  {
    return;
  }
  port_hal_can_drain_rx(can);
}

void HAL_FDCAN_RxBufferNewMessageCallback(FDCAN_HandleTypeDef *handle)
{
  usr_port_hal_can_instance_t *can =
      usr_port_board_can_find_instance(handle);

  if ((can == NULL) || (can->resource->rx_location == FDCAN_RX_FIFO0) ||
      (can->resource->rx_location == FDCAN_RX_FIFO1))
  {
    return;
  }
  port_hal_can_drain_rx(can);
}

void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *handle)
{
  usr_port_hal_can_instance_t *can =
      usr_port_board_can_find_instance(handle);

  if (can != NULL)
  {
    port_hal_can_record_error(can);
  }
}

void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *handle,
                                   uint32_t interrupts)
{
  usr_port_hal_can_instance_t *can =
      usr_port_board_can_find_instance(handle);

  (void)interrupts;
  if (can != NULL)
  {
    port_hal_can_record_error(can);
  }
}

const dev_can_ops_t port_hal_can_ops =
{
  .init = port_hal_can_init,
  .deinit = port_hal_can_deinit,
  .send = port_hal_can_send,
  .recv = port_hal_can_recv,
  .get_status = port_hal_can_get_status,
};
