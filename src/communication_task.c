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

uint8_t buffer[1024];
struct sockaddr_in sender_addr;
socklen_t sender_len = sizeof(sender_addr);

_Noreturn void communication_task() {
    printf("Setup Wifi\n");

    while (!setup_wifi()) {
        vTaskDelay(500);
    }

    printf("Setup midi server\n");

    while (!setup_midi_server()) {
        vTaskDelay(500);
    }

    printf("Start communication loop\n");

    while (1) {
        // TODO: This only answers the first connection request
        handle_incoming_packets(buffer, 1024, sender_addr, sender_len);
        vTaskDelay(1);
    }
}