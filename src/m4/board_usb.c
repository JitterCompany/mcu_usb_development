#include "board_usb.h"
#include <one_time_heap/one_time_heap.h>
#include "status_led.h"
#include <string.h>
#include <stdio.h>
#include <mcu_timing/delay.h>

#include "chip.h"

#include <mcu_usb/mcu_usb.h>
#include <mcu_usb/usb_descriptors.h>

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


USBEndpoint *usb0_endpoint_control_out;
USBEndpoint *usb0_endpoint_control_in;
USBEndpoint *ep_1_in; 
USBEndpoint *ep_1_out; 

OneTimeHeap heap;

void* usb_alloc_callback(size_t size, size_t alignment)
{
	return one_time_heap_alloc_aligned(&heap, size, alignment);
}

static bool usb_init_done = false;

//static const usb_request_handler_fn vendor_request_handler[] = {};
//static const uint32_t vendor_request_handler_count = 0;

const USBRequestHandlers request_handlers = {
	.standard = usb_standard_request,
	.class = 0,
	.vendor = 0, 
	.reserved = 0,
};

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
    .number = 1, // Why 1?
    .speed = USB_SPEED_HIGH,
    .descriptor = NULL,
};

USBConfiguration* usb0_configurations[] = {
    &usb0_configuration,
    0,
};

USBDevice usb_device;

