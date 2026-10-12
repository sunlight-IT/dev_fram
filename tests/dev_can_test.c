#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dev_can.h"

typedef struct
{
  uint32_t init_calls;
  uint32_t send_calls;
  uint32_t recv_calls;
  dev_can_config_t init_config;
  dev_can_frame_t sent_frame;
  usr_status_t init_status;
  usr_status_t send_status;
  usr_status_t recv_status;
} fake_can_t;

static usr_status_t fake_init(void *ctx, const void *cfg)
{
  fake_can_t *fake = (fake_can_t *)ctx;

  fake->init_calls++;
  if (cfg != NULL)
  {
    fake->init_config = *(const dev_can_config_t *)cfg;
  }
  return fake->init_status;
}

static usr_status_t fake_deinit(void *ctx)
{
  (void)ctx;
  return USR_OK;
}

static usr_status_t fake_send(void *ctx, const void *frame,
                              uint32_t timeout_ms)
{
  fake_can_t *fake = (fake_can_t *)ctx;

  (void)timeout_ms;
  fake->send_calls++;
  fake->sent_frame = *(const dev_can_frame_t *)frame;
  return fake->send_status;
}

static usr_status_t fake_recv(void *ctx, void *frame)
{
  fake_can_t *fake = (fake_can_t *)ctx;

  fake->recv_calls++;
  if (fake->recv_status == USR_OK)
  {
    *(dev_can_frame_t *)frame = fake->sent_frame;
  }
  return fake->recv_status;
}

static usr_status_t fake_get_status(void *ctx, void *status)
{
  (void)ctx;
  memset(status, 0, sizeof(dev_can_status_t));
  return USR_OK;
}

static const dev_can_ops_t s_fake_ops =
{
  .init = fake_init,
  .deinit = fake_deinit,
  .send = fake_send,
  .recv = fake_recv,
  .get_status = fake_get_status,
};

static dev_can_config_t test_config(void)
{
  dev_can_config_t config = {0};

  config.format = DEV_CAN_FORMAT_CLASSIC;
  config.direction = DEV_CAN_DIRECTION_TX_RX;
  config.rx_mode = DEV_CAN_RX_INTERRUPT;
  config.nominal.prescaler = 4u;
  config.nominal.sync_jump_width = 1u;
  config.nominal.time_seg1 = 11u;
  config.nominal.time_seg2 = 4u;
  config.data.prescaler = 2u;
  config.data.sync_jump_width = 1u;
  config.data.time_seg1 = 5u;
  config.data.time_seg2 = 2u;
  config.filter.enabled = true;
  config.filter.id_type = DEV_CAN_ID_STANDARD;
  config.filter.identifier = 0x321u;
  config.filter.mask = 0x7ffu;
  return config;
}

static can_device_t test_device(fake_can_t *fake)
{
  can_device_t device = {0};

  device.ctx = fake;
  device.ops = &s_fake_ops;
  return device;
}

static void test_init_commits_only_after_success(void)
{
  fake_can_t fake = {0};
  can_device_t device = test_device(&fake);
  dev_can_config_t config = test_config();

  fake.init_status = USR_ERR_BUS;
  assert(dev_can_init(&device, &config) == USR_ERR_BUS);
  assert(fake.init_calls == 1u);
  assert(!device.initialized);

  fake.init_status = USR_OK;
  assert(dev_can_init(&device, &config) == USR_OK);
  assert(device.initialized);
  assert(device.config.filter.identifier == 0x321u);
}

static void test_classic_mode_ignores_data_timing(void)
{
  fake_can_t fake = {0};
  can_device_t device = test_device(&fake);
  dev_can_config_t config = test_config();

  config.data.prescaler = 0u;
  config.data.sync_jump_width = 0u;
  config.data.time_seg1 = 0u;
  config.data.time_seg2 = 0u;
  assert(dev_can_init(&device, &config) == USR_OK);
  assert(fake.init_calls == 1u);
}

static void test_fd_mode_requires_data_timing(void)
{
  fake_can_t fake = {0};
  can_device_t device = test_device(&fake);
  dev_can_config_t config = test_config();

  config.format = DEV_CAN_FORMAT_FD;
  config.data.prescaler = 0u;
  assert(dev_can_init(&device, &config) == USR_ERR_PARAM);
  assert(fake.init_calls == 0u);
}

static void test_invalid_nominal_timing_is_rejected(void)
{
  fake_can_t fake = {0};
  can_device_t device = test_device(&fake);
  dev_can_config_t config = test_config();

  config.nominal.prescaler = 0u;
  assert(dev_can_init(&device, &config) == USR_ERR_PARAM);
  config.nominal.prescaler = 513u;
  assert(dev_can_init(&device, &config) == USR_ERR_PARAM);
  config.nominal.prescaler = 4u;
  config.nominal.sync_jump_width = 5u;
  assert(dev_can_init(&device, &config) == USR_ERR_PARAM);
  assert(fake.init_calls == 0u);
}

static void test_rejects_invalid_frame_before_ops(void)
{
  fake_can_t fake = {0};
  can_device_t device = test_device(&fake);
  dev_can_config_t config = test_config();
  dev_can_frame_t frame = {0};

  assert(dev_can_init(&device, &config) == USR_OK);
  frame.id_type = DEV_CAN_ID_STANDARD;
  frame.id = 0x800u;
  frame.data_len = 1u;
  assert(dev_can_send(&device, &frame, 10u) == USR_ERR_PARAM);
  assert(fake.send_calls == 0u);

  frame.id = 0x321u;
  frame.data_len = 9u;
  assert(dev_can_send(&device, &frame, 10u) == USR_ERR_PARAM);
  assert(fake.send_calls == 0u);
}

static void test_accepts_fd_length_encoding(void)
{
  fake_can_t fake = {0};
  can_device_t device = test_device(&fake);
  dev_can_config_t config = test_config();
  dev_can_frame_t frame = {0};

  config.format = DEV_CAN_FORMAT_FD;
  assert(dev_can_init(&device, &config) == USR_OK);
  frame.id_type = DEV_CAN_ID_EXTENDED;
  frame.id = 0x12345678u;
  frame.format = DEV_CAN_FORMAT_FD;
  frame.data_len = 12u;
  frame.data[11] = 0xa5u;
  assert(dev_can_send(&device, &frame, 10u) == USR_OK);
  assert(fake.send_calls == 1u);
  assert(fake.sent_frame.data_len == 12u);
  assert(fake.sent_frame.data[11] == 0xa5u);
}

static void test_recv_propagates_empty_status(void)
{
  fake_can_t fake = {0};
  can_device_t device = test_device(&fake);
  dev_can_config_t config = test_config();
  dev_can_frame_t frame = {0};

  fake.recv_status = USR_BUSY;
  assert(dev_can_init(&device, &config) == USR_OK);
  assert(dev_can_recv(&device, &frame) == USR_BUSY);
  assert(fake.recv_calls == 1u);
}

int main(void)
{
  test_init_commits_only_after_success();
  test_classic_mode_ignores_data_timing();
  test_fd_mode_requires_data_timing();
  test_invalid_nominal_timing_is_rejected();
  test_rejects_invalid_frame_before_ops();
  test_accepts_fd_length_encoding();
  test_recv_propagates_empty_status();
  puts("dev_can_test: PASS");
  return 0;
}
