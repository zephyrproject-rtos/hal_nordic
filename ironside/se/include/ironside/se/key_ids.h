/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IRONSIDE_SE_KEY_IDS_H_
#define IRONSIDE_SE_KEY_IDS_H_

/* IronSide key IDs with implementation-defined properties */
#define IRONSIDE_KEY_ID_REVOCABLE_MIN (0x40002000UL)
#define IRONSIDE_KEY_ID_REVOCABLE_MAX (0x4FFFFFFFUL)

/* CRACEN built-in key IDs */
#define CRACEN_KEY_ID_IAK  (0x7FFFC001UL)
#define CRACEN_KEY_ID_MKEK (0x7FFFC002UL)
#define CRACEN_KEY_ID_MEXT (0x7FFFC003UL)

/* Location value for CRACEN built-in keys */
#define CRACEN_KEY_LOCATION (0x804E00UL)

#endif /* IRONSIDE_SE_KEY_IDS_H_ */
