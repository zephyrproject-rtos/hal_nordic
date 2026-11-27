/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - client core implementation.
 *
 * Branches to IronSide TE's single, fixed-address secure-gateway entry. This is a
 * plain indirect call to an absolute address: the SG instruction that performs
 * the secure transition lives in the secure-side veneer, so the non-secure caller
 * needs no -mcmse and links no secure-side artifact.
 */

#include <ironside_client/internal/client.h>
#include <ironside_client/integration.h>

/* ironside_client_call_values() reaches only the handles that carry the bit. */
_Static_assert((IRONSIDE_CLIENT_PLATFORM_RRAMC_LOWPOWERCONFIG_HANDLE &
		IRONSIDE_CLIENT_HANDLE_VALUES) != 0,
	       "RRAMC low-power mode takes values");
_Static_assert((IRONSIDE_CLIENT_PLATFORM_SYSTEM_OFF_HANDLE & IRONSIDE_CLIENT_HANDLE_VALUES) != 0,
	       "System OFF takes values");
_Static_assert((IRONSIDE_CLIENT_PLATFORM_SYSTEM_RESET_HANDLE & IRONSIDE_CLIENT_HANDLE_VALUES) != 0,
	       "system reset takes values");

/*
 * Default client lock: no-ops, valid for a single-threaded bare-metal app.
 * Override with strong definitions; see integration.h.
 */
__attribute__((weak)) void ironside_client_lock_acquire(void)
{
}

__attribute__((weak)) void ironside_client_lock_release(void)
{
}

int32_t ironside_client_call(const struct ironside_client_call_params *params)
{
	const struct ironside_client_call_vectors vectors = {
		.in_vec = params->in_vec,
		.in_len = params->in_len,
		.out_vec = params->out_vec,
		.out_len = params->out_len,
	};
	int32_t status = PSA_ERROR_GENERIC_ERROR;

	ironside_client_lock_acquire();
	status = IRONSIDE_CLIENT_ENTRY(params->handle, (uint32_t)(uintptr_t)&vectors, 0U, 0U);
	ironside_client_lock_release();

	return status;
}
