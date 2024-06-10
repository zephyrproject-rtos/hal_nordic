/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EPERIPHS_COMMON_H
#define EPERIPHS_COMMON_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/ipc/ipc_service.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief FLPR ePeripheral configuration type. */
typedef enum
{
    NRF_ICMG_CONFIG_EGPIO  = 0, ///< Configure FLPR to work as Emulated GPIO.
} nrf_icmg_config_t;

#ifdef __cplusplus
}
#endif

#endif /* EPERIPHS_COMMON_H */
