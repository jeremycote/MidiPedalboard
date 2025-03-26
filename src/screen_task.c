//
// Created by Jeremy Cote on 2025-03-24.
//

#include "screen_task.h"

#include "FreeRTOS.h"
#include "task.h"

#include "analog_signals.h"

#include <ssd1306.h>
#include <stdio.h>
#include "analog_signals.h"
#include "../lib/pico-ssd1306/ssd1306.h"

#define DELAY 500 // in milliseconds

#define SCREEN_TASK_STACK_SIZE 4024

StaticTask_t screen_task_buffer;
StackType_t screen_task_stack[SCREEN_TASK_STACK_SIZE];

_Noreturn void screen_task();

void start_screen_task() {
    printf("Starting Screen Task\n");
    TaskHandle_t task = xTaskCreateStatic(screen_task, "Screen", SCREEN_TASK_STACK_SIZE, NULL, 1, screen_task_stack,
                                          &screen_task_buffer);

    vTaskCoreAffinitySet(task, 0b01);
}

_Noreturn void screen_task() {
    printf("Hello from screen task\n");
    ssd1306_t oled;
    oled.external_vcc = false;

    printf("Init screen\n");
    bool res = ssd1306_init(&oled, 128, 32, 0x3C, i2c1);

    // If setup is OK, write the test text on the OLED
    if (res) {
        printf("Clearing Screen\n");
        ssd1306_clear(&oled);
        ssd1306_draw_string(&oled, 0, 2, 2, "Hello!");
        ssd1306_draw_line(&oled, 2, 25, 80, 25);
        ssd1306_show(&oled);
    } else {
        printf("OLED Init failed\n");
    }

    char buffer[32];

    while (true) {
        uint8_t sound = analog_get(ADC_IN5);

        // Format string for OLED
        snprintf(buffer, sizeof(buffer), "Sound level : %d", sound);

        // Clear, draw, and display on OLED
        ssd1306_clear(&oled);
        if (sound > 130) {
            ssd1306_draw_string(&oled, 0, 2, 1.5, "SOUND WARNING");
        } else {
            ssd1306_draw_string(&oled, 0, 2, 1.5, "Sound good");
        }

        ssd1306_draw_string(&oled, 0, 15, 1, buffer);
        ssd1306_show(&oled);

        vTaskDelay(250); // Delay for readability
    }
}