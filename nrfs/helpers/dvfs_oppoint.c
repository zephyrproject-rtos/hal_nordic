/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dvfs_oppoint.h"
#include <nrfs_config.h>
#include <zephyr/kernel.h>

#define MOVE_AND_MASK_32(x, mask, move) (((uint32_t)(x) << move) & (uint32_t)mask)

#if !defined(NRFS_UNIT_TESTS_ENABLED) && defined(NRF_SECURE)

/* TODO: Use MDK when HM-21530 is fixed */
#define ABB_TRIM_LOCKRANGE_LOCKRANGELOWN_Pos_L                                                     \
	(0UL) /*!< Position of LOCKRANGELOWN field.                                    */
#define ABB_TRIM_LOCKRANGE_LOCKRANGELOWN_Msk_L (0xFFUL << ABB_TRIM_LOCKRANGE_LOCKRANGELOWN_Pos_L)

#define ABB_TRIM_LOCKRANGE_LOCKRANGEHIGHN_Pos_L                                                    \
	(8UL) /*!< Position of LOCKRANGEHIGHN field.                                  */
#define ABB_TRIM_LOCKRANGE_LOCKRANGEHIGHN_Msk_L (0xFFUL << ABB_TRIM_LOCKRANGE_LOCKRANGEHIGHN_Pos_L)

#define ABB_TRIM_LOCKRANGE_LOCKRANGELOWP_Pos_L                                                     \
	(16UL) /*!< Position of LOCKRANGELOWP field.                                     */
#define ABB_TRIM_LOCKRANGE_LOCKRANGELOWP_Msk_L (0xFFUL << ABB_TRIM_LOCKRANGE_LOCKRANGELOWP_Pos_L)

#define ABB_TRIM_LOCKRANGE_LOCKRANGEHIGHP_Pos_L                                                    \
	(24UL) /*!< Position of LOCKRANGEHIGHP field.                                   */
#define ABB_TRIM_LOCKRANGE_LOCKRANGEHIGHP_Msk_L (0xFFUL << ABB_TRIM_LOCKRANGE_LOCKRANGEHIGHP_Pos_L)

#define ABB_LOCKRANGE(low_n, high_n, low_p, high_p)                                                \
	(MOVE_AND_MASK_32((low_n),                                                                 \
			  (ABB_TRIM_LOCKRANGE_LOCKRANGELOWN_Msk_L),                                \
			  (ABB_TRIM_LOCKRANGE_LOCKRANGELOWN_Pos_L)) |                              \
	 MOVE_AND_MASK_32((high_n),                                                                \
			  (ABB_TRIM_LOCKRANGE_LOCKRANGEHIGHN_Msk_L),                               \
			  (ABB_TRIM_LOCKRANGE_LOCKRANGEHIGHN_Pos_L)) |                             \
	 MOVE_AND_MASK_32((low_p),                                                                 \
			  (ABB_TRIM_LOCKRANGE_LOCKRANGELOWP_Msk_L),                                \
			  (ABB_TRIM_LOCKRANGE_LOCKRANGELOWP_Pos_L)) |                              \
	 MOVE_AND_MASK_32((high_p),                                                                \
			  (ABB_TRIM_LOCKRANGE_LOCKRANGEHIGHP_Msk_L),                               \
			  (ABB_TRIM_LOCKRANGE_LOCKRANGEHIGHP_Pos_L)))

#define ABB_RINGO(ringo_target_n, ringo_target_p)                                                  \
	(MOVE_AND_MASK_32((ringo_target_n),                                                        \
			  (ABB_TRIM_RINGO_RINGOTARGETVALN_Msk),                                    \
			  (ABB_TRIM_RINGO_RINGOTARGETVALN_Pos)) |                                  \
	 MOVE_AND_MASK_32((ringo_target_p),                                                        \
			  (ABB_TRIM_RINGO_RINGOTARGETVALP_Msk),                                    \
			  (ABB_TRIM_RINGO_RINGOTARGETVALP_Pos)))
#else
#define ABB_LOCKRANGE(low_n, high_n, low_p, high_p) (0)
#define ABB_RINGO(ringo_target_n, ringo_target_p) (0)
#endif

static const struct dvfs_oppoint_data dvfs_oppoints_data[DVFS_FREQ_COUNT] = {
	/* ABB oppoint 0.8V */
	{
		.freq_setting	  = DVFS_FREQ_HIGH,
		.opp_mv		  = DVFS_HIGH_OPPOINT_MV,
		.abb_ringo	  = ABB_RINGO(524, 519),
		.abb_lockrange	  = ABB_LOCKRANGE(105, 202, 102, 211),
		.abb_pvtmoncycles = 2,
		.new_f_mult	  = 20,
		.new_f_trim_entry = 0,
		.max_hsfll_freq	  = 320,
	},
	/* ABB oppoint 0.6V */
	{
		.freq_setting	  = DVFS_FREQ_MEDLOW,
		.opp_mv		  = DVFS_MEDLOW_OPPOINT_MV,
		.abb_ringo	  = ABB_RINGO(424, 389),
		.abb_lockrange	  = ABB_LOCKRANGE(75, 168, 65, 176),
		.abb_pvtmoncycles = 4,
		.new_f_mult	  = 8,
		.new_f_trim_entry = 2,
		.max_hsfll_freq	  = 128,
	},
	/* ABB oppoint 0.5V */
	{
		.freq_setting	  = DVFS_FREQ_LOW,
		.opp_mv		  = DVFS_LOW_OPPOINT_MV,
		.abb_ringo	  = ABB_RINGO(471, 414),
		.abb_lockrange	  = ABB_LOCKRANGE(73, 206, 60, 203),
		.abb_pvtmoncycles = 9,
		.new_f_mult	  = 4,
		.new_f_trim_entry = 3,
		.max_hsfll_freq	  = 64,
	},
};

const struct dvfs_oppoint_data *get_dvfs_oppoint_data(enum dvfs_frequency_setting oppoint)
{
	if (oppoint >= DVFS_FREQ_COUNT) {
		return &dvfs_oppoints_data[DVFS_FREQ_COUNT - 1];
	}

	return &dvfs_oppoints_data[oppoint];
}

uint16_t get_frequency_for_frequency_setting(enum dvfs_frequency_setting freq_setting)
{
	return dvfs_oppoints_data[freq_setting].max_hsfll_freq;
}
