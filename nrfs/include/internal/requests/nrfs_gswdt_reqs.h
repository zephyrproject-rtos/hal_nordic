/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_GSWDT_REQS_H
#define NRFS_GSWDT_REQS_H

#include "nrfs_reqs_common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	NRFS_GSWDT_TIMEOUT_SET_REQ = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_GSWDT, 0x01),
	NRFS_GSWDT_STOP_REQ	   = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_GSWDT, 0x02),
};

#ifdef __cplusplus
}
#endif

#endif /* NRFS_GSWDT_REQS_H */
