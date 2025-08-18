/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_GDFS_H
#define NRFS_GDFS_H

#include <internal/services/nrfs_gdfs.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Global Domain Frequency Scaling service event types. */
typedef enum __NRFS_PACKED {
	NRFS_GDFS_EVT_REJECT		= 0, /** General purpose event for rejected requests. */
	NRFS_GDFS_EVT_FREQ_CONFIRMED	= 1, /** Frequency has been achieved. */
} nrfs_gdfs_evt_type_t;

/** @brief Global Domain Frequency Scaling service event. */
typedef struct {
	nrfs_gdfs_evt_type_t type;			/** Event type. */
} nrfs_gdfs_evt_t;

/** @brief Global Domain Frequency Scaling service event handler type. */
typedef void (*nrfs_gdfs_evt_handler_t)(nrfs_gdfs_evt_t const * p_evt, void * context);

/**
 * @brief Function for initializing the Global Domain Frequency Scaling service.
 *
 * @param[in] handler Function called as a response to the request.
 *
 * @retval NRFS_SUCCESS           Service initialized successfully.
 * @retval NRFS_ERR_INVALID_STATE Service was already initialized.
 */
nrfs_err_t nrfs_gdfs_init(nrfs_gdfs_evt_handler_t handler);

/**
 * @brief Function for uninitializing the Global Domain Frequency Scaling service.
 *
 * @warning Notifications from previous requests are dropped after service uninitialization.
 */
void nrfs_gdfs_uninit(void);

/**
 * @brief Function for requesting a frequency change.
 * @note The @p target_freq requirement might not be met by the system
 * until the NRFS_GDFS_EVT_FREQ_CONFIRMED response is triggered.
 *
 * @param[in] target_freq  Minimal required frequency
 * @param[in] p_context    Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_gdfs_request_freq(enum gdfs_frequency_setting target_freq, void * p_context);

/**
 * @brief Function for requesting a frequency change.
 * @note The response @p NRFS_GDFS_EVT_FREQ_CONFIRMED will not be triggered.
 *
 * @param[in] target_freq  Minimal required frequency
 * @param[in] p_context    Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_gdfs_request_freq_no_rsp(enum gdfs_frequency_setting target_freq, void * p_context);

#ifdef __cplusplus
}
#endif

#endif /* NRFS_GDFS_H */
