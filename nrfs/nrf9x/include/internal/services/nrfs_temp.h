/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_INTERNAL_TEMP_H
#define NRFS_INTERNAL_TEMP_H

#include <internal/services/nrfs_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Subscribe request data structure. */
typedef struct __NRFS_PACKED {
	uint16_t measure_rate_ms; /** Maximal acceptable time between subsequent temperature checks. */
	int32_t lower_threshold; /** Temperature lower threshold in a raw 2's complement signed format,
				   *  causing notification to be sent when crossed.
				   */
	int32_t upper_threshold; /** Temperature upper threshold in a raw 2's complement signed format,
				   *  causing notification to be sent when crossed.
				   */
} nrfs_temp_subscribe_data_t;

/** @brief Temperature service notification data structure. */
typedef struct __NRFS_PACKED {
	int32_t raw_temp; /** Raw temperature in a 2's complement signed value representation. */
} nrfs_temp_rsp_data_t;

/** @brief Temperature measure request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_temp_measure_t;

/** @brief Subscribe request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;			 /**< Header of the message. */
	nrfs_ctx_t ctx;			 /**< Context of the message. */
	nrfs_temp_subscribe_data_t data; /**< Data of the request. */
} nrfs_temp_subscribe_t;

/** @brief Unsubscribe request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_temp_unsubscribe_t;

/** @brief Temperature service notification structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;		   /**< Header of the message. */
	nrfs_ctx_t ctx;		   /**< Context of the message. */
	nrfs_temp_rsp_data_t data; /**< Data of the notification. */
} nrfs_temp_rsp_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_INTERNAL_TEMP_H */
