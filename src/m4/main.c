#include "chip.h"
#include <string.h>
#include <math.h>

#include <lpc_tools/irq.h>
#include <lpc_tools/clock.h>
#include <lpc_tools/ipc.h>

#include "status_led.h"
#include <mcu_timing/delay.h>
#include <mcu_timing/profile.h>
#include "stacktools.h"

#include "power_saving.h"

#include <lpc_tools/boardconfig.h>
#include "board_specific_config.h"


// new USB stuff
#include "usb_stack.h"
#include "usb_standard_request.h"
#include "usb_device.h"
#include "usb_queue.h"
#include "usb_endpoint.h"

#define CPU_FREQ 60000000
#define TICKRATE_HZ (5)
#define member_size(type, member) sizeof(((type *)0)->member)

static const usb_request_handler_fn vendor_request_handler[] = {};
static const uint32_t vendor_request_handler_count = 0;

const usb_request_handlers_t usb1_request_handlers = {
	.standard = 0,
	.class = 0,
	.vendor = 0,
	.reserved = 0,
};
const usb_request_handlers_t usb0_request_handlers = {
	.standard = usb_standard_request,
	.class = 0,
	.vendor = 0,
	.reserved = 0,
};


unsigned int stack_value = 0x5A5A5A5A;
extern void _vStackTop(void);
extern void _pvHeapStart(void);

void usb_configuration_changed(
	usb_device_t* const device
) {
	
	if( device->configuration->number == 1 ) {
		
	} else {
		
	}
}

void Timer1_init() {

    Chip_TIMER_Init(LPC_TIMER1);
	Chip_RGU_TriggerReset(RGU_TIMER1_RST);
	while (Chip_RGU_InReset(RGU_TIMER1_RST)) {}

	uint32_t timerFreq = Chip_Clock_GetRate(CLK_MX_TIMER1);

	Chip_TIMER_Reset(LPC_TIMER1);
	Chip_TIMER_MatchEnableInt(LPC_TIMER1, 1);
	Chip_TIMER_SetMatch(LPC_TIMER1, 1, (timerFreq / TICKRATE_HZ));
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER1, 1);
	Chip_TIMER_Enable(LPC_TIMER1);

	NVIC_EnableIRQ(TIMER1_IRQn);
	NVIC_ClearPendingIRQ(TIMER1_IRQn);
}

void TIMER1_IRQHandler() {

    if (Chip_TIMER_MatchPending(LPC_TIMER1, 1))     {
		Chip_TIMER_ClearMatch(LPC_TIMER1, 1);
        status_led_toggle(BLUE);
    }
}



void init()
{

    board_setup();
    board_setup_NVIC();
    fpuInit();

    clock_set_frequency(CPU_FREQ);
    delay_init();
    //power_saving_disable_peripherals();
    
    board_LED_init();

    // memset(g_device_id, 0, sizeof(g_device_id));
    // if(!device_id_get_str(g_device_id, sizeof(g_device_id))) {
    //     strlcpy(g_device_id, "unknown", sizeof(g_device_id));
    // }

    Timer1_init();

}

void HardFault_Handler(void)
{
    status_led_set(RED, 1);
}


int main(void)
{
    
    init();

    usb_set_configuration_changed_cb(usb_configuration_changed);
	usb_peripheral_reset(&usb_devices[0]);
	
	usb_device_init(&usb_devices[0]);
	
	usb_queue_init(&usb0_endpoint_control_out_queue);
	usb_queue_init(&usb0_endpoint_control_in_queue);

    usb_endpoint_init(&usb0_endpoint_control_out);
	usb_endpoint_init(&usb0_endpoint_control_in);

    usb_run(&usb_devices[0]);

    while (1) {         

        if(!stack_valid(&_pvHeapStart, stack_value)) {
            status_led_set(RED, 1);
        }
        status_led_set(GREEN, 1);

    }
    return 0;
}

