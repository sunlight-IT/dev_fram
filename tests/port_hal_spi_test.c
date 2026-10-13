#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../port_bsp/port_hal_spi.c"

enum
{
  CALL_POLL_TX = 1,
  CALL_POLL_RX,
  CALL_POLL_TX_RX,
  CALL_IT_TX,
  CALL_IT_RX,
  CALL_IT_TX_RX,
  CALL_DMA_TX,
  CALL_DMA_RX,
  CALL_DMA_TX_RX,
};

enum
{
  ASYNC_COMPLETE = 0,
  ASYNC_ERROR,
  ASYNC_TIMEOUT,
  ASYNC_IRQ_ERROR,
};

uint32_t spi_test_primask;
uint32_t spi_test_ipsr;
uint32_t spi_test_basepri;
uint32_t spi_test_faultmask;

static SPI_TypeDef s_registers;
static SPI_HandleTypeDef s_handle;
static DMA_Stream_TypeDef s_streams[2];
static DMA_HandleTypeDef s_dma[2];
static spi_device_t s_device;
static usr_port_hal_spi_resource_t s_resource;
static usr_port_hal_spi_instance_t s_instance;
static uint32_t s_time;
static uint32_t s_calls[10];
static uint32_t s_last_timeout;
static uint16_t s_last_frames;
static uint32_t s_abort_calls;
static uint32_t s_dma_init_calls;
static uint32_t s_dma_fail_step;
static uint32_t s_dma_deinit_failure;
static uint32_t s_irq_calls;
static uint32_t s_spi_irq_enable_calls;
static uint32_t s_spi_irq_disable_calls;
static uint32_t s_spi_irq_clear_calls;
static uint32_t s_gpio_af[3];
static uint32_t s_gpio_calls;
static uint32_t s_buffer_checks;
static bool s_last_buffer_receive;
static bool s_buffer_valid;
static bool s_async_pending;
static uint32_t s_async_behavior;
static HAL_StatusTypeDef s_start_status;
static HAL_StatusTypeDef s_poll_status;
static uint32_t s_poll_tick_advance;
static bool s_abort_failure;
static bool s_abort_stuck;
static uint32_t s_abort_tick_advance;
static bool s_complete_abort_on_tick;
static bool s_dma_irq_disabled[2];
static bool s_complete_on_irq_restore;
static bool s_stale_complete_on_prestart_restore;

usr_port_hal_spi_instance_t *
usr_port_board_spi_find_instance(const SPI_HandleTypeDef *handle)
{
  return (handle == &s_handle) ? &s_instance : NULL;
}

usr_port_hal_spi_instance_t *usr_port_board_spi_find_dma_instance(IRQn_Type irq)
{
  return ((irq == DMA2_Stream0_IRQn) || (irq == DMA2_Stream1_IRQn))
             ? &s_instance
             : NULL;
}

bool port_hal_dma_present(const port_hal_dma_resource_t *resource)
{
  return (resource != NULL) &&
         ((resource->handle != NULL) || (resource->instance != NULL));
}

bool port_hal_dma_enabled(const port_hal_dma_resource_t *resource)
{
  return port_hal_dma_present(resource) && (resource->handle != NULL) &&
         (resource->instance != NULL) && (resource->clock_enable != NULL);
}

HAL_StatusTypeDef port_hal_dma_init(const port_hal_dma_resource_t *resource)
{
  s_dma_init_calls++;
  if (s_dma_init_calls == s_dma_fail_step)
  {
    return HAL_ERROR;
  }
  resource->handle->Instance = resource->instance;
  resource->handle->Init = resource->init;
  resource->handle->State = HAL_DMA_STATE_READY;
  return HAL_OK;
}

HAL_StatusTypeDef port_hal_dma_deinit(const port_hal_dma_resource_t *resource)
{
  if (s_dma_deinit_failure != 0u)
  {
    return HAL_ERROR;
  }
  resource->handle->Instance = NULL;
  resource->handle->Parent = NULL;
  resource->handle->State = HAL_DMA_STATE_RESET;
  return HAL_OK;
}

void port_hal_dma_irq_enable(const port_hal_dma_resource_t *resource)
{
  (void)resource;
}

void port_hal_dma_irq_disable(const port_hal_dma_resource_t *resource)
{
  (void)resource;
}

void port_hal_dma_irq(const port_hal_dma_resource_t *resource)
{
  assert(resource->handle->Parent == &s_handle);
  s_irq_calls++;
}

static HAL_StatusTypeDef clock_config(void)
{
  return HAL_OK;
}

static void clock_gate(void)
{
}

static uint32_t kernel_hz(void)
{
  return 64000000u;
}

static bool dma_buffer_valid(const void *buffer, uint16_t length, bool receive)
{
  assert(buffer != NULL);
  assert(length != 0u);
  s_buffer_checks++;
  s_last_buffer_receive = receive;
  return s_buffer_valid;
}

