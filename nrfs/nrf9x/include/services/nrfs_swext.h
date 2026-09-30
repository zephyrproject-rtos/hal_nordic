/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_SWEXT_H
#define NRFS_SWEXT_H

#include <internal/services/nrfs_swext.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief SWEXT control service response notification types. */
typedef enum __NRFS_PACKED {
	NRFS_SWEXT_EVT_ENABLED		= 0, /** Output enabled. */
	NRFS_SWEXT_EVT_OVERCURRENT	= 1, /** Overcurrent detected. SWEXT is powered down. */
	NRFS_SWEXT_EVT_REJECTED		= 2, /** Request rejected. */
} nrfs_swext_evt_type_t;

/** @brief SWEXT control service response data structure. */
typedef struct {
	nrfs_swext_evt_type_t type;
} nrfs_swext_evt_t;

/** @brief SWEXT control service event handler type. */
typedef void (*nrfs_swext_evt_handler_t)(nrfs_swext_evt_t const *p_evt, void *context);

/**
 * @brief Function for initializing SWEXT (SWitch EXTernal) peripheral.
 *
 * @param[in] handler             Function called as a response to the request.
 *
 * @retval NRFS_SUCCESS           Service initialized successfully.
 * @retval NRFS_ERR_INVALID_STATE Service was already initialized.
 */
nrfs_err_t nrfs_swext_init(nrfs_swext_evt_handler_t handler);

/**
 * @brief Function for uninitializing SWEXT (SWitch EXTernal) peripheral.
 *
 * @warning Notifications from previous requests are dropped after service uninitialization.
 */
void nrfs_swext_uninit(void);

/**
 * @brief Function for enabling SWEXT (SWitch EXTernal) peripheral.

 * @param[in] load_current Load current in raw format.
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_swext_power_up(uint8_t load_current, void *p_context);

/**
 * @brief Function for disabling SWEXT (SWitch EXTernal) peripheral.

 * @param[in] pd_clamp Enable/disable power down clamp.
 * @param[in] p_context Opaque user datathat will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_swext_power_down(swext_pd_clamp_t pd_clamp, void *p_context);

/**
 * @brief Function for converting a load current value in uA to raw data.
 *
 * The returned value is represented in 500uA steps rounded up.
 *
 * @param[in] load_current Estimated load current attached to SWEXT peripheral.
 *
 * @retval Load current in raw format.
 */

static inline uint8_t nrfs_swext_load_current_to_raw(uint16_t load_current)
{
	return (load_current + 499) / 500;
}

#ifdef __cplusplus
}
#endif

#endif /* NRFS_SWEXT_H */
