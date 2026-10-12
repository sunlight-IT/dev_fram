#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Compile the real adapter in this translation unit to test private helpers. */
#include "../port_bsp/port_hal_can.c"

uint32_t can_test_primask;
static FDCAN_GlobalTypeDef s_registers;
static FDCAN_HandleTypeDef s_handle;
static can_device_t s_device;
static dev_can_frame_t s_storage[4];
static usr_port_hal_can_resource_t s_resource;
static usr_port_hal_can_instance_t s_instance;
static uint32_t s_rx_pending;
static uint32_t s_rx_dlc;
static uint32_t s_rx_location;
static uint32_t s_notifications;
static uint32_t s_init_calls;
static uint32_t s_deinit_calls;
static uint32_t s_stop_calls;
static uint32_t s_tick;
static uint32_t s_tx_free;
static uint32_t s_fail_step;
static bool s_inject_rx;
static FDCAN_TxHeaderTypeDef s_tx_header;
static FDCAN_FilterTypeDef s_filter;
static uint8_t s_tx_data[64];

usr_port_hal_can_instance_t *usr_port_board_can_find_instance(
    const FDCAN_HandleTypeDef *handle)
{
  return handle == &s_handle ? &s_instance : NULL;
}

static HAL_StatusTypeDef test_clock_config(void)
{
  return HAL_OK;
}

static void test_clock_gate(void)
{
}

void Error_Handler(void)
{
  assert(false);
}

