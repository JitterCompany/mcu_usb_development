#ifndef __USB_CORE_H__
#define __USB_CORE_H__

// TODO: Refactor to support high performance operations without having to
// expose USBTransferDescriptor. Or usb_endpoint_prime(). Or, or, or...
#include "usb.h"
#include "usb_type.h"

#define NUM_USB_CONTROLLERS 2
#define NUM_USB0_ENDPOINTS 6
#define NUM_USB1_ENDPOINTS 4

void usb_peripheral_reset();

void usb_bus_reset(
	USBDevice* const device
);

USBQueueHead* usb_queue_head(
	const uint_fast8_t endpoint_address,
	const USBDevice* const device
);

USBEndpoint* usb_endpoint_from_address(
	const uint_fast8_t endpoint_address,
	const USBDevice* const device
);

uint_fast8_t usb_endpoint_address(
	const USBTransferDirection direction,
	const uint_fast8_t number
);

void usb_device_init(
	USBDevice* const device
);

void usb_controller_reset(
	USBDevice* const device
);


void usb_controller_run(
	const USBDevice* const device
);

void usb_run(
	USBDevice* const device
);

void usb_run_tasks(
	const USBDevice* const device
);

USBSpeed usb_speed(
	const USBDevice* const device
);

uint32_t usb_get_status(
	const USBDevice* const device
);

uint32_t usb_get_endpoint_setup_status(
	const USBDevice* const device
);

void usb_clear_endpoint_setup_status(
	const uint32_t endpoint_setup_status,
	const USBDevice* const device
);


uint32_t usb_get_endpoint_ready(
	const USBDevice* const device
);

uint32_t usb_get_endpoint_complete(
	const USBDevice* const device
);

void usb_clear_endpoint_complete(
	const uint32_t endpoint_complete,
	const USBDevice* const device
);

void usb_set_address_immediate(
	const USBDevice* const device,
	const uint_fast8_t address
);

void usb_set_address_deferred(
	const USBDevice* const device,
	const uint_fast8_t address
);

void usb_endpoint_init_without_descriptor(
	const USBEndpoint* const endpoint,
  uint_fast16_t max_packet_size,
  USBTransferType transfer_type
);

void usb_endpoint_init(
	const USBEndpoint* const endpoint
);

void usb_endpoint_stall(
	const USBEndpoint* const endpoint
);

void usb_endpoint_disable(
	const USBEndpoint* const endpoint
);

void usb_endpoint_flush(
	const USBEndpoint* const endpoint
);

bool usb_endpoint_is_ready(
	const USBEndpoint* const endpoint
);

void usb_endpoint_prime(
	const USBEndpoint* const endpoint,
	USBTransferDescriptor* const first_td
);

void usb_endpoint_schedule_wait(
	const USBEndpoint* const endpoint,
        USBTransferDescriptor* const td
);

void usb_endpoint_schedule_append(
        const USBEndpoint* const endpoint,
        USBTransferDescriptor* const tail_td,
        USBTransferDescriptor* const new_td
);

#endif