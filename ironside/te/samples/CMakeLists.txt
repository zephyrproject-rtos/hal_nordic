# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# IronSide Integration Kit - bare-metal sample build (no Zephyr/west). Pick a sample with
# -DSAMPLE=<name> (default hello_world). scripts/build.py wraps the CMake
# configure + Ninja build into a single command; this file is the build recipe.
#
# Dependencies: a bare-metal Arm GCC (via cmake/arm-none-eabi.cmake), nrfx
# (drivers + HAL + MDK) and CMSIS-Core, consumed from deps/ (deps/nrfx,
# deps/CMSIS_6) and overridable with IRONSIDE_INTEGRATION_KIT_NRFX /
# IRONSIDE_INTEGRATION_KIT_CMSIS. A RISC-V GCC is an OPTIONAL extra, needed only
# by a sample that runs code on a VPR core (cmake/riscv-vpr.cmake). See
# PREREQUISITES.md for setup.

cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(integration_kit_sample LANGUAGES C ASM)

if(NOT SAMPLE)
  set(SAMPLE hello_world)
endif()

set(SAMPLES_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(SDK_ROOT "${SAMPLES_ROOT}/..")
# IronSide TE Client glue: sets IRONSIDE_CLIENT_ROOT and defines the
# ironside_client_add[_headers]() helpers a sample.cmake calls.
include("${SAMPLES_ROOT}/cmake/ironside_client.cmake")
# VPR (FLPR) glue: defines integration_kit_vpr_add_stub(), which a sample.cmake calls to
# assemble code for the VPR core with the optional RISC-V toolchain. Including
# this probes for nothing - a build acquires that dependency only by calling it.
include("${SAMPLES_ROOT}/cmake/riscv-vpr.cmake")
# A buildable dir lives under this samples/ tree (demonstration apps that also
# self-check, ships in the IronSide Integration Kit release) or a sibling tests directory
# (test-only firmware verifying an IronSide/UROT guarantee, dev/CI-only -
# absent from a release bundle, so this fallback is a no-op there); resolve the
# name from either - names are unique across the two. build.py resolves it the
# same way.
set(SAMPLE_DIR "${SAMPLES_ROOT}/${SAMPLE}")
if(NOT IS_DIRECTORY "${SAMPLE_DIR}")
  set(SAMPLE_DIR "${SAMPLES_ROOT}/../../test/tests/${SAMPLE}")  # INTEGRATION-KIT-DEV-ONLY
endif()
if(NOT IS_DIRECTORY "${SAMPLE_DIR}")
  message(FATAL_ERROR "no such sample/test: ${SAMPLE} (looked in samples/ and the sibling tests directory)")
endif()

# Dependency paths - deps/ checkouts by default, overridable.
if(NOT IRONSIDE_INTEGRATION_KIT_NRFX)
  set(IRONSIDE_INTEGRATION_KIT_NRFX "${SAMPLES_ROOT}/deps/nrfx")
endif()
if(NOT IRONSIDE_INTEGRATION_KIT_CMSIS)
  set(IRONSIDE_INTEGRATION_KIT_CMSIS "${SAMPLES_ROOT}/deps/CMSIS_6/CMSIS/Core/Include")
endif()
get_filename_component(NRFX  "${IRONSIDE_INTEGRATION_KIT_NRFX}"  ABSOLUTE)
get_filename_component(CMSIS "${IRONSIDE_INTEGRATION_KIT_CMSIS}" ABSOLUTE)
set(MDK "${NRFX}/bsp/stable/mdk")
set(BSP "${NRFX}/bsp/stable")

foreach(p "${MDK}" "${CMSIS}")
  if(NOT IS_DIRECTORY "${p}")
    message(FATAL_ERROR "missing dependency path: ${p}")
  endif()
endforeach()

# Which part to build for. These are bare-metal builds with no Zephyr and no
# Kconfig, so the SoC cannot come from a board target -- the caller states it
# (scripts/build.py passes --soc through as IRONSIDE_INTEGRATION_KIT_SOC).
#
# Everything that differs between parts lives in this one table. Note what is NOT
# in it: the App linker script (lib/app.ld is part-independent -- it derives every
# address from ironside_client/layout.h, which resolves them from the MDK device
# macro below), the core frequency (both supported parts boot at 128 MHz), and the
# console UARTE instance. UARTE20 is right on both DKs: the nRF54LM20 DK's board
# dtsi picks uart20 for the console, and IronSide's own `ite` board variant moves
# the nRF54LV10 DK's console to uart20 as well, precisely so the host-side port
# helpers and this build need no per-board knowledge.
set(IRONSIDE_INTEGRATION_KIT_SOC "" CACHE STRING "MDK device to build samples for")
if(IRONSIDE_INTEGRATION_KIT_SOC STREQUAL "")
  message(FATAL_ERROR "IRONSIDE_INTEGRATION_KIT_SOC is not set. Name the target SoC, such as "
    "-DIRONSIDE_INTEGRATION_KIT_SOC=nrf54lm20b.")
elseif(IRONSIDE_INTEGRATION_KIT_SOC STREQUAL "nrf54lm20a"
       OR IRONSIDE_INTEGRATION_KIT_SOC STREQUAL "nrf54lm20b")
  # The two LM20 dice share a memory map and MDK startup; the A headers serve both.
  set(INTEGRATION_KIT_MDK_DEVICE "nrf54lm20a")
  set(INTEGRATION_KIT_MDK_MACRO  "NRF54LM20A_XXAA")
elseif(IRONSIDE_INTEGRATION_KIT_SOC STREQUAL "nrf54lv10a")
  set(INTEGRATION_KIT_MDK_DEVICE "nrf54lv10a")
  set(INTEGRATION_KIT_MDK_MACRO  "NRF54LV10A_XXAA")
elseif(IRONSIDE_INTEGRATION_KIT_SOC STREQUAL "nrf54l15")
  set(INTEGRATION_KIT_MDK_DEVICE "nrf54l15")
  set(INTEGRATION_KIT_MDK_MACRO  "NRF54L15_XXAA")
else()
  message(FATAL_ERROR
    "IRONSIDE_INTEGRATION_KIT_SOC=${IRONSIDE_INTEGRATION_KIT_SOC} is not a supported part. "
    "Add it here with its MDK startup file and device macro, and give it a branch in "
    "client/include/ironside_client/internal/layout_abi.h.")
endif()
message(STATUS
  "IronSide Integration Kit samples: ${IRONSIDE_INTEGRATION_KIT_SOC} -> ${INTEGRATION_KIT_MDK_MACRO}")

# The configuration of the IronSide bundle the App runs under
# (ironside_bundle_config.h, written by scripts/ironside_bundle.py
# export-config --app-header). client/include/ironside_client/layout.h places the App's
# NVM slot and RAM from it.
set(IRONSIDE_BUNDLE_CONFIG_DIR "" CACHE PATH "directory holding ironside_bundle_config.h")
set(IRONSIDE_BUNDLE_CONFIG_H "${IRONSIDE_BUNDLE_CONFIG_DIR}/ironside_bundle_config.h")
if(NOT EXISTS "${IRONSIDE_BUNDLE_CONFIG_H}")
  message(FATAL_ERROR "IRONSIDE_BUNDLE_CONFIG_DIR must name the directory holding "
    "ironside_bundle_config.h for the IronSide bundle the App runs under. Write it with "
    "scripts/ironside_bundle.py export-config --app-header, or build with scripts/build.py --bundle.")
endif()
message(STATUS "integration_kit samples: bundle configuration ${IRONSIDE_BUNDLE_CONFIG_H}")

# nRF54LM20A application core. Startup + system come from the nrfx MDK. The MDK
# Reset_Handler still runs on every reset (it copies .data); -D__START=main only
# makes it branch to main at the end instead of the libc _start. We add
# -D__STARTUP_CLEAR_BSS so Reset_Handler also zeroes .bss/.sbss/.tbss: without it
# BSS keeps stale values across a warm reset (only a full RAM wipe / `nrfutil
# device recover` happened to zero it), which intermittently hung library code
# that assumes zero-initialised statics (e.g. MPSL inside mpsl_init).
#
# The sample is a NON-SECURE image: it runs behind UROT, which owns the secure
# peripherals (clock/oscillators, TAMPC, FICR trims, ...). It must therefore be
# built with NRF_TRUSTZONE_NONSECURE (so the MDK uses the 0x4xxxxxxx non-secure
# peripheral aliases and skips the secure-only SystemInit block) and
# NRF_SKIP_CLOCK_CONFIGURATION (the OSCILLATORS->PLL.FREQ write in SystemInit is
# NOT behind the TrustZone guard, and OSCILLATORS stays secure - UROT configures
# the clock). Without these the very first SystemInit access (OSCILLATORS @
# 0x50120800) raises a SecureFault the moment UROT jumps to the App.
file(GLOB SAMPLE_SOURCES "${SAMPLE_DIR}/*.c")

add_executable(${SAMPLE}
  "${MDK}/gcc_startup_${INTEGRATION_KIT_MDK_DEVICE}_application.S"
  "${MDK}/system_nrf54l.c"
  "${NRFX}/drivers/src/nrfx_uarte.c"
  "${SAMPLES_ROOT}/lib/integration_kit_uart.c"
  ${SAMPLE_SOURCES}
)
set_target_properties(${SAMPLE} PROPERTIES SUFFIX ".elf")

target_compile_definitions(${SAMPLE} PRIVATE
  ${INTEGRATION_KIT_MDK_MACRO} NRF_APPLICATION __START=main __STARTUP_CLEAR_BSS
  NRF_TRUSTZONE_NONSECURE NRF_SKIP_CLOCK_CONFIGURATION
  # A non-secure image cannot rely on the default clock (the secure image owns
  # NRF_OSCILLATORS), so the HAL requires the CPU frequency explicitly. The
  # Both supported parts boot at 128 MHz (SystemInit's CK128M default), so this
  # is not in the per-SoC table above.
  NRF_CONFIG_CPU_FREQ_MHZ=128
  # Every sample uses the shared console helper (lib/integration_kit_uart.c), which
  # needs the nrfx UARTE *driver* (nrfx_uarte.c, added above) rather than the
  # HAL directly. The driver is gated behind NRFX_UARTE_ENABLED/NRFX_UARTE20_ENABLED
  # in nrfx_config (the per-device template defines them with #ifndef, so
  # defining them here wins without editing any nrfx file). NRFX_PRS_ENABLED=0:
  # PRS (peripheral resource sharing) only arbitrates when several driver
  # "personalities" share one SERIAL instance (UARTE20 vs SPIM20/TWIM20 ...) --
  # every sample uses only UARTE20, so there is nothing to arbitrate, and this
  # drops nrfx_prs.c from the build.
  NRFX_UARTE_ENABLED=1
  NRFX_UARTE20_ENABLED=1
  NRFX_PRS_ENABLED=0
)
target_compile_options(${SAMPLE} PRIVATE
  -mcpu=cortex-m33 -mthumb -ffreestanding -Os -g -Wall -Wextra
  # C only: the MDK startup .S is preprocessed too, and a C declaration there
  # does not assemble.
  "$<$<COMPILE_LANGUAGE:C>:SHELL:-include ${SAMPLES_ROOT}/lib/integration_kit_main.h>"
)
target_include_directories(${SAMPLE} PRIVATE
  "${SAMPLES_ROOT}/lib"          # project nrfx_config.h/nrfx_glue.h, integration_kit_uart.h
  "${NRFX}"                      # nrfx.h, drivers/, hal/, haly/, helpers/
  "${NRFX}/drivers/include"      # nrfx_uarte.h (driver API + #include <nrfx_log.h>)
  "${NRFX}/templates"            # no-op nrfx_log.h template, nrfx_config_common.h
  "${BSP}/templates"             # nrfx_config dispatcher + per-device nrfx_config_*.h
  "${BSP}"                       # <soc/...> (nrfx_irqs, nrfx_coredep, interconnect)
  "${MDK}"                       # device headers, nrf.h
  "${CMSIS}"
  "${SAMPLE_DIR}"
  "${IRONSIDE_BUNDLE_CONFIG_DIR}" # ironside_bundle_config.h, for ironside_client/layout.h
)
# The App linker script derives its FLASH/RAM regions from the single source of
# truth - the client's ironside_client/layout.h. ld can't #include a C header
# or expand macros, so preprocess the script with the C compiler first (the
# client on the include path, MDK device macro defined). nrf_common.ld is left
# for ld to INCLUDE at link time.
set(APP_LD_SRC "${SAMPLES_ROOT}/lib/app.ld")
set(APP_LD "${CMAKE_BINARY_DIR}/app.generated.ld")
add_custom_command(
  OUTPUT "${APP_LD}"
  COMMAND "${CMAKE_C_COMPILER}" -E -P -x c
          -I "${IRONSIDE_CLIENT_ROOT}/include" -I "${IRONSIDE_BUNDLE_CONFIG_DIR}"
          -D${INTEGRATION_KIT_MDK_MACRO}
          "${APP_LD_SRC}" -o "${APP_LD}"
  DEPENDS "${APP_LD_SRC}" "${IRONSIDE_CLIENT_ROOT}/include/ironside_client/layout.h"
          "${IRONSIDE_CLIENT_ROOT}/include/ironside_client/internal/layout_abi.h"
          "${IRONSIDE_BUNDLE_CONFIG_H}"
  COMMENT "Preprocessing App linker script from ironside_client/layout.h"
  VERBATIM
)
add_custom_target(${SAMPLE}_linker_script DEPENDS "${APP_LD}")
add_dependencies(${SAMPLE} ${SAMPLE}_linker_script)
# Re-link when the generated script changes (a -T file isn't a tracked input).
set_property(TARGET ${SAMPLE} APPEND PROPERTY LINK_DEPENDS "${APP_LD}")

target_link_options(${SAMPLE} PRIVATE
  -mcpu=cortex-m33 -mthumb
  -T "${APP_LD}"
  -L "${MDK}"
  -nostartfiles --specs=nosys.specs -Wl,--gc-sections
)

# Optional per-sample build tweaks. A sample may ship a sample.cmake next to its
# sources to extend the build - e.g. add compile definitions, extra sources,
# include dirs, or link options to the ${SAMPLE} target. It runs after the
# defaults above (so it can append/override) with ${SAMPLE}, ${SAMPLE_DIR},
# ${NRFX}, ${MDK}, ${CMSIS} and the cmake/ironside_client.cmake helpers in
# scope. Absent for most samples.
include("${SAMPLE_DIR}/sample.cmake" OPTIONAL)

# Emit a .hex next to the .elf and print a size summary - mirrors build.py.
add_custom_command(TARGET ${SAMPLE} POST_BUILD
  COMMAND "${CMAKE_OBJCOPY}" -O ihex "$<TARGET_FILE:${SAMPLE}>" "$<TARGET_FILE_DIR:${SAMPLE}>/${SAMPLE}.hex"
  COMMAND "${IRONSIDE_INTEGRATION_KIT_SIZE}" "$<TARGET_FILE:${SAMPLE}>"
  VERBATIM
  COMMENT "Creating ${SAMPLE}.hex"
)
