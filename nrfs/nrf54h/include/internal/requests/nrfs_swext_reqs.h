/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_SWEXT_REQS_H
#define NRFS_SWEXT_REQS_H

#include "nrfs_reqs_common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	NRFS_SWEXT_REQ_POWER_UP	  = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_SWEXT, 0x01),
	NRFS_SWEXT_REQ_POWER_DOWN = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_SWEXT, 0x02),
};

#ifdef __cplusplus
}
#endif

#endif /* NRFS_SWEXT_REQS_H */
