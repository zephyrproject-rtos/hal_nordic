/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_AUDIOPLL_H
#define NRFS_AUDIOPLL_H

#include <internal/services/nrfs_audiopll.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Audio PLL service event types. */
typedef enum __NRFS_PACKED {
	NRFS_AUDIOPLL_EVT_ENABLED		= 0, /** Audio PLL enabled. */
	NRFS_AUDIOPLL_EVT_DISABLED		= 1, /** Audio PLL disabled. */
	NRFS_AUDIOPLL_EVT_FREQ_CONFIRMED	= 2, /** Audio PLL new frequency setting confirmed. */
	NRFS_AUDIOPLL_EVT_PRESCALER_CONFIRMED	= 3, /** Audio PLL new prescaler setting confirmed. */
	NRFS_AUDIOPLL_EVT_FREQ_INC_CONFIRMED	= 4, /** Audio PLL new frequency inc setting. */
	NRFS_AUDIOPLL_EVT_REJECT		= 5, /** Request rejected. */
} nrfs_audiopll_evt_type_t;

/** @brief Audio PLL service response data structure. */
typedef struct {
	nrfs_audiopll_evt_type_t type;
} nrfs_audiopll_evt_t;

/** @brief Audio PLL service event handler type. */
typedef void (*nrfs_audiopll_evt_handler_t)(nrfs_audiopll_evt_t const * p_evt, void * context);

/**
 * @brief Function for initializing the audio PLL service.
 *
 * @param[in] handler Function called as a response to the request.
 *
 * @retval NRFS_SUCCESS           Service initialized successfully.
 * @retval NRFS_ERR_INVALID_STATE Service was already initialized.
 */
nrfs_err_t nrfs_audiopll_init(nrfs_audiopll_evt_handler_t handler);

/**
 * @brief Function to uninit audio PLL service.
 *
 * @warning Notifications from previous requests are dropped after service is uninitialized.
 */
void nrfs_audiopll_uninit(void);

/**
 * @brief Function used to request audio PLL peripheral initialization.
 *
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_audiopll_enable_request(void * p_context);

/**
 * @brief Function used to request audio PLL peripheral to be disabled.
 *
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_audiopll_disable_request(void * p_context);

/**
 * @brief Function for requesting audio PLL frequency divider change.
 *
 * @param[in] pll_freq_fraction  PLL frequency fractional divider ratio 0-65535
 * @note The @p pll_freq_fraction use smaller values (under 2000) to apply fine tuning on
 * audiopll, otherwise new base frequency will be applied resulting in going out of lock state
 * for some time.
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_audiopll_request_freq(uint16_t pll_freq_fraction, void *p_context);

/**
 * @brief Function for requesting audio PLL frequency divider change.
 *
 * @param[in] pll_div one of the predefined audio pll prescaler setting, default AUDIOPLL_DIV_12
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_audiopll_request_prescaler(enum audiopll_prescaler_div pll_div, void *p_context);

/**
 * @brief Function for requesting audio PLL frequency periodic change.
 *
 * @param[in] inc_val increment value in range -128 to 127, 0 stops functionality.
 * @param[in] inc_period increment period in ms, 0 stops functionality.
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_audiopll_request_freq_inc(int8_t inc_val, uint16_t inc_period,
	void *p_context);

#ifdef __cplusplus
}
#endif

#endif /* NRFS_AUDIOPLL_H */
