#ifndef RING_BUF_H
#define RING_BUF_H

#include <stdint.h>

typedef enum
{
  RING_BUF_MODE_SOFTWARE = 0,
  RING_BUF_MODE_DMA_CIRCULAR,
} ring_buf_mode_t;

typedef struct
{
  uint8_t *buffer;
  uint16_t capacity;
  ring_buf_mode_t mode;
  volatile uint16_t read_pos;
  volatile uint16_t write_pos;
  volatile uint32_t read_epoch;
  volatile uint32_t write_epoch;
  volatile uint32_t overflow;
} ring_buf_t;

void ring_buf_reset(ring_buf_t *ring);
uint16_t ring_buf_write(ring_buf_t *ring, const uint8_t *data,
                        uint16_t length);
uint16_t ring_buf_dma_commit(ring_buf_t *ring, uint16_t length);
uint16_t ring_buf_read(ring_buf_t *ring, uint8_t *data, uint16_t capacity);
uint16_t ring_buf_available(ring_buf_t *ring);
uint32_t ring_buf_get_overflow(ring_buf_t *ring);

#endif /* RING_BUF_H */
