/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_INTERNAL_USB_H
#define NRFS_INTERNAL_USB_H

#include <internal/services/nrfs_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief USB VBUS detector service notification data structure for enable request. */
typedef struct __NRFS_PACKED {
	bool pll_ok;	    /** True, when USBHSPLL PLL is locked. */
	bool vreg_ok;	    /** True, when VREGUSB 0V8 and 3V3 voltages are settled. */
	bool vbus_detected; /** True, when VBUS is detected. */
} nrfs_usb_rsp_data_t;

/** @brief USB VBUS detector service enable request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_usb_enable_req_t;

/** @brief USB VBUS detector service disable request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_usb_disable_req_t;

/** @brief USB D+ pull-up enable request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_usb_ld_dplus_pullup_enable_req_t;

/** @brief USB D+ pull-up disable request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_usb_ld_dplus_pullup_disable_req_t;


/** @brief USB VBUS detector service notification structure for enable request. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;		  /**< Header of the message. */
	nrfs_ctx_t ctx;		  /**< Context of the message. */
	nrfs_usb_rsp_data_t data; /**< Data of the notification. */
} nrfs_usb_rsp_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_INTERNAL_USB_H */
