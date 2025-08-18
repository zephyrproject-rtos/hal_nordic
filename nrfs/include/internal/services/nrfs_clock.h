/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_INTERNAL_CLOCK_H
#define NRFS_INTERNAL_CLOCK_H

#include <zephyr/sys/util_macro.h>
#include <internal/services/nrfs_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Clock service event reason.
 *
 * @note The values are in bit position intervals so that they can be used as masked values.
 */
typedef enum __NRFS_PACKED {
	NRFS_CLOCK_EVENT_REASON_LFCLK_ACCURACY_CHANGED = BIT(0),
	NRFS_CLOCK_EVENT_REASON_LFCLK_PRECISION_CHANGED = BIT(1),
	NRFS_CLOCK_EVENT_REASON_LFCLK_NO_CHANGE = BIT(2),
	NRFS_CLOCK_EVENT_REASON_ALL = 0xFF
} nrfs_clock_evt_reason_t;

/** @brief Clock source option. */
typedef enum __NRFS_PACKED {
	NRFS_CLOCK_SRC_LFCLK_DEFAULT		= 0,
	NRFS_CLOCK_SRC_LFCLK_XO_DEFAULT		= 1,
	NRFS_CLOCK_SRC_LFCLK_XO_DEFAULT_HP	= 2,

	NRFS_CLOCK_SRC_LFCLK_LFLPRC		= 3,
	NRFS_CLOCK_SRC_LFCLK_LFRC		= 4,

	NRFS_CLOCK_SRC_LFCLK_XO_PIXO		= 5,
	NRFS_CLOCK_SRC_LFCLK_XO_PIERCE		= 6,
	NRFS_CLOCK_SRC_LFCLK_XO_EXT_SINE	= 7,
	NRFS_CLOCK_SRC_LFCLK_XO_EXT_SQUARE	= 8,

	NRFS_CLOCK_SRC_LFCLK_SYNTH		= 9,

	NRFS_CLOCK_SRC_LFCLK_XO_PIERCE_HP	= 10,
	NRFS_CLOCK_SRC_LFCLK_XO_EXT_SINE_HP	= 11,
}nrfs_clock_src_t;

/** @brief HSFLL120 Mode options. */
typedef enum __NRFS_PACKED {
	NRFS_CLOCK_HSFLL_MODE_OPEN = 0,
	NRFS_CLOCK_HSFLL_MODE_CLOSED = 1,
}nrfs_clock_hsfll_mode_t;

/** @brief Clock service notification data structure. */
typedef struct __NRFS_PACKED {
	nrfs_clock_evt_reason_t reason; /**< Reason for the event. */
	nrfs_clock_src_t        src;    /**< New clock source. */
} nrfs_clock_rsp_data_t;

/** @brief Clock service notification structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t            hdr;  /**< Header of the message. */
	nrfs_ctx_t            ctx;  /**< Context of the message. */
	nrfs_clock_rsp_data_t data; /**< Data of the notification. */
} nrfs_clock_rsp_t;

/** @brief Subscribe request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;        /**< Header of the message. */
	nrfs_ctx_t ctx;        /**< Context of the message. */
	uint8_t    event_mask; /**< Events to be subscribed to. */
} nrfs_clock_subscribe_t;

/** @brief Unsubscribe request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_clock_unsubscribe_t;

/** @brief LFCLK source request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t       hdr; /**< Header of the message. */
	nrfs_ctx_t       ctx; /**< Context of the message. */
	nrfs_clock_src_t src; /**< LFCLK source. */
} nrfs_clock_lfclk_src_req_t;

/** @brief HSFLL mode request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
	nrfs_clock_hsfll_mode_t mode;
} nrfs_clock_hsfll_mode_req_t;

/** @brief HSFLL mode notification structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
	nrfs_clock_hsfll_mode_t data;
} nrfs_clock_hsfll_mode_rsp_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_INTERNAL_CLOCK_H */
