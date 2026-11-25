# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Optional RISC-V toolchain - for a sample that runs code on a VPR core (FLPR).
#
# The dependency is OPTIONAL: nothing here probes for a compiler until a
# sample.cmake calls integration_kit_vpr_add_stub(), so every other sample builds with
# no RISC-V toolchain installed. See samples/PREREQUISITES.md.

# Tried in this order. Override with IRONSIDE_INTEGRATION_KIT_RISCV_GCC (a name on PATH, or a
# full path).
set(INTEGRATION_KIT_RISCV_GCC_NAMES
  riscv64-zephyr-elf-gcc
  riscv64-unknown-elf-gcc
  riscv32-unknown-elf-gcc
  riscv-none-elf-gcc
)

# Resolve the assembler and its objcopy, or fail saying what to install.
function(_integration_kit_vpr_toolchain out_gcc out_objcopy)
  if(IRONSIDE_INTEGRATION_KIT_RISCV_GCC AND IS_ABSOLUTE "${IRONSIDE_INTEGRATION_KIT_RISCV_GCC}")
    set(_gcc "${IRONSIDE_INTEGRATION_KIT_RISCV_GCC}")
    if(NOT EXISTS "${_gcc}")
      message(FATAL_ERROR "IRONSIDE_INTEGRATION_KIT_RISCV_GCC=${_gcc} does not exist")
    endif()
  else()
    if(IRONSIDE_INTEGRATION_KIT_RISCV_GCC)
      set(_names "${IRONSIDE_INTEGRATION_KIT_RISCV_GCC}")
    else()
      set(_names ${INTEGRATION_KIT_RISCV_GCC_NAMES})
    endif()
    find_program(IRONSIDE_INTEGRATION_KIT_RISCV_GCC_EXE NAMES ${_names})
    set(_gcc "${IRONSIDE_INTEGRATION_KIT_RISCV_GCC_EXE}")
  endif()

  if(NOT _gcc)
    string(REPLACE ";" ", " _tried "${_names}")
    message(FATAL_ERROR
      "sample ${SAMPLE} runs code on the VPR and needs a RISC-V GCC, which was not found.\n"
      "Tried: ${_tried}.\n"
      "Install one, put it on PATH or set IRONSIDE_INTEGRATION_KIT_RISCV_GCC to its full path "
      "- see samples/PREREQUISITES.md. This toolchain is optional: every sample that does not run "
      "VPR code builds without it.")
  endif()

  # Sibling binutils, derived the way arm-none-eabi.cmake derives its own.
  get_filename_component(_dir "${_gcc}" DIRECTORY)
  get_filename_component(_name "${_gcc}" NAME)
  string(REPLACE "gcc" "objcopy" _objcopy_name "${_name}")
  if(_dir)
    set(_objcopy "${_dir}/${_objcopy_name}")
  else()
    set(_objcopy "${_objcopy_name}")
  endif()
  if(_dir AND NOT EXISTS "${_objcopy}")
    message(FATAL_ERROR "found ${_gcc} but no objcopy beside it at ${_objcopy}")
  endif()

  set(${out_gcc} "${_gcc}" PARENT_SCOPE)
  set(${out_objcopy} "${_objcopy}" PARENT_SCOPE)
endfunction()

# Pick the first -march the compiler accepts. The CSR instructions this code
# needs are spelled as a separate extension (rv32emc_zicsr) from GCC 12 on and
# are part of the base ISA before that, so trying both spellings covers either
# vintage - and the instructions emitted are the same, so which one wins does
# not change the blob.
function(_integration_kit_vpr_pick_march out_march gcc mabi)
  set(_probe "${CMAKE_BINARY_DIR}/vpr/march_probe.c")
  file(WRITE "${_probe}" "int probe(void) { return 0; }\n")

  foreach(_candidate IN LISTS ARGN)
    execute_process(
      COMMAND "${gcc}" -march=${_candidate} -mabi=${mabi} -nostdlib
              -c "${_probe}" -o "${_probe}.o"
      RESULT_VARIABLE _rc
      OUTPUT_QUIET
      ERROR_QUIET
    )
    if(_rc EQUAL 0)
      set(${out_march} "${_candidate}" PARENT_SCOPE)
      return()
    endif()
  endforeach()

  string(REPLACE ";" ", " _tried "${ARGN}")
  message(FATAL_ERROR
    "${gcc} accepted none of the RISC-V ISA strings this stub needs (tried: ${_tried}, "
    "with -mabi=${mabi}). It is probably not a bare-metal RV32E toolchain - see "
    "samples/PREREQUISITES.md.")
