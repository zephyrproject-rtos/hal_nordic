/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_CLOCK_H
#define NRFS_CLOCK_H

#include <internal/services/nrfs_clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Clock service event types. */
typedef enum __NRFS_PACKED {
	NRFS_CLOCK_EVT_APPLIED	= 0, /** Request applied successfully. */
	NRFS_CLOCK_EVT_REJECT	= 1, /** Request rejected. */
	NRFS_CLOCK_EVT_CHANGE	= 2, /** Clock event changed. */
} nrfs_clock_evt_type_t;

/** @brief Clock service event. */
typedef struct {
	nrfs_clock_evt_type_t type; /** Event type. */
	nrfs_clock_rsp_data_t data; /** Event data. */
} nrfs_clock_evt_t;

/** @brief Clock service event handler type. */
typedef void (*nrfs_clock_evt_handler_t)(nrfs_clock_evt_t const * p_evt, void * context);

/**
 * @brief Function for initializing the clock service.
 *
 * @param[in] handler Function called as a response to the request.
 *
 * @retval NRFS_SUCCESS           Service initialized successfully.
 * @retval NRFS_ERR_INVALID_STATE Service was already initialized.
 */
nrfs_err_t nrfs_clock_init(nrfs_clock_evt_handler_t handler);

/**
 * @brief Function for uninitializing the clock service.
 *
 * @warning Notifications from previous requests are dropped after service uninitialization.
 */
void nrfs_clock_uninit(void);

/**
 * @brief Function for subscribing to events in the clock service.
 *
 * @note If subscription is already active, calling this function again
 *       will overwrite the subscription parameters.
 *
 * @param[in] event_mask Mask of events to be subscribed to. Use values of nrfs_clock_evt_reason_t.
 * @param[in] p_context  Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_clock_subscribe(uint8_t event_mask, void * p_context);

/**
 * @brief Function for unsubscribing from events in the clock service.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_clock_unsubscribe(void);

/**
 * @brief Function for setting LFCLK source.
 *
 * @param[in] src       LFCLK source to be set.
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_clock_lfclk_src_set(nrfs_clock_src_t src, void * p_context);

/**
 * @brief Function for setting LFCLK source with no response.
 *
 * @param[in] src       LFCLK source to be set.
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_clock_lfclk_src_set_no_rsp(nrfs_clock_src_t src, void * p_context);

/**
 * @brief Function for setting HSFLL_120 mode.
 *
 * @param[in] mode      HSFLL mode to be set.
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_clock_hsfll_mode_set(nrfs_clock_hsfll_mode_t mode, void * p_context);

/**
 * @brief Function for setting HSFLL_120 mode with no response.
 *
 * @param[in] mode      HSFLL mode to be set.
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_clock_hsfll_mode_set_no_rsp(nrfs_clock_hsfll_mode_t mode, void * p_context);

#ifdef __cplusplus
}
#endif

#endif /* NRFS_CLOCK_H */
