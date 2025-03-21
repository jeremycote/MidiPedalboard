//
// Created by Jeremy Cote on 2025-02-13.
//

#include <pico/stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <pico/printf.h>
#include <pico/stdio_usb.h>
#include "hardware/adc.h"
#include "hardware/gpio.h"

#include "communication_task.h"
#include "adc_task.h"

#include "pico/stdlib.h"
#include <ssd1306.h>
#include <stdio.h>
#include "../lib/pico-ssd1306/ssd1306.h"

#define DELAY 500 // in milliseconds

void drawTest(ssd1306_t *pOled) {
    ssd1306_draw_string(pOled, 0, 2, 2, "Hello!");
    ssd1306_draw_line(pOled, 2, 25, 80, 25);
}

int main() {
    stdio_init_all();

    // TODO: Remove this for production. Used for debugging.
    while (!stdio_usb_connected()) {

    }

    printf("Starting Midi Controller\n");

    sleep_ms(2000);
    printf("GO\n");

    // Setup the OLED screen
    i2c_init(i2c1, 400000);
    gpio_pull_up(2);
    gpio_pull_up(3);
    gpio_set_function(2, GPIO_FUNC_I2C);
    gpio_set_function(3, GPIO_FUNC_I2C);

    ssd1306_t oled;
    oled.external_vcc = false;
    
    bool res = ssd1306_init(&oled, 128, 32, 0x3C, i2c1);

    // If setup is OK, write the test text on the OLED
    if (res) {
        ssd1306_clear(&oled);
        drawTest(&oled);
        ssd1306_show(&oled);
    } else {
        printf("OLED Init failed\n");
    }

    // Initialize ADC
    adc_init();
    adc_gpio_init(32); // GPIO 32 is ADC1
    adc_select_input(1); // Select ADC1

    char buffer[32];

    while (true) {
        uint16_t raw = adc_read(); // 12-bit ADC result (0-4095)


        
        // Format string for OLED
        snprintf(buffer, sizeof(buffer), "Sound level : %d", raw);

        // Clear, draw, and display on OLED
        ssd1306_clear(&oled);
        if (raw>500){
            ssd1306_draw_string(&oled, 0, 2, 1.5, "SOUND WARNING");
        } 
        else{
            ssd1306_draw_string(&oled, 0, 2, 1.5, "Sound good");
        }
        
        ssd1306_draw_string(&oled, 0, 15, 1, buffer);
        ssd1306_show(&oled);

        sleep_ms(200); // Delay for readability
    }


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