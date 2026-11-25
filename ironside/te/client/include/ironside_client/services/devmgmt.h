/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - device-management API.
 *
 * Two capabilities today. They are unrelated to each other except in shape: each
 * is one-shot. Closing the access port lasts for the device's life; the executable
 * RAM region lasts until the next reset.
 *
 *   - ironside_devmgmt_ap_protect() - close the non-secure access port, the
 *     device-side half of taking a product into production. One-way: no call
 *     reopens it.
 *   - ironside_devmgmt_xram_region_define() - define APP-XRAM, the executable
 *     non-secure RAM window used for RAM-resident code under W^X.
 *
 * APP-XRAM usage is two steps, and the order is the whole point:
 *
 *   1. The App copies its RAM functions into the top of the non-secure RAM. No
 *      call to IronSide TE is involved: until the region is defined the whole non-secure
 *      RAM is ordinary APP-RAM, so those bytes are plain writable data.
 *   2. ironside_devmgmt_xram_region_define(size) - IronSide TE carves the top `size` bytes
 *      off the non-secure RAM as APP-XRAM: APP-RAM shrinks to the remainder and
 *      loses write access to the window, which becomes read+execute. The App can now call
 *      into it, and nothing can rewrite it for the rest of the boot.
 *
 * So defining the region is the write->execute switch, and it is one-shot: the
 * window is never writable and executable at the same time, and a second call is
 * refused. An App that never calls it has no executable RAM.
 *
 * Outside the Open and PRoT Provisioning states, no other non-secure RAM is
 * executable. In those two states all non-secure RAM is executable.
 *
 * nRF54L15 does not support APP-XRAM. There, the whole non-secure RAM is
 * executable in every lifecycle state and the define call is refused.
 */

#ifndef IRONSIDE_CLIENT_SERVICES_DEVMGMT_H
#define IRONSIDE_CLIENT_SERVICES_DEVMGMT_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Define the APP-XRAM (executable non-secure RAM) region.
 *
 * Carves @p size bytes off the END of the non-secure RAM as APP-XRAM: APP-RAM
 * shrinks to the remainder below it, and the window becomes read+execute - the App
 * can no longer write it. Load the code into those bytes before calling this (while
 * they are still ordinary writable APP-RAM); afterwards, call into them.
 *
 * One-shot and one-way: the split is fixed for the rest of the boot, so a second
 * call is refused and write is never re-granted. The non-secure side must not access
 * the window during the call.
 *
 * @param size APP-XRAM size in bytes: non-zero, a multiple of 4 KiB, and small
 *             enough to leave a non-empty APP-RAM below it.
 *
 * @retval 0     The region is defined and executable.
 * @retval -135  (PSA_ERROR_INVALID_ARGUMENT) @p size is zero, misaligned, or does
 *               not fit the non-secure RAM.
 * @retval -133  (PSA_ERROR_NOT_PERMITTED) a region was already defined this boot.
 * @retval -134  (PSA_ERROR_NOT_SUPPORTED) the part has no APP-XRAM (nRF54L15).
 * @retval -249  (IRONSIDE_ERROR_CALL_IN_FLIGHT) another call is in flight; see integration.h.
 */
int32_t ironside_devmgmt_xram_region_define(uint32_t size);

/**
 * @brief Which lock ironside_devmgmt_ap_protect() engages.
 *
 * Both modes close the non-secure access port.
 */
enum ironside_devmgmt_approtect_mode {
	/** Permanent hardware close: set UICR.APPROTECT. */
	IRONSIDE_DEVMGMT_APPROTECT_HW_LOCK = 1,
	/** The port is closed by default, but an authenticated debug session (ADAC) can reopen it.
	 */
	IRONSIDE_DEVMGMT_APPROTECT_AUTH_LOCK = 2,
};

/**
 * @brief Close the non-secure access port.
 *
 * **Irreversible.** Both modes engage the AUTH_LOCK latch, and the latch is what
 * makes this one-way: nothing here reopens the port, and once the latch is set
 * every further call is refused with -133 rather than re-applied - so the mode
 * chosen the first time is the mode the device keeps. The latch lives in write-once
 * NVM, so a reset does not undo it.
 *
 * @param mode Which lock to engage; see @ref ironside_devmgmt_approtect_mode.
 *
 * @retval 0     The port is closed and the latch is engaged.
 * @retval -135  (PSA_ERROR_INVALID_ARGUMENT) @p mode is not one of the two defined
 *               values, or the request is malformed.
 * @retval -133  (PSA_ERROR_NOT_PERMITTED) AUTH_LOCK is already engaged: the port
 *               is closed and this call is latched shut. Nothing was written.
 * @retval -147  (PSA_ERROR_HARDWARE_FAILURE) the latch did not read back as
 *               engaged after being written.
 * @retval -249  (IRONSIDE_ERROR_CALL_IN_FLIGHT) another call is in flight; see integration.h.
 */
int32_t ironside_devmgmt_ap_protect(enum ironside_devmgmt_approtect_mode mode);

#endif /* IRONSIDE_CLIENT_SERVICES_DEVMGMT_H */
