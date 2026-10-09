#ifndef __PORT_HAL_DMA_H__
#define __PORT_HAL_DMA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "stm32wlxx_hal.h"

typedef void (*port_hal_dma_clock_enable_fn_t)(void);

typedef struct
{
  DMA_HandleTypeDef *handle;
  DMA_Channel_TypeDef *instance;
  DMA_InitTypeDef init;
  IRQn_Type irq;
  uint32_t irq_priority;
  uint32_t irq_subpriority;
  port_hal_dma_clock_enable_fn_t clock_enable;
} port_hal_dma_resource_t;

bool port_hal_dma_present(const port_hal_dma_resource_t *resource);
bool port_hal_dma_enabled(const port_hal_dma_resource_t *resource);
HAL_StatusTypeDef port_hal_dma_init(const port_hal_dma_resource_t *resource);
HAL_StatusTypeDef port_hal_dma_deinit(const port_hal_dma_resource_t *resource);
void port_hal_dma_irq_enable(const port_hal_dma_resource_t *resource);
void port_hal_dma_irq_disable(const port_hal_dma_resource_t *resource);
void port_hal_dma_irq(const port_hal_dma_resource_t *resource);

void port_hal_dma_controller_clock_enable(void);

#ifdef __cplusplus
}
#endif

#endif /* __PORT_HAL_DMA_H__ */