void Error_Handler(void)
{
  assert(false);
}

static void complete_async(usr_status_t result)
{
  s_async_pending = false;
  s_handle.State = HAL_SPI_STATE_READY;
  if ((result != USR_OK) && s_complete_abort_on_tick)
  {
    s_dma[0].State = HAL_DMA_STATE_READY;
    s_streams[0].CR &= ~DMA_SxCR_EN;
    s_dma[1].State = HAL_DMA_STATE_ABORT;
    s_streams[1].CR &= ~DMA_SxCR_EN;
    (void)usr_port_hal_spi_complete(&s_handle, result);
    return;
  }
  for (uint32_t index = 0u; index < 2u; index++)
  {
    s_dma[index].State = HAL_DMA_STATE_READY;
    s_streams[index].CR &= ~DMA_SxCR_EN;
  }
  (void)usr_port_hal_spi_complete(&s_handle, result);
}

void spi_test_set_primask(uint32_t value)
{
  spi_test_primask = value;
  if ((value == 0u) && s_complete_on_irq_restore && (s_time >= 4u))
  {
    s_complete_on_irq_restore = false;
    complete_async(USR_OK);
  }
}

uint32_t HAL_GetTick(void)
{
  uint32_t now = s_time++;
  if (s_complete_abort_on_tick)
  {
    for (uint32_t index = 0u; index < 2u; index++)
    {
      if ((s_dma[index].State == HAL_DMA_STATE_ABORT) &&
          !s_dma_irq_disabled[index])
      {
        s_dma[index].State = HAL_DMA_STATE_READY;
        s_streams[index].CR &= ~DMA_SxCR_EN;
      }
    }
  }
  if (s_async_pending && (s_async_behavior == ASYNC_IRQ_ERROR))
  {
    s_async_pending = false;
    usr_port_hal_spi_irq(&s_handle);
  }
  else if (s_async_pending && (s_async_behavior != ASYNC_TIMEOUT))
  {
    complete_async((s_async_behavior == ASYNC_COMPLETE) ? USR_OK
                                                         : USR_ERR_BUS);
  }
  return now;
}

void HAL_GPIO_Init(GPIO_TypeDef *port, const GPIO_InitTypeDef *config)
{
  (void)port;
  s_gpio_af[s_gpio_calls++ % 3u] = config->Alternate;
}

void HAL_GPIO_DeInit(GPIO_TypeDef *port, uint32_t pins)
{
  (void)port;
  (void)pins;
}

void HAL_NVIC_SetPriority(IRQn_Type irq, uint32_t priority, uint32_t subpriority)
{
  (void)irq;
  (void)subpriority;
  assert(priority == 5u);
}

void HAL_NVIC_EnableIRQ(IRQn_Type irq)
{
  if (irq == SPI3_IRQn)
  {
    s_spi_irq_enable_calls++;
  }
  else if (irq == DMA2_Stream0_IRQn)
  {
    s_dma_irq_disabled[0] = false;
  }
  else if (irq == DMA2_Stream1_IRQn)
  {
    s_dma_irq_disabled[1] = false;
  }
}

void HAL_NVIC_DisableIRQ(IRQn_Type irq)
{
  if (irq == SPI3_IRQn)
  {
    s_spi_irq_disable_calls++;
  }
  else if (irq == DMA2_Stream0_IRQn)
  {
    s_dma_irq_disabled[0] = true;
  }
  else if (irq == DMA2_Stream1_IRQn)
  {
    s_dma_irq_disabled[1] = true;
  }
}

void HAL_NVIC_ClearPendingIRQ(IRQn_Type irq)
{
  if (irq == SPI3_IRQn)
  {
    s_spi_irq_clear_calls++;
  }
}

HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef *handle)
{
  (void)usr_port_hal_spi_msp_init(handle);
  handle->State = HAL_SPI_STATE_READY;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_SPI_DeInit(SPI_HandleTypeDef *handle)
{
  (void)usr_port_hal_spi_msp_deinit(handle);
  handle->State = HAL_SPI_STATE_RESET;
  return HAL_OK;
}

static HAL_StatusTypeDef start_async(SPI_HandleTypeDef *handle, uint16_t frames,
                                     uint32_t kind)
{
  s_calls[kind]++;
  s_last_frames = frames;
  if (s_start_status != HAL_OK)
  {
    if (s_start_status == HAL_ERROR)
    {
      handle->State = HAL_SPI_STATE_BUSY_TX_RX;
    }
    return s_start_status;
  }
  if (s_stale_complete_on_prestart_restore)
  {
    s_stale_complete_on_prestart_restore = false;
    (void)usr_port_hal_spi_complete(&s_handle, USR_OK);
  }
  handle->State = HAL_SPI_STATE_BUSY_TX_RX;
  if ((kind == CALL_DMA_RX) || (kind == CALL_DMA_TX_RX))
  {
    s_dma[0].State = HAL_DMA_STATE_BUSY;
    s_streams[0].CR |= DMA_SxCR_EN;
  }
  if ((kind == CALL_DMA_TX) || (kind == CALL_DMA_TX_RX))
  {
    s_dma[1].State = HAL_DMA_STATE_BUSY;
    s_streams[1].CR |= DMA_SxCR_EN;
  }
  s_async_pending = true;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_SPI_Transmit_IT(SPI_HandleTypeDef *handle,
                                      const uint8_t *data, uint16_t frames)
{
  (void)data;
  return start_async(handle, frames, CALL_IT_TX);
}

HAL_StatusTypeDef HAL_SPI_Receive_IT(SPI_HandleTypeDef *handle, uint8_t *data,
                                     uint16_t frames)
{
  (void)data;
  return start_async(handle, frames, CALL_IT_RX);
}

HAL_StatusTypeDef HAL_SPI_TransmitReceive_IT(SPI_HandleTypeDef *handle,
                                             const uint8_t *tx, uint8_t *rx,
                                             uint16_t frames)
{
  (void)tx;
  (void)rx;
  return start_async(handle, frames, CALL_IT_TX_RX);
}

HAL_StatusTypeDef HAL_SPI_Transmit_DMA(SPI_HandleTypeDef *handle,
                                       const uint8_t *data, uint16_t frames)
{
  (void)data;
  return start_async(handle, frames, CALL_DMA_TX);
}

HAL_StatusTypeDef HAL_SPI_Receive_DMA(SPI_HandleTypeDef *handle, uint8_t *data,
                                      uint16_t frames)
{
  (void)data;
  return start_async(handle, frames, CALL_DMA_RX);
}

HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(SPI_HandleTypeDef *handle,
                                              const uint8_t *tx, uint8_t *rx,
                                              uint16_t frames)
{
  (void)tx;
  (void)rx;
  return start_async(handle, frames, CALL_DMA_TX_RX);
}

static HAL_StatusTypeDef poll(uint16_t frames, uint32_t timeout, uint32_t kind)
{
  s_calls[kind]++;
  s_last_frames = frames;
  s_last_timeout = timeout;
  s_time += s_poll_tick_advance;
  return s_poll_status;
}

HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *handle, const uint8_t *data,
                                   uint16_t frames, uint32_t timeout)
{
  (void)handle;
  (void)data;
  return poll(frames, timeout, CALL_POLL_TX);
}

HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *handle, uint8_t *data,
                                  uint16_t frames, uint32_t timeout)
{
  (void)handle;
  (void)data;
  return poll(frames, timeout, CALL_POLL_RX);
}

HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *handle,
                                          const uint8_t *tx, uint8_t *rx,
                                          uint16_t frames, uint32_t timeout)
{
  (void)handle;
  (void)tx;
  (void)rx;
  return poll(frames, timeout, CALL_POLL_TX_RX);
}

HAL_StatusTypeDef HAL_SPI_Abort(SPI_HandleTypeDef *handle)
{
  s_abort_calls++;
  s_async_pending = false;
  s_time += s_abort_tick_advance;
  if (s_abort_stuck)
  {
    return HAL_ERROR;
  }
  handle->State = HAL_SPI_STATE_READY;
  for (uint32_t index = 0u; index < 2u; index++)
  {
    if (s_dma[index].State == HAL_DMA_STATE_BUSY)
    {
      s_dma[index].State = HAL_DMA_STATE_READY;
      s_streams[index].CR &= ~DMA_SxCR_EN;
    }
  }
  return s_abort_failure ? HAL_ERROR : HAL_OK;
}

HAL_StatusTypeDef HAL_DMA_Abort(DMA_HandleTypeDef *handle)
{
  if (s_abort_stuck)
  {
    return HAL_ERROR;
  }
  handle->State = HAL_DMA_STATE_READY;
  ((DMA_Stream_TypeDef *)handle->Instance)->CR &= ~DMA_SxCR_EN;
  return HAL_OK;
}

void HAL_SPI_IRQHandler(SPI_HandleTypeDef *handle)
{
  if (s_async_behavior == ASYNC_IRQ_ERROR)
  {
    handle->ErrorCode = HAL_SPI_ERROR_OVR;
    for (uint32_t index = 0u; index < 2u; index++)
    {
      if (s_dma[index].State == HAL_DMA_STATE_BUSY)
      {
        s_dma[index].State = HAL_DMA_STATE_ABORT;
        s_streams[index].CR &= ~DMA_SxCR_EN;
      }
    }
  }
}

static void set_dma_resource(port_hal_dma_resource_t *resource,
                             DMA_HandleTypeDef *handle,
                             DMA_Stream_TypeDef *stream, IRQn_Type irq,
                             uint32_t direction)
{
  memset(resource, 0, sizeof(*resource));
  resource->handle = handle;
  resource->instance = stream;
  resource->irq = irq;
  resource->irq_priority = 5u;
  resource->clock_enable = clock_gate;
  resource->init.Mode = DMA_NORMAL;
  resource->init.Direction = direction;
}

