#include "buttons.h"
#include <stdint.h>
#include <hardware/gpio.h>
#include <pico/printf.h>

#define NUM_BUTTONS 8
const uint32_t BUTTON_PINS[NUM_BUTTONS] = {8, 9, 10, 11, 12, 13, 14, 15};
const uint8_t LED_PINS[NUM_BUTTONS] = {0, 22, 21, 20, 19, 18, 17, 16};

uint8_t buttons_pressed;

void gpio_callback(uint gpio, uint32_t events) {
    printf(".");
    if (events != GPIO_IRQ_EDGE_FALL || gpio < 8 || gpio >= 16)
        return;

    buttons_set(gpio - 8, true);
}

void init_buttons() {
    // Initialize buttons and LEDs
    for (int i = 0; i < NUM_BUTTONS; i++) {
        gpio_init(BUTTON_PINS[i]);
        gpio_set_dir(BUTTON_PINS[i], GPIO_IN);
        gpio_pull_up(BUTTON_PINS[i]); // Active low

        gpio_init(LED_PINS[i]);
        gpio_set_dir(LED_PINS[i], GPIO_OUT);
        gpio_put(LED_PINS[i], 1); // Turn on LEDs initially
    }

//    gpio_set_irq_enabled_with_callback(10, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

}

bool buttons_get(button_input_t input) {
    return buttons_pressed & (1 << input);
}

void buttons_set(button_input_t input, bool state) {
    if (state) {
        buttons_pressed |= 1 << input;
    } else {
        buttons_pressed &= ~(1 << input);
    }
}

void led_set(button_input_t input, bool state) {
    printf("Set LED %u\n", LED_PINS[input]);
    gpio_put(LED_PINS[input], state);
}