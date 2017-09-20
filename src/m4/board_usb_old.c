#include "board_usb_old.h"
#include "43xx_utils.h"
#include "status_led.h"
#include "EvaUSBClass.h"
//#include "global_defs.h"
#include <string.h>
#include <stdio.h>
#include <c_utils/f2strn.h>
#include <mcu_timing/delay.h>

#define CORENUM (0)
// Note: bulk should always be 512 for high speed!
#define Prot_Ep_In_SIZE		512
#define Prot_Ep_Out_SIZE	512
#define DATA_EP_IN_SIZE     512

//const uint8_t ep_1 = 1;
const uint8_t ep_2 = 2;
const uint8_t ep_3 = 3;
const uint8_t ep_4 = 4;
const uint8_t ep_protocol = 1;

// interrupt type: multiple of (3*sizeof(float32))
// bulk: MUST be 512
#define DATA_EP_DUMMY_SENSOR_OUT_SIZE	512

#define PRODUCT_STR ("Pressure Sensor")
#define VENDOR_STR ("Jitter")
#define VENDOR_ID (0x3853)
#define PRODUCT_ID (0x0021)
#define PRODUCT_REV (0x0200)

volatile char * receive_buffer[512];

USBReceiveResult receive_cb(void *data, size_t n) {
    status_led_toggle(RED);
    snprintf(receive_buffer, 100, "Device: msg size: %d \n message: %s, \n ----- \n ", n, (char*)data);
    
    Endpoint_sendData(0, ep_protocol, (uint8_t*)receive_buffer, strlen(receive_buffer));

    return USB_RECEIVE_DONE;
}

void board_usb_init(const char *serial_string)
{
#if (EN_USB)
    // create USB descriptors //
    USB_StdDescriptor_Device_t *device_descriptor =
        descriptor_make_device(VENDOR_ID, PRODUCT_ID, PRODUCT_REV);

    device_descriptor->iManufacturer = descriptor_string(VENDOR_STR);
    device_descriptor->iProduct = descriptor_string(PRODUCT_STR);
    device_descriptor->iSerialNumber = descriptor_string(serial_string);

    USB_StdDescriptor_Configuration_Header_t *config_descriptor =
        descriptor_make_configuration(device_descriptor, 1,
                                      USB_CONFIG_ATTR_BUSPOWERED, USB_CONFIG_POWER_MA(500));

    USB_StdDescriptor_Interface_t *interface_descriptor =
        descriptor_make_interface(config_descriptor, 0, 0);

    USB_StdDescriptor_Endpoint_t *ep_desc_protocol_in =
        descriptor_make_endpoint(
            config_descriptor,
            interface_descriptor,
            ep_protocol | ENDPOINT_DIR_IN,
            EP_TYPE_BULK,
            Prot_Ep_In_SIZE,
            1);

    USB_StdDescriptor_Endpoint_t *ep_desc_protocol_out =
        descriptor_make_endpoint(
            config_descriptor,
            interface_descriptor,
            ep_protocol | ENDPOINT_DIR_OUT,
            EP_TYPE_BULK,
            Prot_Ep_Out_SIZE,
            1);


    if(!descriptor_ok()) {
        fatal_error(BLUE);
    }

    EvaUSB_use_endpoint(ep_desc_protocol_in, ENDPOINT_BANK_SINGLE, NULL);
    EvaUSB_use_endpoint(ep_desc_protocol_out, ENDPOINT_BANK_SINGLE, receive_cb); // TODO: assing USB cmd input callback

    EvaUSB_init(CORENUM, ep_protocol, Prot_Ep_In_SIZE);
#endif
}

bool board_usb_is_connected(void)
{
#if (EN_USB)
    // TODO implement this properly.
    // For now we assume that initialized means connected
    return EvaUSB_is_initialized();
#else
    return false;
#endif
}

void board_usb_try_start(const char *serial_string)
{
#if (EN_USB)
    if(!EvaUSB_is_initialized()) {
        board_usb_init(serial_string);
    }
    
    delay_timeout_t connect_timeout;
    // this delay should be as short as possible or eliminated entirely,
    // as it makes resume from deep sleep slow.
    delay_timeout_set(&connect_timeout, 1000000);

    while(!EvaUSB_device_is_configured()) {
        EvaUSB_USBTasks();
        if(delay_timeout_done(&connect_timeout)) {
            board_usb_stop(); 
            break;
        }
    }
#endif
}

void board_usb_stop()
{
#if (EN_USB)
    EvaUSB_deinit(CORENUM);
#endif
}

void board_usb_tasks()
{
#if (EN_USB)
    EvaUSB_USBTasks();
//    usb_command_poll();
#endif
}

