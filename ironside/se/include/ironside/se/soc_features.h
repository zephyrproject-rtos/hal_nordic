/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IRONSIDE_SE_SOC_FEATURES_H_
#define IRONSIDE_SE_SOC_FEATURES_H_

/**
 * @defgroup ironside_se_soc_features SoC features
 * @ingroup ironside_se
 *
 * Selects the feature header of the SoC being built for and supplies a neutral value for every
 * hook such a header may fill in.
 *
 * A SoC without a feature header gets the neutral values, which add no members to the shared
 * structures. SoC-specific API therefore lives entirely in ironside/se/soc/ and never needs a
 * guard in the shared headers.
 * @{
 */

#if defined(NRF9220_XXAA)
#include <ironside/se/soc/nrf9220.h>
#endif

#ifndef IRONSIDE_SE_EVENT_REPORT_SOC_MEMBERS
/**
 * @brief Trailing SoC-specific members of @ref ironside_se_event_report.
 *
 * Expands to nothing unless the SoC feature header appends members.
 */
#define IRONSIDE_SE_EVENT_REPORT_SOC_MEMBERS
#endif

/** @} */

#endif /* IRONSIDE_SE_SOC_FEATURES_H_ */
