/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IRONSIDE_SE_PERIPHERAL_INTERFACE_H_
#define IRONSIDE_SE_PERIPHERAL_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup ironside_se_hw
 *  @{
 */

/** @name Bellboard indices (application core)
 * @{
 */

/**
 * Task index in the secure domain BELLBOARD for IPC TX from the application core to the secure
 * domain.
 */
#define IRONSIDE_SE_APPLICATION_BELLBOARD_IPC_TX_BELL_IDX   12
/**
 * Event index in the application core BELLBOARD for IPC RX from the secure domain to the
 * application core.
 */
#define IRONSIDE_SE_APPLICATION_BELLBOARD_IPC_RX_BELL_IDX   0
/**
 * Event index in the application core BELLBOARD for event report RX from the secure domain to the
 * application core.
 */
#define IRONSIDE_SE_APPLICATION_BELLBOARD_EVENT_RX_BELL_IDX 1

/** @} */

/** @name Bellboard indices (radio core)
 * @{
 */

/**
 * Task index in the secure domain BELLBOARD for IPC TX from the radio core to the secure domain.
 */
#define IRONSIDE_SE_RADIOCORE_BELLBOARD_IPC_TX_BELL_IDX   18
/**
 * Event index in the radio core BELLBOARD for IPC RX from the secure domain to the radio core.
 */
#define IRONSIDE_SE_RADIOCORE_BELLBOARD_IPC_RX_BELL_IDX   0
/**
 * Event index in the radio core BELLBOARD for event report RX from the secure domain to the radio
 * core.
 */
#define IRONSIDE_SE_RADIOCORE_BELLBOARD_EVENT_RX_BELL_IDX 1

/** @} */

/**
 * Override index in the radio core MPC used to give other cores access to the radio core local RAM.
 */
#define IRONSIDE_SE_RADIOCORE_MPC_RADIORAM_ACCESS_OVERRIDE_IDX 4

#if defined(NRF_APPLICATION)

/** @name Bellboard indices (current core)
 * @{
 */

#define IRONSIDE_SE_BELLBOARD_IPC_TX_BELL_IDX   IRONSIDE_SE_APPLICATION_BELLBOARD_IPC_TX_BELL_IDX
#define IRONSIDE_SE_BELLBOARD_IPC_RX_BELL_IDX   IRONSIDE_SE_APPLICATION_BELLBOARD_IPC_RX_BELL_IDX
#define IRONSIDE_SE_BELLBOARD_EVENT_RX_BELL_IDX IRONSIDE_SE_APPLICATION_BELLBOARD_EVENT_RX_BELL_IDX

/** @} */

#elif defined(NRF_RADIOCORE)

/** @name Bellboard indices (current core)
 * @{
 */

#define IRONSIDE_SE_BELLBOARD_IPC_TX_BELL_IDX   IRONSIDE_SE_RADIOCORE_BELLBOARD_IPC_TX_BELL_IDX
#define IRONSIDE_SE_BELLBOARD_IPC_RX_BELL_IDX   IRONSIDE_SE_RADIOCORE_BELLBOARD_IPC_RX_BELL_IDX
#define IRONSIDE_SE_BELLBOARD_EVENT_RX_BELL_IDX IRONSIDE_SE_RADIOCORE_BELLBOARD_EVENT_RX_BELL_IDX

/** @} */

#endif

/** @} */

#ifdef __cplusplus
}
#endif
#endif /* IRONSIDE_SE_PERIPHERAL_INTERFACE_H_ */
