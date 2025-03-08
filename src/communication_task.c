//
// Created by Jeremy Cote on 2025-02-13.
//

#include "communication_task.h"

#include <pico/printf.h>
#include <lwip/sockets.h>
#include "FreeRTOS.h"
#include "task.h"

#include "communication.h"

// TODO: Calculate proper stack size
#define COMMUNICATION_TASK_STACK_SIZE 2048

StaticTask_t communication_task_buffer;
StackType_t communication_task_stack[COMMUNICATION_TASK_STACK_SIZE];

_Noreturn void communication_task();

void start_communication_task() {
    printf("Starting Communication Task\n");
    TaskHandle_t task = xTaskCreateStatic(communication_task, "Comms", COMMUNICATION_TASK_STACK_SIZE, NULL, 1, communication_task_stack,
                      &communication_task_buffer);
    vTaskCoreAffinitySet(task, 0b11); // Wifi can only run on core 0
}

_Noreturn void communication_task() {
    printf("Setup Wifi\n");

    while (!setup_wifi()) {
        printf("Failed to setup wifi. Trying again in 500ms\n");
        vTaskDelay(500);
    }

    printf("Setup midi server\n");

    while (!setup_midi_server()) {
        printf("Failed to setup midi server. Trying again in 500ms\n");
        vTaskDelay(500);
    }

    while (!setup_bonjour()) {
        printf("Failed to setup bonjour. Trying again in 500ms\n");
        vTaskDelay(500);
    }

    printf("Start communication loop\n");

    while (1) {
        // TODO: This only answers the first connection request
        handle_incoming_packets();
        vTaskDelay(1);
    }
}