/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IRONSIDE_SE_API_H_
#define IRONSIDE_SE_API_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <nrfx.h>

#include <ironside/se/memory_map.h>

#include <ironside/se/boot_report.h>
#include <ironside/se/periphconf.h>
#include <ironside/se/mpcconf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ironside_se_ipc IPC interface
 * @ingroup ironside_se
 * @{
 */

/**
 * @defgroup ironside_se_ipc_update Update
 * @ingroup ironside_se_ipc
 * @{
 */

/**
 * @defgroup ironside_se_update_errors Error codes
 * @ingroup ironside_se_ipc_update
 * @{
 */

/** Caller does not have access to the provided update candidate buffer. */
#define IRONSIDE_SE_UPDATE_ERROR_NOT_PERMITTED     (1)
/** Failed to write the update metadata to SICR. */
#define IRONSIDE_SE_UPDATE_ERROR_SICR_WRITE_FAILED (2)
/** Update is placed outside of valid range */
#define IRONSIDE_SE_UPDATE_ERROR_INVALID_ADDRESS   (3)

/**
 * @}
 */

/** Length of the update manifest in bytes */
#define IRONSIDE_SE_UPDATE_MANIFEST_LENGTH  (256)
/** Length of the update public key in bytes. */
#define IRONSIDE_SE_UPDATE_PUBKEY_LENGTH    (32)
/** Length of the update signature in bytes. */
#define IRONSIDE_SE_UPDATE_SIGNATURE_LENGTH (64)

/** @brief IronSide SE update blob (maximum size @ref IRONSIDE_SE_UPDATE_BLOB_SIZE). */
struct ironside_se_update_blob {
	/** Update manifest (@ref IRONSIDE_SE_UPDATE_MANIFEST_LENGTH bytes). */
	uint8_t manifest[IRONSIDE_SE_UPDATE_MANIFEST_LENGTH];
	/** Update signing public key (@ref IRONSIDE_SE_UPDATE_PUBKEY_LENGTH bytes). */
	uint8_t pubkey[IRONSIDE_SE_UPDATE_PUBKEY_LENGTH];
	/** Signature over the update (@ref IRONSIDE_SE_UPDATE_SIGNATURE_LENGTH bytes). */
	uint8_t signature[IRONSIDE_SE_UPDATE_SIGNATURE_LENGTH];
	/** Firmware image payload (variable size). */
	uint32_t firmware[];
};

/**
 * @brief Request a firmware upgrade of the IronSide SE.
 *
 * This invokes the IronSide SE update service. The device must be restarted for the update
 * to be installed. Check the update status in the application boot report to see if the update
 * was successfully installed.
 *
 * Negative service codes are listed in @ref ironside_se_update_errors.
 *
 * @param update Pointer to update blob
 *
 * @retval 0 on a successful request (although the update itself may still fail).
 * @retval -IRONSIDE_SE_UPDATE_ERROR_NOT_PERMITTED if missing access to the update candidate.
 * @retval -IRONSIDE_SE_UPDATE_ERROR_SICR_WRITE_FAILED if writing update parameters to SICR failed.
 * @returns Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
 */
int ironside_se_update(const struct ironside_se_update_blob *update);

/** @} */

/**
 * @defgroup ironside_se_ipc_cpuconf CPUCONF
 * @ingroup ironside_se_ipc
 * @{
 */

/**
 * @defgroup ironside_se_cpuconf_errors Error codes
 * @ingroup ironside_se_ipc_cpuconf
 * @{
 */

/** An invalid or unsupported processor ID was specified. */
#define IRONSIDE_SE_CPUCONF_ERROR_WRONG_CPU         (1)
/** The boot message is too large to fit in the buffer. */
#define IRONSIDE_SE_CPUCONF_ERROR_MESSAGE_TOO_LARGE (2)
/** CPU boot blocked due to corrupted memory. */
#define IRONSIDE_SE_CPUCONF_ERROR_CORRUPTED_MEMORY  (3)

/**
 * @}
 */

/** Maximum size of the CPUCONF message parameter. */
#define IRONSIDE_SE_CPUCONF_REQ_MSG_MAX_SIZE (4 * sizeof(uint32_t))

/**
 * @brief Boot a local domain CPU
 *
 * @param cpu The CPU to be booted
 * @param vector_table Pointer to the vector table used to boot the CPU.
 * @param cpu_wait When this is true, the CPU will WAIT even if the CPU has clock.
 * @param msg A message that can be placed in cpu's boot report.
 * @param msg_size Size of the message in bytes.
 *
 * @note cpu_wait is only intended to be enabled for debug purposes
 * and it is only supported that a debugger resumes the CPU.
 *
 * @note the call always sends IRONSIDE_SE_CPUCONF_REQ_MSG_MAX_SIZE message bytes.
 * If the given msg_size is less than that, the remaining bytes are set to zero.
 *
 * Negative service codes are listed in @ref ironside_se_cpuconf_errors.
 *
 * @retval 0 on success or if the CPU has already booted.
 * @retval -IRONSIDE_SE_CPUCONF_ERROR_WRONG_CPU if cpu is unrecognized.
 * @retval -IRONSIDE_SE_CPUCONF_ERROR_MESSAGE_TOO_LARGE if msg_size is greater than
 * IRONSIDE_SE_CPUCONF_REQ_MSG_MAX_SIZE.
 * @retval -IRONSIDE_SE_CPUCONF_ERROR_CORRUPTED_MEMORY if the target CPU boot memory
 * region has been flagged as corrupted. Applies only to 92 Series devices.
 * @returns Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
 */
int ironside_se_cpuconf(NRF_PROCESSORID_Type cpu, const void *vector_table, bool cpu_wait,
			const uint8_t *msg, size_t msg_size);

/** @} */

/**
 * @defgroup ironside_se_ipc_tdd TDD
 * @ingroup ironside_se_ipc
 * @{
 */

/**
 * @defgroup ironside_se_tdd_errors Error codes
 * @ingroup ironside_se_ipc_tdd
 * @{
 */

/** Invalid configuration enum value. */
#define IRONSIDE_SE_TDD_ERROR_INVALID_CONFIG (1)

/**
 * @}
 */

/** Trace and Debug Domain (TDD) configuration selector. */
enum ironside_se_tdd_config {
	/** Turn off the TDD. */
	IRONSIDE_SE_TDD_CONFIG_OFF = 1,
	/** Turn on the TDD with default configuration. */
	IRONSIDE_SE_TDD_CONFIG_ON_DEFAULT = 2,
};

/**
 * @brief Control the Trace and Debug Domain (TDD).
 *
 * Negative service codes are listed in @ref ironside_se_tdd_errors.
 *
 * @param config The configuration to be applied.
 *
 * @retval 0 on success.
 * @retval -IRONSIDE_SE_TDD_ERROR_INVALID_CONFIG if the configuration is invalid.
 * @returns Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
 */
int ironside_se_tdd_configure(const enum ironside_se_tdd_config config);

/** @} */

/**
 * @defgroup ironside_se_ipc_dvfs DVFS
 * @ingroup ironside_se_ipc
 * @{
 */

/** Supported DVFS operational points. */
enum ironside_se_dvfs_oppoint {
	IRONSIDE_SE_DVFS_OPP_HIGH = 0,
	IRONSIDE_SE_DVFS_OPP_MEDLOW = 1,
	IRONSIDE_SE_DVFS_OPP_LOW = 2
};

/**
 * @brief Number of DVFS oppoints supported by IronSide.
 *
 * This is the number of different DVFS oppoints that can be set on IronSide.
 * The oppoints are defined in the @ref ironside_se_dvfs_oppoint enum.
 */
#define IRONSIDE_SE_DVFS_OPPOINT_COUNT (3)

/**
 * @defgroup ironside_se_dvfs_errors Error codes
 * @ingroup ironside_se_ipc_dvfs
 * @{
 */

/** The requested DVFS oppoint is not allowed. */
#define IRONSIDE_SE_DVFS_ERROR_WRONG_OPPOINT    (1)
/** Failed to change the DVFS oppoint due to other ongoing operations. */
#define IRONSIDE_SE_DVFS_ERROR_BUSY             (2)
/** Currently unused. */
#define IRONSIDE_SE_DVFS_ERROR_OPPOINT_DATA     (3)
/** The caller does not have permission to change the DVFS oppoint. */
#define IRONSIDE_SE_DVFS_ERROR_PERMISSION       (4)
/** The requested DVFS oppoint is already set, no change needed. */
#define IRONSIDE_SE_DVFS_ERROR_NO_CHANGE_NEEDED (5)
/** The operation timed out, possibly due to a hardware issue. */
#define IRONSIDE_SE_DVFS_ERROR_TIMEOUT          (6)

/**
 * @}
 */

/**
 * @brief Change the current DVFS oppoint.
 *
 * Requests a change of the current DVFS oppoint to the specified value.
 * It will block until the change is applied.
 *
 * Negative service codes are listed in @ref ironside_se_dvfs_errors.
 *
 * @param dvfs_oppoint The new DVFS oppoint to set.
 *
 * @retval 0 on success.
 * @retval -IRONSIDE_SE_DVFS_ERROR_WRONG_OPPOINT if the requested DVFS oppoint is not allowed.
 * @retval -IRONSIDE_SE_DVFS_ERROR_BUSY if hardware is busy.
 * @retval -IRONSIDE_SE_DVFS_ERROR_PERMISSION if the caller does not have permission to change the
 * DVFS oppoint.
 * @retval -IRONSIDE_SE_DVFS_ERROR_NO_CHANGE_NEEDED if the requested DVFS oppoint is already set.
 * @retval -IRONSIDE_SE_DVFS_ERROR_TIMEOUT if the operation timed out, possibly due to a hardware
 * issue.
 * @returns Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
 */
int ironside_se_dvfs_req_oppoint(enum ironside_se_dvfs_oppoint dvfs_oppoint);

/**
 * @brief Check if the given oppoint is valid.
 *
 * @param dvfs_oppoint The oppoint to check.
 * @return true if the oppoint is valid, false otherwise.
 */
static inline bool ironside_se_dvfs_is_oppoint_valid(enum ironside_se_dvfs_oppoint dvfs_oppoint)
{
	if (dvfs_oppoint != IRONSIDE_SE_DVFS_OPP_HIGH &&
	    dvfs_oppoint != IRONSIDE_SE_DVFS_OPP_MEDLOW &&
	    dvfs_oppoint != IRONSIDE_SE_DVFS_OPP_LOW) {
		return false;
	}

	return true;
}

/** @} */

/**
 * @defgroup ironside_se_ipc_bootmode Boot mode
 * @ingroup ironside_se_ipc
 * @{
 */

/**
 * @defgroup ironside_se_bootmode_errors Error codes
 * @ingroup ironside_se_ipc_bootmode
 * @{
 */

/** Invalid/unsupported boot mode transition. */
#define IRONSIDE_SE_BOOTMODE_ERROR_UNSUPPORTED_MODE  (1)
/** Failed to reboot into the boot mode due to other activity preventing a reset. */
#define IRONSIDE_SE_BOOTMODE_ERROR_BUSY              (2)
/** The boot message is too large to fit in the buffer. */
#define IRONSIDE_SE_BOOTMODE_ERROR_MESSAGE_TOO_LARGE (3)

/**
 * @}
 */

/** Maximum size of the message parameter. */
#define IRONSIDE_SE_BOOTMODE_REQ_MSG_MAX_SIZE (4 * sizeof(uint32_t))

/**
 * @brief Request a reboot into the secondary firmware boot mode.
 *
 * This invokes the IronSide boot mode service to restart the system into the secondary boot
 * mode. In this mode, the secondary configuration defined in UICR is applied instead of the
 * primary one. The system immediately reboots without a reply if the request succeeds.
 *
 * The given message data is passed to the boot report of the CPU booted in the secondary boot mode.
 *
 * @note This function does not return if the request is successful.
 * @note The device will boot into the secondary firmware instead of primary firmware.
 * @note The request does not fail if the secondary firmware is not defined.
 *
 * Negative service codes are listed in @ref ironside_se_bootmode_errors.
 *
 * @param msg A message that can be placed in the cpu's boot report.
 * @param msg_size Size of the message in bytes.
 *
 * @retval 0 on success.
 * @retval -IRONSIDE_SE_BOOTMODE_ERROR_UNSUPPORTED_MODE if the secondary boot mode is unsupported.
 * @retval -IRONSIDE_SE_BOOTMODE_ERROR_BUSY if the reboot was blocked.
 * @retval -IRONSIDE_SE_BOOTMODE_ERROR_MESSAGE_TOO_LARGE if msg_size is greater than
 * @ref IRONSIDE_SE_BOOTMODE_REQ_MSG_MAX_SIZE.
 * @returns Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
 */
int ironside_se_bootmode_secondary_reboot(const uint8_t *msg, size_t msg_size);

/** @} */

/**
 * @defgroup ironside_se_ipc_counter Counter
 * @ingroup ironside_se_ipc
 * @{
 */

/**
 * @defgroup ironside_se_counter_errors Error codes
 * @ingroup ironside_se_ipc_counter
 * @{
 */

/** Counter value is lower than current value (monotonic violation). */
#define IRONSIDE_SE_COUNTER_ERROR_TOO_LOW         (1)
/** Invalid counter ID. */
#define IRONSIDE_SE_COUNTER_ERROR_INVALID_ID      (2)
/** Counter is locked and cannot be modified. */
#define IRONSIDE_SE_COUNTER_ERROR_LOCKED          (3)
/** Invalid parameter. */
#define IRONSIDE_SE_COUNTER_ERROR_INVALID_PARAM   (4)
/** Storage operation failed. */
#define IRONSIDE_SE_COUNTER_ERROR_STORAGE_FAILURE (5)

/**
 * @}
 */

/** Maximum value for a counter */
#define IRONSIDE_SE_COUNTER_MAX_VALUE UINT32_MAX

/** Number of counters */
#define IRONSIDE_SE_COUNTER_NUM 4

/**
 * @brief Counter identifiers.
 */
enum ironside_se_counter {
	IRONSIDE_SE_COUNTER_0 = 0,
	IRONSIDE_SE_COUNTER_1,
	IRONSIDE_SE_COUNTER_2,
	IRONSIDE_SE_COUNTER_3
};

/**
 * @brief Set a counter value.
 *
 * This sets the specified counter to the given value. The counter is monotonic,
 * so the new value must be greater than or equal to the current value.
 * If the counter is locked, the operation will fail.
 *
 * @note Counters are automatically initialized to 0 during the first boot in LCS ROT.
 *       The monotonic constraint applies to all subsequent writes.
 *
 * Negative service codes are listed in @ref ironside_se_counter_errors.
 *
 * @param counter_id Counter identifier.
 * @param value New counter value.
 *
 * @retval 0 on success.
 * @retval -IRONSIDE_SE_COUNTER_ERROR_INVALID_ID if counter_id is invalid.
 * @retval -IRONSIDE_SE_COUNTER_ERROR_TOO_LOW if value is lower than current value.
 * @retval -IRONSIDE_SE_COUNTER_ERROR_LOCKED if counter is locked.
 * @retval -IRONSIDE_SE_COUNTER_ERROR_STORAGE_FAILURE if storage operation failed.
 * @returns Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
 */
int ironside_se_counter_set(enum ironside_se_counter counter_id, uint32_t value);

/**
 * @brief Get a counter value.
 *
 * This retrieves the current value of the specified counter.
 *
 * @note Counters are automatically initialized to 0 during the first boot in LCS ROT,
 *       so this function will always succeed for valid counter IDs.
 *
 * Negative service codes are listed in @ref ironside_se_counter_errors.
 *
 * @param counter_id Counter identifier.
 * @param value Pointer to store the counter value.
 *
 * @retval 0 on success.
 * @retval -IRONSIDE_SE_COUNTER_ERROR_INVALID_ID if counter_id is invalid.
 * @retval -IRONSIDE_SE_COUNTER_ERROR_INVALID_PARAM if value is NULL.
 * @retval -IRONSIDE_SE_COUNTER_ERROR_STORAGE_FAILURE if storage operation failed.
 * @returns Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
 */
int ironside_se_counter_get(enum ironside_se_counter counter_id, uint32_t *value);

/**
 * @brief Lock a counter for the current boot.
 *
 * This locks the specified counter, preventing any further modifications until the next reboot.
 * The lock state is not persistent and will be cleared on reboot.
 *
 * @note The intended use case is for a bootloader to lock a counter before transferring control
 *       to the next boot stage, preventing that image from modifying the counter value.
 *
 * Negative service codes are listed in @ref ironside_se_counter_errors.
 *
 * @param counter_id Counter identifier.
 *
 * @retval 0 on success.
 * @retval -IRONSIDE_SE_COUNTER_ERROR_INVALID_ID if counter_id is invalid.
 * @returns Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
 */
int ironside_se_counter_lock(enum ironside_se_counter counter_id);

/** @} */

/**
 * @defgroup ironside_se_ipc_events Hardware events
 * @ingroup ironside_se_ipc
 * @{
 */

/**
 * @defgroup ironside_se_event_enable_errors Error codes
 * @ingroup ironside_se_ipc_events
 * @{
 */

/** Invalid event set. */
#define IRONSIDE_SE_EVENT_ENABLE_ERROR_INVALID_EVENT (1)

/**
 * @}
 */

/**
 * @defgroup ironside_se_event_mask_pos Event mask bit positions for per-instance control
 * @ingroup ironside_se_ipc_events
 *
 * The event mask uses 64 bits to allow per-instance enable/disable:
 * - Bits 0-12:  SPU instances (SPU110-SPU137)
 * - Bits 13-16: MPC instances (MPC110, MPC111, MPC120, MPC130)
 * - Bits 17-18: MRAMC ECCERROR (MRAMC110, MRAMC111)
 * - Bits 19-20: MRAMC ECCERRORCORR (MRAMC110, MRAMC111)
 * - Bits 21-22: MRAMC ACCESSERR (MRAMC110, MRAMC111)
 * @{
 */

/** Bit position for SPU instance 110 in the event mask. */
#define IRONSIDE_SE_EVENT_SPU110_POS (0UL)
/** Bit position for SPU instance 111 in the event mask. */
#define IRONSIDE_SE_EVENT_SPU111_POS (1UL)
/** Bit position for SPU instance 120 in the event mask. */
#define IRONSIDE_SE_EVENT_SPU120_POS (2UL)
/** Bit position for SPU instance 121 in the event mask. */
#define IRONSIDE_SE_EVENT_SPU121_POS (3UL)
/** Bit position for SPU instance 122 in the event mask. */
#define IRONSIDE_SE_EVENT_SPU122_POS (4UL)
/** Bit position for SPU instance 130 in the event mask. */
#define IRONSIDE_SE_EVENT_SPU130_POS (5UL)
/** Bit position for SPU instance 131 in the event mask. */
#define IRONSIDE_SE_EVENT_SPU131_POS (6UL)
/** Bit position for SPU instance 132 in the event mask. */
#define IRONSIDE_SE_EVENT_SPU132_POS (7UL)
/** Bit position for SPU instance 133 in the event mask. */
#define IRONSIDE_SE_EVENT_SPU133_POS (8UL)
/** Bit position for SPU instance 134 in the event mask. */
#define IRONSIDE_SE_EVENT_SPU134_POS (9UL)
/** Bit position for SPU instance 135 in the event mask. */
#define IRONSIDE_SE_EVENT_SPU135_POS (10UL)
/** Bit position for SPU instance 136 in the event mask. */
#define IRONSIDE_SE_EVENT_SPU136_POS (11UL)
/** Bit position for SPU instance 137 in the event mask. */
#define IRONSIDE_SE_EVENT_SPU137_POS (12UL)

/** Bit position for MPC instance 110 in the event mask. */
#define IRONSIDE_SE_EVENT_MPC110_POS (13UL)
/** Bit position for MPC instance 111 in the event mask. */
#define IRONSIDE_SE_EVENT_MPC111_POS (14UL)
/** Bit position for MPC instance 120 in the event mask. */
#define IRONSIDE_SE_EVENT_MPC120_POS (15UL)
/** Bit position for MPC instance 130 in the event mask. */
#define IRONSIDE_SE_EVENT_MPC130_POS (16UL)

/** Bit position for MRAMC instance 110 ECCERROR in the event mask. */
#define IRONSIDE_SE_EVENT_MRAMC110_ECCERROR_POS (17UL)
/** Bit position for MRAMC instance 111 ECCERROR in the event mask. */
#define IRONSIDE_SE_EVENT_MRAMC111_ECCERROR_POS (18UL)

/** Bit position for MRAMC instance 110 ECCERRORCORR in the event mask. */
#define IRONSIDE_SE_EVENT_MRAMC110_ECCERRORCORR_POS (19UL)
/** Bit position for MRAMC instance 111 ECCERRORCORR in the event mask. */
#define IRONSIDE_SE_EVENT_MRAMC111_ECCERRORCORR_POS (20UL)

/** Bit position for MRAMC instance 110 ACCESSERR in the event mask. */
#define IRONSIDE_SE_EVENT_MRAMC110_ACCESSERR_POS (21UL)
/** Bit position for MRAMC instance 111 ACCESSERR in the event mask. */
#define IRONSIDE_SE_EVENT_MRAMC111_ACCESSERR_POS (22UL)

/** @} */

/**
 * @defgroup ironside_se_event_mask_values Event mask values for per-instance control
 * @ingroup ironside_se_ipc_events
 * @{
 */

/** Bit mask for SPU instance 110 event. */
#define IRONSIDE_SE_EVENT_SPU110_MASK (1ULL << IRONSIDE_SE_EVENT_SPU110_POS)
/** Bit mask for SPU instance 111 event. */
#define IRONSIDE_SE_EVENT_SPU111_MASK (1ULL << IRONSIDE_SE_EVENT_SPU111_POS)
/** Bit mask for SPU instance 120 event. */
#define IRONSIDE_SE_EVENT_SPU120_MASK (1ULL << IRONSIDE_SE_EVENT_SPU120_POS)
/** Bit mask for SPU instance 121 event. */
#define IRONSIDE_SE_EVENT_SPU121_MASK (1ULL << IRONSIDE_SE_EVENT_SPU121_POS)
/** Bit mask for SPU instance 122 event. */
#define IRONSIDE_SE_EVENT_SPU122_MASK (1ULL << IRONSIDE_SE_EVENT_SPU122_POS)
/** Bit mask for SPU instance 130 event. */
#define IRONSIDE_SE_EVENT_SPU130_MASK (1ULL << IRONSIDE_SE_EVENT_SPU130_POS)
/** Bit mask for SPU instance 131 event. */
#define IRONSIDE_SE_EVENT_SPU131_MASK (1ULL << IRONSIDE_SE_EVENT_SPU131_POS)
/** Bit mask for SPU instance 132 event. */
#define IRONSIDE_SE_EVENT_SPU132_MASK (1ULL << IRONSIDE_SE_EVENT_SPU132_POS)
/** Bit mask for SPU instance 133 event. */
#define IRONSIDE_SE_EVENT_SPU133_MASK (1ULL << IRONSIDE_SE_EVENT_SPU133_POS)
/** Bit mask for SPU instance 134 event. */
#define IRONSIDE_SE_EVENT_SPU134_MASK (1ULL << IRONSIDE_SE_EVENT_SPU134_POS)
/** Bit mask for SPU instance 135 event. */
#define IRONSIDE_SE_EVENT_SPU135_MASK (1ULL << IRONSIDE_SE_EVENT_SPU135_POS)
/** Bit mask for SPU instance 136 event. */
#define IRONSIDE_SE_EVENT_SPU136_MASK (1ULL << IRONSIDE_SE_EVENT_SPU136_POS)
/** Bit mask for SPU instance 137 event. */
#define IRONSIDE_SE_EVENT_SPU137_MASK (1ULL << IRONSIDE_SE_EVENT_SPU137_POS)

/** Bit mask for MPC instance 110 event. */
#define IRONSIDE_SE_EVENT_MPC110_MASK (1ULL << IRONSIDE_SE_EVENT_MPC110_POS)
/** Bit mask for MPC instance 111 event. */
#define IRONSIDE_SE_EVENT_MPC111_MASK (1ULL << IRONSIDE_SE_EVENT_MPC111_POS)
/** Bit mask for MPC instance 120 event. */
#define IRONSIDE_SE_EVENT_MPC120_MASK (1ULL << IRONSIDE_SE_EVENT_MPC120_POS)
/** Bit mask for MPC instance 130 event. */
#define IRONSIDE_SE_EVENT_MPC130_MASK (1ULL << IRONSIDE_SE_EVENT_MPC130_POS)

/** Bit mask for MRAMC instance 110 ECCERROR event. */
#define IRONSIDE_SE_EVENT_MRAMC110_ECCERROR_MASK (1ULL << IRONSIDE_SE_EVENT_MRAMC110_ECCERROR_POS)
/** Bit mask for MRAMC instance 111 ECCERROR event. */
#define IRONSIDE_SE_EVENT_MRAMC111_ECCERROR_MASK (1ULL << IRONSIDE_SE_EVENT_MRAMC111_ECCERROR_POS)

/** Bit mask for MRAMC instance 110 ECCERRORCORR event. */
#define IRONSIDE_SE_EVENT_MRAMC110_ECCERRORCORR_MASK                                               \
	(1ULL << IRONSIDE_SE_EVENT_MRAMC110_ECCERRORCORR_POS)
/** Bit mask for MRAMC instance 111 ECCERRORCORR event. */
#define IRONSIDE_SE_EVENT_MRAMC111_ECCERRORCORR_MASK                                               \
	(1ULL << IRONSIDE_SE_EVENT_MRAMC111_ECCERRORCORR_POS)

/** Bit mask for MRAMC instance 110 ACCESSERR event. */
#define IRONSIDE_SE_EVENT_MRAMC110_ACCESSERR_MASK (1ULL << IRONSIDE_SE_EVENT_MRAMC110_ACCESSERR_POS)
/** Bit mask for MRAMC instance 111 ACCESSERR event. */
#define IRONSIDE_SE_EVENT_MRAMC111_ACCESSERR_MASK (1ULL << IRONSIDE_SE_EVENT_MRAMC111_ACCESSERR_POS)

/** @} */

/**
 * @defgroup ironside_se_event_masks Convenience masks for event reporting
 * @ingroup ironside_se_ipc_events
 * @{
 */

/** Bit mask for all SPU instance events. */
#define IRONSIDE_SE_EVENT_SPU_ALL_MASK                                                             \
	(IRONSIDE_SE_EVENT_SPU110_MASK | IRONSIDE_SE_EVENT_SPU111_MASK |                           \
	 IRONSIDE_SE_EVENT_SPU120_MASK | IRONSIDE_SE_EVENT_SPU121_MASK |                           \
	 IRONSIDE_SE_EVENT_SPU122_MASK | IRONSIDE_SE_EVENT_SPU130_MASK |                           \
	 IRONSIDE_SE_EVENT_SPU131_MASK | IRONSIDE_SE_EVENT_SPU132_MASK |                           \
	 IRONSIDE_SE_EVENT_SPU133_MASK | IRONSIDE_SE_EVENT_SPU134_MASK |                           \
	 IRONSIDE_SE_EVENT_SPU135_MASK | IRONSIDE_SE_EVENT_SPU136_MASK |                           \
	 IRONSIDE_SE_EVENT_SPU137_MASK)

/** Bit mask for all MPC instance events. */
#define IRONSIDE_SE_EVENT_MPC_ALL_MASK                                                             \
	(IRONSIDE_SE_EVENT_MPC110_MASK | IRONSIDE_SE_EVENT_MPC111_MASK |                           \
	 IRONSIDE_SE_EVENT_MPC120_MASK | IRONSIDE_SE_EVENT_MPC130_MASK)

/** Bit mask for all MRAMC ECCERROR instance events. */
#define IRONSIDE_SE_EVENT_MRAMC_ECCERROR_ALL_MASK                                                  \
	(IRONSIDE_SE_EVENT_MRAMC110_ECCERROR_MASK | IRONSIDE_SE_EVENT_MRAMC111_ECCERROR_MASK)

/** Bit mask for all MRAMC ECCERRORCORR instance events. */
#define IRONSIDE_SE_EVENT_MRAMC_ECCERRORCORR_ALL_MASK                                              \
	(IRONSIDE_SE_EVENT_MRAMC110_ECCERRORCORR_MASK |                                            \
	 IRONSIDE_SE_EVENT_MRAMC111_ECCERRORCORR_MASK)

/** Bit mask for all MRAMC ACCESSERR instance events. */
#define IRONSIDE_SE_EVENT_MRAMC_ACCESSERR_ALL_MASK                                                 \
	(IRONSIDE_SE_EVENT_MRAMC110_ACCESSERR_MASK | IRONSIDE_SE_EVENT_MRAMC111_ACCESSERR_MASK)

/** Bit mask for all event report events. */
#define IRONSIDE_SE_EVENT_ALL_MASK                                                                 \
	(IRONSIDE_SE_EVENT_SPU_ALL_MASK | IRONSIDE_SE_EVENT_MPC_ALL_MASK |                         \
	 IRONSIDE_SE_EVENT_MRAMC_ECCERROR_ALL_MASK |                                               \
	 IRONSIDE_SE_EVENT_MRAMC_ECCERRORCORR_ALL_MASK |                                           \
	 IRONSIDE_SE_EVENT_MRAMC_ACCESSERR_ALL_MASK)

/** @} */

/**
 * @brief Enable hardware events.
 *
 * This will enable the event and corresponding interrupt in hardware.
 * IronSide SE will communicate the events to the local domains through the event report mechanism.
 * The event data is located in the event report region in RAM.
 * The BELLBOARD associated with event report is signalled whenever a new event occurs.
 * Events will not re-trigger when they are set. Hence, a local domain must clear an event from the
 * event report for it to occur again.
 *
 * @note Even though an event is not cleared from the local domain, the IRQ for enabled events
 *       will still occur on the Secure Domain CPU (which executes IronSide SE).
 *       As a result of this, the only way to avoid waking the Secure Domain is to have the event
 *       disabled. To avoid power consumption from repeatedly waking the Secure Domain, keep events
 *       disabled when not required.
 *
 * Negative service codes are listed in @ref ironside_se_event_enable_errors.
 *
 * @param event_mask Mask specifying which events to enable the IRQ and event reporting for.
 *                   Unsupported fields being set result in an error.
 *                   Use @ref ironside_se_event_mask_values and @ref ironside_se_event_masks.
 *
 * @retval 0 on success.
 * @retval -IRONSIDE_SE_EVENT_ENABLE_ERROR_INVALID_EVENT if invalid event is indicated.
 * @returns Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
 */
int ironside_se_events_enable(uint64_t event_mask);

/**
 * @brief Disable hardware events.
 *
 * See @ref ironside_se_events_enable
 *
 * Negative service codes are listed in @ref ironside_se_event_enable_errors.
 *
 * @param event_mask Mask specifying which events to disable the IRQ and event reporting for.
 *                   Unsupported fields being set result in an error.
 *                   Use @ref ironside_se_event_mask_values and @ref ironside_se_event_masks.
 *
 * @retval 0 on success.
 * @retval -IRONSIDE_SE_EVENT_ENABLE_ERROR_INVALID_EVENT if invalid event is indicated.
 * @returns Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
 */
int ironside_se_events_disable(uint64_t event_mask);

/** @} */

/**
 * @defgroup ironside_se_ipc_snapshot Snapshot
 * @ingroup ironside_se_ipc
 * @{
 */

/**
 * @defgroup ironside_se_snapshot_ipc_errors Error codes
 * @ingroup ironside_se_ipc_snapshot
 * @{
 */

/** Invalid capture mode. */
#define IRONSIDE_SE_SNAPSHOT_ERROR_INVALID_MODE      (1)
/** Capture/recovery was expected to reset and not return. */
#define IRONSIDE_SE_SNAPSHOT_ERROR_UNEXPECTED_RETURN (2)
/** Snapshot feature has not been enabled in OTP. */
#define IRONSIDE_SE_SNAPSHOT_ERROR_NOT_ENABLED       (3)

/**
 * @}
 */

/**
 * @brief Snapshot capture modes.
 */
enum ironside_se_snapshot_capture_mode {
	/* Capture operation does not increment the monotonic capture counter. */
	IRONSIDE_SE_SNAPSHOT_CAPTURE_NO_INCREMENT = 0,
	/* Capture operation that increments the monotonic capture counter. */
	IRONSIDE_SE_SNAPSHOT_CAPTURE_INCREMENT_COUNTER,
};

/**
 * @brief Perform a snapshot capture.
 *
 * A successful capture request results in a reset and does not return. The capture operation
 * itself is performed by the secure domain ROM as part of the system boot following the reset.
 * The result of the capture operation is found in the boot report.
 *
 * Snapshots can be captured with incrementing the capture counter in order to prevent downgrading
 * to an earlier snapshot capture. This counter is monotonic and only has a finite amount of
 * possible captures.
 *
 * Capturing without incrementing the counter has no limit on the amount of possible captures.
 *
 * Negative service codes are listed in @ref ironside_se_snapshot_ipc_errors.
 *
 * @note The intended use case for capturing with incrementing the capture counter is for enforcing
 *       downgrade prevention after production while the device is the in the field.
 *
 * @warning Incrementing the capture counter is a permanent operation that cannot be undone in a
 *          device's lifetime.
 *
 * @note On success, function does not return.
 *
 * @retval -IRONSIDE_SE_SNAPSHOT_ERROR_UNEXPECTED_RETURN
 * @retval -IRONSIDE_SE_SNAPSHOT_ERROR_INVALID_MODE for an invalid mode.
 * @retval -IRONSIDE_SE_SNAPSHOT_ERROR_NOT_ENABLED if snapshot is not enabled.
 * @returns Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
 */
int ironside_se_snapshot_capture(enum ironside_se_snapshot_capture_mode mode);

/**
 * @brief Perform a snapshot recovery.
 *
 * Recovery restores the system state from the last successfully captured snapshot, allowing the
 * system to return to a known good state.
 *
 * A successful recovery request results in a reset and does not return. The recovery operation
 * itself is performed by the secure domain ROM as part of the system boot following the reset.
 * The result of the recovery operation is found in the boot report.
 *
 * Negative service codes are listed in @ref ironside_se_snapshot_ipc_errors.
 *
 * @note On success, function does not return.
 *
 * @retval -IRONSIDE_SE_SNAPSHOT_ERROR_UNEXPECTED_RETURN
 * @retval -IRONSIDE_SE_SNAPSHOT_ERROR_NOT_ENABLED if snapshot is not enabled.
 * @returns Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
 */
int ironside_se_snapshot_recovery(void);

/** @} */

/**
 * @defgroup ironside_se_ipc_periphconf PERIPHCONF
 * @ingroup ironside_se_ipc
 * @{
 */

/**
 * @defgroup ironside_se_periphconf_errors Error codes
 * @ingroup ironside_se_ipc_periphconf
 * @{
 */

/** Read/Write: Register count is too large for the IPC buffer. */
#define IRONSIDE_SE_PERIPHCONF_ERROR_COUNT_TOO_LARGE         (1)
/** Read/Write: Attempted to read/write an address that is not permitted. */
#define IRONSIDE_SE_PERIPHCONF_ERROR_REGISTER_NOT_PERMITTED  (2)
/** Write: Mismatch between the value written to and read back from the register. */
#define IRONSIDE_SE_PERIPHCONF_ERROR_READBACK_MISMATCH       (3)
/** Read/Write: Buffer points to disallowed memory area. */
#define IRONSIDE_SE_PERIPHCONF_ERROR_MEMORY_NOT_PERMITTED    (4)
/** Write: Not permitted based on the current register value. */
#define IRONSIDE_SE_PERIPHCONF_ERROR_VALUE_OLD_NOT_PERMITTED (5)
/** Write: Not permitted based on the new register value. */
#define IRONSIDE_SE_PERIPHCONF_ERROR_VALUE_NEW_NOT_PERMITTED (6)
/** Read: Buffer pointer/size is not aligned to the cache data unit width. */
#define IRONSIDE_SE_PERIPHCONF_ERROR_POINTER_UNALIGNED       (7)

/**
 * @}
 */

/** Result from a PERIPHCONF API call. */
struct ironside_se_periphconf_status {
	/** Negative error \c status from @ref ironside_se_periphconf_errors on PERIPHCONF error.
	 *  Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
	 *  Zero if successful.
	 */
	int16_t status;
	/** Index of the PERIPHCONF entry that caused an error.
	 *  Only valid if status is a negative error number.
	 */
	uint16_t index;
};

/** Maximum number of registers that can be read by passing the data inline in the IPC buffer.
 * If more registers than this are read, the entries pointer is passed instead.
 */
#define IRONSIDE_SE_PERIPHCONF_INLINE_READ_MAX_COUNT (6)

/**
 * @brief Read register values from the peripherals managed through PERIPHCONF.
 *
 * The entries argument serves both to specify which addresses to read, and as
 * output for the read values.
 *
 * Data is either transferred inline in the IPC buffer or directly using the provided buffer
 * pointer, depending on whether the number of registers is greater than
 * @ref IRONSIDE_SE_PERIPHCONF_INLINE_READ_MAX_COUNT.
 *
 * The result status consists of an error code and an array index.
 * If \c status is -@ref IRONSIDE_SE_PERIPHCONF_ERROR_REGISTER_NOT_PERMITTED,
 * the index points to the array index that caused the error.
 * If the index > 0 in this situation, entries up to but not including the reported index
 * contain valid data.
 * For any other negative \c status from @ref ironside_se_periphconf_errors, the index is always set
 * to 0.
 *
 * @note The API currently does not support bounce buffer allocations for the output buffer, because
 * the alignment requirements of the entry structure should ensure that it is never needed.
 *
 * @param entries Pointer to a list of register addresses and output values.
 * @param count Number of entries to read.
 * @returns @ref ironside_se_periphconf_status. Positive \c status from
 * @ref ironside_se_call_status_codes if reported by IronSide call.
 */
struct ironside_se_periphconf_status ironside_se_periphconf_read(struct periphconf_entry *entries,
								 size_t count);

/** Maximum number of registers that can be written by passing the data inline in the IPC buffer.
 * If more registers than this are written, the entries pointer is passed instead.
 */
#define IRONSIDE_SE_PERIPHCONF_INLINE_WRITE_MAX_COUNT (3)

/**
 * @brief Write register values to the peripherals managed through PERIPHCONF.
 *
 * The entries argument is used to specify the (register pointer, value) pairs to write.
 * Note that unlike the UICR PERIPHCONF interface, the register count must be exact,
 * the processing does not terminate on an all-ones register pointer.
 *
 * Data is either transferred inline in the IPC buffer or directly using the provided buffer
 * pointer, depending on whether the number of registers is greater than
 * @ref IRONSIDE_SE_PERIPHCONF_INLINE_WRITE_MAX_COUNT.
 *
 * The result status consists of an error code and an array index.
 * If \c status is
 * -@ref IRONSIDE_SE_PERIPHCONF_ERROR_REGISTER_NOT_PERMITTED,
 * -@ref IRONSIDE_SE_PERIPHCONF_ERROR_READBACK_MISMATCH,
 * -@ref IRONSIDE_SE_PERIPHCONF_ERROR_VALUE_OLD_NOT_PERMITTED, or
 * -@ref IRONSIDE_SE_PERIPHCONF_ERROR_VALUE_NEW_NOT_PERMITTED
 * (@ref ironside_se_periphconf_errors),
 * the index points to the array index that caused the error.
 * If the index > 0 in this situation, entries up to but not including the reported index
 * were written successfully.
 * For any other negative \c status from @ref ironside_se_periphconf_errors, the index is always set
 * to 0.
 *
 * @param entries Pointer to entries to write.
 * @param count Number of entries to write.
 * @returns @ref ironside_se_periphconf_status. Positive \c status from
 * @ref ironside_se_call_status_codes if reported by IronSide call.
 */
struct ironside_se_periphconf_status
ironside_se_periphconf_write(const struct periphconf_entry *entries, size_t count);

/**
 * @brief Finish peripheral initialization, restricting @ref ironside_se_periphconf_write.
 *
 * Calling this API also locks all SPU registers in hardware, preventing peripheral permissions
 * from being modified in any way until the next reset.
 *
 * At system start the write interface is configured for initialization.
 * In the initialization stage it is possible to modify the same set of registers as in the blob
 * pointed to by the PERIPHCONF field in UICR.
 *
 * Once initialization is complete, this API should be called to enter the normal operation stage.
 * In the normal operation stage, there are caller based limitations on which registers can be
 * written. Some registers also become unavailable for writing after initialization is done.
 * The read API is not affected by finishing the initialization.
 * See the IronSide SE documentation for additional details.
 *
 * Calling this API multiple times is allowed.
 *
 * @note A system reset is required to re-enter the initialization stage.
 *
 * Negative service codes are listed in @ref ironside_se_periphconf_errors.
 *
 * @retval 0 on success.
 * @returns Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
 */
int ironside_se_periphconf_finish_init(void);

/** @} */

/**
 * @defgroup ironside_se_ipc_mpcconf MPCCONF
 * @ingroup ironside_se_ipc
 * @{
 */

/**
 * @defgroup ironside_se_mpcconf_errors Error codes
 * @ingroup ironside_se_ipc_mpcconf
 * @{
 */

/** Read/Write: Attempted to read/write an MPC register address that is not permitted. */
#define IRONSIDE_SE_MPCCONF_ERROR_REGISTER_NOT_PERMITTED  (1)
/** Write: Attempted to configure a memory address range that is not permitted. */
#define IRONSIDE_SE_MPCCONF_ERROR_ADDRESSES_NOT_PERMITTED (2)
/** Write: Mismatch between the value written to and read back from the register. */
#define IRONSIDE_SE_MPCCONF_ERROR_READBACK_MISMATCH       (3)
/** Read/Write: Buffer points to disallowed memory area. */
#define IRONSIDE_SE_MPCCONF_ERROR_MEMORY_NOT_PERMITTED    (4)
/** Read: Buffer pointer/size is not aligned to the cache data unit width. */
#define IRONSIDE_SE_MPCCONF_ERROR_POINTER_UNALIGNED       (5)

/**
 * @}
 */

/** Result from an MPCCONF API call. */
struct ironside_se_mpcconf_status {
	/** Negative error \c status from @ref ironside_se_mpcconf_errors on PERIPHCONF error.
	 *  Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
	 *  Zero if successful.
	 */
	int16_t status;
	/** Index of the MPCCONF entry that caused an error.
	 *  Only valid if status is a negative error number.
	 */
	uint16_t index;
};

/**
 * @brief Read register values from the MPC instances managed through MPCCONF.
 *
 * For each element in the entries list, the register pointer field must be initialized to
 * specify which set of MPC registers to read. The API call will then overwrite the entry's
 * other fields in place, based on the values read from the register set.
 *
 * The result status consists of an error code and an array index.
 * If \c status is -@ref IRONSIDE_SE_MPCCONF_ERROR_REGISTER_NOT_PERMITTED,
 * the index points to the array index that caused the error.
 * If the index > 0 in this situation, entries up to but not including the reported index
 * contain valid data.
 * For any other negative \c status from @ref ironside_se_mpcconf_errors, the index is always set to
 * 0.
 *
 * @note The API currently does not support bounce buffer allocations for the output buffer, because
 * the alignment requirements of the entry structure should ensure that it is never needed.
 *
 * @param entries Pointer to a list of MPC configuration entries.
 * @param count Number of entries to read.
 * @returns @ref ironside_se_mpcconf_status. Positive \c status from
 * @ref ironside_se_call_status_codes if reported by IronSide call.
 */
struct ironside_se_mpcconf_status ironside_se_mpcconf_read(struct mpcconf_entry *entries,
							   size_t count);

/**
 * @brief Write register values to the MPC instances managed through MPCCONF.
 *
 * The entries argument is used to specify the MPC configuration entries to apply.
 * Note that unlike the UICR MPCCONF interface, the entry count must be exact,
 * the processing does not terminate on an all-ones register pointer.
 *
 * The result status consists of an error code and an array index.
 * If \c status is one of
 * -@ref IRONSIDE_SE_MPCCONF_ERROR_REGISTER_NOT_PERMITTED,
 * -@ref IRONSIDE_SE_MPCCONF_ERROR_ADDRESSES_NOT_PERMITTED, or
 * -@ref IRONSIDE_SE_MPCCONF_ERROR_READBACK_MISMATCH
 * (members of @ref ironside_se_mpcconf_errors),
 * the index points to the array index that caused the error.
 * If the index > 0 in this situation, entries up to but not including the reported index
 * were written successfully.
 * For any other negative \c status from @ref ironside_se_mpcconf_errors, the index is always set to
 * 0.
 *
 * @param entries Pointer to entries to write.
 * @param count Number of entries to write.
 * @returns @ref ironside_se_mpcconf_status. Positive \c status from
 * @ref ironside_se_call_status_codes if reported by IronSide call.
 */
struct ironside_se_mpcconf_status ironside_se_mpcconf_write(const struct mpcconf_entry *entries,
							    size_t count);

/**
 * @brief Access modes for @ref ironside_se_mpcconf_radioram_access.
 */
enum ironside_se_mpcconf_radioram_access {
	/** Close radiocore RAM access, restoring default MPC configuration. */
	IRONSIDE_SE_MPCCONF_RADIORAM_ACCESS_CLOSE = 0,
	/** Open radiocore RAM for read/write access from all domains. */
	IRONSIDE_SE_MPCCONF_RADIORAM_ACCESS_OPEN = 1,
};

/**
 * @brief Open or close access to the radiocore RAM from all domains.
 *
 * When opened, configures a RADIOCORE MPC override to grant read/write access to
 * the radiocore RAM from all domains. When closed, the override is disabled and
 * default access restrictions apply.
 *
 * This call is only permitted while the mpcconf service is in the initialization
 * stage (before @ref ironside_se_mpcconf_finish_init has been called).
 *
 * @param access @ref IRONSIDE_SE_MPCCONF_RADIORAM_ACCESS_OPEN or
 *               @ref IRONSIDE_SE_MPCCONF_RADIORAM_ACCESS_CLOSE.
 * @retval 0 on success.
 * @retval -IRONSIDE_SE_MPCCONF_ERROR_REGISTER_NOT_PERMITTED if called after initialization.
 * @retval -IRONSIDE_SE_MPCCONF_ERROR_READBACK_MISMATCH if the configuration could not be verified.
 * @returns Positive \c status from @ref ironside_se_call_status_codes if reported by IronSide call.
 */
int ironside_se_mpcconf_radioram_access(enum ironside_se_mpcconf_radioram_access access);

/**
 * @brief Finish MPC initialization, restricting @ref ironside_se_mpcconf_write.
 *
 * Calling this API also locks all MPC registers in hardware, preventing the memory configuration
 * from being modified in any way until the next reset.
 *
 * At system start the write interface is configured for initialization.
 * In the initialization stage it is possible to modify the same set of registers as in the blob
 * pointed to by the MPCCONF field in UICR.
 *
 * Once initialization is complete, this API should be called to enter the normal operation stage.
 * The read API is not affected by finishing the initialization.
 *
 * Calling this API multiple times is allowed.
 *
 * @note A system reset is required to re-enter the initialization stage.
 *
 * Negative service codes are listed in @ref ironside_se_mpcconf_errors.
 *
 * @retval 0 on success.
 * @returns Positive status from @ref ironside_se_call_status_codes if reported by IronSide call.
 */
int ironside_se_mpcconf_finish_init(void);

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif
#endif /* IRONSIDE_SE_API_H_ */
