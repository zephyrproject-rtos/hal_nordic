/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_INTERNAL_GDFS_H
#define NRFS_INTERNAL_GDFS_H

#include <internal/services/nrfs_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

enum __NRFS_PACKED gdfs_frequency_setting {
	GDFS_FREQ_HIGH		= 0,
	GDFS_FREQ_MEDHIGH	= 1,
	GDFS_FREQ_MEDLOW	= 2,
	GDFS_FREQ_LOW		= 3,
	GDFS_FREQ_COUNT
};

/** @brief Global Domain Frequency Scaling service request data structure. */
typedef struct __NRFS_PACKED {
	enum gdfs_frequency_setting target_freq; /** Requested frequency oppoint. */
} nrfs_gdfs_req_data_t;

/** @brief Global Domain Frequency Scaling frequency change request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;		       /**< Header of the message. */
	nrfs_ctx_t ctx;		       /**< Context of the message. */
	nrfs_gdfs_req_data_t data; /**< Data of the request. */
} nrfs_gdfs_req_t;

/** @brief Global Domain Frequency Scaling service notification structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;		   /**< Header of the message. */
	nrfs_ctx_t ctx;		   /**< Context of the message. */
} nrfs_gdfs_rsp_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_INTERNAL_GDFS_H */
