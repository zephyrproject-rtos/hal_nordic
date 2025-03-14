/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_CALLBACKS_H
#define NRFS_CALLBACKS_H

#include <nrfs_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Service callback type. */
typedef void (*nrfs_service_cb_t)(void *p_notification, size_t size);

/**
 * @brief Function for notifying the Temperature service about incoming message.
 *
 * This function is called internally by the dispatcher when the corresponding message arrives.
 *
 * @param[in] p_notification Pointer to the notification payload.
 * @param[in] size           Notification payload size.
 */
void nrfs_temp_service_notify(void *p_notification, size_t size);

/**
 * @brief Function for notifying the Reset service about incoming message.
 *
 * This function is called internally by the dispatcher when the corresponding message arrives.
 *
 * @param[in] p_notification Pointer to the notification payload.
 * @param[in] size           Notification payload size.
 */
void nrfs_reset_service_notify(void *p_notification, size_t size);

/**
 * @brief Function for notifying the MRAM latency service about incoming message.
 *
 * This function is called internally by the dispatcher when the corresponding message arrives.
 *
 * @param[in] p_notification Pointer to the notification payload.
 * @param[in] size           Notification payload size.
 */
void nrfs_mram_service_notify(void *p_notification, size_t size);

/**
 * @brief Function for notifying the USB VBUS detector service about incoming message.
 *
 * This function is called internally by the dispatcher when the corresponding message arrives.
 *
 * @param[in] p_notification Pointer to the notification payload.
 * @param[in] size           Notification payload size.
 */
void nrfs_usb_service_notify(void *p_notification, size_t size);

/**
 * @brief Function for notifying the PMIC service about incoming message.
 *
 * This function is called internally by the dispatcher when the corresponding message arrives.
 *
 * @param[in] p_notification Pointer to the notification payload.
 * @param[in] size           Notification payload size.
 */
void nrfs_pmic_service_notify(void *p_notification, size_t size);

/**
 * @brief Function for notifying the System Diagnostics service about incoming message.
 *
 * This function is called internally by the dispatcher when the corresponding message arrives.
 *
 * @param[in] p_notification Pointer to the notification payload.
 * @param[in] size           Notification payload size.
 */
void nrfs_diag_service_notify(void *p_notification, size_t size);

/**
 * @brief Function for notifying the DVFS service about incoming message.
 *
 * This function is called internally by the dispatcher when the corresponding message arrives.
 *
 * @param[in] p_notification Pointer to the notification payload.
 * @param[in] size           Notification payload size.
 */
void nrfs_dvfs_service_notify(void *p_notification, size_t size);

/**
 * @brief Function for notifying the clock service about incoming message.
 *
 * This function is called internally by the dispatcher when the corresponding message arrives.
 *
 * @param[in] p_notification Pointer to the notification payload.
 * @param[in] size           Notification payload size.
 */
void nrfs_clock_service_notify(void *p_notification, size_t size);

/**
 * @brief Function for notifying the global domain power request service about incoming message.
 *
 * This function is called internally by the dispatcher when the corresponding message arrives.
 *
 * @param[in] p_notification Pointer to the notification payload.
 * @param[in] size           Notification payload size.
 */
void nrfs_gdpwr_service_notify(void *p_notification, size_t size);

/**
 * @brief Function for notifying the GDFS service about incoming message.
 *
 * This function is called internally by the dispatcher when the corresponding message arrives.
 *
 * @param[in] p_notification Pointer to the notification payload.
 * @param[in] size           Notification payload size.
 */
void nrfs_gdfs_service_notify(void *p_notification, size_t size);

/**
 * @brief Function for notifying the SWEXT (SWitch EXTernal) control service about incoming message.
 *
 * This function is called internally by the dispatcher when the corresponding message arrives.
 *
 * @param[in] p_notification Pointer to the notification payload.
 * @param[in] size           Notification payload size.
 */
void nrfs_swext_service_notify(void *p_notification, size_t size);

/**
 * @brief Function for notifying the AUDIOPLL service about incoming message.
 *
 * This function is called internally by the dispatcher when the corresponding message arrives.
 *
 * @param[in] p_notification Pointer to the notification payload.
 * @param[in] size           Notification payload size.
 */
void nrfs_audiopll_service_notify(void *p_notification, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* NRFS_CALLBACKS_H */
