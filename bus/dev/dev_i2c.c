/**
  ******************************************************************************
  * @file    usr_bus_i2c.c
  * @brief   Platform-independent I2C access with configurable recovery.
  ******************************************************************************
  */
#include "dev_i2c.h"

// static uint32_t dev_i2c_timeout(const i2c_device_t *bus,
//                                     uint32_t timeout_ms)
// {
//   return (timeout_ms != 0u) ? timeout_ms : bus->cfg.default_timeout_ms;
// }

static usr_status_t dev_i2c_lock(i2c_device_t *i2c)
{
  usr_status_t status;

  if (i2c->in_use)
  {
    return USR_BUSY;
  }
  i2c->in_use = true;
//   if (i2c->fault && (i2c->backend.ops->recover != NULL))
//   {
//     status = i2c->backend.ops->recover(i2c->backend.ctx);
//     if (status != USR_OK)
//     {
//       i2c->in_use = false;
//       return status;
//     }
//     i2c->fault = false;
//   }
  return USR_OK;
}

static usr_status_t dev_i2c_retry(i2c_device_t *i2c,
                                      usr_status_t (*operation)(void *arg),
                                      void *arg)
{
  usr_status_t status = operation(arg);
//   uint8_t retry = 0u;

//   while ((status != USR_OK) && (retry < bus->cfg.recovery_retries) &&
//          (bus->backend.ops->recover != NULL))
//   {
//     status = bus->backend.ops->recover(bus->backend.ctx);
//     if (status != USR_OK)
//     {
//       break;
//     }
//     retry++;
//     status = operation(arg);
//   }
//   bus->fault = (status != USR_OK);
//   bus->in_use = false;
  return status;
}

typedef struct
{
  i2c_device_t *bus;
  uint8_t addr7;
  uint16_t reg;
  uint8_t reg_len;
  uint8_t *read_data;
  const uint8_t *write_data;
  uint16_t len;
  uint32_t timeout_ms;
} usr_bus_i2c_request_t;

usr_status_t dev_i2c_init(i2c_device_t *i2c)
{
  if ((i2c == NULL) || (i2c->ops == NULL) || (i2c->ops->init == NULL))
  {
    return USR_ERR_PARAM;
  }
  if (i2c->in_use)
  {
    return USR_BUSY;
  }
  return i2c->ops->init(i2c->ctx);
}

// static usr_status_t usr_bus_i2c_read_once(void *arg)
// {
//   usr_bus_i2c_request_t *request = (usr_bus_i2c_request_t *)arg;

//   return request->bus->backend.ops->mem_read(
//       request->bus->backend.ctx, request->addr7, request->reg,
//       request->reg_len, request->read_data, request->len, request->timeout_ms);
// }

// static usr_status_t usr_bus_i2c_write_once(void *arg)
// {
//   usr_bus_i2c_request_t *request = (usr_bus_i2c_request_t *)arg;

//   return request->bus->backend.ops->mem_write(
//       request->bus->backend.ctx, request->addr7, request->reg,
//       request->reg_len, request->write_data, request->len, request->timeout_ms);
// }

// usr_status_t dev_i2c_init(usr_bus_i2c_t *bus, usr_i2c_backend_t backend,
//                               const usr_bus_i2c_cfg_t *cfg)
// {
//   if ((bus == NULL) || (backend.ops == NULL) || (cfg == NULL) ||
//       (cfg->default_timeout_ms == 0u) || (cfg->probe_trials == 0u))
//   {
//     return USR_ERR_PARAM;
//   }
//   bus->backend = backend;
//   bus->cfg = *cfg;
//   bus->in_use = false;
//   bus->fault = false;
//   if (backend.ops->init != NULL)
//   {
//     return backend.ops->init(backend.ctx);
//   }
//   return USR_OK;
// }

usr_status_t dev_i2c_read_reg(i2c_device_t *i2c, uint8_t addr7,
                                  uint16_t reg, uint8_t reg_len,
                                  uint8_t *data, uint16_t len,
                                  uint32_t timeout_ms)
{
  usr_status_t status;
  const dev_i2c_ops_t *ops;

  if ((i2c == NULL) || (i2c->ops == NULL) ||
      (i2c->ops->mem_read == NULL) || (addr7 > 0x7fu) ||
      ((reg_len != 1u) && (reg_len != 2u)) || (data == NULL) || (len == 0u))
  {
    return USR_ERR_PARAM;
  }
  
  status = dev_i2c_lock(i2c);
  if (status != USR_OK)
  {
    return status;
  }

  ops = i2c->ops;
//   request.bus = bus;
//   request.addr7 = addr7;
//   request.reg = reg;
//   request.reg_len = reg_len;
//   request.read_data = data;
//   request.write_data = NULL;
//   request.len = len;
//   request.timeout_ms = usr_bus_i2c_timeout(bus, timeout_ms);
//   return usr_bus_i2c_retry(bus,c_read_once,c_read_once, &request);
  status = ops->mem_read(i2c->ctx, addr7, reg, reg_len, data, len, timeout_ms);
  i2c->in_use = false;
  return status;
}

