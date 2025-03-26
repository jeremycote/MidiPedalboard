//
// Created by Jeremy Cote on 2025-03-24.
//

#ifndef MIDICONTROLLER_BUTTONS_H
#define MIDICONTROLLER_BUTTONS_H

#include <stdbool.h>

typedef enum {
    BUTTON_0,
    BUTTON_1,
    BUTTON_2,
    BUTTON_3,
    BUTTON_4,
    BUTTON_5,
    BUTTON_6,
    BUTTON_7,
} button_input_t;

void init_buttons();

bool buttons_get(button_input_t input);
void buttons_set(button_input_t input, bool state);

void led_set(button_input_t input, bool state);

#endif //MIDICONTROLLER_BUTTONS_H
