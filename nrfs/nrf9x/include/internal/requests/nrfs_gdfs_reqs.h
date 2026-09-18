/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_GDFS_REQS_H
#define NRFS_GDFS_REQS_H

#include "nrfs_reqs_common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	NRFS_GDFS_REQ_FREQ	     = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_GDFS, 0x01),
};

#ifdef __cplusplus
}
#endif

#endif /* NRFS_GDFS_REQS_H */
