#ifndef BOARD_USB_H
#define BOARD_USB_H

#include <stdbool.h>

void board_usb_init();

// try to start an usb connection. Calls board_usb_init() if required.
void board_usb_try_start();

bool board_usb_is_connected(void);
void board_usb_stop();
void board_usb_tasks();

#endif

