/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IRONSIDE_SE_SOC_NRF9220_EVENT_REPORT_H_
#define IRONSIDE_SE_SOC_NRF9220_EVENT_REPORT_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup ironside_se_soc_nrf9220
 *
 * These accessors operate on the members that
 * @c IRONSIDE_SE_EVENT_REPORT_SOC_MEMBERS adds to @ref ironside_se_event_report, so
 * ironside/se/event_report.h includes this file once that structure is complete. Nothing else
 * should include it.
 * @{
 */

/**
 * @brief Function to check if the CELL domain reset event is set.
 *
 * @param report Pointer to the event report structure
 *
 * @retval true if event is set
 * @retval false if event is not set
 */
static inline bool ironside_se_cell_reset_event_check(const struct ironside_se_event_report *report)
{
	return report->cell_reset.reset != 0;
}

/**
 * @brief Function to clear the CELL domain reset event.
 *
 * Once this event has been set, this must be called for future events to be reported.
 *
 * @param report Pointer to the event report structure
 */
static inline void ironside_se_cell_reset_event_clear(struct ironside_se_event_report *report)
{
	report->cell_reset.reset = 0;
	report->cell_reset.resetreas = 0;
}

/**
 * @brief Function to get the CELL domain reset reason.
 *
 * @param report Pointer to the event report structure
 * @param[out] resetreas Pointer to where to store the reset reason.
 */
static inline void ironside_se_cell_resetreas_get(const struct ironside_se_event_report *report,
						  uint32_t *resetreas)
{
	*resetreas = report->cell_reset.resetreas;
}

/** @} */

#ifdef __cplusplus
}
#endif
#endif /* IRONSIDE_SE_SOC_NRF9220_EVENT_REPORT_H_ */
