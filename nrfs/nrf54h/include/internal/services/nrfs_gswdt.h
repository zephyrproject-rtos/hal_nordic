/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_INTERNAL_GSWDT_H
#define NRFS_INTERNAL_GSWDT_H

#include <internal/services/nrfs_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Global Software Watchdog timeout set request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
	uint32_t timeout_ms; /**< Watchdog timeout in milliseconds. */
} nrfs_gswdt_timeout_set_req_t;

/** @brief Global Software Watchdog stop request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_gswdt_stop_req_t;

/** @brief Global Software Watchdog service notification structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_gswdt_rsp_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_INTERNAL_GSWDT_H */
