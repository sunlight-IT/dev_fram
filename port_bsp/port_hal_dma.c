#include "port_hal_dma.h"

bool port_hal_dma_present(const port_hal_dma_resource_t *resource)
{
  return ((resource != NULL) &&
          ((resource->handle != NULL) || (resource->instance != NULL)));
}

static bool port_hal_dma_resource_valid(
    const port_hal_dma_resource_t *resource)
{
  return (port_hal_dma_present(resource) &&
          (resource->handle != NULL) &&
          (resource->instance != NULL) &&
          (resource->clock_enable != NULL));
}

bool port_hal_dma_enabled(const port_hal_dma_resource_t *resource)
{
  return port_hal_dma_resource_valid(resource);
}

HAL_StatusTypeDef port_hal_dma_init(const port_hal_dma_resource_t *resource)
{
  if (!port_hal_dma_present(resource))
  {
    return HAL_OK;
  }
  if (!port_hal_dma_resource_valid(resource))
  {
    return HAL_ERROR;
  }

  resource->clock_enable();
  resource->handle->Instance = resource->instance;
  resource->handle->Init = resource->init;
  HAL_StatusTypeDef status = HAL_DMA_Init(resource->handle);
  if (status != HAL_OK)
  {
    resource->handle->Instance = NULL;
    resource->handle->Parent = NULL;
  }
  return status;
}

HAL_StatusTypeDef port_hal_dma_deinit(const port_hal_dma_resource_t *resource)
{
  HAL_StatusTypeDef status;

  if (!port_hal_dma_enabled(resource) || (resource->handle == NULL))
  {
    return HAL_OK;
  }

  status = HAL_DMA_DeInit(resource->handle);
  resource->handle->Instance = NULL;
  resource->handle->Parent = NULL;
  return status;
}

void port_hal_dma_irq_enable(const port_hal_dma_resource_t *resource)
{
  if (!port_hal_dma_resource_valid(resource))
  {
    return;
  }

  HAL_NVIC_SetPriority(resource->irq, resource->irq_priority,
                       resource->irq_subpriority);
  HAL_NVIC_EnableIRQ(resource->irq);
}

void port_hal_dma_irq_disable(const port_hal_dma_resource_t *resource)
{
  if (!port_hal_dma_enabled(resource))
  {
    return;
  }

  HAL_NVIC_DisableIRQ(resource->irq);
  HAL_NVIC_ClearPendingIRQ(resource->irq);
}

void port_hal_dma_irq(const port_hal_dma_resource_t *resource)
{
  if (!port_hal_dma_enabled(resource) ||
      (resource->handle->Instance == NULL) ||
      (resource->handle->Parent == NULL))
  {
    return;
  }

  HAL_DMA_IRQHandler(resource->handle);
}

void port_hal_dma_controller_clock_enable(void) {
  #ifndef STM32H743xx
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  #endif
  __HAL_RCC_DMA1_CLK_ENABLE();
}
