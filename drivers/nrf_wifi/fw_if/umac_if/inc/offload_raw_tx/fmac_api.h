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
 * @brief Header containing API declarations for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_API_H__
#define __FMAC_API_H__

#include "osal_api.h"
#include "host_rpu_umac_if.h"
#include "host_rpu_data_if.h"
#include "host_rpu_sys_if.h"

#include "fmac_structs.h"
#include "fmac_cmd.h"
#include "fmac_event.h"
#include "fmac_vif.h"
#include "fmac_bb.h"
#include "fmac_api_common.h"


/**
 * @brief Initialize the UMAC IF layer.
 *
 * This function initializes the UMAC IF layer of the RPU WLAN FullMAC driver.
 *     It does the following:
 *
 *		- Creates and initializes the context for the UMAC IF layer.
 *		- Initialize the OS abstraction Layer
 *		- Initialize the HAL layer.
 *		- Registers the driver to the underlying Operating System.
 *
 * @return	Pointer to the context of the UMAC IF layer.
 */
struct nrf_wifi_fmac_priv *nrf_wifi_fmac_init_offloaded_raw_tx(void);

/**
 * @brief De-initialize the UMAC IF layer.
 * @param fpriv Pointer to the context of the UMAC IF layer.
 *
 * This function de-initializes the UMAC IF layer of the RPU WLAN FullMAC
 *	    driver. It does the following:
 *
 *	- De-initializes the HAL layer.
 *	- Frees the context for the UMAC IF layer.
 *
 * @return None
 */
void nrf_wifi_fmac_deinit_offloaded_raw_tx(struct nrf_wifi_fmac_priv *fpriv);

/**
 * @brief Removes a RPU instance.
 * @param fmac_dev_ctx Pointer to the context of the RPU instance to be removed.
 *
 * This function handles the removal of an RPU instance at the UMAC IF layer.
 *
 * @return None.
 */
void nrf_wifi_fmac_dev_rem_offloaded_raw_tx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);


/**
 * @brief Initialize a RPU instance.
 * @param fmac_dev_ctx Pointer to the context of the RPU instance to be removed.
 * @param sleep_type Type of RPU sleep.
 * @param phy_calib PHY calibration flags to be passed to the RPU.
 * @param op_band Operating band of the RPU.
 * @param beamforming Enable/disable Wi-Fi beamforming.
 * @param tx_pwr_ctrl TX power control parameters to be passed to the RPU.
 * @param tx_pwr_ceil_params TX power ceiling parameters to be passed to the RPU.
 * @param board_params Board parameters to be passed to the RPU.
 *
 * This function initializes the firmware of an RPU instance.
 *
 * @retval NRF_WIFI_STATUS_SUCCESS On Success
 * @retval NRF_WIFI_STATUS_FAIL On failure to execute command
 */
enum nrf_wifi_status nrf_wifi_fmac_dev_init_offloaded_raw_tx(
		struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
#if defined(NRF_WIFI_LOW_POWER) || defined(__DOXYGEN__)
		int sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
		unsigned int phy_calib,
		enum op_band op_band,
		bool beamforming,
		struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl,
		struct nrf_wifi_tx_pwr_ceil_params *tx_pwr_ceil_params,
		struct nrf_wifi_board_params *board_params);

/**
 * @brief De-initialize a RPU instance.
 * @param fmac_dev_ctx Pointer to the context of the RPU instance to be removed.
 *
 * This function de-initializes the firmware of an RPU instance.
 *
 * @return None.
 */
void nrf_wifi_fmac_dev_deinit_offloaded_raw_tx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

enum nrf_wifi_status nrf_wifi_offloaded_raw_tx_conf(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						    struct beacon_offloaded_raw_tx *params);

enum nrf_wifi_status nrf_wifi_offloaded_raw_tx_start(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

enum nrf_wifi_status nrf_wifi_offloaded_raw_tx_stop(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);
/**
 * @}
 */
#endif /* __FMAC_API_H__ */
