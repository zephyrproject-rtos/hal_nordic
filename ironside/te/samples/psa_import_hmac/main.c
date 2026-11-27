/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_import_hmac.
 *
 * Imports a *user* HMAC key into UROT with psa_import_key() and uses it to
 * compute HMAC-SHA-256 over the standard PSA Crypto API through the
 * Non-Secure-Callable veneer. Same key-management path as psa_import_aes, but
 * with the HMAC key type and the MAC module's HMAC algorithm.
 *
 * The key is *volatile* (default lifetime): a RAM key slot, gone on reset.
 *
 * Test vector: RFC 4231, HMAC-SHA-256 Test Case 2.
 *   Key  = "Jefe"                          (4 bytes)
 *   Data = "what do ya want for nothing?"  (28 bytes)
 *   HMAC = 5bdcc146 bf60754e 6a042426 089575c7
 *          5a003f08 9d273983 9dec58b9 64ec3843
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

static const uint8_t g_key[4] = { 'J', 'e', 'f', 'e' };
static const char g_data[] = "what do ya want for nothing?"; /* 28 bytes, no NUL */
static const uint8_t g_expected_hmac[32] = {
	0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e, 0x6a, 0x04, 0x24,
	0x26, 0x08, 0x95, 0x75, 0xc7, 0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27,
	0x39, 0x83, 0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43,
};

static bool import_and_hmac_ok(void)
{
	static uint8_t mac[32];
	size_t mac_len = 0;
	psa_key_id_t key = 0;
	bool ok = true;
	const psa_algorithm_t alg = PSA_ALG_HMAC(PSA_ALG_SHA_256);

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);
	psa_set_key_bits(&attr, 8 * sizeof(g_key));
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_algorithm(&attr, alg);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	if (psa_import_key(&attr, g_key, sizeof(g_key), &key) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_import_key (HMAC) failed\r\n");
		return false;
	}

	if (psa_mac_compute(key, alg, (const uint8_t *)g_data, sizeof(g_data) - 1, mac, sizeof(mac),
			    &mac_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_mac_compute (HMAC) failed\r\n");
		ok = false;
	} else {
		integration_kit_uart_puts("HMAC-SHA256: ");
		integration_kit_uart_hexdump(mac, mac_len);
		integration_kit_uart_puts("\r\n");
		ok = (mac_len == sizeof(g_expected_hmac)) &&
		     (memcmp(mac, g_expected_hmac, sizeof(g_expected_hmac)) == 0);
	}

	if (psa_destroy_key(key) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_destroy_key failed\r\n");
		ok = false;
	}

	return ok;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts(
		"psa_import_hmac: import HMAC key + HMAC-SHA256 via UROT PSA service\r\n");

	(void)psa_crypto_init();

	bool ok = import_and_hmac_ok();

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_import_hmac") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_import_hmac"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