static void fixture(spi_transfer_mode_t tx_mode, spi_transfer_mode_t rx_mode,
                    bool tx_dma, bool rx_dma)
{
  memset(&s_instance, 0, sizeof(s_instance));
  memset(&s_resource, 0, sizeof(s_resource));
  memset(&s_handle, 0, sizeof(s_handle));
  memset(&s_registers, 0, sizeof(s_registers));
  memset(s_dma, 0, sizeof(s_dma));
  memset(s_streams, 0, sizeof(s_streams));
  memset(&s_device, 0, sizeof(s_device));
  memset(s_calls, 0, sizeof(s_calls));
  memset(s_gpio_af, 0, sizeof(s_gpio_af));
  s_time = 0u;
  s_last_timeout = 0u;
  s_last_frames = 0u;
  s_abort_calls = 0u;
  s_dma_init_calls = 0u;
  s_dma_fail_step = 0u;
  s_dma_deinit_failure = 0u;
  s_irq_calls = 0u;
  s_spi_irq_enable_calls = 0u;
  s_spi_irq_disable_calls = 0u;
  s_spi_irq_clear_calls = 0u;
  s_gpio_calls = 0u;
  s_buffer_checks = 0u;
  s_last_buffer_receive = false;
  s_buffer_valid = true;
  s_async_pending = false;
  s_async_behavior = ASYNC_COMPLETE;
  s_start_status = HAL_OK;
  s_poll_status = HAL_OK;
  s_poll_tick_advance = 0u;
  s_abort_failure = false;
  s_abort_stuck = false;
  s_abort_tick_advance = 0u;
  s_complete_abort_on_tick = false;
  s_complete_on_irq_restore = false;
  s_stale_complete_on_prestart_restore = false;
  memset(s_dma_irq_disabled, 0, sizeof(s_dma_irq_disabled));
  spi_test_primask = 0u;
  spi_test_ipsr = 0u;
  spi_test_basepri = 0u;
  spi_test_faultmask = 0u;

  s_resource.instance = &s_registers;
  s_resource.clock_config = clock_config;
  s_resource.clock_enable = clock_gate;
  s_resource.clock_disable = clock_gate;
  s_resource.kernel_clock_hz = kernel_hz;
  s_resource.dma_buffer_valid = dma_buffer_valid;
  s_resource.irq = SPI3_IRQn;
  s_resource.irq_priority = 5u;
  s_resource.enable_irq = true;
  for (uint32_t index = 0u; index < 3u; index++)
  {
    s_resource.pins[index].port = GPIOB;
    s_resource.pins[index].pin = (uint16_t)(1u << (index + 3u));
    s_resource.pins[index].alternate = (index == 2u) ? 7u : 6u;
    s_resource.pins[index].clock_enable = clock_gate;
  }
  if (rx_dma)
  {
    set_dma_resource(&s_resource.rx_dma, &s_dma[0], &s_streams[0],
                     DMA2_Stream0_IRQn, DMA_PERIPH_TO_MEMORY);
  }
  if (tx_dma)
  {
    set_dma_resource(&s_resource.tx_dma, &s_dma[1], &s_streams[1],
                     DMA2_Stream1_IRQn, DMA_MEMORY_TO_PERIPH);
  }
  s_resource.default_config = (spi_device_config_t){
      .max_speed_hz = 6400000u,
      .mode = DEV_SPI_MODE_1,
      .bit_order = DEV_SPI_BIT_ORDER_MSB_FIRST,
      .data_width = DEV_SPI_DATA_BITS_8,
      .tx_mode = tx_mode,
      .rx_mode = rx_mode,
  };
  s_instance.resource = &s_resource;
  s_instance.handle = &s_handle;
  s_instance.device = &s_device;
}

static usr_status_t initialize(void)
{
  return port_hal_spi_ops.init(&s_instance, NULL);
}

static usr_status_t claim(void)
{
  return port_hal_spi_ops.claim(&s_instance);
}

static usr_status_t release(void)
{
  return port_hal_spi_ops.release(&s_instance);
}

static usr_status_t transfer(const uint8_t *tx, uint16_t tx_length, uint8_t *rx,
                             uint16_t rx_length, uint32_t timeout)
{
  return port_hal_spi_ops.transmit(&s_instance, tx, tx_length, rx, rx_length,
                                   timeout);
}

