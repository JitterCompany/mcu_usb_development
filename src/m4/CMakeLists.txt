cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)

set(CMAKE_TOOLCHAIN_FILE
    ${TOPLEVEL_SRC_DIR}/cmake/toolchain-gcc-arm-embedded.cmake)
project(m4)

#------------------------------------------------------------------------------
# Target Specifig Build Settings
#------------------------------------------------------------------------------

# target specific
set(EXE_NAME                ${PROJECT_NAME})
set(FLASH_ADDR              0x1A008000)
set(DEBUG_BREAKPOINT_LIMIT  6)
set(DEBUG_WATCHPOINT_LIMIT  4)

set(MCU_PLATFORM            43xx_m4)

add_definitions(-D__FPU_PRESENT -D__ARM_ARCH_7M__)
set(FLOAT_FLAGS         "-mfloat-abi=hard -mfpu=fpv4-sp-d16")

set(TARGET_C_FLAGS      "-DCORE_M4 -mcpu=cortex-m4 -DUSB_DEVICE_ONLY \
    -D__LPC43XX__  ${FLOAT_FLAGS}")
set(TARGET_L_FLAGS      "-mcpu=cortex-m4 ${FLOAT_FLAGS}")

#------------------------------------------------------------------------------
# Include Common Settings & CPM Modules
#------------------------------------------------------------------------------

include(${TOPLEVEL_SRC_DIR}/cmake/common_settings.cmake)


#------------------------------------------------------------------------------
# Target Specific CPM Modules
#------------------------------------------------------------------------------

CPM_AddModule("43xx_m4_startup"
    GIT_REPOSITORY "https://github.com/JitterCompany/43xx_m4_startup.git"
    GIT_TAG "1.0")

CPM_AddModule("lpc_chip_43xx_m4"
    GIT_REPOSITORY "https://github.com/JitterCompany/lpc_chip_43xx_m4.git"
    GIT_TAG "3.0.1")

CPM_AddModule("mcu_timing"
    GIT_REPOSITORY "https://github.com/JitterCompany/mcu_timing.git"
    GIT_TAG "1.4.0")

CPM_AddModule("lpc_tools"
    GIT_REPOSITORY "https://github.com/JitterCompany/lpc_tools.git"
    GIT_TAG "2.3")

CPM_AddModule("lpc_usb_lib"
    GIT_REPOSITORY "https://github.com/JitterCompany/lpc_usb_lib.git"
    GIT_TAG "2.0.2-beta")

CPM_AddModule("fatfs_lib"
    GIT_REPOSITORY "https://github.com/JitterCompany/fatfs_lib.git"
    GIT_TAG "1.2")

CPM_AddModule("one_time_heap"
    GIT_REPOSITORY "https://github.com/JitterCompany/one_time_heap.git"
    GIT_TAG "2.0")

CPM_Finish()


#------------------------------------------------------------------------------
# Build Setup & Targets
#------------------------------------------------------------------------------

file(GLOB SPECIFIC_SOURCES 
"${CMAKE_SOURCE_DIR}/src/m4/*.c"
)
list(APPEND SOURCES ${SPECIFIC_SOURCES})

include(${CMAKE_SOURCE_DIR}/cmake/common_build.cmake)

