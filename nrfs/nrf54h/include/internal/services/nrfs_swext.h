/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_INTERNAL_SWEXT_H
#define NRFS_INTERNAL_SWEXT_H

#include <internal/services/nrfs_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief SWEXT pdclamp options. */
typedef enum __NRFS_PACKED {
	SWEXT_PD_CLAMP_DISABLED = 0,
	SWEXT_PD_CLAMP_ENABLED  = 1,
} swext_pd_clamp_t;

/** @brief SWEXT data structure. */
typedef union __NRFS_PACKED {
	swext_pd_clamp_t pd_clamp_control;	/** < Powerdown ground clamp configuration. */
	uint8_t load_current;			/** < Load current in 500uA steps. */
} nrfs_swext_data_t;

/** @brief SWEXT request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;		/**< Header of the message. */
	nrfs_ctx_t ctx;		/**< Context of the message. */
	nrfs_swext_data_t data;	/**< Data of the request. */
} nrfs_swext_req_t;

/** @brief SWEXT response status. */
typedef enum __NRFS_PACKED {
	SWEXT_OUTPUT_ENABLED = 0,
	SWEXT_OVERCURRENT    = 1,
} nrfs_swext_status_t;

/** @brief SWEXT notification structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;			/**< Header of the message. */
	nrfs_ctx_t ctx;			/**< Context of the message. */
	nrfs_swext_status_t status;	/**< Data of the request. */
} nrfs_swext_rsp_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_INTERNAL_SWEXT_H */
