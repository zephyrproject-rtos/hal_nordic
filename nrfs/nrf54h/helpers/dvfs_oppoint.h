/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef DVFS_OPPOINT_H
#define DVFS_OPPOINT_H

#include <nrfs_common.h>
#include <internal/services/nrfs_dvfs.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dvfs_oppoint_data {
	enum dvfs_frequency_setting freq_setting;
	uint16_t opp_mv;
	uint32_t abb_ringo;
	uint32_t abb_lockrange;
	uint32_t abb_pvtmoncycles;
	uint32_t new_f_mult;
	uint32_t new_f_trim_entry;
	uint16_t max_hsfll_freq;
};

#define DVFS_HIGH_OPPOINT_MV	((uint16_t)800)
#define DVFS_MEDLOW_OPPOINT_MV	((uint16_t)600)
#define DVFS_LOW_OPPOINT_MV	((uint16_t)500)

/**
 * @brief Get the DVFS operating point (oppoint) data object.
 *
 * @param oppoint desired operating point.
 * @return struct dvfs_oppoint_data* pointer to the oppoint data structure.
 * @return NULL pointer when oppoint is invalid.
 */
const struct dvfs_oppoint_data *get_dvfs_oppoint_data(enum dvfs_frequency_setting oppoint);

/**
 * @brief Get the frequency for frequency setting object.
 *
 * @param freq_setting item from enum dvfs_frequency_setting.
 * @return uint16_t frequency in MHz.
 */
uint16_t get_frequency_for_frequency_setting(enum dvfs_frequency_setting freq_setting);

#ifdef __cplusplus
}
#endif

#endif /* DVFS_OPPOINT_H */