HAL_StatusTypeDef HAL_FDCAN_Init(FDCAN_HandleTypeDef *handle)
{
  s_init_calls++;
  handle->State = HAL_FDCAN_STATE_READY;
  return s_fail_step == 1u ? HAL_ERROR : HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_DeInit(FDCAN_HandleTypeDef *handle)
{
  s_deinit_calls++;
  handle->State = HAL_FDCAN_STATE_RESET;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_ConfigGlobalFilter(FDCAN_HandleTypeDef *handle,
    uint32_t std_policy, uint32_t ext_policy, uint32_t remote_std,
    uint32_t remote_ext)
{
  (void)handle;
  (void)remote_std;
  (void)remote_ext;
  assert(std_policy == s_resource.nonmatching_std);
  assert(ext_policy == s_resource.nonmatching_ext);
  return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_ConfigFilter(FDCAN_HandleTypeDef *handle,
    const FDCAN_FilterTypeDef *filter)
{
  (void)handle;
  s_filter = *filter;
  return s_fail_step == 2u ? HAL_ERROR : HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_Start(FDCAN_HandleTypeDef *handle)
{
  handle->State = HAL_FDCAN_STATE_BUSY;
  return s_fail_step == 3u ? HAL_ERROR : HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_Stop(FDCAN_HandleTypeDef *handle)
{
  s_stop_calls++;
  handle->State = HAL_FDCAN_STATE_READY;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_ActivateNotification(FDCAN_HandleTypeDef *handle,
    uint32_t notifications, uint32_t buffers)
{
  (void)buffers;
  s_notifications = notifications;
  if (s_inject_rx)
  {
    s_rx_pending = 1u;
    HAL_FDCAN_RxFifo0Callback(handle, FDCAN_IT_RX_FIFO0_NEW_MESSAGE);
  }
  return s_fail_step == 4u ? HAL_ERROR : HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_DeactivateNotification(FDCAN_HandleTypeDef *handle,
    uint32_t notifications)
{
  (void)handle;
  (void)notifications;
  return HAL_OK;
}

uint32_t HAL_GetTick(void)
{
  return s_tick++;
}

uint32_t HAL_FDCAN_GetTxFifoFreeLevel(const FDCAN_HandleTypeDef *handle)
{
  (void)handle;
  return s_tx_free;
}

HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *handle,
    const FDCAN_TxHeaderTypeDef *header, const uint8_t *payload)
{
  (void)handle;
  s_tx_header = *header;
  memcpy(s_tx_data, payload, sizeof(s_tx_data));
  return HAL_OK;
}

uint32_t HAL_FDCAN_GetError(const FDCAN_HandleTypeDef *handle)
{
  return handle->ErrorCode;
}

uint32_t HAL_FDCAN_GetRxFifoFillLevel(const FDCAN_HandleTypeDef *handle,
    uint32_t fifo)
{
  (void)handle;
  assert(fifo == s_resource.rx_location);
  return s_rx_pending;
}

uint32_t HAL_FDCAN_IsRxBufferMessageAvailable(FDCAN_HandleTypeDef *handle,
    uint32_t index)
{
  uint32_t mask = 1u << (index & 31u);
  volatile uint32_t *flags = index < 32u ?
                            &handle->Instance->NDAT1 : &handle->Instance->NDAT2;
  uint32_t available = (*flags & mask) != 0u;

  *flags &= ~mask;
  return available;
}

HAL_StatusTypeDef HAL_FDCAN_GetRxMessage(FDCAN_HandleTypeDef *handle,
    uint32_t location, FDCAN_RxHeaderTypeDef *header, uint8_t *payload)
{
  (void)handle;
  s_rx_location = location;
  memset(header, 0, sizeof(*header));
  header->Identifier = 0x321u;
  header->IdType = FDCAN_STANDARD_ID;
  header->RxFrameType = FDCAN_DATA_FRAME;
  header->FDFormat = FDCAN_FD_CAN;
  header->DataLength = s_rx_dlc;
  memset(payload, 0xa5, 64u);
  if (s_rx_pending != 0u)
  {
    s_rx_pending--;
  }
  return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_GetProtocolStatus(const FDCAN_HandleTypeDef *handle,
    FDCAN_ProtocolStatusTypeDef *status)
{
  (void)handle;
  memset(status, 0, sizeof(*status));
  return HAL_OK;
}

void HAL_GPIO_Init(GPIO_TypeDef *port, const GPIO_InitTypeDef *init)
{
  (void)port;
  (void)init;
}

void HAL_GPIO_DeInit(GPIO_TypeDef *port, uint32_t pins)
{
  (void)port;
  (void)pins;
}

void HAL_NVIC_SetPriority(IRQn_Type irq, uint32_t priority, uint32_t subpriority)
{
  (void)irq;
  (void)priority;
  (void)subpriority;
}

void HAL_NVIC_EnableIRQ(IRQn_Type irq) { (void)irq; }
void HAL_NVIC_DisableIRQ(IRQn_Type irq) { (void)irq; }
void HAL_NVIC_ClearPendingIRQ(IRQn_Type irq) { (void)irq; }

static void reset_fixture(void)
{
  memset(&s_handle, 0, sizeof(s_handle));
  memset(&s_registers, 0, sizeof(s_registers));
  memset(&s_device, 0, sizeof(s_device));
  memset(&s_instance, 0, sizeof(s_instance));
  memset(&s_resource, 0, sizeof(s_resource));
  s_rx_pending = s_init_calls = s_deinit_calls = s_stop_calls = 0u;
  s_tick = s_fail_step = 0u;
  s_inject_rx = false;
  can_test_primask = 0u;
  s_tx_free = 1u;
  s_rx_dlc = FDCAN_DLC_BYTES_8;
  s_resource.instance = &s_registers;
  s_resource.rx_gpio_port = GPIOA;
  s_resource.tx_gpio_port = GPIOA;
  s_resource.rx_gpio_pin = GPIO_PIN_11;
  s_resource.tx_gpio_pin = GPIO_PIN_12;
  s_resource.clock_config = test_clock_config;
  s_resource.clock_enable = test_clock_gate;
  s_resource.clock_disable = test_clock_gate;
  s_resource.rx_gpio_clock_enable = test_clock_gate;
  s_resource.tx_gpio_clock_enable = test_clock_gate;
  s_resource.mode = FDCAN_MODE_NORMAL;
  s_resource.message_ram_word_capacity = 2560u;
  s_resource.std_filter_count = 1u;
  s_resource.ext_filter_count = 1u;
  s_resource.rx_fifo0_element_count = 16u;
  s_resource.rx_fifo0_element_size = FDCAN_DATA_BYTES_8;
  s_resource.tx_fifo_queue_count = 8u;
  s_resource.tx_fifo_queue_mode = FDCAN_TX_FIFO_OPERATION;
  s_resource.tx_element_size = FDCAN_DATA_BYTES_64;
  s_resource.filter_config = FDCAN_FILTER_TO_RXFIFO0;
  s_resource.rx_location = FDCAN_RX_FIFO0;
  s_resource.nonmatching_std = FDCAN_REJECT;
  s_resource.nonmatching_ext = FDCAN_REJECT;
  s_resource.rx_queue_storage = s_storage;
  s_resource.rx_queue_capacity = 4u;
  s_resource.default_config.direction = DEV_CAN_DIRECTION_TX_RX;
  s_resource.default_config.rx_mode = DEV_CAN_RX_INTERRUPT;
  s_resource.default_config.nominal.prescaler = 4u;
  s_resource.default_config.nominal.sync_jump_width = 1u;
  s_resource.default_config.nominal.time_seg1 = 11u;
  s_resource.default_config.nominal.time_seg2 = 4u;
  s_resource.default_config.filter.enabled = true;
  s_resource.default_config.filter.identifier = 0x321u;
  s_resource.default_config.filter.mask = 0x7ffu;
  s_instance.resource = &s_resource;
  s_instance.handle = &s_handle;
  s_instance.device = &s_device;
}

static usr_status_t initialize(void)
{
  return port_hal_can_ops.init(&s_instance, &s_resource.default_config);
}

static void test_zero_size_in_unallocated_regions(void)
{
  reset_fixture();
  assert(initialize() == USR_OK);
  assert(s_init_calls == 1u);
  assert(s_handle.Init.RxFifo0ElmtsNbr == 16u);
  assert(s_handle.Init.RxFifo0ElmtSize == FDCAN_DATA_BYTES_8);
  assert(s_handle.Init.RxBuffersNbr == 0u);
  assert(s_handle.Init.RxBufferSize == 0u);
}

static void test_resource_drives_mode_layout_and_filter(void)
{
  reset_fixture();
  s_resource.mode = FDCAN_MODE_INTERNAL_LOOPBACK;
  s_resource.rx_fifo0_element_count = 0u;
  s_resource.rx_fifo0_element_size = 0u;
  s_resource.rx_fifo1_element_count = 3u;
  s_resource.rx_fifo1_element_size = FDCAN_DATA_BYTES_12;
  s_resource.rx_location = FDCAN_RX_FIFO1;
  s_resource.filter_config = FDCAN_FILTER_TO_RXFIFO1;
  s_resource.tx_buffer_count = 2u;
  s_resource.tx_event_count = 1u;
  s_resource.tx_fifo_queue_count = 5u;
  s_resource.tx_fifo_queue_mode = FDCAN_TX_QUEUE_OPERATION;
  s_resource.tx_element_size = FDCAN_DATA_BYTES_16;
  assert(initialize() == USR_OK);
  assert(s_handle.Init.Mode == FDCAN_MODE_INTERNAL_LOOPBACK);
  assert(s_handle.Init.RxFifo1ElmtsNbr == 3u);
  assert(s_handle.Init.RxFifo1ElmtSize == FDCAN_DATA_BYTES_12);
  assert(s_handle.Init.TxBuffersNbr == 2u);
  assert(s_handle.Init.TxEventsNbr == 1u);
  assert(s_handle.Init.TxFifoQueueElmtsNbr == 5u);
  assert(s_handle.Init.TxFifoQueueMode == FDCAN_TX_QUEUE_OPERATION);
  assert(s_handle.Init.TxElmtSize == FDCAN_DATA_BYTES_16);
  assert(s_filter.FilterConfig == FDCAN_FILTER_TO_RXFIFO1);
  assert((s_notifications & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) != 0u);
  s_rx_pending = 1u;
  HAL_FDCAN_RxFifo1Callback(&s_handle, FDCAN_IT_RX_FIFO1_NEW_MESSAGE);
  {
    dev_can_frame_t frame;
    assert(port_hal_can_ops.recv(&s_instance, &frame) == USR_OK);
    assert(frame.id == 0x321u && frame.data_len == 8u);
    assert(s_rx_location == FDCAN_RX_FIFO1);
  }
}

static void test_notification_does_not_erase_first_frame(void)
{
  dev_can_frame_t frame;

  reset_fixture();
  s_inject_rx = true;
  assert(initialize() == USR_OK);
  assert(port_hal_can_ops.recv(&s_instance, &frame) == USR_OK);
  assert(frame.id == 0x321u);
}

static void test_buffer_route_requires_exact_mask(void)
{
  reset_fixture();
  s_resource.rx_fifo0_element_count = 0u;
  s_resource.rx_buffer_count = 2u;
  s_resource.rx_buffer_size = FDCAN_DATA_BYTES_8;
  s_resource.rx_location = FDCAN_RX_BUFFER1;
  s_resource.filter_config = FDCAN_FILTER_TO_RXBUFFER;
  s_resource.default_config.filter.mask = 0x700u;
  assert(initialize() == USR_ERR_UNSUPPORTED);
  assert(s_deinit_calls == 1u);
  s_resource.default_config.filter.mask = 0x7ffu;
  assert(initialize() == USR_OK);
  assert(s_filter.RxBufferIndex == 1u);
  s_handle.Instance->NDAT1 = 1u << 1u;
  HAL_FDCAN_RxBufferNewMessageCallback(&s_handle);
  {
    dev_can_frame_t frame;
    assert(port_hal_can_ops.recv(&s_instance, &frame) == USR_OK);
    assert(s_rx_location == FDCAN_RX_BUFFER1);
  }
}

static void test_ram_overflow_rejected_before_hal(void)
{
  reset_fixture();
  s_resource.message_ram_offset = UINT32_MAX;
  assert(initialize() == USR_ERR_PARAM);
  assert(s_init_calls == 0u);
  s_resource.message_ram_offset = 2500u;
  assert(initialize() == USR_ERR_PARAM);
  assert(s_init_calls == 0u);
}

static void test_queue_full_wrap_and_order(void)
{
  dev_can_frame_t input = {0};
  dev_can_frame_t output;
  uint32_t index;

  reset_fixture();
  for (index = 0u; index < 4u; index++)
  {
    input.id = index;
    assert(port_hal_can_queue_push(&s_instance, &input));
  }
  assert(!port_hal_can_queue_push(&s_instance, &input));
  assert(s_instance.rx_queue_overflow == 1u);
  for (index = 0u; index < 4u; index++)
  {
    assert(port_hal_can_queue_pop(&s_instance, &output));
    assert(output.id == index);
  }
  s_instance.rx_read_pos = s_instance.rx_write_pos = UINT16_MAX - 1u;
  input.id = 42u;
  assert(port_hal_can_queue_push(&s_instance, &input));
  assert(port_hal_can_queue_pop(&s_instance, &output));
  assert(output.id == 42u);
}

static void test_dlc_and_polling(void)
{
  static const uint8_t lengths[] = {
    0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u,
    8u, 12u, 16u, 20u, 24u, 32u, 48u, 64u,
  };
  dev_can_frame_t frame = {0};
  uint32_t index;

  reset_fixture();
  s_resource.rx_fifo0_element_size = FDCAN_DATA_BYTES_64;
  s_resource.default_config.rx_mode = DEV_CAN_RX_POLLING;
  assert(initialize() == USR_OK);
  for (index = 0u; index < 16u; index++)
  {
    frame.data_len = lengths[index];
    assert(port_hal_can_ops.send(&s_instance, &frame, 0u) == USR_OK);
    assert(s_tx_header.DataLength == index);
    s_rx_pending = 1u;
    s_rx_dlc = index;
    assert(port_hal_can_ops.recv(&s_instance, &frame) == USR_OK);
    assert(frame.data_len == lengths[index]);
  }
  assert(port_hal_can_ops.recv(&s_instance, &frame) == USR_BUSY);
}

static void test_timeout_and_failure_cleanup(void)
{
  dev_can_frame_t frame = {0};
  uint32_t step;

  for (step = 1u; step <= 4u; step++)
  {
    reset_fixture();
    s_fail_step = step;
    assert(initialize() == USR_ERR_BUS);
    assert(s_deinit_calls == 1u);
    assert(s_instance.lifecycle == USR_PORT_CAN_LIFECYCLE_RESET);
  }
  reset_fixture();
  assert(initialize() == USR_OK);
  s_tx_free = 0u;
  assert(port_hal_can_ops.send(&s_instance, &frame, 0u) == USR_BUSY);
  assert(port_hal_can_ops.send(&s_instance, &frame, 3u) == USR_ERR_TIMEOUT);
  assert(port_hal_can_ops.deinit(&s_instance) == USR_OK);
  assert(s_stop_calls == 1u && s_deinit_calls == 1u);
  assert(initialize() == USR_OK);
}

int main(void)
{
  test_zero_size_in_unallocated_regions();
  test_resource_drives_mode_layout_and_filter();
  test_notification_does_not_erase_first_frame();
  test_buffer_route_requires_exact_mask();
  test_ram_overflow_rejected_before_hal();
  test_queue_full_wrap_and_order();
  test_dlc_and_polling();
  test_timeout_and_failure_cleanup();
  puts("port_hal_can_test: PASS (8 scenarios)");
  return 0;
}
