#include "ring_buf.h"

#if defined(__arm__) || defined(__thumb__)
#define RING_BUF_MEMORY_BARRIER() __asm volatile("dmb" ::: "memory")
#else
#define RING_BUF_MEMORY_BARRIER() __asm volatile("" ::: "memory")
#endif

static uint32_t ring_buf_pending(const ring_buf_t *ring)
{
  uint32_t epoch_delta;
  uint32_t position_delta;

  if ((ring == 0) || (ring->capacity == 0u))
  {
    return 0u;
  }
  epoch_delta = ring->write_epoch - ring->read_epoch;
  position_delta = (uint32_t)ring->write_pos - ring->read_pos;
  return (epoch_delta * ring->capacity) + position_delta;
}

static uint32_t ring_buf_safe_capacity(const ring_buf_t *ring)
{
  if (ring->mode == RING_BUF_MODE_DMA_CIRCULAR)
  {
    return (uint32_t)ring->capacity / 2u;
  }
  return ring->capacity;
}

static void ring_buf_drop_oldest(ring_buf_t *ring, uint32_t count)
{
  uint32_t next;

  next = ((uint32_t)ring->read_pos + count) / ring->capacity;
  ring->read_pos = (uint16_t)(((uint32_t)ring->read_pos + count) %
                              ring->capacity);
  ring->read_epoch += next;
  ring->overflow += count;
}

static void ring_buf_advance_write(ring_buf_t *ring, uint16_t length)
{
  uint32_t next;

  next = ((uint32_t)ring->write_pos + length) / ring->capacity;
  ring->write_pos = (uint16_t)(((uint32_t)ring->write_pos + length) %
                               ring->capacity);
  ring->write_epoch += next;
}

static uint32_t ring_buf_normalize(ring_buf_t *ring)
{
  uint32_t pending;
  uint32_t safe_capacity;

  pending = ring_buf_pending(ring);
  safe_capacity = ring_buf_safe_capacity(ring);
  if (pending > safe_capacity)
  {
    ring_buf_drop_oldest(ring, pending - safe_capacity);
    pending = safe_capacity;
  }
  return pending;
}

void ring_buf_reset(ring_buf_t *ring)
{
  if (ring == 0)
  {
    return;
  }
  ring->read_pos = 0u;
  ring->write_pos = 0u;
  ring->read_epoch = 0u;
  ring->write_epoch = 0u;
  ring->overflow = 0u;
  RING_BUF_MEMORY_BARRIER();
}

uint16_t ring_buf_write(ring_buf_t *ring, const uint8_t *data,
                        uint16_t length)
{
  uint16_t index;

  if ((ring == 0) || (data == 0) || (length == 0u) ||
      (ring->buffer == 0) || (ring->capacity == 0u) ||
      (ring->mode != RING_BUF_MODE_SOFTWARE))
  {
    return 0u;
  }

  for (index = 0u; index < length; index++)
  {
    ring->buffer[ring->write_pos] = data[index];
    RING_BUF_MEMORY_BARRIER();
    ring_buf_advance_write(ring, 1u);
  }
  return length;
}

uint16_t ring_buf_dma_commit(ring_buf_t *ring, uint16_t length)
{
  if ((ring == 0) || (ring->buffer == 0) || (ring->capacity == 0u) ||
      (length == 0u) || (ring->mode != RING_BUF_MODE_DMA_CIRCULAR) ||
      ((ring->capacity % 2u) != 0u) ||
      (length != (uint16_t)(ring->capacity / 2u)))
  {
    return 0u;
  }

  RING_BUF_MEMORY_BARRIER();
  ring_buf_advance_write(ring, length);
  return length;
}

uint16_t ring_buf_read(ring_buf_t *ring, uint8_t *data, uint16_t capacity)
{
  uint32_t available;
  uint16_t count;
  uint16_t index;
  uint16_t read_pos;
  uint32_t read_epoch;
  uint16_t write_pos;
  uint32_t write_epoch;

  if ((ring == 0) || (data == 0) || (capacity == 0u) ||
      (ring->buffer == 0) || (ring->capacity == 0u))
  {
    return 0u;
  }

  available = ring_buf_normalize(ring);
  count = (available < capacity) ? (uint16_t)available : capacity;
  read_pos = ring->read_pos;
  read_epoch = ring->read_epoch;
  write_pos = ring->write_pos;
  write_epoch = ring->write_epoch;
  RING_BUF_MEMORY_BARRIER();
  for (index = 0u; index < count; index++)
  {
    data[index] = ring->buffer[read_pos];
    read_pos++;
    if (read_pos >= ring->capacity)
    {
      read_pos = 0u;
      read_epoch++;
    }
  }
  RING_BUF_MEMORY_BARRIER();
  if ((write_pos != ring->write_pos) || (write_epoch != ring->write_epoch))
  {
    return 0u;
  }
  ring->read_pos = read_pos;
  ring->read_epoch = read_epoch;
  return count;
}

uint16_t ring_buf_available(ring_buf_t *ring)
{
  uint32_t available;

  if (ring == 0)
  {
    return 0u;
  }
  available = ring_buf_normalize(ring);
  return (available > UINT16_MAX) ? UINT16_MAX : (uint16_t)available;
}

uint32_t ring_buf_get_overflow(ring_buf_t *ring)
{
  if (ring == 0)
  {
    return 0u;
  }
  (void)ring_buf_normalize(ring);
  return ring->overflow;
}
