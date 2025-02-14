/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_INTERNAL_MRAM_H
#define NRFS_INTERNAL_MRAM_H

#include <internal/services/nrfs_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief MRAM set latency requests. */
typedef enum __NRFS_PACKED {
	MRAM_LATENCY_ALLOWED	  = 0,
	MRAM_LATENCY_NOT_ALLOWED  = 1,
	MRAM_LATENCY_INTERNAL_REQ = 2,
} mram_latency_request_t;

/** @brief MRAM set latency data structure. */
typedef struct __NRFS_PACKED {
	mram_latency_request_t mram_latency_request;
} nrfs_mram_set_latency_data_t;

/** @brief MRAM set latency request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;			   /**< Header of the message. */
	nrfs_ctx_t ctx;			   /**< Context of the message. */
	nrfs_mram_set_latency_data_t data; /**< Data of the request. */
} nrfs_mram_set_latency_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_INTERNAL_MRAM_H */