static void test_dma_modes(void)
{
  uint8_t tx[8] = {0};
  uint8_t rx[8] = {0};

  fixture(DEV_SPI_TRANSFER_DMA, DEV_SPI_TRANSFER_DMA, true, true);
  assert(initialize() == USR_OK);
  assert(s_dma_init_calls == 2u);
  assert(s_handle.hdmarx == &s_dma[0]);
  assert(s_handle.hdmatx == &s_dma[1]);
  assert(claim() == USR_OK);
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_OK);
  assert(transfer(NULL, 0u, rx, sizeof(rx), 10u) == USR_OK);
  assert(transfer(tx, sizeof(tx), rx, sizeof(rx), 10u) == USR_OK);
  assert(s_calls[CALL_DMA_TX] == 1u);
  assert(s_calls[CALL_DMA_RX] == 1u);
  assert(s_calls[CALL_DMA_TX_RX] == 1u);
  assert(s_buffer_checks == 4u);
  assert(usr_port_hal_spi_dma_irq(DMA2_Stream0_IRQn));
  assert(usr_port_hal_spi_dma_irq(DMA2_Stream1_IRQn));
  assert(s_irq_calls == 2u);
  assert(release() == USR_OK);
  assert(port_hal_spi_ops.deinit(&s_instance) == USR_OK);
}

static void test_independent_modes_and_fallback(void)
{
  uint8_t tx[8] = {0};
  uint8_t rx[8] = {0};

  fixture(DEV_SPI_TRANSFER_POLLING, DEV_SPI_TRANSFER_DMA, false, true);
  assert(initialize() == USR_OK);
  assert(s_dma_init_calls == 1u);
  assert(s_handle.hdmatx == NULL);
  assert(s_handle.hdmarx == &s_dma[0]);
  assert(claim() == USR_OK);
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_OK);
  assert(s_calls[CALL_POLL_TX] == 1u);
  assert(s_buffer_checks == 0u);
  assert(transfer(NULL, 0u, rx, sizeof(rx), 10u) == USR_OK);
  assert(s_calls[CALL_DMA_RX] == 1u);
  assert(s_buffer_checks == 1u && s_last_buffer_receive);
  s_buffer_valid = false;
  assert(transfer(tx, sizeof(tx), rx, sizeof(rx), 10u) == USR_OK);
  assert(s_calls[CALL_POLL_TX_RX] == 1u);
  assert(s_buffer_checks == 1u);
  s_buffer_valid = true;
  assert(transfer(tx, 2u, rx, sizeof(rx), 20u) == USR_OK);
  assert(s_calls[CALL_POLL_TX] == 2u);
  assert(s_calls[CALL_DMA_RX] == 2u);
  assert(s_buffer_checks == 2u);
}

static void test_interrupt_modes(void)
{
  uint8_t tx[4] = {0};
  uint8_t rx[4] = {0};

  fixture(DEV_SPI_TRANSFER_INTERRUPT, DEV_SPI_TRANSFER_INTERRUPT, false, false);
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_OK);
  assert(transfer(NULL, 0u, rx, sizeof(rx), 10u) == USR_OK);
  assert(transfer(tx, sizeof(tx), rx, sizeof(rx), 10u) == USR_OK);
  assert(s_calls[CALL_IT_TX] == 1u);
  assert(s_calls[CALL_IT_RX] == 1u);
  assert(s_calls[CALL_IT_TX_RX] == 1u);
  assert(s_buffer_checks == 0u);

  fixture(DEV_SPI_TRANSFER_INTERRUPT, DEV_SPI_TRANSFER_POLLING, false, false);
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  assert(transfer(tx, sizeof(tx), rx, sizeof(rx), 10u) == USR_OK);
  assert(s_calls[CALL_POLL_TX_RX] == 1u);
  assert(s_calls[CALL_IT_TX_RX] == 0u);
}

static void test_full_duplex_mode_matrix(void)
{
  static const spi_transfer_mode_t modes[] = {
      DEV_SPI_TRANSFER_POLLING,
      DEV_SPI_TRANSFER_INTERRUPT,
      DEV_SPI_TRANSFER_DMA,
  };
  uint8_t tx[4] = {0};
  uint8_t rx[4] = {0};

  for (uint32_t tx_index = 0u; tx_index < 3u; tx_index++)
  {
    for (uint32_t rx_index = 0u; rx_index < 3u; rx_index++)
    {
      spi_transfer_mode_t tx_mode = modes[tx_index];
      spi_transfer_mode_t rx_mode = modes[rx_index];
      fixture(tx_mode, rx_mode, tx_mode == DEV_SPI_TRANSFER_DMA,
              rx_mode == DEV_SPI_TRANSFER_DMA);
      assert(initialize() == USR_OK);
      assert(claim() == USR_OK);
      assert(transfer(tx, sizeof(tx), rx, sizeof(rx), 10u) == USR_OK);
      if ((tx_mode == DEV_SPI_TRANSFER_DMA) &&
          (rx_mode == DEV_SPI_TRANSFER_DMA))
      {
        assert(s_calls[CALL_DMA_TX_RX] == 1u);
        assert(s_buffer_checks == 2u);
      }
      else if ((tx_mode == DEV_SPI_TRANSFER_INTERRUPT) &&
               (rx_mode == DEV_SPI_TRANSFER_INTERRUPT))
      {
        assert(s_calls[CALL_IT_TX_RX] == 1u);
        assert(s_buffer_checks == 0u);
      }
      else
      {
        assert(s_calls[CALL_POLL_TX_RX] == 1u);
        assert(s_buffer_checks == 0u);
      }
    }
  }
}

