# File containing common functions, macros and variables used across all examples.
# This file should be included in each CMakeLists.txt if compiled with west tool under Zephyr.

# Add common Kconfig file to project.
list(APPEND EXTRA_CONF_FILE "${CMAKE_CURRENT_LIST_DIR}/common.conf")

# Function stripping the version from a BOARD string, useful for matching <board>.conf files
# e.g. nrf9160dk@0.14.0/nrf9160 -> nrf9160dk/nrf9160
function(strip_board_version INPUT OUTPUT)
    # Check if "@" exists
    string(FIND "${INPUT}" "@" AT_POS)
    if(AT_POS EQUAL -1)
        set(${OUTPUT} "${INPUT}" PARENT_SCOPE)
        return()
    endif()

    # Find position of first "/" after "@"
    string(SUBSTRING "${INPUT}" ${AT_POS} -1 SUB_AFTER_AT)
    string(FIND "${SUB_AFTER_AT}" "/" SLASH_RELATIVE_POS)
    if(SLASH_RELATIVE_POS EQUAL -1)
        # If no "/", remove from @ to end
        string(SUBSTRING "${INPUT}" 0 ${AT_POS} PREFIX)
        set(${OUTPUT} "${PREFIX}" PARENT_SCOPE)
        return()
    endif()

    math(EXPR SLASH_POS "${AT_POS} + ${SLASH_RELATIVE_POS}")

    # Get parts before "@" and after "/"
    string(SUBSTRING "${INPUT}" 0 ${AT_POS} PREFIX)
    math(EXPR AFTER_SLASH_POS "${SLASH_POS} + 1")
    string(SUBSTRING "${INPUT}" ${AFTER_SLASH_POS} -1 SUFFIX)

    # Join parts
    set(${OUTPUT} "${PREFIX}/${SUFFIX}" PARENT_SCOPE)
endfunction()

function(get_board_from_sysbuild_cache SYSBUILD_BOARD)
    # Read the sysbuild cache file into a variable
    file(READ "${SYSBUILD_CACHE}" _sysbuild_cache_content)

    # Extract the BOARD value
    string(REGEX MATCH "BOARD:STRING=([^\n\r]+)" _match "${_sysbuild_cache_content}")

    # The captured group is in CMAKE_MATCH_1
    set(${SYSBUILD_BOARD} "${CMAKE_MATCH_1}" PARENT_SCOPE)

    message(STATUS "BOARD from Sysbuild cache: ${CMAKE_MATCH_1}")

endfunction()

# When building with Sysbuild, get the BOARD variable from cache as it is not present.
if(SYSBUILD)
    get_board_from_sysbuild_cache(BOARD)
endif()

# Macro adding <board>.overlay and <board>.conf file for specified board in given directory.
# If board-specific files have been found they are added to DTC_OVERLAY_FILE and EXTRA_CONF_FILE
# symbols that are used in west build system.
macro(GET_DEVICE_CONFIG_FILES BOARD BOARDS_DIR)
    if(NOT IS_ABSOLUTE BOARDS_DIR)
        set(_BOARDS_DIR ${CMAKE_CURRENT_LIST_DIR}/${BOARDS_DIR})
        get_filename_component(BOARDS_DIR_ABSOLUTE "${_BOARDS_DIR}" ABSOLUTE)
    else()
        set(BOARDS_DIR_ABSOLUTE {BOARDS_DIR})
    endif()

    # for the <board>.conf and <board>.overlay file path remove version
    strip_board_version(${BOARD} BOARD_WITHOUT_VERSION)
    string(REPLACE "/" "_" BOARD_UNDERSCORES ${BOARD_WITHOUT_VERSION})

    set(OVERLAY_FILE "${BOARDS_DIR_ABSOLUTE}/${BOARD_UNDERSCORES}.overlay")
    if(EXISTS ${OVERLAY_FILE})
        list(APPEND DTC_OVERLAY_FILE "${OVERLAY_FILE}")
    endif()

    set(BOARD_CONFIG_FILE "${BOARDS_DIR_ABSOLUTE}/${BOARD_UNDERSCORES}.conf")
    if(EXISTS ${BOARD_CONFIG_FILE})
        list(APPEND EXTRA_CONF_FILE "${BOARD_CONFIG_FILE}")
    endif()
endmacro()
