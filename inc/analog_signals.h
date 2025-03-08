//
// Created by Jeremy Cote on 2025-03-08.
//

#ifndef MIDICONTROLLER_ANALOG_SIGNALS_H
#define MIDICONTROLLER_ANALOG_SIGNALS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ADC_IN0 = 0,
    ADC_IN1,
    ADC_IN2,
    ADC_IN3,
    ADC_IN4,
    ADC_IN5
} adc_input_t;

bool analog_init();

bool analog_start_conversion(adc_input_t input);

bool analog_conversion_complete();

bool analog_read_conversion();

/**
 * Get adc value, converted to a range of 0...255
 * @param input
 * @return
 */
uint8_t analog_get(adc_input_t input);

#endif //MIDICONTROLLER_ANALOG_SIGNALS_H
