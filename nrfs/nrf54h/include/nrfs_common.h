/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_COMMON_H
#define NRFS_COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define __NRFS_PACKED __attribute__((__packed__))

/**
 * @brief Macro for getting the number of elements in an array.
 *
 * @param[in] array Name of the array.
 *
 * @return Array element count.
 */
#define NRFS_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

/** @brief Return codes. */
typedef enum __NRFS_PACKED {
	NRFS_SUCCESS		= 0, /** Success. */
	NRFS_ERR_INVALID_STATE	= 1, /** Invalid state. */
	NRFS_ERR_IPC		= 2, /** IPC error. */
} nrfs_err_t;

#endif /* NRFS_COMMON_H */
