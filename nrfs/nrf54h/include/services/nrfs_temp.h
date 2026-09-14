/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_TEMP_H
#define NRFS_TEMP_H

#include <internal/services/nrfs_temp.h>
#include <nrfs_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Temperature service event types. */
typedef enum __NRFS_PACKED {
	NRFS_TEMP_EVT_MEASURE_DONE	= 0, /** Temperature measurement done. */
#ifdef NRFS_TEMP_SERVICE_SUBSCRIPTION_ENABLED
	NRFS_TEMP_EVT_CHANGE		= 1, /** Temperature threshold crossed. */
#endif
	NRFS_TEMP_EVT_REJECT		= 2, /** Request rejected. */
} nrfs_temp_evt_type_t;

/** @brief Temperature service event. */
typedef struct {
	nrfs_temp_evt_type_t type; /** Event type. */
	int32_t raw_temp; /** Raw temperature in a 2's complement signed value representation.
                                    *  Valid for @p NRFS_TEMP_EVT_MEASURE_DONE and @p NRFS_TEMP_EVT_CHANGE. */
} nrfs_temp_evt_t;

/** @brief Temperature service event handler type. */
typedef void (*nrfs_temp_evt_handler_t)(nrfs_temp_evt_t const * p_evt, void * context);

/**
 * @brief Function for initializing the Temperature service.
 *
 * @param[in] handler Function called as a response to the request.
 *
 * @retval NRFS_SUCCESS           Service initialized successfully.
 * @retval NRFS_ERR_INVALID_STATE Service was already initialized.
 */
nrfs_err_t nrfs_temp_init(nrfs_temp_evt_handler_t handler);

/**
 * @brief Function for uninitializing the Temperature service.
 *
 * @warning Notifications from previous requests are dropped after service uninitialization.
 */
void nrfs_temp_uninit(void);

/**
 * @brief Function for requesting a temperature measurement in Temperature service.
 *
 * @param[in] p_context Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_temp_measure_request(void * p_context);

#ifdef NRFS_TEMP_SERVICE_SUBSCRIPTION_ENABLED
/**
 * @brief Function for subscribing to the temperature monitoring in Temperature service.
 *
 * @note If subscription is already active, calling this function again
 *       will overwrite the subscription parameters.
 *
 * @param[in] measure_rate_ms  Maximal acceptable time between subsequent temperature checks.
 * @param[in] lower_threshold  Temperature lower threshold in a raw 2's complement signed format.
 * @param[in] upper_threshold  Temperature upper threshold in a raw 2's complement signed format.
 * @param[in] p_context        Opaque user data that will be passed to registered callback.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_temp_subscribe(uint16_t measure_rate_ms,
			       int32_t lower_threshold,
			       int32_t upper_threshold,
			       void * p_context);

/**
 * @brief Function for unsubscribing from the temperature monitoring in Temperature service.
 *
 * @retval NRFS_SUCCESS           Request sent successfully.
 * @retval NRFS_ERR_INVALID_STATE Service is uninitialized.
 * @retval NRFS_ERR_IPC           Backend returned error during request sending.
 */
nrfs_err_t nrfs_temp_unsubscribe(void);
#endif /* NRFS_TEMP_SERVICE_SUBSCRIPTION_ENABLED */

/**
 * @brief Function for converting the temperature value from raw data to Celsius scale.
 *
 * The returned temperature value is in Celsius scale, multiplied by 100.
 * For example, the actual temperature of 25.75[C] will be returned as a 2575 signed integer.
 * Measurement accuracy is 0.25[C].
 *
 * @param[in] raw_temp Temperature value in a 2's complement signed value representation.
 *
 * @return Temperature in Celsius scale.
 */
static inline int32_t nrfs_temp_from_raw(int32_t raw_temp)
{
	/* Raw temperature is a 2's complement signed value. Moreover, it is represented
     * by 0.25[C] intervals, so division by 4 is needed. To preserve
     * fractional part, raw value is multiplied by 100 before division.*/

	return (raw_temp * 100) / 4;
}

/**
 * @brief Function for converting a temperature value from Celsius scale to raw data.
 *
 * The returned temperature value is a 2's complement signed value representation.
 * For example, actual temperature of 25.75[C] should be provided as 2575 signed integer,
 * and will be returned as a 103 integer.
 * During conversion, the accuracy will be reduced to 0.25[C] steps.
 *
 * @param[in] temp_c Temperature value in Celsius scale, multiplied by 100.
 *
 * @return Temperature in RAW format.
 */
static inline int32_t nrfs_temp_to_raw(int32_t temp_c)
{
	return (temp_c * 4) / 100;
}

#ifdef __cplusplus
}
#endif

#endif /* NRFS_TEMP_H */
