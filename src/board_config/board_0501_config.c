#include "chip.h"
#include "board_0501_config.h"
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
    {PIN_INT0_IRQn,     5},     // Pin interrupt lower priority than sd card
    {ADC0_IRQn,         6},     // no priority for battery voltage
};


static const PINMUX_GRP_T pinmuxing[] = {

        // Hardware version bits
        {6, 1, (SCU_MODE_FUNC0 
                | SCU_MODE_INBUFF_EN
                | SCU_MODE_PULLUP)},     // HW_VERSION_B0
        {2, 0, (SCU_MODE_FUNC4 
                | SCU_MODE_INBUFF_EN
                | SCU_MODE_PULLUP)},     // HW_VERSION_B1
        {2, 3, (SCU_MODE_FUNC4 
                | SCU_MODE_INBUFF_EN
                | SCU_MODE_PULLUP)},     // HW_VERSION_B2

        // Debug LEDs 
        {4, 9, (SCU_MODE_FUNC4)}, //blue
        {4, 10, (SCU_MODE_FUNC4)}, //red

        // USB
        {4, 8, (SCU_MODE_FUNC4 
                | SCU_MODE_INBUFF_EN)}, //VUSB_as_GPIO

        // SD Card
        {1, 6, SCU_PINIO_FAST 
            |  SCU_MODE_FUNC7}, //cmd
        {1, 9, SCU_PINIO_FAST 
            | SCU_MODE_FUNC7}, //data0
        {1, 10, SCU_PINIO_FAST 
            | SCU_MODE_FUNC7}, //data1
        {1, 11, SCU_PINIO_FAST 
            | SCU_MODE_FUNC7}, //data2
        {1, 12, SCU_PINIO_FAST 
            | SCU_MODE_FUNC7}, //data3
        {1, 3, SCU_MODE_PULLUP 
            | SCU_MODE_FUNC0 | SCU_MODE_INBUFF_EN}, //SD Detect

        // Power
        {1, 13, SCU_MODE_FUNC0}, //DCDC_EN
        {5, 5, (SCU_MODE_FUNC0 
            | SCU_MODE_INBUFF_EN)}, //1V8_OK
        {1, 5, (SCU_MODE_FUNC0 
            | SCU_MODE_INBUFF_EN)}, //3V3_OK
        {5, 3, (SCU_MODE_FUNC0 
            | SCU_MODE_INBUFF_EN)}, //5V_OK
        {5, 4, (SCU_MODE_FUNC0 
            | SCU_MODE_INBUFF_EN)}, //10V_OK

        // ADC
        {1, 14, (SCU_MODE_FUNC6 
            | SCU_MODE_INBUFF_EN | SCU_MODE_INACT)}, // SGPIO10 Qualifier

        {1, 2, (SCU_MODE_FUNC3)},   // SGPIO9 Qualifier_RST
        {9, 6, (SCU_MODE_FUNC6)},   // SGPIO8 SCLK

        {1, 15, (SCU_MODE_FUNC2 
            | SCU_MODE_INBUFF_EN)}, // SGPIO2 DOUT1 
        {9, 5, (SCU_MODE_FUNC6 
            | SCU_MODE_INBUFF_EN)}, // SGPIO3 DOUT2
        {2, 6, (SCU_MODE_FUNC0 
            | SCU_MODE_INBUFF_EN)}, // SGPIO7 DOUT3
        {7, 0, (SCU_MODE_FUNC7 
            | SCU_MODE_INBUFF_EN)}, // SGPIO4 DOUT4
        {7, 1, (SCU_MODE_FUNC7 
            | SCU_MODE_INBUFF_EN)}, // SGPIO5 DOUT5
        {7, 2, (SCU_MODE_FUNC7 
            | SCU_MODE_INBUFF_EN)}, // SGPIO6 DOUT6
        {0, 1, (SCU_MODE_FUNC3 
            | SCU_MODE_INBUFF_EN)}, // SGPIO1 DOUT7
        {0, 0, (SCU_MODE_FUNC3 
            | SCU_MODE_INBUFF_EN)}, // SGPIO0 DOUT8
        {3, 2, (SCU_MODE_FUNC4)},   // !SYNC 
        {1, 17, (SCU_MODE_FUNC6)},  // SGPIO11 CLK
        {7, 3, (SCU_MODE_FUNC0)},   // CLKDIV
        {6, 2, (SCU_MODE_FUNC0)},   // MODE0
        {6, 3, (SCU_MODE_FUNC0)},   // MODE1
        {6, 6, (SCU_MODE_FUNC0)},   // FORMAT0
        {6, 7, (SCU_MODE_FUNC4)},   // FORMAT1
        {6, 8, (SCU_MODE_FUNC4)},   // FORMAT2
        {3, 1, (SCU_MODE_FUNC4)},   // !PWDN1
        {2, 13, (SCU_MODE_FUNC0)},  // !PWDN2
        {2, 12, (SCU_MODE_FUNC0)},  // !PWDN3
        {2, 11, (SCU_MODE_FUNC0)},  // !PWDN4
        {2, 10, (SCU_MODE_FUNC0)},  // !PWDN5
        {6, 12, (SCU_MODE_FUNC0)},  // !PWDN6
        {2, 9, (SCU_MODE_FUNC0)},   // !PWDN7
        {2, 8, (SCU_MODE_FUNC4)},   // !PWDN8

        // IO
        {1, 1, (SCU_MODE_FUNC0)},  // LED_power
        {1, 0, (SCU_MODE_FUNC0)},  // LED_running
        {5, 1, (SCU_MODE_FUNC0)},  // LED_standby
        {5, 0, (SCU_MODE_FUNC0)},  // LED_SDCard
        {5, 2, (SCU_MODE_FUNC0
            | SCU_MODE_INBUFF_EN)},  // BTN_startStop
        {1, 4, (SCU_MODE_FUNC0
            | SCU_MODE_INBUFF_EN)},  // EXT_startStop
       
        // Extra IO
        {4,1, (SCU_MODE_FUNC0)},     //


        // debug shorts
        {4, 6, (SCU_MODE_INACT)},  // HACK to fix crystal as this pin is shorted.
};


