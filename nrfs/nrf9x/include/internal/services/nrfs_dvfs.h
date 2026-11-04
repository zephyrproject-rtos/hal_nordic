/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFS_INTERNAL_DVFS_H
#define NRFS_INTERNAL_DVFS_H

#include <internal/services/nrfs_generic.h>

#ifdef __cplusplus
extern "C" {
#endif

enum __NRFS_PACKED dvfs_frequency_setting {
	DVFS_FREQ_HIGH	  = 0,
	DVFS_FREQ_MEDLOW  = 1,
	DVFS_FREQ_LOW	  = 2,
	DVFS_FREQ_COUNT
};

/** @brief Dynamic Voltage and Frequency Scaling service notification data structure. */
typedef struct __NRFS_PACKED {
	bool scaling_prepare; /**< true if given response is a request for scaling preparation,
				*   false if given response is just a confirmation.
				*/
	enum dvfs_frequency_setting freq;	/**< Maximum allowed HSFLL frequency oppoint. */
} nrfs_dvfs_rsp_data_t;

/** @brief Dynamic Voltage and Frequency Scaling service request data structure. */
typedef struct __NRFS_PACKED {
	enum dvfs_frequency_setting target_freq; /** Requested frequency oppoint. */
} nrfs_dvfs_opp_req_data_t;

/** @brief Dynamic Voltage and Frequency Scaling operating frequency change request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;		       /**< Header of the message. */
	nrfs_ctx_t ctx;		       /**< Context of the message. */
	nrfs_dvfs_opp_req_data_t data; /**< Data of the request. */
} nrfs_dvfs_opp_req_t;

/** @brief Dynamic Voltage and Frequency Scaling general request structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr; /**< Header of the message. */
	nrfs_ctx_t ctx; /**< Context of the message. */
} nrfs_dvfs_req_t;

/** @brief Dynamic Voltage and Frequency Scaling service notification structure. */
typedef struct __NRFS_PACKED {
	nrfs_hdr_t hdr;		   /**< Header of the message. */
	nrfs_ctx_t ctx;		   /**< Context of the message. */
	nrfs_dvfs_rsp_data_t data; /**< Data of the notification. */
} nrfs_dvfs_rsp_t;

#ifdef __cplusplus
}
#endif

#endif /* NRFS_INTERNAL_DVFS_H */
