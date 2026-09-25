/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_CTX_H
#define NRFS_CTX_H

#include <nrfs_common.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __NRFS_PACKED {
	uint32_t ctx;
} nrfs_ctx_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_CTX_H */
