/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_TEMP_REQS_H
#define NRFS_TEMP_REQS_H

#include "nrfs_reqs_common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	NRFS_TEMP_REQ_MEASURE	  = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_TEMP, 0x01),
	NRFS_TEMP_REQ_SUBSCRIBE	  = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_TEMP, 0x02),
	NRFS_TEMP_REQ_UNSUBSCRIBE = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_TEMP, 0x03),
};

#ifdef __cplusplus
}
#endif

#endif /* NRFS_TEMP_REQS_H */
