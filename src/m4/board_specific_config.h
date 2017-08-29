#ifndef BOARD_SPECIFIC_CONFIG_H
#define BOARD_SPECIFIC_CONFIG_H

void board_setup(void);
void board_LED_init(void);

#ifdef BOARD_DEV
    #include "board_config/board_dev_config.h"
#endif
#ifdef BOARD_0501
    #include "board_config/board_0501_config.h"
#endif

#endif

