//
// Created by Jeremy Cote on 2025-02-13.
//

#include "adc_task.h"

#include <pico/printf.h>
#include "FreeRTOS.h"
#include "task.h"

#include "analog_signals.h"

#include "pico/stdlib.h"
#include <ssd1306.h>
#include <stdio.h>
#include "analog_signals.h"
#include "../lib/pico-ssd1306/ssd1306.h"
#include "communication.h"
#include <stdio.h>
#include <hardware/adc.h>

#define DELAY 500 // in milliseconds

#define ADC_TASK_STACK_SIZE 4096

StaticTask_t adc_task_buffer;
StackType_t adc_task_stack[ADC_TASK_STACK_SIZE];

_Noreturn void adc_task();

void start_adc_task() {
    printf("Starting ADC Task\n");
    TaskHandle_t task = xTaskCreateStatic(adc_task, "ADC", ADC_TASK_STACK_SIZE, NULL, 2, adc_task_stack,
                      &adc_task_buffer);

//    vTaskCoreAffinitySet(task, 0b01);
}

_Noreturn void adc_task() {
    analog_init();

    ssd1306_t oled;
    oled.external_vcc = false;

    printf("Init screen\n");
    bool res = ssd1306_init(&oled, 128, 32, 0x3C, i2c1);

    char buffer[32];

    while (true) {
        for (int i = 3; i < 6; i += 2) {
            analog_read(i);
            vTaskDelay(20);
        }


        uint8_t sound = analog_get(ADC_IN5);

        // Format string for OLED
        snprintf(buffer, sizeof(buffer), "Sound level : %d", sound);

        // Clear, draw, and display on OLED
        ssd1306_clear(&oled);
        if (sound > 25) {
            ssd1306_draw_string(&oled, 0, 2, 1.5, "SOUND WARNING");
        } else {
            ssd1306_draw_string(&oled, 0, 2, 1.5, "Sound good");
        }

        ssd1306_draw_string(&oled, 0, 10, 1, buffer);

        ssd1306_draw_string(&oled, 0, 20, 1, is_connected() ? "Wifi Connected" : "No Wifi");

        ssd1306_show(&oled);

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