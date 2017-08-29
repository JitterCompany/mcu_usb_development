#-----------------------------------------------------------------------
# Setup source
#-----------------------------------------------------------------------

get_property(startup_linker GLOBAL PROPERTY startup_linker)
set(LINKER_FILES "-L \"${CMAKE_SOURCE_DIR}/src/\" -T ${startup_linker}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${L_FLAGS} \
${LINKER_FILES}")


file(GLOB COMMON_SOURCES 
"${CMAKE_SOURCE_DIR}/src/*.c"
)

include_directories("${CMAKE_SOURCE_DIR}/src")
list(APPEND SOURCES "${COMMON_SOURCES}")

#-----------------------------------------------------------------------
# Setup executable
#-----------------------------------------------------------------------

if(NOT TARGET prepare)
    add_custom_target(prepare

        # empty flash file
        COMMAND echo "Prepare empty flash file"
        COMMAND > "${FLASH_FILE}"
        )
endif(NOT TARGET prepare)

if(NOT TARGET bin)
    add_custom_target(bin ALL
        )
endif(NOT TARGET bin)

if(NOT TARGET version)
    set(FW_VERSION_FILE "${CMAKE_SOURCE_DIR}/src/generated/firmware_version.h")

    add_custom_command(OUTPUT ${FW_VERSION_FILE}
        DEPENDS version
        )

    add_custom_target(version
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/version.cmake "${FW_VERSION_FILE}" "${BUILD_OPTS}" 
        )

endif(NOT TARGET version)

add_executable(${EXE_NAME} ${SOURCES} ${FW_VERSION_FILE})
target_link_libraries(${EXE_NAME} ${CPM_LIBRARIES})
target_link_libraries(${EXE_NAME} ${SYSTEM_LIBRARIES})

add_custom_target(${EXE_NAME}.bin
    DEPENDS ${EXE_NAME}

    COMMAND echo "building ${EXE_NAME}.bin"
    COMMAND ${CMAKE_OBJCOPY} -O binary ${EXE_NAME} ${EXE_NAME}.bin
    COMMAND python ${FW_SIGN_TOOL} ${EXE_NAME}.bin

    # append flash file
    COMMAND echo "${EXE_PATH} ${FLASH_ADDR} ${FLASH_CFG} ${MCU_PLATFORM}" >> "${FLASH_FILE}"
    )

add_dependencies(bin ${EXE_NAME}.bin)

