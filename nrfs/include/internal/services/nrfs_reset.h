/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_INTERNAL_RESET_H
#define NRFS_INTERNAL_RESET_H

#include <internal/services/nrfs_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Reset request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_reset_req_t;

/** @brief Reset service notification structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. This is left for compatibility purposes. Should always be zero. */
} nrfs_reset_rsp_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_INTERNAL_RESET_H */
