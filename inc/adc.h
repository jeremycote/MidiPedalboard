//
// Created by Jeremy Cote on 2025-03-08.
//

#ifndef MIDICONTROLLER_ADC_H
#define MIDICONTROLLER_ADC_H

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

bool adc_init();

bool adc_start_conversion(adc_input_t input);

bool adc_conversion_complete();

bool adc_read_conversion();

/**
 * Get adc value, converted to a range of 0...255
 * @param input
 * @return
 */
uint8_t adc_get(adc_input_t input);

#endif //MIDICONTROLLER_ADC_H
