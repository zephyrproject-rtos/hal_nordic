/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_INTERNAL_GENERIC_H
#define NRFS_INTERNAL_GENERIC_H

#include <internal/nrfs_hdr.h>
#include <internal/nrfs_ctx.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Structure describing generic request or notification. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;	   /**< Header of the message. */
	nrfs_ctx_t ctx;	   /**< Context of the message. */
	uint8_t payload[]; /**< Message payload. */
} nrfs_generic_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_INTERNAL_GENERIC_H */
