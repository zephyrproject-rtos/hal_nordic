# Add project source files
list(APPEND APP_SPECIFIC_SOURCES ${CMAKE_SOURCE_DIR}/main.c)

set(EXAMPLE_BOILERPLATE_PATH "source/tests/scripts.cmake/boilerplate_example.cmake")
include(${CMAKE_CURRENT_LIST_DIR}/../../nrfx-verification/${EXAMPLE_BOILERPLATE_PATH})

project(nrfx_example LANGUAGES C ASM)
