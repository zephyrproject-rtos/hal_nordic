/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_DISPATCHER_H
#define NRFS_DISPATCHER_H

#include <nrfs_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function for notifying the dispatcher about incoming message.
 *
 * This function is called internally by the backend when a message arrives.
 *
 * @param[in] p_notification Pointer to the notification payload.
 * @param[in] size           Notification payload size.
 */
void nrfs_dispatcher_notify(void *message, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* NRFS_DISPATCHER_H */
