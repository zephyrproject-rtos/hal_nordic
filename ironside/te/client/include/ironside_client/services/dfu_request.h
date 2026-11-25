/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - DFU-entry request API.
 *
 * How a non-secure image asks to be booted into the DFU image instead of the App
 * next time: an application that has decided it needs recovery - a host told it to
 * update, a self-check failed, a user held a button - calls
 * ironside_dfu_request_set() and then reboots.
 *
 * IronSide TE keeps the request in reset-persistent state of its own and consumes it
 * on the next boot, so it is **one-shot**: the boot after the recovery boot selects
 * the App again by the normal rules. Nothing here says where that state lives, and
 * a caller must not assume - this call is the whole contract.
 *
 * Setting the request does **not** reset the device. The caller chooses when to
 * hand over, so it can finish what it was doing first (flush a log, park a
 * peripheral, answer a host) and then reset itself the ordinary way.
 *
 * The request selects between two images IronSide TE was willing to boot anyway: it
 * still verifies the DFU image's signature and rollback counter before jumping
 * there, and boots the App instead if that fails. So a request is a preference, not
 * a guarantee - there is no way for a non-secure image to force an unbootable
 * device, and none of these calls can fail for a permission reason.
 */

#ifndef IRONSIDE_CLIENT_SERVICES_DFU_REQUEST_H
#define IRONSIDE_CLIENT_SERVICES_DFU_REQUEST_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Ask IronSide TE to boot the DFU image on the next boot.
 *
 * Idempotent - requesting again while a request is already pending succeeds and
 * changes nothing. Does not reset the device: reset when ready.
 *
 * @retval 0     The request is pending.
 * @retval -129  (PSA_ERROR_PROGRAMMER_ERROR) the running IronSide TE predates
 *               this service.
 * @retval -249  (IRONSIDE_ERROR_CALL_IN_FLIGHT) another call is in flight; see integration.h.
 */
int32_t ironside_dfu_request_set(void);

/**
 * @brief Withdraw a pending DFU-entry request.
 *
 * Idempotent - clearing when nothing is pending succeeds. Useful when the reason
 * for requesting recovery went away before the reboot, or for an image that wants
 * to be sure it left nothing behind.
 *
 * @retval 0     No request is pending.
 * @retval -129  (PSA_ERROR_PROGRAMMER_ERROR) the running IronSide TE predates
 *               this service.
 * @retval -249  (IRONSIDE_ERROR_CALL_IN_FLIGHT) another call is in flight; see integration.h.
 */
int32_t ironside_dfu_request_clear(void);

/**
 * @brief Read whether a DFU-entry request is pending.
 *
 * Reads the same flag the next boot will consume, without consuming it.
 *
 * @param pending Set to true iff a request is pending. Must not be NULL.
 *
 * @retval 0     @p pending was written.
 * @retval -135  (PSA_ERROR_INVALID_ARGUMENT) @p pending is unwritable non-secure
 *               memory.
 * @retval -129  (PSA_ERROR_PROGRAMMER_ERROR) the running IronSide TE predates
 *               this service.
 * @retval -249  (IRONSIDE_ERROR_CALL_IN_FLIGHT) another call is in flight; see integration.h.
 */
int32_t ironside_dfu_request_get(bool *pending);

#endif /* IRONSIDE_CLIENT_SERVICES_DFU_REQUEST_H */
