#include "EvaUSBClass.h"
#include <lpc_usb_lib/USB.h>
//#include "logging.h"

#define MAX_EP_NUM 5
static uint8_t corenum;
static uint8_t protocol_ep;
static ep_cfg_t ep_cfgs[2 * MAX_EP_NUM];
static bool is_initialized = false;

void EvaUSB_use_endpoint(USB_StdDescriptor_Endpoint_t *ep_desc,
                         uint8_t banks, endpoint_cb cb)
{
    if(!ep_desc) {
        return;
    }
    uint8_t num = ep_desc->bEndpointAddress & (~ENDPOINT_DIR_MASK);
    uint8_t dir = ep_desc->bEndpointAddress & ENDPOINT_DIR_MASK;

    initialize_endpoint(num, ep_desc->bmAttributes, dir,
                        ep_desc->wMaxPacketSize, banks, cb);
}

void initialize_endpoint(uint8_t num, uint8_t transfer_type, uint8_t dir,
                         uint16_t size,
                         uint8_t banks, endpoint_cb cb)
{
    uint8_t	index = 2 *(num-1) + (dir == ENDPOINT_DIR_OUT);
    ep_cfgs[index].num = num;
    ep_cfgs[index].transfer_type = transfer_type;
    ep_cfgs[index].direction = dir;
    ep_cfgs[index].banks = banks;
    ep_cfgs[index].size = size;
    ep_cfgs[index].cb = cb;
}

void EvaUSB_init(uint8_t usb_corenum,
                 uint8_t protocol_endpoint, uint16_t protocol_ep_size)
{
    //log_debug("usb init");
    corenum = usb_corenum;
    protocol_ep = protocol_endpoint;
    USB_Init(corenum, USB_MODE_Device);
    is_initialized = true;
}

void EvaUSB_deinit(uint8_t usb_corenum)
{
    //log_debug("usb deinit");
    USB_Disable(corenum, USB_MODE_Device);
    is_initialized = false;
}

bool EvaUSB_is_initialized(void)
{
    return is_initialized;
}


bool EvaUSB_device_is_configured()
{
    return USB_DeviceState[corenum] == DEVICE_STATE_Configured;
}

void EvaUSB_USBTasks(void)
{
    if(!EvaUSB_is_initialized()) {
        return;
    }
    if (USB_DeviceState[corenum] == DEVICE_STATE_Configured) {
        for (int i = 1; i < MAX_EP_NUM * 2; i+=2) {
            ep_cfg_t ep = ep_cfgs[i];
            if (ep.num > 0) {

                if(Endpoint_IsOUTReceived_new(corenum, ep.num)) {
                    uint16_t size;
                    uint8_t *data = Endpoint_receiveData(corenum,
                                                         ep.num, &size);

                    USBReceiveResult result = USB_RECEIVE_DONE;
                    //call endpoint callback
                    if (ep.cb != NULL) {
                        result = ep.cb(data, size);
                    }
                    if(result == USB_RECEIVE_DONE) {
                        Endpoint_ClearOUT_new(corenum, ep.num);
                    }
                }
            }
        }
    }
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
    for (int i = 0; i < MAX_EP_NUM * 2; i++) {
        ep_cfg_t ep = ep_cfgs[i];
        if (ep.num > 0) {
            Endpoint_ConfigureEndpoint(corenum, ep.num, ep.transfer_type,
                                       ep.direction, ep.size, ep.banks);
        }
    }
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{

}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{

}

