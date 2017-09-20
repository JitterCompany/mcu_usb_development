/*
 * EvaUSBClass.h
 *
 *  Created on: Jan 12, 2014
 *      Author: Ingmar
 */

#ifndef EVAUSBCLASS_H_
#define EVAUSBCLASS_H_

#include <stdint.h>
#include <stdbool.h>

#include "dynamic_descriptors_old.h"

typedef enum {
    USB_RECEIVE_DONE,
    USB_RECEIVE_NOT_YET
} USBReceiveResult;

typedef USBReceiveResult (*endpoint_cb)(void*, size_t);

void EvaUSB_init(uint8_t usb_corenum,
                 uint8_t protocol_endpoint, uint16_t protocol_ep_size);

void EvaUSB_deinit(uint8_t usb_corenum);

bool EvaUSB_is_initialized(void);
void EvaUSB_USBTasks(void);
bool EvaUSB_device_is_configured();

void EVENT_USB_Device_Connect(void);
void EVENT_USB_Device_Disconnect(void);
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);
void EVENT_USB_Device_StartOfFrame(void);

extern const uint8_t ep_log;
extern const uint8_t ep_1;

typedef struct {
    uint8_t num;
    uint8_t transfer_type;
    uint8_t direction;
    uint8_t banks;
    uint16_t size;
    endpoint_cb cb;
} ep_cfg_t;


/*
 * This is an alternative for initialize_endpoint():
 * initialize directly from the relevant endpoint descriptor.
 * See dynamic_descriptors.h
 *
 * Most of the ep_cfg fields are also in the descriptors,
 * but for now let's be backwards compatible
 */
void EvaUSB_use_endpoint(USB_StdDescriptor_Endpoint_t *ep_desc,
                         uint8_t banks, endpoint_cb cb);

void initialize_endpoint(uint8_t num, uint8_t transfer_type, uint8_t dir,
                         uint16_t size,
                         uint8_t banks, endpoint_cb cb);


#endif /* EVAUSBCLASS_H_ */
