//
// Created by Jeremy Cote on 2025-02-13.
//

#include <pico/stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <pico/stdio_usb.h>

#include "communication_task.h"
#include "adc_task.h"
#include "screen_task.h"

#include <ssd1306.h>
#include <stdio.h>
#include "../lib/pico-ssd1306/ssd1306.h"
#include "buttons.h"

int main() {
    stdio_init_all();

    // TODO: Remove this for production. Used for debugging.
    while (!stdio_usb_connected()) {

    }

    i2c_init(i2c1, 400000);
    gpio_pull_up(2);
    gpio_pull_up(3);
    gpio_set_function(2, GPIO_FUNC_I2C);
    gpio_set_function(3, GPIO_FUNC_I2C);

    init_buttons();

    // Start tasks
    start_communication_task();
    start_adc_task();
    start_screen_task();

    printf("Starting Scheduler!\n");
    // Hand off control to FreeRTOS scheduler
    vTaskStartScheduler();

    printf("ERROR: Scheduler exited!\n");

    // Should never exit from scheduler
    // TODO: If reached, reboot the system
    while (1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    // Handle the stack overflow here.
    printf("Stack overflow detected in task: %s\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for(;;);
}