#include "hardware/timer.h"

#ifndef BUTTON_H
#define BUTTON_H

#define DEBOUNCE_DELAY_MS 50

volatile bool button_a_pressed = false;
volatile bool button_b_pressed = false;
volatile bool joystick_pressed = false;

volatile uint32_t last_button_a_time = 0;
volatile uint32_t last_button_b_time = 0;
volatile uint32_t last_joystick_time = 0;

// Callback de interrupção para os botões
void gpio_button_callback(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    if (gpio == BUTTON_A_PIN && (current_time - last_button_a_time) > DEBOUNCE_DELAY_MS) {
        button_a_pressed = true;
        last_button_a_time = current_time;
    } else if (gpio == BUTTON_B_PIN && (current_time - last_button_b_time) > DEBOUNCE_DELAY_MS) {
        button_b_pressed = true;
        last_button_b_time = current_time;
    } else if (gpio == JOYSTICK_SW_PIN && (current_time - last_joystick_time) > DEBOUNCE_DELAY_MS) {
        joystick_pressed = true;
        last_joystick_time = current_time;
    }
}
#endif