endfunction()

# integration_kit_vpr_add_stub(<target>
#   SOURCE    <file.S>     assembler source for the VPR core
#   HEADER    <name.inc>   generated header; the App does #include "<name.inc>"
#   ARRAY     <symbol>     name of the array it defines
#   [ALIGN    <bytes>]     array alignment (default 128 - VPR00.INITPC's)
#   [MARCH    <isa>...]    candidates, first accepted wins (default
#                          rv32emc_zicsr then rv32emc)
#   [MABI     <abi>]       default ilp32e
#   [DEFINES  <-Dx>...]    preprocessor defines for SOURCE
#   [INCLUDES <dir>...]    include directories for SOURCE
# )
#
# Assembles SOURCE, takes its .text as a flat binary and emits HEADER next to
# the target's other build output, on the target's include path. The blob is
# position-independent machine code the App copies nowhere: it executes in
# place, out of the App's own image.
function(integration_kit_vpr_add_stub target)
  cmake_parse_arguments(ARG "" "SOURCE;HEADER;ARRAY;ALIGN;MABI" "DEFINES;INCLUDES;MARCH" ${ARGN})
  foreach(_required SOURCE HEADER ARRAY)
    if(NOT ARG_${_required})
      message(FATAL_ERROR "integration_kit_vpr_add_stub: ${_required} is required")
    endif()
  endforeach()
  if(NOT ARG_ALIGN)
    set(ARG_ALIGN 128)
  endif()
  if(NOT ARG_MARCH)
    set(ARG_MARCH rv32emc_zicsr rv32emc)
  endif()
  if(NOT ARG_MABI)
    set(ARG_MABI ilp32e)
  endif()

  _integration_kit_vpr_toolchain(_gcc _objcopy)
  _integration_kit_vpr_pick_march(_march "${_gcc}" "${ARG_MABI}" ${ARG_MARCH})
  message(STATUS "IronSide Integration Kit samples: VPR stub via ${_gcc} (-march=${_march})")

  set(_dir "${CMAKE_BINARY_DIR}/vpr")
  set(_obj "${_dir}/${ARG_ARRAY}.o")
  set(_bin "${_dir}/${ARG_ARRAY}.bin")
  set(_header "${_dir}/${ARG_HEADER}")

  set(_includes "")
  foreach(_include IN LISTS ARG_INCLUDES)
    list(APPEND _includes -I "${_include}")
  endforeach()

  get_filename_component(_source_name "${ARG_SOURCE}" NAME)
  add_custom_command(
    OUTPUT "${_header}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${_dir}"
    # -nostdlib: no startup, no libc - this is a bare loop for a core with
    # neither. Only .text is kept, so the blob is exactly the instructions.
    # -MMD/-MF: SOURCE goes through the C preprocessor, so it can #include the
    # same headers the App does; the depfile makes a change to one of those
    # rebuild the blob.
    COMMAND "${_gcc}" -march=${_march} -mabi=${ARG_MABI} -nostdlib -c
            -MMD -MF "${_obj}.d"
            ${ARG_DEFINES} ${_includes} "${ARG_SOURCE}" -o "${_obj}"
    COMMAND "${_objcopy}" -O binary --only-section=.text "${_obj}" "${_bin}"
    COMMAND "${CMAKE_COMMAND}" -DBIN=${_bin} -DHEADER=${_header} -DARRAY=${ARG_ARRAY}
            -DALIGN=${ARG_ALIGN} -DFROM=${_source_name}
            -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/bin2c.cmake"
    DEPENDS "${ARG_SOURCE}"
    DEPFILE "${_obj}.d"
    COMMENT "Assembling ${_source_name} for the VPR"
    VERBATIM
  )
  add_custom_target(${target}_vpr_stub DEPENDS "${_header}")
  add_dependencies(${target} ${target}_vpr_stub)
  target_include_directories(${target} PRIVATE "${_dir}")
endfunction()
