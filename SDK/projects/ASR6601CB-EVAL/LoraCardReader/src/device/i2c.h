#ifndef I2C_H_
#define I2C_H_

#include <stdio.h>
#include <stdint.h>
#include "tremo_rcc.h"
#include "tremo_gpio.h"
#include "tremo_i2c.h"

extern void device_i2c_init(void);
extern size_t device_i2c_write(uint8_t addr, size_t count, uint8_t *data);
extern void device_i2c_begin_read(uint8_t addr, size_t count);
extern uint8_t device_i2c_read();
extern bool device_i2c_read_available();
extern void device_i2c_stop_read();
extern void device_i2c_stop_read();
extern void delay(uint32_t nms);
extern void cr_reset();
void cr_pin_test();

#endif