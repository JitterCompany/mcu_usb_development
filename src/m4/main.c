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

#include "usb_queue.h"
#include "dynamic_descriptors.h"

#define CPU_FREQ 60000000
#define TICKRATE_HZ (1)
#define member_size(type, member) sizeof(((type *)0)->member)

static const usb_request_handler_fn vendor_request_handler[] = {};
static const uint32_t vendor_request_handler_count = 0;

static bool usb_init_done = false;

USBEndpoint ep_1_in; 
USBEndpoint ep_1_out; 

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
	USBDevice* const device
) {
    status_led_toggle(YELLOW);
    
    usb_endpoint_init(&ep_1_in);
    usb_endpoint_init(&ep_1_out);
    usb_init_done = true;

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
#define USB_WORD(x)	(x & 0xFF), ((x >> 8) & 0xFF)
#define USB_STRING_LANGID		(0x0409)
#define PRODUCT_STR ("Generic Sensor")
#define MANUFACTURER_STR ("Jitter")
#define VENDOR_ID (0x3853)
#define PRODUCT_ID (0x0021)
#define PRODUCT_REV (0x0200)
/** Macro to calculate the power value for the configuration descriptor, from a given number of milliamperes.
 *
 *  @param     mA  Maximum number of milliamps the device consumes when the given configuration is selected.
 */
#define USB_CONFIG_POWER_MA(mA)           ((mA) >> 1)
#define USB_CONFIG_ATTR_BUSPOWERED        0x80

uint8_t descriptor_string_languages[] = {
	0x04,			    // bLength
	USB_DESCRIPTOR_TYPE_STRING,	    // bDescriptorType
	USB_WORD(USB_STRING_LANGID),	// wLANGID
};

const USBDescriptorString *descriptor_strings[] = {
	(USBDescriptorString*) &descriptor_string_languages,
	0,
	0,
	0,
	0,		// TERMINATOR
};

USBConfiguration usb0_configuration = {
    .number = 1,
    .speed = USB_SPEED_HIGH,
    .descriptor = 0,
};

USBConfiguration* new_usb0_configurations[] = {
    &usb0_configuration,
    0,
};

USBDevice usb_device;

USBEndpoint usb0_endpoint_control_out;
USBEndpoint usb0_endpoint_control_in;
//USB_DECLARE_QUEUE(usb0_endpoint_control_out);
//USB_DECLARE_QUEUE(usb0_endpoint_control_in);//
USBEndpoint usb0_endpoint_control_out = {
    .address = 0x00,
    .device = &usb_device,
    .in = &usb0_endpoint_control_in,
    .out = &usb0_endpoint_control_out,
    .setup_complete = usb_setup_complete,
    .transfer_complete = usb_control_out_complete,
};
USB_DEFINE_QUEUE(usb0_endpoint_control_out, 4);

USBEndpoint usb0_endpoint_control_in = {
    .address = 0x80,
    .device = &usb_device,
    .in = &usb0_endpoint_control_in,
    .out = &usb0_endpoint_control_out,
    .setup_complete = 0,
    .transfer_complete = usb_control_in_complete,
};
static USB_DEFINE_QUEUE(usb0_endpoint_control_in, 4);

uint8_t usb0_descriptor_device_qualifier[] = {
	10,					                    // bLength
	USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,	// bDescriptorType
	USB_WORD(0x0200),			            // bcdUSB
	0x00,					                // bDeviceClass
	0x00,					                // bDeviceSubClass
	0x00,					                // bDeviceProtocol
	64,					                    // bMaxPacketSize0
	0x00,					                // bNumOtherSpeedConfigurations
	0x00					                // bReserved
};

USBDevice usb_device = 
{
    //.descriptor = device_descriptor,
    //.descriptor_strings = descriptor_strings,
    .qualifier_descriptor = usb0_descriptor_device_qualifier,
    .configurations = &new_usb0_configurations,
    .configuration = 0,
    .controller = 0,
};


void ep_transfer_complete_cb(USBEndpoint* const endpoint)
{
    status_led_set(RED, true);
    usb_queue_transfer_complete(endpoint);
}

int main(void)
{
    
    init();

    char serial_string[] = "0xDEADCAFE";
    USBDescriptorDevice *device_descriptor =
    descriptor_make_device(VENDOR_ID, PRODUCT_ID, PRODUCT_REV);

    const USBDescriptorString *manufacturer_str_desc;
    const USBDescriptorString *product_str_desc;
    const USBDescriptorString *serial_str_desc;
    descriptor_from_string(&manufacturer_str_desc, MANUFACTURER_STR);
    descriptor_from_string(&product_str_desc, PRODUCT_STR);
    descriptor_from_string(&serial_str_desc, serial_string);
    // save reference in descriptor_strings
    descriptor_strings[MANUFACTURER_INDEX] = manufacturer_str_desc;
    descriptor_strings[PRODUCT_INDEX] = product_str_desc;
    descriptor_strings[SERIAL_INDEX] = serial_str_desc;
    
    USBDescriptorConfiguration *config_descriptor =
    descriptor_make_configuration(device_descriptor, 1,
        USB_CONFIG_ATTR_BUSPOWERED, USB_CONFIG_POWER_MA(500));
        
    USBDescriptorInterface * interface_descriptor = 
    descriptor_make_interface(config_descriptor, 0, 0);
    

    descriptor_make_endpoint(
        config_descriptor, 
        interface_descriptor,
        0x81,
        USB_TRANSFER_TYPE_BULK,
        512,
        1 // no NAK?
    );
    make_endpoint(&ep_1_in, 0x81, &usb_device, &ep_1_out, NULL, ep_transfer_complete_cb);
    USB_DEFINE_QUEUE(ep_1_in, 4);
    
    descriptor_make_endpoint(
        config_descriptor, 
        interface_descriptor,
        0x01,
        USB_TRANSFER_TYPE_BULK,
        512,
        1 // no NAK?
    );
    make_endpoint(&ep_1_out, 0x01, &usb_device, &ep_1_in, NULL, ep_transfer_complete_cb);    
    USB_DEFINE_QUEUE(ep_1_out, 4);
    
    usb0_configuration.descriptor = (uint8_t*)config_descriptor;

    usb_device.descriptor = device_descriptor;
    usb_device.descriptor_strings = descriptor_strings;
    
    usb_set_configuration_changed_cb(usb_configuration_changed);
	usb_peripheral_reset(&usb_device);
    
    usb_device_init(&usb_device);
   
	
	usb_queue_init(&usb0_endpoint_control_out_queue);
    usb_queue_init(&usb0_endpoint_control_in_queue);
    
    usb_queue_init(&ep_1_in_queue);
    usb_queue_init(&ep_1_out_queue);

    usb_endpoint_init(&usb0_endpoint_control_out);
	usb_endpoint_init(&usb0_endpoint_control_in);

    usb_run(&usb_device);
    

    status_led_set(GREEN, 1);
    while (1) {         

        if(!stack_valid(&_pvHeapStart, stack_value)) {
            status_led_set(RED, 1);
        }

    }
    return 0;
}

void transfer_complete_cb(void* user_data, unsigned int n)
{
    status_led_set(YELLOW, true);
    
    //usb_endpoint_flush(&ep_1_in);
    //usb_queue_flush_endpoint(&ep_1_in);
}

void TIMER1_IRQHandler() {
    
        if (Chip_TIMER_MatchPending(LPC_TIMER1, 1))     {
            Chip_TIMER_ClearMatch(LPC_TIMER1, 1);
            
            if (usb_init_done) {
                status_led_toggle(BLUE);
                char str[] = "Hello Jitter USB\n";
                int ret = usb_transfer_schedule(&ep_1_in, str, strlen(str), 
                    transfer_complete_cb, NULL);
                if (ret < 0) {
                    //status_led_set(RED, true);
                }
            }
    
    
        }
    }