/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_DVFS_REQS_H
#define NRFS_DVFS_REQS_H

#include "nrfs_reqs_common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	NRFS_DVFS_REQ_INIT_PREPARE   = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_DVFS, 0x01),
	NRFS_DVFS_REQ_INIT_COMPLETE  = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_DVFS, 0x02),
	NRFS_DVFS_REQ_OPPOINT	     = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_DVFS, 0x03),
	NRFS_DVFS_REQ_READY_TO_SCALE = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_DVFS, 0x04),
};

#ifdef __cplusplus
}
#endif

#endif /* NRFS_DVFS_REQS_H */
