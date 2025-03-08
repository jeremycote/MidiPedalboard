//
// Created by Jeremy Cote on 2025-03-08.
//

#include "analog_signals.h"

#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <hardware/adc.h>

// Address is already shifted left
#define ADC_ADDRESS (0x90)

// TODO: Double check this is the correct i2c channel
#define I2C i2c_default

// TODO: Select correct pin
#define CONVERSION_READY_PIN 2
#define ADC_IN4_PIN 0
#define ADC_IN5_PIN 0

#define TIMEOUT 1000

// Order is important. Must match ADS1115 datasheet.
typedef enum {
    CONVERSION_REGISTER = 0,
    CONFIG_REGISTER,
    LOW_THRESHOLD_REGISTER,
    HIGH_THRESHOLD_REGISTER
} ads1115_register_t;

typedef enum {
    ADC_IDLE = 0,
    ADC_CONVERTING,
    ADC_COMPLETE
} adc_state_t;

// Global ADC storage
volatile adc_state_t state = ADC_IDLE;
volatile uint16_t adc_results[6] = {0};  // Stores ADC values for each input
volatile adc_input_t current_channel = ADC_IN0;  // Tracks the current ADC input being read

static bool write_register(ads1115_register_t reg, uint16_t value) {
    uint8_t data[3] = {reg, value >> 8, value};

    if (i2c_write_timeout_us(I2C, ADC_ADDRESS, data, 3, false, TIMEOUT) < 0) {
        return false;
    }

    return true;
}

static bool read_register(ads1115_register_t reg, volatile uint16_t *result) {
    uint8_t data[1] = {reg};

    if (i2c_write_timeout_us(I2C, ADC_ADDRESS, data, 1, false, TIMEOUT) < 0) {
        return false;
    }

    // Directly writing should work because both systems are little endian
    if (i2c_read_timeout_us(I2C, ADC_ADDRESS, (uint8_t*) result, 2, false, TIMEOUT) < 0) {
        return false;
    }

    return true;
}

static void conversion_complete() {
    if (state == ADC_CONVERTING) {
        state = ADC_COMPLETE;
    }
}

bool analog_init() {
    // Setup external ADC
    i2c_init(I2C, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    // Set ready pin to rise on conversion ready
    write_register(HIGH_THRESHOLD_REGISTER, 0x0100);
    write_register(LOW_THRESHOLD_REGISTER, 0x0000);

    // Enable interrupt for conversion ready pin
    gpio_init(CONVERSION_READY_PIN);
    gpio_set_dir(CONVERSION_READY_PIN, GPIO_IN);
    gpio_pull_up(CONVERSION_READY_PIN);
    gpio_set_irq_enabled_with_callback(CONVERSION_READY_PIN, GPIO_IRQ_EDGE_RISE, true, &conversion_complete);

    // Setup internal ADC
    adc_init();

    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(ADC_IN4_PIN);
    adc_gpio_init(ADC_IN5_PIN);

    return true;
}

static bool ads1115_start_conversion(adc_input_t input) {
    uint16_t config = 0;

    // Start one shot conversion
    config |= 0x8000;

    // Select single ended input
    config |= 0x4000;

    // Select input
    config |= input << 12;

    // Select gain (+2.048V)
    config |= 2 << 12;

    // Select single shot mode
    config |= 0x0100;

    // Select data rate (128SPS)
    config |= 4 << 5;

    return write_register(CONFIG_REGISTER, config);
}

static void internal_adc_start_conversion(adc_input_t input) {
    adc_select_input(input - 4);
    adc_results[input] = adc_read();
}

bool analog_start_conversion(adc_input_t input) {
    if (state != ADC_IDLE) {
        return false;
    }

    bool started_conversion = false;

    // Set current channel to know where to send result
    current_channel = input;

    if (input <= 3) {
        // Use external ADC (ads1115)
        if (!ads1115_start_conversion(input)) {
            return false;
        }

        state = ADC_CONVERTING;
    } else {
        // Use internal ADC
        internal_adc_start_conversion(input);
    }

    return true;
}

bool analog_conversion_complete() {
    return state == ADC_COMPLETE;
}

bool analog_read_conversion() {
    if (!read_register(CONVERSION_REGISTER, &adc_results[current_channel])) {
        return false;
    }

    state = ADC_IDLE;
    return true;
}

uint8_t analog_get(adc_input_t input) {
    if (input <= 3)
        // External adc is 16 bits
        return adc_results[input] * 255 / 0xFFFF;
    else
        // Internal adc is 12 bits
        return adc_results[input] * 255 / 0x0FFF;
}
