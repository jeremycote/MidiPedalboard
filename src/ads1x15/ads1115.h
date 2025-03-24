#ifndef ADS1115_H
#define ADS1115_H
#include <stdint.h>

#include "hardware/i2c.h"

#define ADS1115_ADDRESS 0x4A 

void ads1115_init(i2c_inst_t *i2c, uint8_t addr);
int16_t ads1115_read_channel(uint8_t channel);  // 0–3 = A0–A3
float ads1115_raw_to_voltage(int16_t raw);

#endif
