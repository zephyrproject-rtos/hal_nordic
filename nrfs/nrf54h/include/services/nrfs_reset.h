/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_RESET_H
#define NRFS_RESET_H

#include <internal/services/nrfs_reset.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Reset service event types. */
typedef enum __NRFS_PACKED {
	NRFS_RESET_EVT_DONE	= 0, /** Reset done. */
	NRFS_RESET_EVT_REJECT	= 1, /** Request rejected. */
} nrfs_reset_evt_type_t;

/** @brief Reset service event. */
typedef struct {
	nrfs_reset_evt_type_t type; /** Event type. */
} nrfs_reset_evt_t;

/** @brief Reset service event handler type. */
typedef void (*nrfs_reset_evt_handler_t)(nrfs_reset_evt_t const * p_evt);

/**
 * @brief Function for initializing the RESET service.
 *
 * @param[in] handler   Function called as a response to the request.
 *
 * @retval NRFS_SUCCESS           Service initialized successfully.
 * @retval NRFS_ERR_INVALID_STATE Service was already initialized.
 */
nrfs_err_t nrfs_reset_init(nrfs_reset_evt_handler_t handler);

/**
 * @brief Function for uninitializing the RESET service.
 *
 * @warning Notifications from previous requests are dropped after service uninitialization.
 */
void nrfs_reset_uninit(void);

/**
 * @brief Function for requesting a reset of all services.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_request_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* NRFS_RESET_H */