usr_status_t dev_i2c_write_reg(i2c_device_t *i2c, uint8_t addr7,
                                   uint16_t reg, uint8_t reg_len,
                                   const uint8_t *data, uint16_t len,
                                   uint32_t timeout_ms)
{
  usr_status_t status;
  const dev_i2c_ops_t *ops;

  if ((i2c == NULL) || (i2c->ops == NULL) ||
      (i2c->ops->mem_write == NULL) || (addr7 > 0x7fu) ||
      ((reg_len != 1u) && (reg_len != 2u)) || (data == NULL) || (len == 0u))
  {
    return USR_ERR_PARAM;
  }
  status = dev_i2c_lock(i2c);
  if (status != USR_OK)
  {
    return status;
  }

  ops = i2c->ops;
//  request.bus = bus;
//   request.addr7 = addr7;
//   request.reg = reg;
//   request.reg_len = reg_len;
//   request.read_data = NULL;
//   request.write_data = data;
//   request.len = len;
//   request.timeout_ms = usr_bus_i2c_timeout(bus, timeout_ms);
//   return usr_bus_i2c_retry(bus, usr_bus_i2c_write_once, &request);
  status = ops->mem_write(i2c->ctx, addr7, reg, reg_len, data, len, timeout_ms);
  i2c->in_use = false;
  return status;
}

usr_status_t dev_i2c_transmit(i2c_device_t *i2c, uint8_t addr7,
                                   const uint8_t *data, uint16_t len,
                                   uint32_t timeout_ms){
  usr_status_t status;
  const dev_i2c_ops_t *ops;

  if ((i2c == NULL) || (i2c->ops == NULL) ||
      (i2c->ops->transmit == NULL) || (addr7 > 0x7fu) ||
      (data == NULL) || (len == 0u))
  {
    return USR_ERR_PARAM;
  }
  status = dev_i2c_lock(i2c);
  if (status != USR_OK)
  {
    return status;
  }

  ops = i2c->ops;
//  request.bus = bus;
//   request.addr7 = addr7;
//   request.reg = reg;
//   request.reg_len = reg_len;
//   request.read_data = NULL;
//   request.write_data = data;
//   request.len = len;
//   request.timeout_ms = usr_bus_i2c_timeout(bus, timeout_ms);
//   return usr_bus_i2c_retry(bus, usr_bus_i2c_write_once, &request);
  status = ops->transmit(i2c->ctx, addr7, data, len, timeout_ms);
  i2c->in_use = false;
  return status;
}

usr_status_t dev_i2c_receive(i2c_device_t *i2c, uint8_t addr7,
                                   uint8_t *data, uint16_t len,
                                   uint32_t timeout_ms){
  usr_status_t status;
  const dev_i2c_ops_t *ops;

  if ((i2c == NULL) || (i2c->ops == NULL) ||
      (i2c->ops->receive == NULL) || (addr7 > 0x7fu) ||
      (data == NULL) || (len == 0u))
  {
    return USR_ERR_PARAM;
  }
  status = dev_i2c_lock(i2c);
  if (status != USR_OK)
  {
    return status;
  }

  ops = i2c->ops;
//  request.bus = bus;
//   request.addr7 = addr7;
//   request.reg = reg;
//   request.reg_len = reg_len;
//   request.read_data = NULL;
//   request.write_data = data;
//   request.len = len;
//   request.timeout_ms = usr_bus_i2c_timeout(bus, timeout_ms);
//   return usr_bus_i2c_retry(bus, usr_bus_i2c_write_once, &request);
  status = ops->receive(i2c->ctx, addr7, data, len, timeout_ms);
  i2c->in_use = false;
  return status;
}


usr_status_t dev_i2c_probe(i2c_device_t *i2c, uint8_t addr7,
                               uint32_t timeout_ms)
{
  usr_status_t status;

  if ((i2c == NULL) || (i2c->ops == NULL) ||
      (i2c->ops->probe == NULL) || (addr7 > 0x7fu))
  {
    return USR_ERR_PARAM;
  }
  status = dev_i2c_lock(i2c);
  if (status != USR_OK)
  {
    return status;
  }
  status = i2c->ops->probe(i2c->ctx, addr7,
                                   10,
                                   timeout_ms);
//   i2c->fault = ((status != USR_OK) && (status != USR_ERR_NO_DEV));
  i2c->in_use = false;
  return status;
}

