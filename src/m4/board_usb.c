#include "board_usb.h"
#include <one_time_heap/one_time_heap.h>
#include "status_led.h"
#include <string.h>

#include "usb_stack.h"
#include "usb_standard_request.h"
#include "usb_endpoint.h"

#include "usb_queue.h"
#include "dynamic_descriptors.h"

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


USBEndpoint usb0_endpoint_control_out;
USBEndpoint usb0_endpoint_control_in;
USBEndpoint ep_1_in; 
USBEndpoint ep_1_out; 

OneTimeHeap heap;

void* usb_queue_alloc_callback(size_t size)
{
	return one_time_heap_alloc_aligned(&heap, size, 64);
}

static bool usb_init_done = false;

static const usb_request_handler_fn vendor_request_handler[] = {};
static const uint32_t vendor_request_handler_count = 0;

const usb_request_handlers_t request_handlers = {
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

USBDevice usb_device = 
{
    //.descriptor = device_descriptor,
    .descriptor_strings = descriptor_strings,
    .qualifier_descriptor = descriptor_device_qualifier,
    .configurations = &usb0_configurations,
    .configuration = 0,
    .controller = 0,
    .request_handlers = &request_handlers
};

static void usb_configuration_changed(
	USBDevice* const device
) {    
	if( device->configuration->number == 1 ) {
        usb_endpoint_init(&ep_1_in);
        usb_endpoint_init(&ep_1_out);
        usb_init_done = true;
	}
}

uint8_t heapbuffer[2048];

static void ep_transfer_complete_cb(USBEndpoint* const endpoint)
{
    status_led_set(RED, true);
    usb_queue_transfer_complete(endpoint);
}

bool board_usb_init()
{
    one_time_heap_init(&heap, heapbuffer, sizeof(heapbuffer));

    char serial_string[] = "0xDEADCAFE";
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
    
    usb_endpoint_create(&usb0_endpoint_control_out, 0x00, &usb_device, &usb0_endpoint_control_in, usb_setup_complete, usb_control_out_complete);    
    usb_endpoint_create(&usb0_endpoint_control_in, 0x80, &usb_device, &usb0_endpoint_control_out, NULL, usb_control_in_complete);    
    
    descriptor_make_endpoint(
        usb0_configuration.descriptor, 
        interface_descriptor,
        0x81,
        USB_TRANSFER_TYPE_BULK,
        512,
        1 // no NAK?
    );
    usb_endpoint_create(&ep_1_in, 0x81, &usb_device, &ep_1_out, NULL, ep_transfer_complete_cb);
    
    descriptor_make_endpoint(
        usb0_configuration.descriptor, 
        interface_descriptor,
        0x01,
        USB_TRANSFER_TYPE_BULK,
        512,
        1 // no NAK?
    );
    usb_endpoint_create(&ep_1_out, 0x01, &usb_device, &ep_1_in, NULL, ep_transfer_complete_cb);    
    
    
    usb_set_configuration_changed_cb(usb_configuration_changed);
	usb_peripheral_reset(&usb_device);
    
    usb_device_init(&usb_device);
    
    usb_endpoint_alloc_queue(&usb0_endpoint_control_in, 4, usb_queue_alloc_callback);
    usb_endpoint_alloc_queue(&usb0_endpoint_control_out, 4, usb_queue_alloc_callback);
    usb_endpoint_alloc_queue(&ep_1_out, 4, usb_queue_alloc_callback);
    usb_endpoint_alloc_queue(&ep_1_in, 4, usb_queue_alloc_callback);
    
    usb_endpoint_init(&usb0_endpoint_control_out);
    usb_endpoint_init(&usb0_endpoint_control_in);
    
    //TODO check init success
    return true;
}

void board_usb_run()
{
    usb_run(&usb_device);
    
}


void hello_complete_cb(void* user_data, unsigned int n)
{
    status_led_set(YELLOW, true);
}

void board_usb_send_hello()
{
    char str[] = "Hello Jitter USB\n";
    int ret = usb_transfer_schedule(&ep_1_in, str, strlen(str), 
    hello_complete_cb, NULL);
    if (ret < 0) {
        //status_led_set(RED, true);
    }
}

bool board_usb_init_done()
{
    return usb_init_done;
}