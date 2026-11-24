/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IRONSIDE_SE_MEMORY_MAP_H_
#define IRONSIDE_SE_MEMORY_MAP_H_

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ironside_se_hw Hardware interfaces
 * @ingroup ironside_se
 * @{
 */

/** @name Shared memory: radio core
 * Base addresses and sizes for shared regions used for IPC and reports between
 * the secure domain and the radio core.
 * @{
 */

/** Base address of the IPC message buffer shared with the radio core. */
#define IRONSIDE_SE_RADIOCORE_IPC_BUFFER_ADDRESS (0x2F88F800UL)
/** Size in bytes of the radio core IPC buffer. */
#define IRONSIDE_SE_RADIOCORE_IPC_BUFFER_SIZE    (128)

/** Base address of the event report region for the radio core. */
#define IRONSIDE_SE_RADIOCORE_EVENT_REPORT_ADDRESS (0x2F88F880UL)
/** Size in bytes of the radio core event report region. */
#define IRONSIDE_SE_RADIOCORE_EVENT_REPORT_SIZE    (256)

/** Base address of the boot report region for the radio core. */
#define IRONSIDE_SE_RADIOCORE_BOOT_REPORT_ADDRESS (0x2F88F980UL)
/** Size in bytes of the radio core boot report region. */
#define IRONSIDE_SE_RADIOCORE_BOOT_REPORT_SIZE    (512)

/** @} */

/** @name Shared memory: application core
 * Base addresses and sizes for shared regions used for IPC and reports between
 * the secure domain and the application core.
 * @{
 */

/** Base address of the IPC message buffer shared with the application core. */
#define IRONSIDE_SE_APPLICATION_IPC_BUFFER_ADDRESS (0x2F88FB80UL)
/** Size in bytes of the application core IPC buffer. */
#define IRONSIDE_SE_APPLICATION_IPC_BUFFER_SIZE    (128)

/** Base address of the event report region for the application core. */
#define IRONSIDE_SE_APPLICATION_EVENT_REPORT_ADDRESS (0x2F88FC00UL)
/** Size in bytes of the application core event report region. */
#define IRONSIDE_SE_APPLICATION_EVENT_REPORT_SIZE    (256)

/** Base address of the boot report region for the application core. */
#define IRONSIDE_SE_APPLICATION_BOOT_REPORT_ADDRESS (0x2F88FD00UL)
/** Size in bytes of the application core boot report region. */
#define IRONSIDE_SE_APPLICATION_BOOT_REPORT_SIZE    (512)

/** @} */

/** @name Shared memory: current core
 * Aliases that resolve to the application or radio core region depending on build.
 * @{
 */

#if defined(NRF_APPLICATION)

#define IRONSIDE_SE_IPC_BUFFER_ADDRESS   IRONSIDE_SE_APPLICATION_IPC_BUFFER_ADDRESS
#define IRONSIDE_SE_IPC_BUFFER_SIZE      IRONSIDE_SE_APPLICATION_IPC_BUFFER_SIZE
#define IRONSIDE_SE_BOOT_REPORT_ADDRESS  IRONSIDE_SE_APPLICATION_BOOT_REPORT_ADDRESS
#define IRONSIDE_SE_BOOT_REPORT_SIZE     IRONSIDE_SE_APPLICATION_BOOT_REPORT_SIZE
#define IRONSIDE_SE_EVENT_REPORT_ADDRESS IRONSIDE_SE_APPLICATION_EVENT_REPORT_ADDRESS
#define IRONSIDE_SE_EVENT_REPORT_SIZE    IRONSIDE_SE_APPLICATION_EVENT_REPORT_SIZE

#elif defined(NRF_RADIOCORE)

#define IRONSIDE_SE_IPC_BUFFER_ADDRESS   IRONSIDE_SE_RADIOCORE_IPC_BUFFER_ADDRESS
#define IRONSIDE_SE_IPC_BUFFER_SIZE      IRONSIDE_SE_RADIOCORE_IPC_BUFFER_SIZE
#define IRONSIDE_SE_BOOT_REPORT_ADDRESS  IRONSIDE_SE_RADIOCORE_BOOT_REPORT_ADDRESS
#define IRONSIDE_SE_BOOT_REPORT_SIZE     IRONSIDE_SE_RADIOCORE_BOOT_REPORT_SIZE
#define IRONSIDE_SE_EVENT_REPORT_ADDRESS IRONSIDE_SE_RADIOCORE_EVENT_REPORT_ADDRESS
#define IRONSIDE_SE_EVENT_REPORT_SIZE    IRONSIDE_SE_RADIOCORE_EVENT_REPORT_SIZE

#endif

/** @} */

/** @name IronSide SE update
 * @{
 */

/** Size in bytes of the update blob (manifest, key, signature, and firmware). */
#define IRONSIDE_SE_UPDATE_BLOB_SIZE (160 * 1024)

#if defined(NRF54H20_XXAA)

/** Minimum base address for placing an IronSide SE update (nRF54H20). */
#define IRONSIDE_SE_UPDATE_MIN_ADDRESS (0x0E100000)
/** Maximum end address for an IronSide SE update region (nRF54H20). */
#define IRONSIDE_SE_UPDATE_MAX_ADDRESS (0x0E200000 - IRONSIDE_SE_UPDATE_BLOB_SIZE)

#elif defined(NRF9230_ENGB_XXAA)

/** Minimum base address for placing an IronSide SE update (nRF9230). */
#define IRONSIDE_SE_UPDATE_MIN_ADDRESS (0x0E600000)
/** Maximum end address for an IronSide SE update region (nRF9230). */
#define IRONSIDE_SE_UPDATE_MAX_ADDRESS (0x0E72F000 - IRONSIDE_SE_UPDATE_BLOB_SIZE)

#elif defined(NRF9220_XXAA)

#define IRONSIDE_SE_UPDATE_MIN_ADDRESS (0x0E400000)
#define IRONSIDE_SE_UPDATE_BLOB_SIZE   (160 * 1024)
#define IRONSIDE_SE_UPDATE_MAX_ADDRESS (0x0E52F000 - IRONSIDE_SE_UPDATE_BLOB_SIZE)

#endif

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif
#endif /* IRONSIDE_SE_MEMORY_MAP_H_ */
