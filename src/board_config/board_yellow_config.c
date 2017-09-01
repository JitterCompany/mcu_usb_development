#include "chip.h"
#include "board_dev_config.h"
#include <lpc_tools/boardconfig.h>
#include <lpc_tools/GPIO_HAL.h>
#include <c_utils/static_assert.h>
#include "status_led.h"

const uint32_t OscRateIn = 12000000;
const uint32_t ExtRateIn = 0;

static const NVICConfig NVIC_config[] = {
    // priority > 0 is disabled during deep sleep
    {TIMER3_IRQn,       1},     // delay timer: high priority to ensure
                                // timestamps are correct in any context
    {SPI_INT_IRQn,      2},     // SD1521 SPI
    {RITIMER_IRQn,      3},     // SD1521 polling
};


static const PINMUX_GRP_T pinmuxing[] = {

        // Board LEDs 
        {1, 15, (SCU_MODE_FUNC0)}, //green
        {1, 16, (SCU_MODE_FUNC0)}, //blue
        {1, 17, (SCU_MODE_FUNC0)}, //red
        {1, 18, (SCU_MODE_FUNC0)}, //yellow

        // Mock Button
        {2, 5, (SCU_MODE_FUNC4)}, // BTN GND
        {7, 7, (SCU_MODE_FUNC0 
            | SCU_MODE_INBUFF_EN | SCU_MODE_PULLUP)}, // BTN_startStop input

        
        // SD Card
        {1, 5, SCU_MODE_FUNC0}, //power enable
        {1, 6,SCU_PINIO_FAST 
            |  SCU_MODE_FUNC7}, //cmd
        {1, 9, SCU_PINIO_FAST 
            | SCU_MODE_FUNC7}, //data0
        {1, 10, SCU_PINIO_FAST 
            | SCU_MODE_FUNC7}, //data1
        {1, 11, SCU_PINIO_FAST 
            | SCU_MODE_FUNC7}, //data2
        {1, 12, SCU_PINIO_FAST 
            | SCU_MODE_FUNC7}, //data3
        {1, 13, SCU_MODE_FUNC7
            | SCU_MODE_INBUFF_EN}, //SD Detect
};


static const GPIOConfig pin_config[] = {
    {{0, 2}, GPIO_CFG_DIR_OUTPUT_LOW},      // GPIO_ID_LED_power
    {{0, 3 }, GPIO_CFG_DIR_OUTPUT_LOW},     // GPIO_ID_LED_running
    {{0, 12}, GPIO_CFG_DIR_OUTPUT_LOW},     // GPIO_ID_LED_standby
    {{0, 13}, GPIO_CFG_DIR_OUTPUT_LOW},     // GPIO_ID_LED_SDCard

    {{5, 5}, GPIO_CFG_DIR_OUTPUT_LOW},      // GPIO_ID_BTN_GND
    {{3, 15}, GPIO_CFG_DIR_INPUT},          // GPIO_ID_BTN_startStop

    {{1,6 }, GPIO_CFG_DIR_INPUT},           // GPIO_ID_SD_DETECT_INV

    {{1, 8 }, GPIO_CFG_DIR_OUTPUT_LOW}      // GPIO_ID_SDCARD_POWER_ENABLE
};

// pin config struct should match GPIO_ID enum
STATIC_ASSERT( (GPIO_ID_MAX == (sizeof(pin_config)/sizeof(GPIOConfig))));

static const BoardConfig config = {
    .nvic_configs = NVIC_config,
    .nvic_count = sizeof(NVIC_config) / sizeof(NVIC_config[0]),

    .pinmux_configs = pinmuxing,
    .pinmux_count = sizeof(pinmuxing) / sizeof(pinmuxing[0]),
    
    .GPIO_configs = pin_config,
    .GPIO_count = sizeof(pin_config) / sizeof(pin_config[0])
};

void board_setup(void)
{
    board_set_config(&config);

    Chip_SCU_ClockPinMuxSet(0, (SCU_PINIO_FAST | SCU_MODE_FUNC4)); //SD CLK

    board_setup_pins();
}

void board_LED_init(void)
{
    const GPIO *green = board_get_GPIO(GPIO_ID_LED_power);
    const GPIO *blue = board_get_GPIO(GPIO_ID_LED_running);
    const GPIO *red = board_get_GPIO(GPIO_ID_LED_standby);
    const GPIO *yellow = board_get_GPIO(GPIO_ID_LED_SDCard);
	status_led_bind(GREEN, green->port, green->pin);
	status_led_bind(BLUE, blue->port, blue->pin);
	status_led_bind(RED, red->port, red->pin);
	status_led_bind(YELLOW, yellow->port, yellow->pin);
}

