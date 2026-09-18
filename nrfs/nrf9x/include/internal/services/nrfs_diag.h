/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_DIAG_H
#define NRFS_DIAG_H

#include <internal/services/nrfs_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief System Diagnostics register access type. */
typedef enum __NRFS_PACKED {
	DIAG_REG_READ	  = 0, /** Register read */
	DIAG_REG_WRITE	  = 1, /** Register write */
	DIAG_REG_INVALID  = 2, /** Register access invalid */
} diag_reg_access_type_t;

/** @brief System Diagnostics register access. */
typedef struct __NRFS_PACKED {
	diag_reg_access_type_t access_type; /** Register access type. */
	uint32_t addr;			    /** Register address. */
	uint32_t val;			    /** Register value. */
} diag_reg_access_t;

/** @brief System Diagnostics service notification data structure. */
typedef struct __NRFS_PACKED {
	diag_reg_access_type_t access_type; /** Register access type. */
	uint32_t addr;			    /** Register address. */
	uint32_t val;			    /** Register value. */
} nrfs_diag_rsp_data_t;

/** @brief System Diagnostics register request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;	       /**< Header of the message. */
	nrfs_ctx_t ctx;	       /**< Context of the message. */
	diag_reg_access_t reg; /**< Register access. */
} nrfs_diag_reg_req_t;

/** @brief System Diagnostics service notification structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;		   /**< Header of the message. */
	nrfs_ctx_t ctx;		   /**< Context of the message. */
	nrfs_diag_rsp_data_t data; /**< Data of the notification. */
} nrfs_diag_rsp_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_DIAG_H */
