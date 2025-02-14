/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_PMIC_H
#define NRFS_PMIC_H

#include <internal/services/nrfs_pmic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief PMIC Service response notification types. */
typedef enum __NRFS_PACKED {
	NRFS_PMIC_EVT_APPLIED		= 0, /** Request applied succesfully */
	NRFS_PMIC_EVT_REJECT		= 1, /** Request rejected. */
	NRFS_PMIC_EVT_TEST_IF_RSP	= 2, /** Response for TEST_IF request */
	NRFS_PMIC_EVT_INFO_RSP		= 3, /** Response for INFO IF request */
} nrfs_pmic_evt_type_t;

/** @brief PMIC Service response data structure. */
typedef struct {
	nrfs_pmic_evt_type_t type;	    /** Event type. */
	pmic_reg_access_type_t access_type; /** Register access type */
	uint8_t val;			    /** Register read value used by PMIC_TEST_IF request */
} nrfs_pmic_evt_t;

/** @brief PMIC Service info response data structure. */
typedef struct {
	nrfs_pmic_evt_type_t type;	    /** Event type. */
	nrfs_pmic_info_rsp_data_t info;
} nrfs_pmic_info_evt_t;

/** @brief PMIC Service event handler type. */
typedef void (*nrfs_pmic_evt_handler_t)(void const *p_evt, void * p_context);

/**
 * @brief Function for initializing the PMIC service.
 *
 * @param[in] handler    Function called as a response to the request
 *
 * @retval NRFS_SUCCESS           Service initialized successfully.
 * @retval NRFS_ERR_INVALID_STATE Service was already initialized.
 */
nrfs_err_t nrfs_pmic_init(nrfs_pmic_evt_handler_t handler);

/**
 * @brief Function for uninitializing the PMIC service.
 *
 * @warning Notifications from previous requests are dropped after service uninitialization.
 */
void nrfs_pmic_uninit(void);

/**
 * @brief Function for requesting power ON RFFE interface
 *
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_rffe_on(void * p_context);

/**
 * @brief Function for requesting power ON RFFE interface with no response
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_rffe_on_no_rsp(void);

/**
 * @brief Function for requesting power OFF RFFE interface
 *
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_rffe_off(void * p_context);

/**
 * @brief Function for requesting power OFF RFFE interface with no response
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_rffe_off_no_rsp(void);

/**
 * @brief Function for requesting power ON SIM interface
 *
 * @param[in] sim       Requested SIM to power ON.
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_sim_on(pmic_sim_t sim, void * p_context);

/**
 * @brief Function for requesting power ON SIM interface with no response
 *
 * @param[in] sim       Requested SIM to power ON.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_sim_on_no_rsp(pmic_sim_t sim);

/**
 * @brief Function for requesting power OFF SIM interface
 *
 * @param[in] sim       Requested SIM to power OFF.
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_sim_off(pmic_sim_t sim, void * p_context);

/**
 * @brief Function for requesting power OFF SIM interface with no response
 *
 * @param[in] sim       Requested SIM to power OFF.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_sim_off_no_rsp(pmic_sim_t sim);

/**
 * @brief Function for requesting power ON BLE radio
 *
 * @param[in] txpower    Needed TX power
 * @param[in] p_context  Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_ble_radio_on(pmic_ble_radio_txpower_t txpower, void * p_context);

/**
 * @brief Function for requesting power ON BLE radio with no response
 *
 * @param[in] txpower    Needed TX power
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_ble_radio_on_no_rsp(pmic_ble_radio_txpower_t txpower);

/**
 * @brief Function for requesting power OFF BLE radio
 *
 * @param[in] p_context  Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_ble_radio_off(void * p_context);

/**
 * @brief Function for requesting power OFF BLE radio with no response
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_ble_radio_off_no_rsp(void);

/**
 * @brief Function for setting default PWM settings to DCDC converters
 *
 * @param[in] p_context  Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_pwm_default_set(void * p_context);

/**
 * @brief Function for setting default PWM settings to DCDC converters with no response
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_pwm_default_set_no_rsp(void);

/**
 * @brief Function for setting ghost avoidance PWM settings to DCDC converters
 *
 * @param[in] p_context  Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_pwm_ghost_avoid_set(void * p_context);

/**
 * @brief Function for setting ghost avoidance PWM settings to DCDC converters with no response
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_pwm_ghost_avoid_set_no_rsp(void);

/**
 * @brief Function for checking PMIC existence
 *
 * Value will be available for handler.
 *
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_info_read(void * p_context);

/**
 * @brief Function for reading PMIC register
 *
 * Value of register read will be available for handler.
 *
 * @param[in] addr       16-bit register address
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_test_if_read(uint16_t addr, void * p_context);

/**
 * @brief Function for writing PMIC register
 *
 * @param[in] addr       16-bit register address
 * @param[in] val        8-bit value to be written to PMIC register
 * @param[in] p_context  Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_pmic_test_if_write(uint16_t addr, uint8_t val, void * p_context);

#ifdef __cplusplus
}
#endif

#endif /* NRFS_PMIC_H */
