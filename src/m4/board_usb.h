#ifndef BOARD_USB_H
#define BOARD_USB_H

#include <stdbool.h>

bool board_usb_init();
void board_usb_run();
void board_usb_send_hello();
bool board_usb_init_done();

#endif