/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_DIAG_REQS_H
#define NRFS_DIAG_REQS_H

#include "nrfs_reqs_common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	NRFS_DIAG_REG = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_DIAG, 0x01),
};

#ifdef __cplusplus
}
#endif

#endif /* NRFS_DIAG_REQS_H */
