/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_sha256.
 *
 * The first sample that exercises an IronSide *secure service*: a non-secure,
 * bare-metal App that asks UROT to compute SHA-256 over the PSA Crypto API,
 * through the Non-Secure-Callable veneer. It proves the whole
 * chain end-to-end on HW: NS App -> NSC veneer (SG) -> UROT secure dispatch ->
 * PSA Crypto core -> CRACEN -> back to NS.
 *
 * It checks the FIPS-180-4 example "abc" two ways: a single-shot
 * psa_hash_compute(), and IRONSIDE_CLIENT_PSA_CRYPTO_MAX_OPERATIONS concurrent
 * multi-part operations (setup/update/finish, updates interleaved) - which also
 * confirms the secure side supports that many live operation contexts by
 * checking that one more concurrent setup is refused. Prints the digests + the
 * INTEGRATION_KIT_TEST_* markers the test runner asserts.
 *
 * It also shows the other thing an application behind IronSide should do at the
 * top of main(): report the fault that reset the device. Every CPU fault here is
 * taken to the secure side, so an application never sees one -- it is told about
 * the previous one, in the boot report, and this sample stops on it
 * (lib/integration_kit_fault.h).
 *
 * Uses the nrfx UARTE *driver* (nrfx_uarte.c, compiled in via this sample's
 * sample.cmake) rather than poking the HAL directly.
 *
 * Console: UARTE20, TX = P1.16, 115200 8N1 (same as hello_world).
 */

#include <integration_kit_fault.h>
#include <integration_kit_uart.h>
#include <integration_kit_test.h>
#include <ironside_client/services/boot_report.h>
#include <ironside_client/psa/crypto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* SHA-256("abc"), FIPS-180-4. */
static const uint8_t g_expected[32] = {
	0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40,
	0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17,
	0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
};

/* Single-shot: psa_hash_compute("abc") == g_expected. */
static bool single_shot_ok(void)
{
	static uint8_t digest[32];
	size_t digest_len = 0;

	psa_status_t status = psa_hash_compute(PSA_ALG_SHA_256, (const uint8_t *)"abc", 3, digest,
					       sizeof(digest), &digest_len);
	if (status != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_hash_compute failed\r\n");
		return false;
	}
	integration_kit_uart_puts("single-shot digest: ");
	integration_kit_uart_hexdump(digest, digest_len);
	integration_kit_uart_puts("\r\n");
	return digest_len == sizeof(digest) && memcmp(digest, g_expected, sizeof(digest)) == 0;
}

/*
 * Multi-part: run CONC (the number of live operation contexts the secure side
 * supports) SHA-256("abc") operations concurrently, with their updates
 * interleaved so all of them are live at once, and confirm a one-more setup is
 * refused.
 */
static bool multipart_concurrent_ok(void)
{
	enum { CONC = IRONSIDE_CLIENT_PSA_CRYPTO_MAX_OPERATIONS };
	psa_hash_operation_t op[CONC];
	static uint8_t digest[32];
	bool ok = true;

	/* Open CONC operations - all must succeed. */
	for (int i = 0; i < CONC; i++) {
		op[i] = PSA_HASH_OPERATION_INIT;
		if (psa_hash_setup(&op[i], PSA_ALG_SHA_256) != PSA_SUCCESS) {
			integration_kit_uart_puts("multipart: setup failed\r\n");
			ok = false;
		}
	}

	/* One more, while all CONC are still live, must be rejected. */
	psa_hash_operation_t extra = PSA_HASH_OPERATION_INIT;
	if (psa_hash_setup(&extra, PSA_ALG_SHA_256) == PSA_SUCCESS) {
		integration_kit_uart_printf("multipart: setup %d of %d unexpectedly succeeded\r\n",
					    CONC + 1, CONC);
		ok = false;
	} else {
		integration_kit_uart_printf("multipart: setup %d of %d rejected\r\n", CONC + 1,
					    CONC);
	}

	/* Interleave the message across all live operations: "a" then "bc". */
	for (int i = 0; i < CONC; i++) {
		if (psa_hash_update(&op[i], (const uint8_t *)"a", 1) != PSA_SUCCESS) {
			ok = false;
		}
	}
	for (int i = 0; i < CONC; i++) {
		if (psa_hash_update(&op[i], (const uint8_t *)"bc", 2) != PSA_SUCCESS) {
			ok = false;
		}
	}

	/* Finish each and check every digest independently. */
	for (int i = 0; i < CONC; i++) {
		size_t digest_len = 0;
		psa_status_t status = psa_hash_finish(&op[i], digest, sizeof(digest), &digest_len);

		integration_kit_uart_puts("multipart digest: ");
		integration_kit_uart_hexdump(digest, digest_len);
		integration_kit_uart_puts("\r\n");

		if (status != PSA_SUCCESS || digest_len != sizeof(digest) ||
		    memcmp(digest, g_expected, sizeof(digest)) != 0) {
			ok = false;
		}
	}

	return ok;
}

int main(void)
{
	struct ironside_boot_report report;
	size_t report_length = 0U;

	integration_kit_uart_init();
	integration_kit_uart_puts("psa_sha256: SHA-256(\"abc\") via UROT PSA service\r\n");

	/*
	 * Before any work: did the previous boot end in a fault? The banner above
	 * comes first only so a console capture says which image is reporting.
	 * On an ordinary boot this prints nothing and returns; on one that followed
	 * a fault it prints the record and does not return.
	 *
	 * The reported length is not decoration here: a length short of the capacity
	 * offered means this build is newer than the IronSide on the device, so the
	 * fault record was never written and whatever is in those bytes is this
	 * image's own -- which the handler would read as a fault report.
	 */
	if (ironside_boot_report_get(&report, sizeof(report), &report_length) == 0 &&
	    report_length == sizeof(report)) {
		integration_kit_fault_handle(&report.fault);
	}

	(void)psa_crypto_init();

	bool ok = single_shot_ok();
	ok = multipart_concurrent_ok() && ok;

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_sha256") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_sha256"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
