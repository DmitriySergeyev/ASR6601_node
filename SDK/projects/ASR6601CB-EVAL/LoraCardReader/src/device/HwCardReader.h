#ifndef HW_CR_H_
#define HW_CR_H_

#include <stdio.h>
#include <stdint.h>
#include "tremo_delay.h"
#include "tremo_rcc.h"
#include "tremo_gpio.h"
#include "tremo_i2c.h"

extern void hwCR_Init(void);
extern size_t hwCR_Write(uint8_t addr, size_t count, uint8_t *data);
extern void hwCR_ReadBegin(uint8_t addr, size_t count);
extern uint8_t hwCR_Read();
extern bool hwCR_ReadAvailable();
extern void hwCR_ReadStop();
extern void hwCR_Reset();

#endif