static void test_tx_dma_only(void)
{
  uint8_t tx[4] = {0};
  uint8_t rx[4] = {0};

  fixture(DEV_SPI_TRANSFER_DMA, DEV_SPI_TRANSFER_POLLING, true, false);
  assert(initialize() == USR_OK);
  assert(s_dma_init_calls == 1u);
  assert(claim() == USR_OK);
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_OK);
  assert(s_calls[CALL_DMA_TX] == 1u);
  assert(transfer(NULL, 0u, rx, sizeof(rx), 10u) == USR_OK);
  assert(s_calls[CALL_POLL_RX] == 1u);
  assert(transfer(tx, sizeof(tx), rx, sizeof(rx), 10u) == USR_OK);
  assert(s_calls[CALL_POLL_TX_RX] == 1u);
}

static void test_invalid_resources(void)
{
  fixture(DEV_SPI_TRANSFER_DMA, DEV_SPI_TRANSFER_POLLING, false, false);
  assert(initialize() == USR_ERR_PARAM);
  fixture(DEV_SPI_TRANSFER_POLLING, DEV_SPI_TRANSFER_DMA, false, false);
  assert(initialize() == USR_ERR_PARAM);
  fixture(DEV_SPI_TRANSFER_INTERRUPT, DEV_SPI_TRANSFER_POLLING, false, false);
  s_resource.enable_irq = false;
  assert(initialize() == USR_ERR_PARAM);
  fixture(DEV_SPI_TRANSFER_DMA, DEV_SPI_TRANSFER_POLLING, true, false);
  s_resource.dma_buffer_valid = NULL;
  assert(initialize() == USR_ERR_PARAM);
}

static void test_context_and_timeout(void)
{
  uint8_t tx[4] = {0};

  fixture(DEV_SPI_TRANSFER_INTERRUPT, DEV_SPI_TRANSFER_POLLING, false, false);
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  spi_test_ipsr = 1u;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_ERR_STATE);
  spi_test_ipsr = 0u;
  spi_test_primask = 1u;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_ERR_STATE);
  spi_test_primask = 0u;
  spi_test_basepri = 1u;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_ERR_STATE);
  spi_test_basepri = 0u;
  spi_test_faultmask = 1u;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_ERR_STATE);
  spi_test_faultmask = 0u;
  assert(s_calls[CALL_IT_TX] == 0u);
  assert(transfer(tx, sizeof(tx), NULL, 0u, 0u) == USR_ERR_TIMEOUT);
  assert(s_calls[CALL_IT_TX] == 0u);

  fixture(DEV_SPI_TRANSFER_POLLING, DEV_SPI_TRANSFER_POLLING, false, false);
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  spi_test_ipsr = 1u;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_ERR_STATE);
  assert(s_calls[CALL_POLL_TX] == 0u);
}

static void test_claim_and_release_reject_invalid_context(void)
{
  uint32_t *masks[] = {
      &spi_test_ipsr,
      &spi_test_primask,
      &spi_test_basepri,
      &spi_test_faultmask,
  };

  fixture(DEV_SPI_TRANSFER_POLLING, DEV_SPI_TRANSFER_POLLING, false, false);
  assert(initialize() == USR_OK);
  for (uint32_t index = 0u; index < 4u; index++)
  {
    *masks[index] = 1u;
    assert(claim() == USR_ERR_STATE);
    assert(!s_instance.claimed);
    *masks[index] = 0u;
  }

  assert(claim() == USR_OK);
  for (uint32_t index = 0u; index < 4u; index++)
  {
    *masks[index] = 1u;
    assert(!port_hal_spi_ops.is_owner(&s_instance));
    assert(release() == USR_ERR_STATE);
    assert(s_instance.claimed);
    *masks[index] = 0u;
  }
  assert(release() == USR_OK);
}

static void test_shared_timeout_and_tick_wrap(void)
{
  uint8_t tx[4] = {0};
  uint8_t rx[8] = {0};

  fixture(DEV_SPI_TRANSFER_POLLING, DEV_SPI_TRANSFER_DMA, false, true);
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  s_poll_tick_advance = 4u;
  assert(transfer(tx, sizeof(tx), rx, sizeof(rx), 5u) == USR_ERR_TIMEOUT);
  assert(s_calls[CALL_POLL_TX] == 1u);
  assert(s_calls[CALL_DMA_RX] == 0u);
  assert(s_last_timeout < 5u);

  fixture(DEV_SPI_TRANSFER_DMA, DEV_SPI_TRANSFER_DMA, true, true);
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  s_time = UINT32_MAX - 2u;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_OK);
}

