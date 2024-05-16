/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* clang-format off */

#ifndef NRFS_REQS_COMMON_H
#define NRFS_REQS_COMMON_H

#define NRFS_REQUEST_ID_BITS  7uL
#define NRFS_NO_RSP_BITS      1uL
#define NRFS_SERVICE_ID_BITS  6uL
#define NRFS_FILTER_ERR_BITS  1uL
#define NRFS_UNSOLICITED_BITS 1uL

#define NRFS_REQUEST_ID_BITPOS  0uL
#define NRFS_NO_RSP_BITPOS      (NRFS_REQUEST_ID_BITS)
#define NRFS_SERVICE_ID_BITPOS  (NRFS_REQUEST_ID_BITS + NRFS_NO_RSP_BITS)
#define NRFS_FILTER_ERR_BITPOS  (NRFS_REQUEST_ID_BITS + NRFS_NO_RSP_BITS + NRFS_SERVICE_ID_BITS)
#define NRFS_UNSOLICITED_BITPOS (NRFS_REQUEST_ID_BITS + NRFS_NO_RSP_BITS + NRFS_SERVICE_ID_BITS + NRFS_FILTER_ERR_BITS)

#define NRFS_REQUEST_ID_MASK  (((1 << NRFS_REQUEST_ID_BITS)  - 1) << NRFS_REQUEST_ID_BITPOS)
#define NRFS_NO_RSP_MASK      (((1 << NRFS_NO_RSP_BITS)      - 1) << NRFS_NO_RSP_BITPOS)
#define NRFS_SERVICE_ID_MASK  (((1 << NRFS_SERVICE_ID_BITS)  - 1) << NRFS_SERVICE_ID_BITPOS)
#define NRFS_FILTER_ERR_MASK  (((1 << NRFS_FILTER_ERR_BITS)  - 1) << NRFS_FILTER_ERR_BITPOS)
#define NRFS_UNSOLICITED_MASK (((1 << NRFS_UNSOLICITED_BITS) - 1) << NRFS_UNSOLICITED_BITPOS)

#define NRFS_REQUEST_ID_DEF(_srv_id, _req_id) (((uint16_t)(_srv_id) << NRFS_SERVICE_ID_BITPOS) | \
					       ((uint16_t)(_req_id) << NRFS_REQUEST_ID_BITPOS))

#define NRFS_REQUEST_ID_GET(_req)  (((_req) & (NRFS_REQUEST_ID_MASK | NRFS_SERVICE_ID_MASK)))

#define NRFS_SERVICE_ID_GET(_req)  (((_req) & NRFS_SERVICE_ID_MASK) >> NRFS_SERVICE_ID_BITPOS)

#define NRFS_NO_RSP_GET(_req)  (((_req) & NRFS_NO_RSP_MASK) >> NRFS_NO_RSP_BITPOS)

#define NRFS_NO_RSP_SET(_req)  ((_req) |= NRFS_NO_RSP_MASK)

#define NRFS_FILTER_ERR_GET(_req)  (((_req) & NRFS_FILTER_ERR_MASK) >> NRFS_FILTER_ERR_BITPOS)

#define NRFS_FILTER_ERR_SET(_req)  ((_req) |= NRFS_FILTER_ERR_MASK)

/* Warning! All "UNSOLICITED" features are not supported. This is intended for possible future use. */
#define NRFS_UNSOLICITED_GET(_req) (((_req) & NRFS_UNSOLICITED_MASK) >> NRFS_UNSOLICITED_BITPOS)

/* Warning! All "UNSOLICITED" features are not supported. This is intended for possible future use. */
#define NRFS_UNSOLICITED_SET(_req) ((_req) |= NRFS_UNSOLICITED_MASK)

#ifdef __cplusplus
extern "C" {
#endif

enum {
	NRFS_SERVICE_ID_TEMP,
	NRFS_SERVICE_ID_MRAM,
	NRFS_SERVICE_ID_RESET,
	NRFS_SERVICE_ID_USB,
	NRFS_SERVICE_ID_PMIC,
	NRFS_SERVICE_ID_DVFS,
	NRFS_SERVICE_ID_DIAG,
	NRFS_SERVICE_ID_CLOCK,
};

#ifdef __cplusplus
}
#endif

/* clang-format on */

#endif /* NRFS_REQS_COMMON_H */
