/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_INTERNAL_AUDIOPLL_H
#define NRFS_INTERNAL_AUDIOPLL_H

#include <internal/services/nrfs_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AUDIO PLL possible prescaler setting options.
 *
 */
enum __NRFS_PACKED audiopll_prescaler_div {

	AUDIOPLL_DIV_DISABLED	= 0,
	AUDIOPLL_DIV_1		= 1,
	AUDIOPLL_DIV_2		= 2,
	AUDIOPLL_DIV_3		= 3,
	AUDIOPLL_DIV_4		= 4,
	AUDIOPLL_DIV_6		= 5,
	AUDIOPLL_DIV_8		= 6,
	AUDIOPLL_DIV_12		= 7,
	AUDIOPLL_DIV_16		= 8
};

/** @brief AUDIO PLL structure describing pll frequency settings request. */
typedef struct __NRFS_PACKED {
	uint16_t pll_freq_fraction;
} audiopll_freq_req_data_t;

/** @brief AUDIO PLL structure describing pll prescaler settings request. */
typedef struct __NRFS_PACKED {
	enum audiopll_prescaler_div pll_div;
} audiopll_prescaler_req_data_t;

/** @brief AUDIO PLL structure describing frequency increment feature request. */
typedef struct __NRFS_PACKED {
	uint16_t freq_inc_period;
	int8_t freq_inc_val;
} audiopll_freq_inc_req_data_t;

/** @brief Audio PLL service enable request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_audiopll_enable_req_t;

/** @brief Audio PLL service disable request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_audiopll_disable_req_t;

/** @brief Audio PLL service frequency change request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;		          /**< Header of the message. */
	nrfs_ctx_t ctx;		          /**< Context of the message. */
	audiopll_freq_req_data_t data; /**< Data of the request. */
} nrfs_audiopll_freq_req_t;

/** @brief Audio PLL service prescaler divider change request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;		            /**< Header of the message. */
	nrfs_ctx_t ctx;		            /**< Context of the message. */
	audiopll_prescaler_req_data_t data; /**< Data of the request. */
} nrfs_audiopll_prescaler_req_t;

/** @brief Audio PLL service incremental frequency change request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;		           /**< Header of the message. */
	nrfs_ctx_t ctx;		           /**< Context of the message. */
	audiopll_freq_inc_req_data_t data; /**< Data of the request. */
} nrfs_audiopll_freq_inc_req_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_INTERNAL_AUDIOPLL_H */
