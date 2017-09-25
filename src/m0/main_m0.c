#include "chip.h"
#include <lpc_tools/boardconfig.h>
#include "board_specific_config.h"
#include "status_led.h"

unsigned int stack_value = 0x5A5A5A5A;

#include "board_usb.h"

void init(void)
{
    SystemCoreClockUpdate();
    board_setup();
    board_setup_NVIC();
    board_LED_init();
    
}

int main()
{
    init();
    status_led_set(BLUE, true);
    if (!board_usb_init()) {
        status_led_set(RED, true);
    }

    
    while(true) {
        board_usb_tasks();
    }
    
    return 0;
}