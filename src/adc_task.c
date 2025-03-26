//
// Created by Jeremy Cote on 2025-02-13.
//

#include "adc_task.h"

#include <pico/printf.h>
#include "FreeRTOS.h"
#include "task.h"

#include "analog_signals.h"

#include "pico/stdlib.h"
#include <stdio.h>
#include <hardware/adc.h>

#define DELAY 500 // in milliseconds

#define ADC_TASK_STACK_SIZE 1024

StaticTask_t adc_task_buffer;
StackType_t adc_task_stack[ADC_TASK_STACK_SIZE];

_Noreturn void adc_task();

void start_adc_task() {
    printf("Starting ADC Task\n");
    TaskHandle_t task = xTaskCreateStatic(adc_task, "ADC", ADC_TASK_STACK_SIZE, NULL, 1, adc_task_stack,
                      &adc_task_buffer);

    vTaskCoreAffinitySet(task, 0b01);
}

_Noreturn void adc_task() {
    analog_init();

    while (true) {
        for (int i = 2; i < 3; i++) {
            analog_read(i);
            vTaskDelay(10);
        }

//        printf("ADC: %u\n", analog_get(2));

//        printf(
//                "ADC\n1) %u\n2) %u\n3) %u\n4) %u\n5) %u\n6) %u\n",
//                analog_get(0),
//                analog_get(1),
//                analog_get(2),
//                analog_get(3),
//                analog_get(4),
//                analog_get(5)
//        );
    }
}