static const GPIOConfig pin_config[] = {

    // HW VERSION
    {{3, 0 }, GPIO_CFG_DIR_INPUT},          // GPIO_ID_HW_VERSION_B0
    {{5, 0 }, GPIO_CFG_DIR_INPUT},          // GPIO_ID_HW_VERSION_B1
    {{5, 3 }, GPIO_CFG_DIR_INPUT},          // GPIO_ID_HW_VERSION_B2

    // DEBUG
    {{5, 13 }, GPIO_CFG_DIR_OUTPUT_LOW},    // GPIO_ID_LED_BLUE
    {{5, 14 }, GPIO_CFG_DIR_OUTPUT_LOW},    // GPIO_ID_LED_RED
    
    // USB
    {{5, 12 }, GPIO_CFG_DIR_INPUT},         // GPIO_ID_VUSB_as_GPIO

    // SD
    {{0, 10 }, GPIO_CFG_DIR_INPUT},         // GPIO_ID_SD_DETECT_INV

    //POWER
    {{1, 6 }, GPIO_CFG_DIR_OUTPUT_HIGH},    // GPIO_ID_DCDC_EN
    {{2, 14 }, GPIO_CFG_DIR_INPUT},         // GPIO_ID_1V8_OK
    {{1, 8 }, GPIO_CFG_DIR_INPUT},          // GPIO_ID_3V3_OK
    {{2, 12 }, GPIO_CFG_DIR_INPUT},         // GPIO_ID_5V_OK
    {{2, 13 }, GPIO_CFG_DIR_INPUT},         // GPIO_ID_10V_OK

    //ADC 
    // start with SYNC_INV low to reset adc
    {{5, 9 }, GPIO_CFG_DIR_OUTPUT_HIGH},        // GPIO_ID_SYNC_INV
    // CLKDIV fclk/fdata = 512
    {{3, 11 }, GPIO_CFG_DIR_OUTPUT_LOW},   // GPIO_ID_CLKDIV
    // MODE 10 low-speed mode
    {{3, 1 }, GPIO_CFG_DIR_OUTPUT_HIGH},        // GPIO_ID_MODE0
    {{3, 2 }, GPIO_CFG_DIR_OUTPUT_HIGH},        // GPIO_ID_MODE1
    // FORMAT 010 SPI Discrete mode
    {{0, 5 }, GPIO_CFG_DIR_OUTPUT_LOW},     // GPIO_ID_FORMAT0
    {{5, 15 }, GPIO_CFG_DIR_OUTPUT_HIGH},   // GPIO_ID_FORMAT1
    {{5, 16 }, GPIO_CFG_DIR_OUTPUT_LOW},    // GPIO_ID_FORMAT2
    {{5, 8 }, GPIO_CFG_DIR_OUTPUT_HIGH},     // GPIO_ID_PWDN1_INV
    {{1, 13 }, GPIO_CFG_DIR_OUTPUT_HIGH},    // GPIO_ID_PWDN2_INV
    {{1, 12 }, GPIO_CFG_DIR_OUTPUT_HIGH},    // GPIO_ID_PWDN3_INV
    {{1, 11 }, GPIO_CFG_DIR_OUTPUT_HIGH},    // GPIO_ID_PWDN4_INV
    {{0, 14 }, GPIO_CFG_DIR_OUTPUT_HIGH},    // GPIO_ID_PWDN5_INV
    {{2, 8 }, GPIO_CFG_DIR_OUTPUT_HIGH},     // GPIO_ID_PWDN6_INV
    {{1, 10 }, GPIO_CFG_DIR_OUTPUT_HIGH},    // GPIO_ID_PWDN7_INV
    {{5, 7 }, GPIO_CFG_DIR_OUTPUT_HIGH},     // GPIO_ID_PWDN8_INV

    //IO
    {{0, 8 }, GPIO_CFG_DIR_OUTPUT_LOW},     // GPIO_ID_LED_power
    {{0, 4 }, GPIO_CFG_DIR_OUTPUT_LOW},     // GPIO_ID_LED_running
    {{2, 10 }, GPIO_CFG_DIR_OUTPUT_LOW},    // GPIO_ID_LED_standby
    {{2, 9 }, GPIO_CFG_DIR_OUTPUT_LOW},     // GPIO_ID_LED_SDCard
    {{2, 11 }, GPIO_CFG_DIR_INPUT},         // GPIO_ID_BTN_StartStop
    {{0, 11 }, GPIO_CFG_DIR_INPUT},         // GPIO_ID_EXT_StartStop

    // EXTRA GPIO
    {{2, 1}, GPIO_CFG_DIR_OUTPUT_LOW},      // GPIO_ID_EXTRA_1
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
    const GPIO *blue = board_get_GPIO(GPIO_ID_LED_BLUE);
    const GPIO *red = board_get_GPIO(GPIO_ID_LED_RED);
	status_led_bind(BLUE, blue->port, blue->pin);
	status_led_bind(RED, red->port, red->pin);
}

