#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <stdbool.h>
#include <stdint.h>

#define LED_MAX_COUNT 8

bool status_led_bind(uint8_t led_ID, uint8_t port, uint8_t pin);
void status_led_set(uint8_t led_ID, bool on);
void status_led_toggle(uint8_t led_ID);

enum {
    GREEN=0,
    BLUE,
    RED,
    YELLOW
} colors;

struct led_pin {
    uint8_t port;
    uint8_t pin;
};

#endif

