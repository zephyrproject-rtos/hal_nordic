# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Bare-metal Arm GCC toolchain for the IronSide Integration Kit - no Zephyr/west CMake.
# scripts/build.py passes IRONSIDE_INTEGRATION_KIT_GCC; configuring CMake by hand also works:
#   cmake -G Ninja -S samples -B samples/build/hello_world \
#         -DCMAKE_TOOLCHAIN_FILE=samples/cmake/arm-none-eabi.cmake -DSAMPLE=hello_world

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

if(NOT IRONSIDE_INTEGRATION_KIT_GCC)
  set(IRONSIDE_INTEGRATION_KIT_GCC arm-none-eabi-gcc)
endif()

set(CMAKE_C_COMPILER   "${IRONSIDE_INTEGRATION_KIT_GCC}")
set(CMAKE_ASM_COMPILER "${IRONSIDE_INTEGRATION_KIT_GCC}")

# Derive the sibling binutils by swapping the trailing "gcc" in the compiler
# name for the tool, preserving any directory (mirrors the old build.py).
get_filename_component(_dir  "${IRONSIDE_INTEGRATION_KIT_GCC}" DIRECTORY)
get_filename_component(_name "${IRONSIDE_INTEGRATION_KIT_GCC}" NAME)
string(REPLACE "gcc" "objcopy" _objcopy "${_name}")
string(REPLACE "gcc" "size"    _size    "${_name}")
if(_dir)
  set(_objcopy "${_dir}/${_objcopy}")
  set(_size    "${_dir}/${_size}")
endif()
set(CMAKE_OBJCOPY "${_objcopy}" CACHE FILEPATH "arm objcopy")
set(IRONSIDE_INTEGRATION_KIT_SIZE     "${_size}"    CACHE FILEPATH "arm size")

# Bare metal: there is no full libc/startup to link during CMake's compiler
# check, so build a static lib for the probe instead of an executable.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
