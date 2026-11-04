/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_GSWDT_H
#define NRFS_GSWDT_H

#include <internal/services/nrfs_gswdt.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Global Software Watchdog service event types. */
typedef enum __NRFS_PACKED {
	NRFS_GSWDT_EVT_TIMEOUT_SET	= 1, /** Watchdog timeout change accepted. */
	NRFS_GSWDT_EVT_STOP_DONE	= 2, /** Watchdog stop accepted. */
	NRFS_GSWDT_EVT_REJECT		= 3, /** General purpose event for rejected requests. */
} nrfs_gswdt_evt_type_t;

/** @brief Global Software Watchdog service event. */
typedef struct {
	nrfs_gswdt_evt_type_t type; /** Event type. */
} nrfs_gswdt_evt_t;

/** @brief Global Software Watchdog service event handler type. */
typedef void (*nrfs_gswdt_evt_handler_t)(nrfs_gswdt_evt_t const *p_evt, void *context);

/**
 * @brief Function for initializing the Global Software Watchdog service.
 *
 * @param[in] handler Function called as a response to the request.
 *
 * @retval NRFS_SUCCESS           Service initialized successfully.
 * @retval NRFS_ERR_INVALID_STATE Service was already initialized.
 */
nrfs_err_t nrfs_gswdt_init(nrfs_gswdt_evt_handler_t handler);

/**
 * @brief Function for uninitializing the Global Software Watchdog service.
 *
 * @warning Notifications from previous requests are dropped after service uninitialization.
 */
void nrfs_gswdt_uninit(void);

/**
 * @brief Function for requesting a watchdog timeout change.
 *
 * @param[in] timeout_ms  Watchdog timeout in milliseconds.
 * @param[in] p_context   Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_gswdt_timeout_set(uint32_t timeout_ms, void *p_context);

/**
 * @brief Function for requesting the watchdog to stop.
 *
 * @param[in] p_context   Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_gswdt_stop(void *p_context);

#ifdef __cplusplus
}
#endif

#endif /* NRFS_GSWDT_H */