static void test_timeout_error_and_late_callback(void)
{
  uint8_t tx[4] = {0};

  fixture(DEV_SPI_TRANSFER_DMA, DEV_SPI_TRANSFER_DMA, true, true);
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  s_async_behavior = ASYNC_TIMEOUT;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 3u) == USR_ERR_TIMEOUT);
  assert(s_abort_calls == 1u);
  assert(s_instance.timeout_count == 1u);
  assert(s_instance.transfer_state == USR_PORT_SPI_IDLE);
  assert(usr_port_hal_spi_complete(&s_handle, USR_OK));
  assert(s_instance.completed_count == 0u);
  s_async_behavior = ASYNC_COMPLETE;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_OK);
  assert(s_instance.completed_count == 1u);

  s_async_behavior = ASYNC_ERROR;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_ERR_BUS);
  assert(s_abort_calls == 2u);
  assert(s_instance.error_count == 1u);
}

static void test_polling_failure_runs_recovery(void)
{
  uint8_t tx[4] = {0};

  fixture(DEV_SPI_TRANSFER_POLLING, DEV_SPI_TRANSFER_POLLING, false, false);
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  s_poll_status = HAL_TIMEOUT;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_ERR_TIMEOUT);
  assert(s_abort_calls == 1u);
  assert(s_instance.timeout_count == 1u);
  assert(s_instance.transfer_state == USR_PORT_SPI_IDLE);

  s_poll_status = HAL_ERROR;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_ERR_BUS);
  assert(s_abort_calls == 2u);
  assert(s_instance.error_count == 1u);
  assert(s_instance.transfer_state == USR_PORT_SPI_IDLE);

  s_poll_status = HAL_TIMEOUT;
  s_abort_stuck = true;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_ERR_BUS);
  assert(s_instance.faulted != 0u);
  assert(s_instance.transfer_state == USR_PORT_SPI_FAULTED);
}

static void test_polling_recovery_does_not_touch_disabled_spi_irq(void)
{
  uint8_t tx[4] = {0};

  fixture(DEV_SPI_TRANSFER_POLLING, DEV_SPI_TRANSFER_POLLING, false, false);
  s_resource.enable_irq = false;
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  s_poll_status = HAL_TIMEOUT;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_ERR_TIMEOUT);
  assert(s_abort_calls == 1u);
  assert(s_instance.transfer_state == USR_PORT_SPI_IDLE);
  assert(s_spi_irq_enable_calls == 0u);
  assert(s_spi_irq_disable_calls == 0u);
  assert(s_spi_irq_clear_calls == 0u);
}

static void test_stale_callback_cannot_complete_before_start(void)
{
  uint8_t tx[4] = {0};

  fixture(DEV_SPI_TRANSFER_DMA, DEV_SPI_TRANSFER_DMA, true, true);
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  s_stale_complete_on_prestart_restore = true;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_OK);
  assert(!s_stale_complete_on_prestart_restore);
  assert(!s_async_pending);
  assert(s_handle.State == HAL_SPI_STATE_READY);
  assert(s_instance.completed_count == 1u);
}

static void test_dma_error_recovery_budget_starts_at_spi_irq(void)
{
  uint8_t tx[4] = {0};

  fixture(DEV_SPI_TRANSFER_DMA, DEV_SPI_TRANSFER_DMA, true, true);
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  s_async_behavior = ASYNC_IRQ_ERROR;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 1000u) == USR_ERR_BUS);
  assert(s_time < 50u);
  assert(s_instance.error_count == 1u);
  assert(s_instance.faulted != 0u);
  assert(s_instance.transfer_state == USR_PORT_SPI_FAULTED);
}

static void test_blocking_abort_must_fit_recovery_budget(void)
{
  uint8_t tx[4] = {0};

  fixture(DEV_SPI_TRANSFER_DMA, DEV_SPI_TRANSFER_DMA, true, true);
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  s_async_behavior = ASYNC_ERROR;
  s_abort_tick_advance = PORT_HAL_SPI_RECOVERY_TIMEOUT_MS + 1u;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 1000u) == USR_ERR_BUS);
  assert(s_instance.faulted != 0u);
  assert(s_instance.transfer_state == USR_PORT_SPI_FAULTED);
}

static void test_timeout_publishes_aborting_before_irq_restore(void)
{
  uint8_t tx[4] = {0};

  fixture(DEV_SPI_TRANSFER_DMA, DEV_SPI_TRANSFER_DMA, true, true);
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  s_async_behavior = ASYNC_TIMEOUT;
  s_complete_on_irq_restore = true;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 3u) == USR_ERR_TIMEOUT);
  assert(s_instance.completed_count == 0u);
  assert(s_instance.timeout_count == 1u);
}

