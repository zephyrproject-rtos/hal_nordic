/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_sha1.
 *
 * SHA-1 through psa_hash_compute(), against the FIPS 180 "abc" vector. UROT admits
 * SHA-1 as a plain hash only (Matter hashes with it): HMAC over SHA-1 and ECDSA
 * over SHA-1 are refused, so UROT computes no MAC or signature over SHA-1.
 *
 * Console: UARTE20, TX = P1.16, 115200 8N1.
 */

#include <integration_kit_uart.h>
#include <integration_kit_test.h>
#include <ironside_client/psa/crypto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint8_t g_expected[20] = {
	0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e,
	0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d,
};

static bool sha1_ok(void)
{
	uint8_t hash[20];
	size_t hash_len = 0;

	if (psa_hash_compute(PSA_ALG_SHA_1, (const uint8_t *)"abc", 3, hash, sizeof(hash),
			     &hash_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_hash_compute (SHA-1) failed\r\n");
		return false;
	}
	return hash_len == sizeof(g_expected) && memcmp(hash, g_expected, sizeof(hash)) == 0;
}

/* HMAC-SHA-1 and ECDSA over SHA-1 are refused at the algorithm gate, whatever the
 * key, so the refusal is checked with a key of the right type. */
static bool sha1_use_refused(void)
{
	static const uint8_t mac_key[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	uint8_t out[64];
	size_t out_len = 0;
	psa_key_id_t key = 0;
	bool ok = true;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);
	psa_set_key_bits(&attr, 8 * sizeof(mac_key));
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_algorithm(&attr, PSA_ALG_HMAC(PSA_ALG_SHA_1));
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);
	if (psa_import_key(&attr, mac_key, sizeof(mac_key), &key) != PSA_SUCCESS) {
		integration_kit_uart_puts("import of the HMAC key failed\r\n");
		return false;
	}
	if (psa_mac_compute(key, PSA_ALG_HMAC(PSA_ALG_SHA_1), (const uint8_t *)"abc", 3, out,
			    sizeof(out), &out_len) != PSA_ERROR_NOT_SUPPORTED) {
		integration_kit_uart_puts("HMAC-SHA-1 wrongly admitted\r\n");
		ok = false;
	}
	(void)psa_destroy_key(key);

	psa_reset_key_attributes(&attr);
	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDSA(PSA_ALG_SHA_1));
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);
	if (psa_generate_key(&attr, &key) != PSA_SUCCESS) {
		integration_kit_uart_puts("generation of the ECDSA key failed\r\n");
		return false;
	}
	if (psa_sign_message(key, PSA_ALG_ECDSA(PSA_ALG_SHA_1), (const uint8_t *)"abc", 3, out,
			     sizeof(out), &out_len) != PSA_ERROR_NOT_SUPPORTED) {
		integration_kit_uart_puts("ECDSA over SHA-1 wrongly admitted\r\n");
		ok = false;
	}
	(void)psa_destroy_key(key);
	return ok;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts("psa_sha1: SHA-1 as a plain hash via UROT PSA service\r\n");

	(void)psa_crypto_init();

	bool ok = sha1_ok();

	ok = sha1_use_refused() && ok;

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_sha1") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_sha1"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
