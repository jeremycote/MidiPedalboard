//
// Created by Jeremy Cote on 2025-02-13.
//

#include <pico/stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <pico/printf.h>
#include <pico/stdio_usb.h>

#include "communication_task.h"
#include "adc_task.h"

int main() {
    stdio_init_all();

    // TODO: Remove this for production. Used for debugging.
    while (!stdio_usb_connected()) {

    }

    printf("Starting Midi Controller\n");

    // Start tasks
    start_communication_task();
    start_adc_task();

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