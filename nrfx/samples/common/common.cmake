# File containing common functions, macros and variables used across all examples.
# This file should be included in each CMakeLists.txt if compiled with west tool under Zephyr.

# Add common Kconfig file to project.
list(APPEND CONF_FILE "${CMAKE_CURRENT_LIST_DIR}/common.conf")

# Add project Kconfig file if it exists.
set(PRJ_CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/prj.conf")
if(EXISTS ${PRJ_CONF_FILE})
    list(APPEND CONF_FILE ${PRJ_CONF_FILE})
endif()

# Macro adding overlay file and Kconfig file for specified board in given directory.
# If board-specific files have been found they are added to DTC_OVERLAY_FILE and CONF_FILE
# symbols that are used in west build system.
macro(GET_DEVICE_CONFIG_FILES BOARD BOARDS_DIR)
    if(NOT IS_ABSOLUTE BOARDS_DIR)
        set(_BOARDS_DIR ${CMAKE_CURRENT_LIST_DIR}/${BOARDS_DIR})
        get_filename_component(BOARDS_DIR_ABSOLUTE "${_BOARDS_DIR}" ABSOLUTE)
    else()
        set(BOARDS_DIR_ABSOLUTE {BOARDS_DIR})
    endif()

    string(REPLACE "/" "_" BOARD_UNDERSCORES ${BOARD})
    set(OVERLAY_FILE "${BOARDS_DIR_ABSOLUTE}/${BOARD_UNDERSCORES}.overlay")
    if(EXISTS ${OVERLAY_FILE})
        list(APPEND DTC_OVERLAY_FILE "${OVERLAY_FILE}")
    endif()

    set(CONFIG_FILE "${BOARDS_DIR_ABSOLUTE}/${BOARD_UNDERSCORES}.conf")
    if(EXISTS ${CONFIG_FILE})
        list(APPEND CONF_FILE "${CONFIG_FILE}")
    endif()
endmacro()
