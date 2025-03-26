//
// Created by Jeremy Cote on 2025-02-13.
//

#include "communication_task.h"

#include <pico/printf.h>
#include <lwip/sockets.h>
#include <hardware/gpio.h>
#include "FreeRTOS.h"
#include "task.h"

#include "communication.h"
#include "analog_signals.h"
#include "buttons.h"

// TODO: Calculate proper stack size
#define COMMUNICATION_TASK_STACK_SIZE 4096

StaticTask_t communication_task_buffer;
StackType_t communication_task_stack[COMMUNICATION_TASK_STACK_SIZE];

_Noreturn void communication_task();

void start_communication_task() {
    printf("Starting Communication Task\n");
    TaskHandle_t task = xTaskCreateStatic(communication_task, "Comms", COMMUNICATION_TASK_STACK_SIZE, NULL, 1, communication_task_stack,
                      &communication_task_buffer);
    vTaskCoreAffinitySet(task, 0b10);
}

_Noreturn void communication_task() {
    printf("Setup Wifi\n");

    while (!setup_wifi()) {
        printf("Failed to setup wifi. Trying again in 500ms\n");
        vTaskDelay(500);
    }

    printf("Connecting to wifi\n");

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

    uint32_t adc_timestamps[5] = {0, 0, 0, 0, 0};
    uint32_t timestamps[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    while (1) {
        // TODO: This only answers the first connection request
        handle_incoming_packets();
        vTaskDelay(1);

        if (counter++ > 100) {
            for (int i = 2; i < 3; i++) {
                if (xTaskGetTickCount() > adc_timestamps[i] + 100) {
                    send_midi_control_change(102 + i, analog_get(i));
                    adc_timestamps[i] = xTaskGetTickCount();
                }
            }

            for (int i = 0; i < 8; i++) {
                if (!gpio_get(8 + i)) {
                    if (xTaskGetTickCount() > timestamps[i] + 1000) {
                        send_midi_control_change(20 + i, 127);
                        timestamps[i] = xTaskGetTickCount();
                    }
                }
            }

            counter = 0;
        }
    }
}