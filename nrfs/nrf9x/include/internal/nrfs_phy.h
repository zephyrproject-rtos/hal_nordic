/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_PHY_H
#define NRFS_PHY_H

#include <nrfs_common.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	void *p_buffer;
	uint32_t size;
	uint8_t domain_id;
} nrfs_phy_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_PHY_H */
