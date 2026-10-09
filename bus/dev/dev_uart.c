/**
  ******************************************************************************
  * @file    usr_bus_uart.c
  * @brief   Platform-independent, protocol-neutral UART byte stream.
  ******************************************************************************
  */
#include "dev_uart.h"

static uart_device_t s_devices[UART_INDEX_MAX];

uart_device_t *get_uart_device(uint8_t index)
{
  if (index >= UART_INDEX_MAX)
  {
    return NULL;
  }
  return &s_devices[index];
}

static bool dev_uart_data_bits_valid(uart_data_bits_t data_bits)
{
  return (data_bits == DEV_UART_DATA_BITS_7) ||
         (data_bits == DEV_UART_DATA_BITS_8) ||
         (data_bits == DEV_UART_DATA_BITS_9);
}

static bool dev_uart_parity_valid(uart_parity_t parity)
{
  return (parity == DEV_UART_PARITY_NONE) ||
         (parity == DEV_UART_PARITY_EVEN) ||
         (parity == DEV_UART_PARITY_ODD);
}

static bool dev_uart_stop_bits_valid(uart_stop_bits_t stop_bits)
{
  return (stop_bits == DEV_UART_STOP_BITS_1) ||
         (stop_bits == DEV_UART_STOP_BITS_2);
}

static bool dev_uart_direction_valid(uart_direction_t direction)
{
  return (direction == DEV_UART_DIRECTION_TX) ||
         (direction == DEV_UART_DIRECTION_RX) ||
         (direction == DEV_UART_DIRECTION_TX_RX);
}

static bool dev_uart_ring_buf_mode_valid(uart_rx_mode_t mode)
{
  return (mode == DEV_UART_RX_DMA_CIRCULAR) ||
         (mode == DEV_UART_RX_INTERRUPT);
}



static bool dev_uart_config_valid(const uart_param_config_t *config)
{
  return (config != NULL) && (config->baudrate != 0u) &&
         dev_uart_data_bits_valid(config->data_bits) &&
         dev_uart_parity_valid(config->parity) &&
         dev_uart_stop_bits_valid(config->stop_bits) &&
         dev_uart_direction_valid(config->direction) &&
         dev_uart_ring_buf_mode_valid(config->rx_mode);
}

usr_status_t dev_uart_init(uart_device_t *dev,
                           const uart_param_config_t *cfg)
{
  uart_param_config_t candidate;
  usr_status_t status;

  if ((dev == NULL) || (dev->ops == NULL) || (dev->ops->init == NULL))
  {
    return USR_ERR_PARAM;
  }

  if (cfg != NULL)
  {
    if (!dev_uart_config_valid(cfg)) {
      return USR_ERR_PARAM;
    }
    candidate = *cfg;
    status = dev->ops->init(dev->ctx, &candidate);
    if (status == USR_OK)
    {
      dev->config = candidate;
    }
    return status;
  }
  return dev->ops->init(dev->ctx, NULL);
}

usr_status_t dev_uart_deinit(uart_device_t *dev)
{
  if ((dev == NULL) || (dev->ops == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (dev->ops->deinit != NULL)
  {
    return dev->ops->deinit(dev->ctx);
  }
  return USR_OK;
}


usr_status_t dev_uart_read(uart_device_t *dev, uint8_t *data,
                           uint16_t capacity, uint16_t *read_len)
{
  usr_status_t status;

  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->read == NULL) || (data == NULL) ||
      (capacity == 0u) || (read_len == NULL))
  {
    return USR_ERR_PARAM;
  }
  *read_len = 0u;
  status = dev->ops->read(dev->ctx, data, capacity, read_len);
//   if (status == USR_OK)
//   {
//     if (*read_len > capacity)
//     {
//       *read_len = 0u;
//       return USR_ERR_DATA;
//     }
//     dev->bytes_read += *read_len;
//   }
  return status;
}

