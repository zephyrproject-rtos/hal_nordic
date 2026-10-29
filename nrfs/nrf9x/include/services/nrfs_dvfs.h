/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_DVFS_H
#define NRFS_DVFS_H

#include <internal/services/nrfs_dvfs.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Dynamic Voltage and Frequency Scaling service event types. */
typedef enum __NRFS_PACKED {
	NRFS_DVFS_EVT_REJECT			= 0, /** General purpose event for rejected requests. */
	NRFS_DVFS_EVT_INIT_PREPARATION		= 1, /** ABB Oppoint has been set correctly. Ready for second step of initialization sequence. */
	NRFS_DVFS_EVT_INIT_DONE			= 2, /** ABB Oppoint has been set correctly. Initialization sequence completed. */
	NRFS_DVFS_EVT_OPPOINT_REQ_CONFIRMED	= 3, /** Request for operating frequency change has been registered. Please standby. */
	NRFS_DVFS_EVT_OPPOINT_SCALING_PREPARE	= 4, /** Prepare for voltage scaling. Awaiting the @p nrfs_dvfs_ready_to_scale response. */
	NRFS_DVFS_EVT_OPPOINT_SCALING_DONE	= 5, /** Voltage scaling done, new oppoint can be applied. */
} nrfs_dvfs_evt_type_t;

/** @brief Dynamic Voltage and Frequency Scaling service event. */
typedef struct {
	nrfs_dvfs_evt_type_t type; /** Event type. */
	enum dvfs_frequency_setting freq;		   /** Maximum allowed HSFLL frequency oppoint.
                                    *  For the @p NRFS_DVFS_EVT_OPPOINT_SCALING_PREPARE
                                    *  this oppoint dictates, which oppoint needs to be prepared.  */
} nrfs_dvfs_evt_t;

/** @brief Dynamic Voltage and Frequency Scaling service event handler type. */
typedef void (*nrfs_dvfs_evt_handler_t)(nrfs_dvfs_evt_t const * p_evt, void * context);

/**
 * @brief Function for initializing the Dynamic Voltage and Frequency Scaling service.
 *
 * @param[in] handler Function called as a response to the request.
 *
 * @retval NRFS_SUCCESS           Service initialized successfully.
 * @retval NRFS_ERR_INVALID_STATE Service was already initialized.
 */
nrfs_err_t nrfs_dvfs_init(nrfs_dvfs_evt_handler_t handler);

/**
 * @brief Function for uninitializing the Dynamic Voltage and Frequency Scaling service.
 *
 * @warning Notifications from previous requests are dropped after service uninitialization.
 */
void nrfs_dvfs_uninit(void);

/**
 * @brief Function for requesting the first step of the init sequence. This will trigger proper ABB Oppoint setting.
 *
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_dvfs_init_prepare_request(void * p_context);

/**
 * @brief Function for requesting the second step of the init sequence. This will trigger proper ABB Oppoint setting.
 *
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_dvfs_init_complete_request(void * p_context);

/**
 * @brief Function for requesting an operating frequency change.
 * @note The @p target_freq requirement might not be met by the system
 * until the NRFS_DVFS_EVT_OPPOINT_REQ_CONFIRMED response is triggered.
 *
 * @param[in] target_freq  Minimal required frequency oppoint
 * @param[in] p_context    Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_dvfs_oppoint_request(enum dvfs_frequency_setting target_freq, void * p_context);

/**
 * @brief Function for requesting an operating frequency change.
 * @note The response @p NRFS_DVFS_EVT_OPPOINT_REQ_CONFIRMED will not be triggered.
 *
 * @param[in] target_freq  Minimal required frequency oppoint
 * @param[in] p_context    Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_dvfs_oppoint_request_no_rsp(enum dvfs_frequency_setting target_freq, void * p_context);

/**
 * @brief Function for notifying that the requested oppoint has been prepared.
 *
 * @param[in] p_context    Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_dvfs_ready_to_scale(void * p_context);

#ifdef __cplusplus
}
#endif

#endif /* NRFS_DVFS_H */