static void test_start_and_recovery_failures(void)
{
  uint8_t tx[4] = {0};

  fixture(DEV_SPI_TRANSFER_DMA, DEV_SPI_TRANSFER_DMA, true, true);
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  s_start_status = HAL_BUSY;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_BUSY);
  assert(s_abort_calls == 0u);
  assert(s_instance.transfer_state == USR_PORT_SPI_IDLE);
  s_start_status = HAL_ERROR;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_ERR_BUS);
  assert(s_abort_calls == 1u);
  s_start_status = HAL_OK;
  s_async_behavior = ASYNC_TIMEOUT;
  s_abort_stuck = true;
  assert(transfer(tx, sizeof(tx), NULL, 0u, 2u) == USR_ERR_BUS);
  assert(s_instance.faulted != 0u);
  assert(s_instance.transfer_state == USR_PORT_SPI_FAULTED);
  assert(transfer(tx, sizeof(tx), NULL, 0u, 10u) == USR_ERR_STATE);
  assert(release() == USR_OK);
  s_abort_stuck = false;
  assert(port_hal_spi_ops.deinit(&s_instance) == USR_OK);
  assert(initialize() == USR_OK);
}

static void test_hal_dma_abort_it_can_converge(void)
{
  uint8_t tx[4] = {0};

  fixture(DEV_SPI_TRANSFER_DMA, DEV_SPI_TRANSFER_DMA, true, true);
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  s_async_behavior = ASYNC_ERROR;
  s_complete_abort_on_tick = true;
  assert(transfer(tx, sizeof(tx), tx, sizeof(tx), 10u) == USR_ERR_BUS);
  assert(s_instance.faulted == 0u);
  assert(s_dma[1].State == HAL_DMA_STATE_READY);
  assert(s_instance.transfer_state == USR_PORT_SPI_IDLE);
}

static void test_width_and_init_rollback(void)
{
  uint16_t tx[4] = {0};
  uint16_t rx[4] = {0};

  fixture(DEV_SPI_TRANSFER_DMA, DEV_SPI_TRANSFER_DMA, true, true);
  s_resource.default_config.data_width = DEV_SPI_DATA_BITS_16;
  assert(initialize() == USR_OK);
  assert(s_dma[0].Init.MemDataAlignment == DMA_MDATAALIGN_HALFWORD);
  assert(s_dma[1].Init.MemDataAlignment == DMA_MDATAALIGN_HALFWORD);
  assert(claim() == USR_OK);
  assert(transfer((uint8_t *)tx, sizeof(tx), (uint8_t *)rx, sizeof(rx), 10u) ==
         USR_OK);
  assert(s_last_frames == 4u);
  assert(transfer((uint8_t *)tx, 3u, NULL, 0u, 10u) == USR_ERR_PARAM);
  assert(transfer((uint8_t *)tx + 1u, 2u, NULL, 0u, 10u) == USR_ERR_PARAM);

  fixture(DEV_SPI_TRANSFER_DMA, DEV_SPI_TRANSFER_DMA, true, true);
  s_dma_fail_step = 2u;
  assert(initialize() == USR_ERR_BUS);
  assert(s_handle.hdmarx == NULL && s_handle.hdmatx == NULL);
  assert(s_instance.rx_dma_ready == 0u && s_instance.tx_dma_ready == 0u);
}

static void test_claim_and_deinit_recovery(void)
{
  fixture(DEV_SPI_TRANSFER_DMA, DEV_SPI_TRANSFER_DMA, true, true);
  assert(initialize() == USR_OK);
  assert(claim() == USR_OK);
  assert(claim() == USR_BUSY);
  assert(release() == USR_OK);
  s_dma_deinit_failure = 1u;
  assert(port_hal_spi_ops.deinit(&s_instance) == USR_ERR_BUS);
  assert(s_instance.faulted != 0u);
  assert(s_instance.initialized != 0u);
  s_dma_deinit_failure = 0u;
  assert(port_hal_spi_ops.deinit(&s_instance) == USR_OK);
  assert(s_instance.faulted == 0u);
  assert(s_instance.initialized == 0u);
}

int main(void)
{
  test_dma_modes();
  test_independent_modes_and_fallback();
  test_interrupt_modes();
  test_full_duplex_mode_matrix();
  test_tx_dma_only();
  test_invalid_resources();
  test_context_and_timeout();
  test_claim_and_release_reject_invalid_context();
  test_shared_timeout_and_tick_wrap();
  test_timeout_error_and_late_callback();
  test_polling_failure_runs_recovery();
  test_polling_recovery_does_not_touch_disabled_spi_irq();
  test_stale_callback_cannot_complete_before_start();
  test_dma_error_recovery_budget_starts_at_spi_irq();
  test_blocking_abort_must_fit_recovery_budget();
  test_timeout_publishes_aborting_before_irq_restore();
  test_start_and_recovery_failures();
  test_hal_dma_abort_it_can_converge();
  test_width_and_init_rollback();
  test_claim_and_deinit_recovery();
  puts("port_hal_spi_test: PASS (polling, interrupt, DMA, recovery)");
  return 0;
}
