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
#define COMMUNICATION_TASK_STACK_SIZE 4096

StaticTask_t communication_task_buffer;
StackType_t communication_task_stack[COMMUNICATION_TASK_STACK_SIZE];

_Noreturn void communication_task();

void start_communication_task() {
    printf("Starting Communication Task\n");
    TaskHandle_t task = xTaskCreateStatic(communication_task, "Comms", COMMUNICATION_TASK_STACK_SIZE, NULL, 1, communication_task_stack,
                      &communication_task_buffer);
    vTaskCoreAffinitySet(task, 0b01);
}

_Noreturn void communication_task() {
    printf("Setup Wifi\n");

    while (!setup_wifi()) {
        printf("Failed to setup wifi. Trying again in 500ms\n");
        vTaskDelay(500);
    }

    while (!connect_wifi()) {
        printf("Failed to connect wifi. Trying again in 500ms\n");
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

    uint16_t counter = 0;
    uint8_t flip = 0;

    while (1) {
        // TODO: This only answers the first connection request
        handle_incoming_packets();
        vTaskDelay(1);

        if (counter++ > 100) {
            send_midi_control_change(93, flip % 128);
            counter = 0;
            flip++;
        }
    }
}