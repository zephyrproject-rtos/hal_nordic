# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_NRFS)
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/services)
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/helpers)
    add_subdirectory_ifdef(CONFIG_NRFS_LOCAL_DOMAIN src)
    add_subdirectory(helpers)
endif()
