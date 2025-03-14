/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_HDR_H
#define NRFS_HDR_H

#include <internal/nrfs_phy.h>

#include <internal/requests/nrfs_clock_reqs.h>
#include <internal/requests/nrfs_diag_reqs.h>
#include <internal/requests/nrfs_dvfs_reqs.h>
#include <internal/requests/nrfs_gdfs_reqs.h>
#include <internal/requests/nrfs_gdpwr_reqs.h>
#include <internal/requests/nrfs_mram_reqs.h>
#include <internal/requests/nrfs_pmic_reqs.h>
#include <internal/requests/nrfs_swext_reqs.h>
#include <internal/requests/nrfs_reset_reqs.h>
#include <internal/requests/nrfs_temp_reqs.h>
#include <internal/requests/nrfs_usb_reqs.h>
#include <internal/requests/nrfs_audiopll_reqs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __NRFS_PACKED {
	uint16_t req;
} nrfs_hdr_t;

#define NRFS_HDR_GET(_p_phy) ((nrfs_hdr_t *)((_p_phy)->p_buffer))

#define NRFS_HDR_FILL(p_hdr, _req) ((p_hdr)->req = (uint16_t)(_req))

#define NRFS_SERVICE_HDR_FILL(p_srv_req, _req) NRFS_HDR_FILL(&(p_srv_req)->hdr, _req)

#define NRFS_HDR_NO_RSP_GET(_p_hdr) NRFS_NO_RSP_GET((_p_hdr)->req)

#define NRFS_HDR_NO_RSP_SET(_p_hdr) NRFS_NO_RSP_SET((_p_hdr)->req)

#define NRFS_HDR_FILTER_ERR_GET(_p_hdr) NRFS_FILTER_ERR_GET((_p_hdr)->req)

#define NRFS_HDR_FILTER_ERR_SET(_p_hdr) NRFS_FILTER_ERR_SET((_p_hdr)->req)

/* Warning! All "UNSOLICITED" features are not supported. This is intended for possible future use. */
#define NRFS_HDR_UNSOLICITED_GET(_p_hdr) NRFS_UNSOLICITED_GET((_p_hdr)->req)

/* Warning! All "UNSOLICITED" features are not supported. This is intended for possible future use. */
#define NRFS_HDR_UNSOLICITED_SET(_p_hdr) NRFS_UNSOLICITED_SET((_p_hdr)->req)

#ifdef __cplusplus
}
#endif

#endif /* NRFS_HDR_H */
