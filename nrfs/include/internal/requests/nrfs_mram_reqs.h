/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_MRAM_REQS_H
#define NRFS_MRAM_REQS_H

#include "nrfs_reqs_common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	NRFS_MRAM_REQ_SET_LATENCY = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_MRAM, 0x01),
};

#ifdef __cplusplus
}
#endif

#endif /* NRFS_MRAM_REQS_H */
