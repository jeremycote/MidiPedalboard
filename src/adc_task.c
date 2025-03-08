//
// Created by Jeremy Cote on 2025-02-13.
//

#include "adc_task.h"

#include <pico/printf.h>
#include "FreeRTOS.h"
#include "task.h"

#include "adc.h"

#define ADC_TASK_STACK_SIZE 1024

StaticTask_t adc_task_buffer;
StackType_t adc_task_stack[ADC_TASK_STACK_SIZE];

_Noreturn void adc_task();

void start_adc_task() {
    printf("Starting ADC Task\n");
    TaskHandle_t task = xTaskCreateStatic(adc_task, "ADC", ADC_TASK_STACK_SIZE, NULL, 1, adc_task_stack,
                      &adc_task_buffer);
}

_Noreturn void adc_task() {

    while (!adc_init()) {
        printf("Failed to initialize ADC\n");
    }

    uint8_t timeout = 0;
    adc_input_t current_input = ADC_IN0;

    while (1) {
        printf("Reading ADC %u\n", current_input);

        while (!adc_start_conversion(current_input) && timeout++ < 3) {
            vTaskDelay(5);
        }

        if (timeout >= 3)
            goto next;

        timeout = 0;
        vTaskDelay(1);

        while (!adc_conversion_complete() && timeout++ < 10) {
            vTaskDelay(5);
        }

        if (timeout >= 10)
            goto next;

        timeout = 0;
        while (!adc_read_conversion() && timeout++ < 3) {
            vTaskDelay(5);
        }

        if (timeout >= 3)
            goto next;

        printf("ADC %u value: %u\n", current_input, adc_get(current_input));

        next:
            timeout = 0;
            current_input = (current_input + 1) % 6;
            vTaskDelay(10);
    }
}