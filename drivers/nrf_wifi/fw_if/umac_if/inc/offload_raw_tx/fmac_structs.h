/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 *
 * @addtogroup nrf_wifi_api_offloaded_raw_tx FMAC offloaded raw tx API
 * @{
 *
 * TODO: This file is not added doxygen to avoid duplicate warnings.
 *
 * @brief Header containing declarations for utility functions for
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_STRUCTS_H__
#define __FMAC_STRUCTS_H__

#include "osal_api.h"
#include "host_rpu_sys_if.h"
#if !defined(__DOXYGEN__)
#include "fmac_structs_common.h"
#endif

/**
 * @brief  Structure to hold per device context information for the UMAC IF layer.
 *
 * This structure maintains the context information necessary for
 * a single instance of a FullMAC-based RPU.
 */
struct nrf_wifi_fmac_dev_ctx_offloaded_raw_tx {
	/** Handle to the FMAC IF abstraction layer. */
	struct nrf_wifi_fmac_priv *fpriv;
};

/**
 * @}
 */
#endif /* __FMAC_STRUCTS_H__ */