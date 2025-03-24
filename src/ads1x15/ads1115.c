#include "ads1115.h"
#include "pico/stdlib.h"
#include <stdbool.h>

static i2c_inst_t *ads_i2c = NULL;
static uint8_t ads_addr = ADS1115_ADDRESS;

void ads1115_init(i2c_inst_t *i2c, uint8_t addr) {
    ads_i2c = i2c;
    ads_addr = addr;
}

int16_t ads1115_read_channel(uint8_t channel) {
    if (channel > 3) return 0;

    // Config register for single-shot conversion on channel
    uint16_t config = 0x8483 | (channel << 12);  // 0x8483 = 4.096V, single-shot, 128SPS

    uint8_t config_bytes[3] = {
        0x01,               // Config register
        (config >> 8) & 0xFF,
        config & 0xFF
    };
    i2c_write_blocking(ads_i2c, ads_addr, config_bytes, 3, false);

    // Wait for conversion (depends on data rate, ~8ms at 128SPS)
    sleep_ms(9);

    uint8_t pointer = 0x00; // Conversion register
    i2c_write_blocking(ads_i2c, ads_addr, &pointer, 1, true);

    uint8_t result[2];
    i2c_read_blocking(ads_i2c, ads_addr, result, 2, false);

    return (int16_t)((result[0] << 8) | result[1]);
}

float ads1115_raw_to_voltage(int16_t raw) {
    return raw * 4.096f / 32768.0f;  // Gain = ±4.096V
}
