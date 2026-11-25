/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_random.
 *
 * A non-secure, bare-metal App that asks UROT for random bytes over the standard
 * PSA Crypto API (psa_generate_random), through the Non-Secure-Callable veneer.
 * It exercises the same chain as psa_sha256 but for the RNG service: NS App ->
 * NSC veneer (SG) -> UROT secure dispatch -> PSA Crypto core ->
 * CRACEN CTR-DRBG (seeded from the CRACEN TRNG) -> bytes written back into the
 * App's buffer.
 *
 * Random output can't be checked against a fixed vector, so the test applies
 * cheap sanity checks that a working RNG passes and a stuck/zeroed one fails:
 * two independent draws both succeed, are not all-equal-bytes (e.g. not all
 * zero / all 0xff), and differ from each other. A short (odd-length) draw also
 * confirms output_size is honoured. Prints the draws + the INTEGRATION_KIT_TEST_* markers
 * the test runner asserts.
 *
 * Uses the nrfx UARTE *driver* (nrfx_uarte.c, compiled in via this sample's
 * sample.cmake) rather than poking the HAL directly.
 *
 * Console: UARTE20, TX = P1.16, 115200 8N1 (same as hello_world).
 */

#include <integration_kit_uart.h>
#include <integration_kit_test.h>
#include <ironside_client/psa/crypto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* True if every byte of [data, data+len) is the same value (stuck/zeroed RNG). */
static bool all_same(const uint8_t *data, size_t len)
{
	for (size_t i = 1; i < len; i++) {
		if (data[i] != data[0]) {
			return false;
		}
	}
	return true;
}

int main(void)
{
	static uint8_t a[32];
	static uint8_t b[32];
	static uint8_t odd[7];

	integration_kit_uart_init();
	integration_kit_uart_puts(
		"psa_random: random bytes via UROT PSA service (CRACEN CTR-DRBG)\r\n");

	(void)psa_crypto_init();

	bool ok = true;

	memset(a, 0, sizeof(a));
	memset(b, 0, sizeof(b));
	if (psa_generate_random(a, sizeof(a)) != PSA_SUCCESS ||
	    psa_generate_random(b, sizeof(b)) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_generate_random failed\r\n");
		ok = false;
	}

	integration_kit_uart_puts("draw 1: ");
	integration_kit_uart_hexdump(a, sizeof(a));
	integration_kit_uart_puts("\r\n");
	integration_kit_uart_puts("draw 2: ");
	integration_kit_uart_hexdump(b, sizeof(b));
	integration_kit_uart_puts("\r\n");

	/* A working RNG: neither draw is all-one-value, and the two differ. */
	if (all_same(a, sizeof(a)) || all_same(b, sizeof(b))) {
		integration_kit_uart_puts("FAIL: a draw was a single repeated byte\r\n");
		ok = false;
	}
	if (memcmp(a, b, sizeof(a)) == 0) {
		integration_kit_uart_puts("FAIL: two draws were identical\r\n");
		ok = false;
	}

	/* Short, odd-length draw: output_size must be honoured (only 7 bytes set). */
	memset(odd, 0xAA, sizeof(odd));
	if (psa_generate_random(odd, sizeof(odd)) != PSA_SUCCESS || all_same(odd, sizeof(odd))) {
		integration_kit_uart_puts("FAIL: short draw\r\n");
		ok = false;
	}

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_random") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_random"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
