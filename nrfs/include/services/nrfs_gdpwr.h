/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_GDPWR_H
#define NRFS_GDPWR_H

#include <internal/services/nrfs_gdpwr.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Global domain power request service response notification types. */
typedef enum __NRFS_PACKED {
	NRFS_GDPWR_REQ_APPLIED  = 0, /** Request accepted. */
	NRFS_GDPWR_REQ_REJECTED = 1, /** Request rejected. */
} nrfs_gdpwr_evt_type_t;

/** @brief Global domain power request service response data structure. */
typedef struct {
	nrfs_gdpwr_evt_type_t type;
} nrfs_gdpwr_evt_t;

/** @brief Global domain power request service event handler type. */
typedef void (*nrfs_gdpwr_evt_handler_t)(nrfs_gdpwr_evt_t const *p_evt,
						void *context);

/**
 * @brief Function for initializing the global domain power request service.
 *
 * @param[in] handler             Function called as a response to the request.
 *
 * @retval NRFS_SUCCESS           Service initialized successfully.
 * @retval NRFS_ERR_INVALID_STATE Service was already initialized.
 */
nrfs_err_t nrfs_gdpwr_init(nrfs_gdpwr_evt_handler_t handler);

/**
 * @brief Function for uninitializing the global domain power request service.
 *
 * @warning Notifications from previous requests are dropped after service uninitialization.
 */
void nrfs_gdpwr_uninit(void);

/**
 * @brief Function for requesting and releasing power request in the specific global domain.
 *
 * @param[in] power_domain selection of power domain in the global domain.
 * @param[in] req_type set/clear power request for specific power domain.
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_gdpwr_power_request(gdpwr_power_domain_t power_domain,
				    gdpwr_request_type_t req_type,
				    void *p_context);

#ifdef __cplusplus
}
#endif

#endif /* NRFS_GDPWR_H */
