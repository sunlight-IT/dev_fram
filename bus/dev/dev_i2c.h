#ifndef __DEV_I2C_H__
#define __DEV_I2C_H__

#include "dri_ops.h"

typedef struct i2c_device{
    device parent;
    void *ctx;
    bool in_use;
    const dev_i2c_ops_t *ops;
} i2c_device_t;

usr_status_t dev_i2c_init(i2c_device_t *i2c);

usr_status_t dev_i2c_read_reg(i2c_device_t *i2c, uint8_t addr7,
                                  uint16_t reg, uint8_t reg_len,
                                  uint8_t *data, uint16_t len,
                                  uint32_t timeout_ms);

usr_status_t dev_i2c_write_reg(i2c_device_t *i2c, uint8_t addr7,
                                   uint16_t reg, uint8_t reg_len,
                                   const uint8_t *data, uint16_t len,
                                   uint32_t timeout_ms);

usr_status_t dev_i2c_transmit(i2c_device_t *i2c, uint8_t addr7,
                                   const uint8_t *data, uint16_t len,
                                   uint32_t timeout_ms);

usr_status_t dev_i2c_receive(i2c_device_t *i2c, uint8_t addr7,
uint8_t *data, uint16_t len,
uint32_t timeout_ms);

usr_status_t dev_i2c_probe(i2c_device_t *bus, uint8_t addr7,
                               uint32_t timeout_ms);





#endif
