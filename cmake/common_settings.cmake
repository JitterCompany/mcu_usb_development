#------------------------------------------------------------------------------
# CPM init
#------------------------------------------------------------------------------
#
# Required variables:
#
# TOPLEVEL_SRC_DIR
# TOPLEVEL_BIN_DIR

set(CMAKE_SOURCE_DIR ${TOPLEVEL_SRC_DIR})
include(${CMAKE_SOURCE_DIR}/cmake/CPM_setup.cmake)


#------------------------------------------------------------------------------
# Detect Board Specific Settings
#------------------------------------------------------------------------------

# defaults
set(SERVER_PORT         3443)

set(EN_DEBUG            0)
set(EN_PROFILE          0)
set(EN_GPRS             1)
set(EN_USB              1)
set(OPTIMIZE            2)

# find board config
include(${CMAKE_SOURCE_DIR}/board_config.cmake)

add_definitions(-DEN_DEBUG=${EN_DEBUG})
add_definitions(-DEN_PROFILE=${EN_PROFILE})
add_definitions(-DEN_USB=${EN_USB})
add_definitions(-DOPTIMIZE=${OPTIMIZE})

# build options string: only append non-default options

if("${EN_DEBUG}" STREQUAL "1")
    add_definitions(-DDEBUG_ENABLED)        # for backwards compatibility
    list(APPEND BUILD_OPTS "-debug")
endif()

if("${EN_PROFILE}" STREQUAL "1")
    add_definitions(-DPROFILE_ENABLED=1)    # for backwards compatibility
    list(APPEND BUILD_OPTS "-profile")
endif()

if("${EN_USB}" STREQUAL "0")
    list(APPEND BUILD_OPTS "-noUSB")
endif()

if(NOT("${OPTIMIZE}" STREQUAL "2"))
    list(APPEND BUILD_OPTS "-O${OPTIMIZE}")
endif()

string(REPLACE ";" "" BUILD_OPTS "${BUILD_OPTS}")

if("${BOARD}" STREQUAL "BOARD_DEV")
    set(BOARD_CFG_FILE "board_dev_config.c")
elseif("${BOARD}" STREQUAL "BOARD_0501")
    set(BOARD_CFG_FILE "board_0501_config.c")
else()
    message(FATAL_ERROR "${EXE_NAME}: board '${BOARD}' not recognised: \
    \n Set a valid option in board_config.cmake")
endif()

list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/src/board_config/${BOARD_CFG_FILE}")
message(STATUS "${EXE_NAME}: Detected board config '${BOARD}'")
add_definitions("-D${BOARD}")

#------------------------------------------------------------------------------
# Common Config
#------------------------------------------------------------------------------

set(FLASH_CFG               lpc4337_swd)

# system libraries to link, separated by ';'
set(SYSTEM_LIBRARIES m c gcc)

# compile flags
set(C_FLAGS_WARN "-Wall -Wextra -Wno-unused-parameter                   \
    -Wshadow -Wpointer-arith -Winit-self -Wstrict-overflow=4            \
    -Werror=implicit-function-declaration")

set(C_FLAGS "${C_FLAGS_WARN} -O${OPTIMIZE} -g3 -c -fmessage-length=80   \
    -fno-builtin -ffunction-sections -fdata-sections -std=gnu99 -mthumb \
    -fdiagnostics-color=auto")

# linker flags
set(L_FLAGS "${TARGET_L_FLAGS} -fmessage-length=80 -nostdlib -specs=nano.specs \
    -mthumb -Wl,--gc-sections")

set(C_FLAGS_PLATFORM    "-DMCU_PLATFORM_${MCU_PLATFORM}")
set(ALL_C_FLAGS "${TARGET_C_FLAGS} ${C_FLAGS} ${C_FLAGS_WARN} ${C_FLAGS_PLATFORM}")
message(STATUS "C flags: '${ALL_C_FLAGS}")
message(STATUS "L flags: '${L_FLAGS}")
#------------------------------------------------------------------------------
# Common Variables & definitions
#------------------------------------------------------------------------------

set(ELF_PATH            "${CMAKE_CURRENT_BINARY_DIR}/${EXE_NAME}")
set(EXE_PATH            "${ELF_PATH}.bin")
set(FLASH_FILE          ${TOPLEVEL_BIN_DIR}/flash.cfg)
set(FW_UPDATE_CFG_FILE  ${TOPLEVEL_BIN_DIR}/flash.cfg)
set(FW_SIGN_TOOL        ${TOPLEVEL_SRC_DIR}/sign.py)

add_definitions("${TARGET_C_FLAGS} ${C_FLAGS} ${C_FLAGS_WARN} ${C_FLAGS_PLATFORM}")

#------------------------------------------------------------------------------
# Common CPM Modules
#------------------------------------------------------------------------------

CPM_AddModule("openocd_tools"
    GIT_REPOSITORY "https://github.com/JitterCompany/openocd_tools.git"
    GIT_TAG "1.3")

CPM_AddModule("firmware_tools"
    GIT_REPOSITORY "https://github.com/JitterCompany/firmware_tools.git"
    GIT_TAG "1.1")

CPM_AddModule("c_utils"
    GIT_REPOSITORY "https://github.com/JitterCompany/c_utils.git"
    GIT_TAG "1.4.2")

