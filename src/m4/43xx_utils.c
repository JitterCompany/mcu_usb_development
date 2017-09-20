#include "43xx_utils.h"
#include "chip.h"
#include <mcu_timing/delay.h>
#include "status_led.h"

void fatal_error(uint8_t color)
{
    while(true) {
        status_led_toggle(color);
        delay_loop_us(SystemCoreClock, 25000);
    }
}

void assert(bool should_be_true)
{
    if(!should_be_true) {
        fatal_error(RED);
    }
}
