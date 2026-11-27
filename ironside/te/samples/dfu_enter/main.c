/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - dfu_enter.
 *
 * How an App hands the device over to IronSide DFU: ask
 * (ironside_dfu_request_set(), from ironside_client/services/dfu_request.h), then reset
 * through ironside_platform_system_reset(). That is the whole thing - the two calls are the sample.
 *
 * This is what an application does when it has decided it needs recovery rather
 * than an update in place: a host told it to enter DFU, a self-check failed, a
 * user held a button down through boot. The App is still perfectly bootable; it
 * is choosing not to be booted.
 *
 * Three properties worth knowing before you copy this:
 *
 *   - Setting the request does not reset the device. The reset below is this
 *     sample's own, deliberately separate, because the caller is the only one
 *     who knows when it is finished - flush a log, park a peripheral, answer the
 *     host, and only then hand over.
 *   - The request is one-shot. IronSide consumes it on the boot it acts on, so
 *     the boot after the recovery boot selects the App again by the normal
 *     rules. You do not have to clear it afterwards, and cannot be left stuck.
 *   - It is a preference, not a guarantee. IronSide still verifies IronSide
 *     DFU's signature and rollback counter before jumping there, and boots the
 *     App instead if that fails. A non-secure image has no way to force an
 *     unbootable device.
 *
 * Changed your mind before the reset? ironside_dfu_request_clear() withdraws a
 * pending request, and is as idempotent as setting one.
 *
 * This sample resets, so it does not come back - the console output ends with
 * the IronSide DFU banner rather than with this App. That also makes it the one
 * sample with no test.toml: an automated sweep cannot judge an App that hands
 * the device to a different image, and would find the board in DFU rather than
 * where it left it. Reflash any other sample to get back.
 *
 * Console: UARTE20, TX = P1.16, 115200 8N1 (same as the other samples).
 */

#include <integration_kit_uart.h>
#include <ironside_client/psa/error.h>
#include <ironside_client/services/dfu_request.h>
#include <ironside_client/services/platform.h>
#include <stdbool.h>
#include <stdint.h>

static __attribute__((noreturn)) void park(void)
{
	for (;;) {
		__asm__ volatile("wfi");
	}
}

int main(void)
{
	bool pending = false;
	int32_t status = PSA_ERROR_GENERIC_ERROR;

	integration_kit_uart_init();
	integration_kit_uart_puts("dfu_enter: handing the device over to IronSide DFU\r\n");

	/*
	 * 1. Is one already pending? Nothing here needs to ask - setting is
	 *    idempotent - but reading back is how an App reports its own intent,
	 *    and it is the only call of the three that can be given a bad
	 *    argument, so it is the one worth showing a status for.
	 */
	status = ironside_dfu_request_get(&pending);
	integration_kit_uart_printf("  request already pending = %s  (status %d)\r\n",
				    pending ? "yes" : "no", (int)status);

	if (status != 0) {
		/* -134 is PSA_ERROR_NOT_SUPPORTED: the IronSide on this device
		 * predates the service. Nothing to fall back to, so say so
		 * plainly rather than resetting into a boot that will not
		 * change. */
		integration_kit_uart_puts("  service unavailable - not resetting\r\n");
		park();
	}

	/* 2. Ask. */
	status = ironside_dfu_request_set();
	integration_kit_uart_printf("  requesting DFU entry            (status %d)\r\n",
				    (int)status);
	if (status != 0) {
		integration_kit_uart_puts("  request refused - not resetting\r\n");
		park();
	}

	/* 3. Confirm it took, before betting a reboot on it. */
	(void)ironside_dfu_request_get(&pending);
	if (!pending) {
		integration_kit_uart_puts("  request did not stick - not resetting\r\n");
		park();
	}

	/*
	 * 4. Hand over. IronSide makes SYSRESETREQ secure-only, so the App resets
	 *    through the system-reset service rather than NVIC_SystemReset(). The
	 *    reset reason it produces (SREQ) is not itself a DFU trigger, so the
	 *    boot that follows can only come from the request set above.
	 */
	integration_kit_uart_puts("  resetting; the next banner should be IronSide DFU\r\n");
	(void)ironside_platform_system_reset(IRONSIDE_PLATFORM_RESET_SYSTEM);
	integration_kit_uart_puts("  reset refused\r\n");

	park();
}
