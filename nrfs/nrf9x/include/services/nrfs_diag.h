/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_DIAG_H__
#define NRFS_DIAG_H__

#include <internal/services/nrfs_diag.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief System Diagnostics Service response notification types. */
typedef enum __NRFS_PACKED {
	NRFS_DIAG_EVT_APPLIED	= 0, /** Request applied succesfully */
	NRFS_DIAG_EVT_REJECT	= 1, /** Request rejected. */
	NRFS_DIAG_EVT_REG_RSP	= 2, /** Response for register request */
} nrfs_diag_evt_type_t;

/** @brief System Diagnostics Service response data structure. */
typedef struct __NRFS_PACKED {
	nrfs_diag_evt_type_t type; /** Event type. */
	struct {
		diag_reg_access_type_t access_type; /** Register access type. */
		uint32_t addr;			    /** Register address. */
		uint32_t val;			    /** Register value. */
	} reg;					    /** Register request. */
} nrfs_diag_evt_t;

/** @brief System Diagnostics Service response handler type. */
typedef void (*nrfs_diag_response_handler_t)(nrfs_diag_evt_t const * p_evt, void * p_context);

/**
 * @brief Function for initializing the System Diagnostics service.
 *
 * @param[in] handler    Function called as a response to the request
 *
 * @retval NRFS_SUCCESS           Service initialized successfully.
 * @retval NRFS_ERR_INVALID_STATE Service was already initialized.
 */
nrfs_err_t nrfs_diag_init(nrfs_diag_response_handler_t handler);

/**
 * @brief Function for uninitializing the System Diagnostics service.
 *
 * @warning Notifications from previous requests are dropped after service uninitialization.
 */
void nrfs_diag_uninit(void);

/**
 * @brief Function for reading a register
 *
 * Value of register read will be available for handler.
 *
 * @param[in] addr      32-bit register address
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_diag_reg_read(uint32_t addr, void * p_context);

/**
 * @brief Function for writing a register
 *
 * @param[in] addr       32-bit register address
 * @param[in] val        32-bit value to be written to the register
 * @param[in] p_context  Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_diag_reg_write(uint32_t addr, uint32_t val, void * p_context);

#ifdef __cplusplus
}
#endif

#endif /* NRFS_DIAG_H__ */
