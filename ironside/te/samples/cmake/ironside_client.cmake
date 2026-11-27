# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# IronSide TE Client - how the sample build consumes it.
#
# The client (../../client) ships no CMake package of its own: consuming it is
# an include path plus the required client translation units. This glue
# therefore lives on the consumer side, so nothing under client/ has to know
# about this build's ${SAMPLE} convention - a Zephyr or third-party App wires
# the same paths up its own way.
#
# ../CMakeLists.txt includes this file before a sample's sample.cmake runs, so a
# sample opts in with one call:
#
#   ironside_client_add(${SAMPLE})          # any secure call, PSA included
#   ironside_client_add_headers(${SAMPLE})  # headers only, no secure call
#
# Strictly Zephyr-free: the sources added here depend only on a bare-metal C
# toolchain.

# Sibling of samples/ both in the source repo (under integration_kit/) and in the
# release bundle (both at the bundle root).
get_filename_component(IRONSIDE_CLIENT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../client" ABSOLUTE)

# Headers only - for a sample that reads the client's view of the system
# (layout.h, image.h) or hand-builds a call from internal/client.h, but makes no secure call
# through the client core.
function(ironside_client_add_headers target)
  target_include_directories(${target} PRIVATE "${IRONSIDE_CLIENT_ROOT}/include")
endfunction()

# Headers + every compiled unit of the client.
function(ironside_client_add target)
  ironside_client_add_headers(${target})
  target_sources(${target} PRIVATE
    "${IRONSIDE_CLIENT_ROOT}/src/client.c"
    "${IRONSIDE_CLIENT_ROOT}/src/psa_crypto.c"
    "${IRONSIDE_CLIENT_ROOT}/src/services.c"
  )
  # One section per function, for --gc-sections to drop those never called.
  set_source_files_properties("${IRONSIDE_CLIENT_ROOT}/src/psa_crypto.c"
    "${IRONSIDE_CLIENT_ROOT}/src/services.c"
    PROPERTIES COMPILE_OPTIONS -ffunction-sections
  )
endfunction()
