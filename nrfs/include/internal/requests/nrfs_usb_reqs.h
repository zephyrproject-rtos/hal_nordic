/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_USB_REQS_H
#define NRFS_USB_REQS_H

#include "nrfs_reqs_common.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	NRFS_USB_REQ_ENABLE  = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_USB, 0x01),
	NRFS_USB_REQ_DISABLE = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_USB, 0x02),
	NRFS_USB_REQ_DPLUS_PULLUP_ENABLE  = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_USB, 0x03),
	NRFS_USB_REQ_DPLUS_PULLUP_DISABLE = NRFS_REQUEST_ID_DEF(NRFS_SERVICE_ID_USB, 0x04),
};

#ifdef __cplusplus
}
#endif

#endif /* NRFS_USB_REQS_H */