uint8_t descriptor_device_qualifier[] = {
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

void port_change() {

}

void bus_reset() {

}

void suspend() {
    usb_init_done = false;
    usb_disable_phy_clock();
    status_led_set(GREEN, false);
}

void EVRT_IRQHandler() 
{
    if (Chip_EVRT_IsSourceInterrupting(EVRT_SRC_USB0)) {
        // check for HIGH level event or LOW level
        if(LPC_EVRT->HILO & (1 << EVRT_SRC_USB0)) {
            Chip_EVRT_ConfigIntSrcActiveType(EVRT_SRC_USB0, EVRT_SRC_ACTIVE_LOW_LEVEL);
            Chip_Clock_DisablePLL(CGU_USB_PLL);
        } else {
            Chip_EVRT_ConfigIntSrcActiveType(EVRT_SRC_USB0, EVRT_SRC_ACTIVE_HIGH_LEVEL);
            Chip_Clock_EnablePLL(CGU_USB_PLL);
        }
        Chip_EVRT_ClrPendIntSrc(EVRT_SRC_USB0);
    }
}



USBDevice usb_device = 
{
    //.descriptor = device_descriptor,
    .descriptor_strings = descriptor_strings,
    .qualifier_descriptor = descriptor_device_qualifier,
    .configurations = &usb0_configurations,
    .configuration = 0,
    .controller = 0,
    .request_handlers = &request_handlers,
    .start_of_frame = NULL,
    .port_change = port_change,
    .bus_reset = bus_reset,
    .suspend = suspend,
    .attach = NULL,
    .detach = NULL
};

static void usb_configuration_changed(
	USBDevice* const device
) {    
    if( device->configuration->number == 1 ) {
        usb_endpoint_init(ep_1_in);
        usb_endpoint_init(ep_1_out);
        usb_init_done = true;
        status_led_set(GREEN, true);
	}
}

uint8_t heapbuffer[4048];


void ep_cmplt(USBEndpoint* const endpoint)
{
    usb_queue_transfer_complete(endpoint);
}



bool board_usb_init()
{
    status_led_set(GREEN, true);
    

    Chip_EVRT_Init();
    Chip_EVRT_ConfigIntSrcActiveType(EVRT_SRC_USB0, EVRT_SRC_ACTIVE_HIGH_LEVEL);
    Chip_EVRT_SetUpIntSrc(EVRT_SRC_USB0, ENABLE);
    Chip_EVRT_ClrPendIntSrc(EVRT_SRC_USB0);    
    NVIC_EnableIRQ(EVENTROUTER_IRQn);

    one_time_heap_init(&heap, heapbuffer, sizeof(heapbuffer));

//    char serial_string[] = "0xDEADCAFE";
    char serial_string[] = "229f-10d9-6f07" ; //0xDEADCAFE";
    
    USBDescriptorDevice *device_descriptor =
    descriptor_make_device(VENDOR_ID, PRODUCT_ID, PRODUCT_REV);

    descriptor_from_string(&descriptor_strings[MANUFACTURER_INDEX], MANUFACTURER_STR);
    descriptor_from_string(&descriptor_strings[PRODUCT_INDEX], PRODUCT_STR);
    descriptor_from_string(&descriptor_strings[SERIAL_INDEX], serial_string);
    
    usb0_configuration.descriptor = descriptor_make_configuration(device_descriptor, 1,
        USB_CONFIG_ATTR_BUSPOWERED, USB_CONFIG_POWER_MA(500));
    
    usb_device.descriptor = device_descriptor;
        
    USBDescriptorInterface * interface_descriptor = 
        descriptor_make_interface(usb0_configuration.descriptor, 0, 0);

    usb0_endpoint_control_out = usb_endpoint_create(
                                            0x00, 
                                            &usb_device, 
                                            usb_setup_complete, 
                                            usb_control_out_complete, 
                                            4, usb_alloc_callback);    
    usb0_endpoint_control_in = usb_endpoint_create(
                                            0x80, 
                                            &usb_device, 
                                            NULL, 
                                            usb_control_in_complete, 
                                            4, usb_alloc_callback); 

    usb_pair_endpoints(usb0_endpoint_control_in, usb0_endpoint_control_out);
    
    descriptor_make_endpoint(
        usb0_configuration.descriptor, 
        interface_descriptor,
        0x81,
        USB_TRANSFER_TYPE_BULK,
        512,
        1 // no NAK?
    );
    ep_1_in = usb_endpoint_create(0x81, &usb_device, NULL,  usb_queue_transfer_complete, 4, usb_alloc_callback);
    
    descriptor_make_endpoint(
        usb0_configuration.descriptor, 
        interface_descriptor,
        0x01,
        USB_TRANSFER_TYPE_BULK,
        512,
        1 // no NAK?
    );
    ep_1_out = usb_endpoint_create(0x01, &usb_device, NULL,  ep_cmplt, 4, usb_alloc_callback);    
    
    
    usb_set_configuration_changed_cb(usb_configuration_changed);
	usb_peripheral_reset(&usb_device);
    
    usb_device_init(&usb_device);

    if (usb0_endpoint_control_out && usb0_endpoint_control_in) {
        usb_endpoint_init(usb0_endpoint_control_out);
        usb_endpoint_init(usb0_endpoint_control_in);
    } else {
        return false;
    }

    // set charge bit to fool USB peripheral to think a host 
    // is present. To force suspend if bus is inactive. This will
    // put usb in low power state 
    usb_set_vbus_charge(&usb_device, true);

    usb_run(&usb_device);
    
    return true;
}

const volatile int indexes[] = {0,1,2,3};

volatile char * receive_buffer[4][512];
volatile char str[4][100];

void receive_cb(void* user_data, unsigned int n)
{
    int index = *(int*)user_data;
    char * received =  (char*)receive_buffer[index];
    received[n] = '\0';
    char *buf = (char*) str[index];
    snprintf(buf, 100, "Device: transfer index: %d msg size: %d \n message: %s, \n ----- \n ", index,  n, received);
    
    usb_transfer_schedule(ep_1_in, buf, strlen(buf), NULL, NULL);
}


volatile int count = 0;
void board_usb_tasks() 
{
    if (usb_init_done) {
        int max_length = sizeof(receive_buffer)/4;
        int ret = usb_transfer_schedule(ep_1_out, receive_buffer[count], max_length, receive_cb, (void*)&indexes[count]);
        if (ret >= 0) {
            count++;
            if (count >= 4) count = 0;
        }
    }
}

void hello_complete_cb(void* user_data, unsigned int n)
{

}

static char hello[] = "Hello Jitter USB\n";
void board_usb_send_hello()
{
    usb_transfer_schedule(ep_1_in, hello, strlen(hello), 
       hello_complete_cb, NULL);
}

bool board_usb_init_done()
{
    return usb_init_done;
}