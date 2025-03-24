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
#include <stdint.h> 
#include "../lib/pico-ssd1306/ssd1306.h"
#include "ads1115.h"


#define DELAY 500 
#define NUM_BUTTONS 8

const uint32_t BUTTON_PINS[NUM_BUTTONS] = {8, 9, 10, 11, 12, 13, 14, 15};
const uint32_t LED_PINS[NUM_BUTTONS] = {0, 22, 21, 20, 19, 18, 17, 16};



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

    // Init ADS1115 on i2c1
    ads1115_init(i2c1, 0x4A);

    // Initialize buttons and LEDs
    for (int i = 0; i < NUM_BUTTONS; i++) {
        gpio_init(BUTTON_PINS[i]);
        gpio_set_dir(BUTTON_PINS[i], GPIO_IN);
        gpio_pull_up(BUTTON_PINS[i]); // Active low

        gpio_init(LED_PINS[i]);
        gpio_set_dir(LED_PINS[i], GPIO_OUT);
        gpio_put(LED_PINS[i], 0); // Turn off LEDs initially
    }

    bool button_states[NUM_BUTTONS] = {1,1,1,1,1,1,1,1};  // Start assuming all buttons are released

    while (true) {

        for (int i = 0; i < NUM_BUTTONS; i++) {
            bool current = gpio_get(BUTTON_PINS[i]);
            if (!current && button_states[i]) {  // Button just pressed
                gpio_put(LED_PINS[i], 1);        // Turn on LED
                printf("Button %d pressed\n", i + 1);
            } else if (current && !button_states[i]) { // Button just released
                gpio_put(LED_PINS[i], 0);        // Turn off LED
            }
            button_states[i] = current;
        }
        
        // Read from onboard ADC (for sound level)
        uint16_t raw = adc_read(); // 12-bit ADC result (0–4095)
        snprintf(buffer, sizeof(buffer), "Sound: %d", raw);

        // Read from ADS1115 A1
        int16_t pot_raw = ads1115_read_channel(3);
        float pot_voltage = ads1115_raw_to_voltage(pot_raw);
        char pot_buf[32];
        snprintf(pot_buf, sizeof(pot_buf), "POT: %.2f V", pot_voltage);

        // Clear and display both lines
        ssd1306_clear(&oled);
        ssd1306_draw_string(&oled, 0, 2, 1, buffer);     // Line 1: Sound level
        ssd1306_draw_string(&oled, 0, 15, 1, pot_buf);   // Line 2: Pot voltage
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