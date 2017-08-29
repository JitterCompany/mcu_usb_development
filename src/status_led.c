#include "status_led.h"
#include "chip.h"

#ifdef __LPC11U1X__
#define STATUS_LED_PORT LPC_GPIO
#else
#define STATUS_LED_PORT LPC_GPIO_PORT
#endif

static struct led_pin pins[LED_MAX_COUNT];

bool status_led_bind(uint8_t led_ID, uint8_t port, uint8_t pin)
{
    if(led_ID >= LED_MAX_COUNT) {
        return false;
    }
    pins[led_ID] = (struct led_pin) {
        port, pin
    };

    Chip_GPIO_SetPinDIROutput(STATUS_LED_PORT, port, pin);

    return true;
}

void status_led_set(uint8_t led_ID, bool on)
{
    Chip_GPIO_SetPinState(STATUS_LED_PORT, pins[led_ID].port,
                          pins[led_ID].pin, on);
}

void status_led_toggle(uint8_t led_ID)
{
    Chip_GPIO_SetPinToggle(STATUS_LED_PORT, pins[led_ID].port,
                           pins[led_ID].pin);
}
