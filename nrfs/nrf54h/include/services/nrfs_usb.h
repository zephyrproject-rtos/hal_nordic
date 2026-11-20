/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_USB_H
#define NRFS_USB_H

#include <internal/services/nrfs_usb.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief USB VBUS detector service event types. */
typedef enum __NRFS_PACKED {
	NRFS_USB_EVT_VBUS_STATUS_CHANGE = 0, /** VBUS voltage detection status has changed. */
	NRFS_USB_EVT_REJECT		= 1, /** Request rejected. */
} nrfs_usb_evt_type_t;

/** @brief USB VBUS detector service event. */
typedef struct {
	nrfs_usb_evt_type_t type; /** Event type. */
	bool vbus_detected;	  /** True if USB VBUS voltage is detected. */
	bool vregusb_ok;	  /** True if VREG is running OK. */
	bool usbhspll_ok;	  /** True if PLL is running OK. */
} nrfs_usb_evt_t;

/** @brief USB VBUS detector service event handler type. */
typedef void (*nrfs_usb_evt_handler_t)(nrfs_usb_evt_t const * p_evt, void * context);

/**
 * @brief Function for initializing the USB VBUS detector service.
 *
 * @param[in] handler Function called as a response to the request.
 *
 * @retval NRFS_SUCCESS           Service initialized successfully.
 * @retval NRFS_ERR_INVALID_STATE Service was already initialized.
 */
nrfs_err_t nrfs_usb_init(nrfs_usb_evt_handler_t handler);

/**
 * @brief Function for uninitializing the USB VBUS detector service.
 *
 * @warning Notifications from previous requests are dropped after service uninitialization.
 */
void nrfs_usb_uninit(void);

/**
 * @brief Function for requesting USBHSPLL and VREGUSB to be enabled and configured in USB VBUS detector service.
 *
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_usb_enable_request(void * p_context);

/**
 * @brief Function for requesting USBHSPLL and VREGUSB to be disabled in USB VBUS detector service.
 *
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_usb_disable_request(void * p_context);

/**
 * @brief Function for requesting to enable dplus pullup within the USB Service.
 *
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_usb_dplus_pullup_enable(void* p_context);

/**
 * @brief Function for requesting to disable dplus pullup within the USB Service.
 *
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_usb_dplus_pullup_disable(void * p_context);


#ifdef __cplusplus
}
#endif

#endif /* NRFS_USB_H */
