/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_INTERNAL_GDPWR_H
#define NRFS_INTERNAL_GDPWR_H

#include <internal/services/nrfs_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Power domain selection */
typedef enum __NRFS_PACKED {
	GDPWR_POWER_DOMAIN_ACTIVE_FAST,
	GDPWR_POWER_DOMAIN_ACTIVE_SLOW,
	GDPWR_POWER_DOMAIN_MAIN_SLOW
} gdpwr_power_domain_t;

typedef enum __NRFS_PACKED {
	GDPWR_POWER_REQUEST_CLEAR,
	GDPWR_POWER_REQUEST_SET
} gdpwr_request_type_t;

/** @brief Power domain data structure. */
typedef struct __NRFS_PACKED {
	gdpwr_power_domain_t power_domain;
	gdpwr_request_type_t request_type;
} nrfs_gdpwr_power_domain_data_t;

/** @brief global domain power request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;			   /**< Header of the message. */
	nrfs_ctx_t ctx;			   /**< Context of the message. */
	nrfs_gdpwr_power_domain_data_t data; /**< Data of the request. */
} nrfs_gdpwr_req_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_INTERNAL_GDPWR_H */
