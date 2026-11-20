/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_GDPWR_REQS_H
#define NRFS_GDPWR_REQS_H

#include "nrfs_reqs_common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	NRFS_GDPWR_REQ_SET_POWER_REQUEST= NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_GDPWR, 0x01),
};

#ifdef __cplusplus
}
#endif

#endif /* NRFS_GDPWR_REQS_H */
