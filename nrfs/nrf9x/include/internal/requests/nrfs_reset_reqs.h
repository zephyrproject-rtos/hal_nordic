/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_RESET_REQS_H
#define NRFS_RESET_REQS_H

#include "nrfs_reqs_common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	NRFS_RESET_REQ = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_RESET, 0x01),
};

#ifdef __cplusplus
}
#endif

#endif /* NRFS_RESET_REQS_H */
