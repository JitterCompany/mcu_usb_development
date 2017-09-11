#include "usb_device.h"

#include "usb_type.h"

#include "usb_descriptor.h"

usb_configuration_t usb0_configuration_high_speed = {
	.number = 1,
	.speed = USB_SPEED_HIGH,
	.descriptor = usb0_descriptor_configuration_high_speed,
};

usb_configuration_t usb0_configuration_full_speed = {
	.number = 1,
	.speed = USB_SPEED_FULL,
	.descriptor = usb0_descriptor_configuration_full_speed,
};

usb_configuration_t* usb0_configurations[] = {
	&usb0_configuration_high_speed,
	//&usb0_configuration_full_speed,
	0,
};

usb_device_t usb_devices[] = {
	{
		.descriptor = usb0_descriptor_device,
		//.descriptor_strings = usb0_descriptor_strings,
		//.qualifier_descriptor = usb0_descriptor_device_qualifier,
		.configurations = &usb0_configurations,
		.configuration = 0,
		.controller = 0,
	}
};