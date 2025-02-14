/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_MRAM_H
#define NRFS_MRAM_H

#include <internal/services/nrfs_mram.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief MRAM latency service response notification types. */
typedef enum __NRFS_PACKED {
	NRFS_MRAM_LATENCY_REQ_APPLIED  = 0, /** sent only as a response to the request MRAM_LATENCY_NOT_ALLOWED */
	NRFS_MRAM_LATENCY_REQ_REJECTED = 1, /** Request rejected. */
} nrfs_mram_latency_evt_type_t;

/** @brief MRAM latency service response data structure. */
typedef struct {
	nrfs_mram_latency_evt_type_t type;
} nrfs_mram_latency_evt_t;

/** @brief MRAM latency event handler type. */
typedef void (*nrfs_mram_latency_evt_handler_t)(nrfs_mram_latency_evt_t const * p_evt,
						void * context);

/**
 * @brief Function for initializing the MRAM latency service.
 *
 * @param[in] handler             Function called as a response to the request.
 *
 * @retval NRFS_SUCCESS           Service initialized successfully.
 * @retval NRFS_ERR_INVALID_STATE Service was already initialized.
 */
nrfs_err_t nrfs_mram_init(nrfs_mram_latency_evt_handler_t handler);

/**
 * @brief Function for uninitializing the MRAM latency service.
 *
 * @warning Notifications from previous requests are dropped after service uninitialization.
 */
void nrfs_mram_uninit(void);

/**
 * @brief Function for requesting a new acceptable MRAM latency.
 *
 * @param[in] mram_latency_request Enum value:  MRAM_LATENCY_ALLOWED
                                                MRAM_LATENCY_NOT_ALLOWED
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_mram_set_latency(mram_latency_request_t mram_latency_request, void * p_context);

#ifdef __cplusplus
}
#endif

#endif /* NRFS_MRAM_H */
