/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IRONSIDE_SE_SOC_NRF9220_H_
#define IRONSIDE_SE_SOC_NRF9220_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ironside_se_soc_nrf9220 nRF9220
 * @ingroup ironside_se_soc_features
 *
 * IronSide SE API that exists only on the nRF9220. Reached through
 * ironside/se/soc_features.h, which includes this file when building for that SoC. Nothing else
 * should include it directly.
 * @{
 */

/** @brief One domain reset event as stored in the event report. */
struct ironside_se_domain_reset {
	/** Non-zero indicates a reset event occurred for the domain. */
	uint32_t reset;
	/** Domain reset reason (RESETHUB.RESETREAS.DOMAIN[domain]). */
	uint32_t resetreas;
};

/**
 * @brief Trailing nRF9220-specific members of @ref ironside_se_event_report.
 *
 * Appends @c cell_reset, the CELL domain reset event.
 */
#define IRONSIDE_SE_EVENT_REPORT_SOC_MEMBERS struct ironside_se_domain_reset cell_reset;

/**
 * @brief Accessors for the members above, pulled in by ironside/se/event_report.h.
 *
 * They need the completed @ref ironside_se_event_report, so they cannot be defined here.
 */
#define IRONSIDE_SE_EVENT_REPORT_SOC_EXT_FILE_PATH "ironside/se/soc/nrf9220_event_report.h"

/** @} */

#ifdef __cplusplus
}
#endif
#endif /* IRONSIDE_SE_SOC_NRF9220_H_ */
