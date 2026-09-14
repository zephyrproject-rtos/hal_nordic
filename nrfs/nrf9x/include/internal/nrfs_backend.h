/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_BACKEND_H
#define NRFS_BACKEND_H

#include <nrfs_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function for sending a message over the chosen transport backend.
 *
 * This function is used by services to send requests to the System Controller.
 *
 * @param[in] message Pointer to the message payload.
 * @param[in] size    Message payload size.
 *
 * @retval NRFS_SUCCESS Message sent successfully.
 * @retval NRFS_ERR_IPC Backend returned error during message sending.
 */
nrfs_err_t nrfs_backend_send(void *message, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* NRFS_BACKEND_H */