usr_status_t dev_uart_write(uart_device_t *dev, const uint8_t *data,
                            uint16_t len, uint32_t timeout_ms)
{
  usr_status_t status;

  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->write == NULL) || (data == NULL) || (len == 0u))
  {
    return USR_ERR_PARAM;
  }
  status = dev->ops->write(dev->ctx, data, len, timeout_ms);
//   if (status == USR_OK)
//   {
//     dev->bytes_written += len;
//   }
  return status;
}

uint32_t dev_uart_rx_overflow(uart_device_t *dev)
{
  if ((dev == NULL) || (dev->ops == NULL) ||
      (dev->ops->rx_overflow == NULL))
  {
    return 0u;
  }
  return dev->ops->rx_overflow(dev->ctx);
}

usr_status_t dev_uart_set_config(uart_device_t *dev, uint32_t baudrate)
{
  return dev_uart_control(dev, DEV_UART_CMD_SET_BAUDRATE, &baudrate);
}

usr_status_t dev_uart_control(uart_device_t *dev, uint32_t cmd, void *arg)
{
  uart_param_config_t candidate;
  usr_status_t status;

  if ((dev == NULL) || (dev->ops == NULL) || (dev->ops->control == NULL))
  {
    return USR_ERR_PARAM;
  }
  switch (cmd)
  {
    case DEV_UART_CMD_SET_CONFIG:
      if ((arg == NULL) ||
          !dev_uart_config_valid((const uart_param_config_t *)arg))
      {
        return USR_ERR_PARAM;
      }
      candidate = *(const uart_param_config_t *)arg;
      break;
    case DEV_UART_CMD_GET_CONFIG:
      if (arg == NULL)
      {
        return USR_ERR_PARAM;
      }
      *(uart_param_config_t *)arg = dev->config;
      return USR_OK;
    case DEV_UART_CMD_SET_BAUDRATE:
      if ((arg == NULL) || (*(const uint32_t *)arg == 0u))
      {
        return USR_ERR_PARAM;
      }
      candidate = dev->config;
      candidate.baudrate = *(const uint32_t *)arg;
      break;
    case DEV_UART_CMD_SET_PARITY:
      if ((arg == NULL) ||
          !dev_uart_parity_valid(*(const uart_parity_t *)arg))
      {
        return USR_ERR_PARAM;
      }
      candidate = dev->config;
      candidate.parity = *(const uart_parity_t *)arg;
      break;
    case DEV_UART_CMD_SET_STOP_BITS:
      if ((arg == NULL) ||
          !dev_uart_stop_bits_valid(*(const uart_stop_bits_t *)arg))
      {
        return USR_ERR_PARAM;
      }
      candidate = dev->config;
      candidate.stop_bits = *(const uart_stop_bits_t *)arg;
      break;
    case DEV_UART_CMD_SET_DATA_BITS:
      if ((arg == NULL) ||
          !dev_uart_data_bits_valid(*(const uart_data_bits_t *)arg))
      {
        return USR_ERR_PARAM;
      }
      candidate = dev->config;
      candidate.data_bits = *(const uart_data_bits_t *)arg;
      break;
    case DEV_UART_CMD_SET_DIRECTION:
      if ((arg == NULL) ||
          !dev_uart_direction_valid(*(const uart_direction_t *)arg))
      {
        return USR_ERR_PARAM;
      }
      candidate = dev->config;
      candidate.direction = *(const uart_direction_t *)arg;
      break;
    case DEV_UART_CMD_START_RX:
    case DEV_UART_CMD_STOP_RX:
      return dev->ops->control(dev->ctx, cmd, NULL);
    default:
      return USR_ERR_UNSUPPORTED;
  }

  if (!dev_uart_config_valid(&candidate))
  {
    return USR_ERR_PARAM;
  }
  status = dev->ops->control(dev->ctx, DEV_UART_CMD_SET_CONFIG, &candidate);
  if (status != USR_OK)
  {
    return status;
  }
  dev->config = candidate;
  return status;
}
