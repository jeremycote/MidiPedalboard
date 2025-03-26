//
// Created by Jeremy Cote on 2025-03-08.
//

#include "analog_signals.h"

#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/adc.h>
#include <pico/printf.h>

// Address is already shifted left
#define ADC_ADDRESS (0x4A)

#define I2C i2c1

#define ADC_IN4_PIN 31
#define ADC_IN5_PIN 32

// Global ADC storage
volatile uint16_t adc_results[6] = {0};  // Stores ADC values for each input

bool analog_init() {
    // Setup internal ADC
    adc_init();
    adc_gpio_init(ADC_IN4_PIN);
    adc_gpio_init(ADC_IN5_PIN);

    return true;
}

static float ads1115_read(uint8_t channel) {
    if (channel > 3) return 0;

    // Config register for single-shot conversion on channel
    uint16_t config = 0x8483 | (channel << 12);  // 0x8483 = 4.096V, single-shot, 128SPS

    uint8_t config_bytes[3] = {
            0x01,               // Config register
            (config >> 8) & 0xFF,
            config & 0xFF
    };
    i2c_write_blocking(I2C, ADC_ADDRESS, config_bytes, 3, false);

    // Wait for conversion (depends on data rate, ~8ms at 128SPS)
    sleep_ms(9);

    uint8_t pointer = 0x00; // Conversion register
    i2c_write_blocking(I2C, ADC_ADDRESS, &pointer, 1, true);

    uint8_t result[2];
    i2c_read_blocking(I2C, ADC_ADDRESS, result, 2, false);

    float v = (result[0] << 8) | result[1];

    return v * 4.096f / 32768.0f;
}

static uint8_t internal_adc_read(adc_input_t input) {
    adc_select_input(input - 4);
    float raw = (float) adc_read(); // 12-bit ADC result (0–4095)
    return (uint8_t ) (raw * 255.0 / 4095.0);
}

bool analog_read(adc_input_t input) {
    uint8_t result;
    if (input <= 3) {
        float v = ads1115_read(input);
        printf("Voltage: %f\n", v);
    } else {
        result = internal_adc_read(input);
    }

    adc_results[input] = result;

    return true;
}

uint8_t analog_get(adc_input_t input) {
    return adc_results[input];
}
