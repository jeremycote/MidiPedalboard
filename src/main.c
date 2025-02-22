//
// Created by Jeremy Cote on 2025-02-13.
//

#include <pico/stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <pico/printf.h>
#include <pico/cyw43_arch.h>
#include <pico/stdio_usb.h>

#include "communication_task.h"

int main() {
    stdio_init_all();

    while (!stdio_usb_connected()) {

    }

    printf("Starting Midi Controller\n");

    // Start tasks
    start_communication_task();

    // Hand off control to FreeRTOS scheduler
    vTaskStartScheduler();

    printf("ERROR: Scheduler exited!\n");

    // Should never exit from scheduler
    // TODO: If reached, reboot the system
    while (1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char * pcTaskName ) {
    while(1) {
        // TODO: Implement visual feedback of stack overflow
    }
}