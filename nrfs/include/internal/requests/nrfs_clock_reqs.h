/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_CLOCK_REQS_H
#define NRFS_CLOCK_REQS_H

#include "nrfs_reqs_common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	NRFS_CLOCK_REQ_SUBSCRIBE   = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_CLOCK, 0x01),
	NRFS_CLOCK_REQ_UNSUBSCRIBE = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_CLOCK, 0x02),
	NRFS_CLOCK_REQ_LFCLK_SRC   = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_CLOCK, 0x03),
	NRFS_CLOCK_REQ_HSFLL_MODE  = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_CLOCK, 0x04),
};

#ifdef __cplusplus
}
#endif

#endif /* NRFS_CLOCK_REQS_H */
