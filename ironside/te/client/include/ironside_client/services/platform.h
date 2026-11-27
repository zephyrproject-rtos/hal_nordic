/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - platform services API.
 */

#ifndef IRONSIDE_CLIENT_SERVICES_PLATFORM_H
#define IRONSIDE_CLIENT_SERVICES_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

/** One register write for ironside_platform_mem_write(). */
struct ironside_client_platform_mem_write_pair {
	uint32_t addr;
	uint32_t value;
};

/* ---- Batched register read/write -----------------------------------------
 *
 * Refer to "Register read and write" section of doc/secure-services.md
 * for the details on the register read and write policies.
 */

/**
 * @brief Read @p count 32-bit registers at the secure addresses in @p addrs.
 *
 * @param addrs   Array of @p count absolute secure addresses, each 4-byte aligned.
 * @param values  Receives @p count words. Untouched unless the call succeeds.
 * @param count   Number of registers to read; must be non-zero.
 *
 * @retval 0     Success.
 * @retval -133  (PSA_ERROR_NOT_PERMITTED) a register is outside the read policy.
 * @retval -135  (PSA_ERROR_INVALID_ARGUMENT) malformed request.
 * @retval -129  (PSA_ERROR_PROGRAMMER_ERROR) the running IronSide TE predates this request.
 * @retval -249  (IRONSIDE_ERROR_CALL_IN_FLIGHT) another call is in flight; see integration.h.
 */
int32_t ironside_platform_mem_read(const uint32_t *addrs, uint32_t *values, size_t count);

/**
 * @brief Write @p count register pairs in one call.
 *
 * @param pairs          Array of {addr, value} pairs.
 * @param count          Number of pairs; must be non-zero.
 * @param readback       Optional; receives the value read back from each written
 *                       register through the secure alias. Pass NULL to skip.
 * @param readback_count Number of readback slots; may be less than @p count.
 *
 * A pair may target a UICR.OTP word, which is written only while it is erased.
 * The UICR.OTP words of the call are written first, then its registers. A value
 * of 0xFFFFFFFF is not written, so the word stays erased.
 *
 * @retval 0     Success; all pairs were written.
 * @retval -133  (PSA_ERROR_NOT_PERMITTED) a pair is refused; nothing is applied.
 * @retval -135  (PSA_ERROR_INVALID_ARGUMENT) malformed request.
 * @retval -139  (PSA_ERROR_ALREADY_EXISTS) a UICR.OTP word is not erased, or two
 *               pairs target the same word; nothing is applied.
 * @retval -147  (PSA_ERROR_HARDWARE_FAILURE) a UICR.OTP word does not read back as
 *               written; no register is written, but the other UICR.OTP words
 *               might already be written.
 * @retval -129  (PSA_ERROR_PROGRAMMER_ERROR) the running IronSide TE predates this request.
 * @retval -249  (IRONSIDE_ERROR_CALL_IN_FLIGHT) another call is in flight; see integration.h.
 */
int32_t ironside_platform_mem_write(const struct ironside_client_platform_mem_write_pair *pairs,
				    size_t count, uint32_t *readback, size_t readback_count);

/**
 * @brief Read one 32-bit register through the secure alias.
 */
int32_t ironside_platform_reg_read(uint32_t addr, uint32_t *value);

/**
 * @brief Write one 32-bit register through the secure alias.
 *
 * @param readback Optional applied-value readback; pass NULL to skip.
 */
int32_t ironside_platform_reg_write(uint32_t addr, uint32_t value, uint32_t *readback);

/* ---- RRAM power mode ------------------------------------------------------ */

/** RRAM power mode for ironside_platform_rram_power_mode_set(). */
enum ironside_platform_rram_power_mode {
	/** The RRAM powers off while idle, to save current. The mode at boot. */
	IRONSIDE_PLATFORM_RRAM_POWER_MODE_DEFAULT = 0,
	/** The RRAM stays in standby while idle, for a predictable NVM latency. */
	IRONSIDE_PLATFORM_RRAM_POWER_MODE_LOW_LATENCY = 1,
};

/**
 * @brief Select the RRAM power mode.
 *
 * Never refused with IRONSIDE_ERROR_CALL_IN_FLIGHT; see integration.h.
 *
 * @param mode Power mode to select.
 *
 * @retval 0     Success.
 * @retval -135  (PSA_ERROR_INVALID_ARGUMENT) an unknown @p mode.
 */
int32_t ironside_platform_rram_power_mode_set(enum ironside_platform_rram_power_mode mode);

/* ---- Bulk NVM write ------------------------------------------------------- */

int32_t ironside_platform_nvm_write(uint32_t addr, const void *src, uint32_t len);

/* ---- System OFF ----------------------------------------------------------- */

int32_t ironside_platform_system_off(void);

/* ---- System reset --------------------------------------------------------- */

/** Reset type for ironside_platform_system_reset(). */
enum ironside_platform_reset_type {
	/** Soft reset through SYSRESETREQ. The boot report shows SREQ. */
	IRONSIDE_PLATFORM_RESET_SYSTEM = 0,
	/** CTRL-AP soft reset. The boot report shows CTRLAPSOFT. */
	IRONSIDE_PLATFORM_RESET_CTRLAP_SOFT = 1,
	/** CTRL-AP pin reset. The boot report shows CTRLAPPIN. */
	IRONSIDE_PLATFORM_RESET_CTRLAP_PIN = 2,
};

/**
 * @brief Reset the device.
 *
 * IronSide TE makes SYSRESETREQ secure-only, so NVIC_SystemReset() from
 * non-secure code does not reset the device and hangs. IronSide TE applies the
 * nRF54L errata [63] workaround before every reset it issues, then resets.
 * Read the reset reason from the boot report, not from the register.
 *
 * There is no CTRL-AP hard reset type: that reset is the debugger's request to
 * enter ADAC mode.
 *
 * Does not return on success.
 *
 * @param type Reset to issue.
 *
 * @retval -135  (PSA_ERROR_INVALID_ARGUMENT) malformed request or an unknown @p type.
 * @retval -129  (PSA_ERROR_PROGRAMMER_ERROR) the running IronSide TE predates this request.
 * @retval -249  (IRONSIDE_ERROR_CALL_IN_FLIGHT) another call is in flight; see integration.h.
 */
int32_t ironside_platform_system_reset(enum ironside_platform_reset_type type);

#endif /* IRONSIDE_CLIENT_SERVICES_PLATFORM_H */
