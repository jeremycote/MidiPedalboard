//
// Created by Jeremy Cote on 2025-03-08.
//

#include "analog_signals.h"

#include <FreeRTOS.h>
#include "task.h"

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

#define SAMPLE_SIZE 5

volatile float adc_results[6 * SAMPLE_SIZE] = {0};  // Stores ADC values for each input
volatile uint8_t adc_counts[6] = {0};

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
    vTaskDelay(9);

    uint8_t pointer = 0x00; // Conversion register
    i2c_write_blocking(I2C, ADC_ADDRESS, &pointer, 1, true);

    uint8_t result[2];
    i2c_read_blocking(I2C, ADC_ADDRESS, result, 2, false);

    int16_t v = (result[0] << 8) | result[1];
    float converted = ((v * 4.096f / 32768.0f) + 0.58f) / 2.7f;
    return converted > 1 ? 1 : converted < 0 ? 0 : converted;
}

static float internal_adc_read(adc_input_t input) {
    adc_select_input(input - 4);
    float raw = (float) adc_read(); // 12-bit ADC result (0–4095)
    return raw / 4095.0;
}

bool analog_read(adc_input_t input) {
    float result;
    if (input <= 3) {
        result = ads1115_read(input);
    } else {
        result = internal_adc_read(input);
    }

    adc_results[input * SAMPLE_SIZE + adc_counts[input]] = result;
    adc_counts[input] = (adc_counts[input] + 1) % SAMPLE_SIZE;

    // Remove
    float total = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        total += adc_results[input * SAMPLE_SIZE + i];
    }
    total /= SAMPLE_SIZE;
    printf("Voltage (%d): %f%%\n", input, total);
    // End remove

    return true;
}

uint8_t analog_get(adc_input_t input) {

    float total = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        total += adc_results[input * SAMPLE_SIZE + i];
    }
    total /= SAMPLE_SIZE;

    uint8_t v = total * 127;

    return v > 127 ? 127 : v < 0 ? 0 : v;
}
