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

#include <lpc_tools/boardconfig.h>
#include "board_specific_config.h"

#include "board_usb.h"


unsigned int stack_value = 0x5A5A5A5A;
extern void _vStackTop(void);
extern void _pvHeapStart(void);

#define CPU_FREQ 60000000
#define TICKRATE_HZ (1)
#define member_size(type, member) sizeof(((type *)0)->member)

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

    board_usb_init();

    board_usb_run();

    status_led_set(GREEN, 1);
    while (1) {         

        if(!stack_valid(&_pvHeapStart, stack_value)) {
            status_led_set(RED, 1);
        }

        board_usb_tasks();

    }
    return 0;
}



void TIMER1_IRQHandler() {
    
        if (Chip_TIMER_MatchPending(LPC_TIMER1, 1))     {
            Chip_TIMER_ClearMatch(LPC_TIMER1, 1);
            
            if (board_usb_init_done()) {
                status_led_toggle(BLUE);
                board_usb_send_hello();
            }
    
    
        }
    }