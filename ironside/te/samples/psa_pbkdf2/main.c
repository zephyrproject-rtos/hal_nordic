/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_pbkdf2.
 *
 * PBKDF2 through the standard PSA key-derivation API, with the password held as a
 * PSA_KEY_TYPE_PASSWORD key:
 *   1. PBKDF2-HMAC-SHA-256 against the RFC 7914 section 11 vector (1 iteration)
 *   2. PBKDF2-HMAC-SHA-256 at 10000 iterations, a call of a few seconds
 *   3. PBKDF2-AES-CMAC-PRF-128 at 16384 iterations, the Thread PSKc derivation,
 *      against a host-computed vector; the driver takes a password of exactly 16 bytes
 *   4. the password key cannot be exported, and PBKDF2 over SHA-1 is refused
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

/* PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_1) - not in the supported surface. */
#define ALG_PBKDF2_HMAC_SHA1_UNSUPPORTED ((psa_algorithm_t)0x08800105)

static const uint8_t g_rfc7914_dk[64] = {
	0x55, 0xac, 0x04, 0x6e, 0x56, 0xe3, 0x08, 0x9f, 0xec, 0x16, 0x91, 0xc2, 0x25,
	0x44, 0xb6, 0x05, 0xf9, 0x41, 0x85, 0x21, 0x6d, 0xde, 0x04, 0x65, 0xe6, 0x8b,
	0x9d, 0x57, 0xc2, 0x0d, 0xac, 0xbc, 0x49, 0xca, 0x9c, 0xcc, 0xf1, 0x79, 0xb6,
	0x45, 0x99, 0x16, 0x64, 0xb3, 0x9d, 0x77, 0xef, 0x31, 0x7c, 0x71, 0xb8, 0x45,
	0xb1, 0xe3, 0x0b, 0xd5, 0x09, 0x11, 0x20, 0x41, 0xd3, 0xa1, 0x97, 0x83,
};
/* PBKDF2-HMAC-SHA-256, password "00000000", salt "SPAKE2P Key Salt", 10000 iterations. */
static const uint8_t g_matter_dk[32] = {
	0xf7, 0xab, 0xf0, 0x80, 0xc8, 0xd9, 0xf9, 0xbe, 0x2c, 0x14, 0xb5,
	0x3e, 0xe8, 0x5a, 0x77, 0x95, 0x27, 0x43, 0x91, 0x25, 0x42, 0xcd,
	0xe9, 0x17, 0x33, 0x4c, 0x28, 0x62, 0x03, 0x14, 0x7d, 0x92,
};
/* PBKDF2-AES-CMAC-PRF-128, password "J01NME-passphras", salt 0102030405060708 "Thread",
 * 16384 iterations (the Thread PSKc count, about 2.5 s on the slowest part). */
static const uint8_t g_cmac_salt[14] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
					 0x08, 'T',  'h',  'r',  'e',  'a',  'd' };
static const uint8_t g_cmac_dk[16] = {
	0x5b, 0xe4, 0xaa, 0xf1, 0xdd, 0xc3, 0x40, 0x61,
	0x1d, 0x0e, 0x2e, 0x27, 0x07, 0xdf, 0x86, 0xcf,
};

static bool import_password(const char *pw, psa_algorithm_t alg, psa_key_id_t *key)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	size_t len = strlen(pw);

	psa_set_key_type(&attr, PSA_KEY_TYPE_PASSWORD);
	psa_set_key_bits(&attr, 8 * len);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, alg);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	return psa_import_key(&attr, (const uint8_t *)pw, len, key) == PSA_SUCCESS;
}

static bool pbkdf2_matches(psa_algorithm_t alg, const char *pw, const uint8_t *salt,
			   size_t salt_len, uint32_t iterations, const uint8_t *expected,
			   size_t dk_len)
{
	static uint8_t dk[64];
	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;
	psa_key_id_t key = 0;
	bool ok = false;

	if (!import_password(pw, alg, &key)) {
		integration_kit_uart_puts("import of the password key failed\r\n");
		return false;
	}

	if (psa_key_derivation_setup(&op, alg) == PSA_SUCCESS &&
	    psa_key_derivation_input_integer(&op, PSA_KEY_DERIVATION_INPUT_COST, iterations) ==
		    PSA_SUCCESS &&
	    psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_SALT, salt, salt_len) ==
		    PSA_SUCCESS &&
	    psa_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_PASSWORD, key) ==
		    PSA_SUCCESS &&
	    psa_key_derivation_output_bytes(&op, dk, dk_len) == PSA_SUCCESS) {
		ok = memcmp(dk, expected, dk_len) == 0;
	}

	(void)psa_key_derivation_abort(&op);
	(void)psa_destroy_key(key);
	return ok;
}

static bool password_key_guarded(void)
{
	uint8_t out[16];
	size_t out_len = 0;
	psa_key_id_t key = 0;
	bool ok = true;

	if (!import_password("passwd", PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_256), &key)) {
		integration_kit_uart_puts("import of the password key failed\r\n");
		return false;
	}
	if (psa_export_key(key, out, sizeof(out), &out_len) == PSA_SUCCESS) {
		integration_kit_uart_puts("password key was exported\r\n");
		ok = false;
	}
	(void)psa_destroy_key(key);

	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;

	if (psa_key_derivation_setup(&op, ALG_PBKDF2_HMAC_SHA1_UNSUPPORTED) !=
	    PSA_ERROR_NOT_SUPPORTED) {
		integration_kit_uart_puts("PBKDF2 over SHA-1 wrongly admitted\r\n");
		ok = false;
	}
	(void)psa_key_derivation_abort(&op);
	return ok;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts(
		"psa_pbkdf2: PBKDF2 from a password key via UROT PSA service\r\n");

	(void)psa_crypto_init();

	bool ok = pbkdf2_matches(PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_256), "passwd",
				 (const uint8_t *)"salt", 4, 1, g_rfc7914_dk, sizeof(g_rfc7914_dk));

	if (!ok) {
		integration_kit_uart_puts("PBKDF2-HMAC-SHA-256 differs from RFC 7914\r\n");
	}

	bool matter_ok = pbkdf2_matches(PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_256), "00000000",
					(const uint8_t *)"SPAKE2P Key Salt", 16, 10000, g_matter_dk,
					sizeof(g_matter_dk));

	if (!matter_ok) {
		integration_kit_uart_puts("PBKDF2-HMAC-SHA-256 at 10000 iterations failed\r\n");
	}
	ok = ok && matter_ok;

	bool cmac_ok = pbkdf2_matches(PSA_ALG_PBKDF2_AES_CMAC_PRF_128, "J01NME-passphras",
				      g_cmac_salt, sizeof(g_cmac_salt), 16384, g_cmac_dk,
				      sizeof(g_cmac_dk));

	if (!cmac_ok) {
		integration_kit_uart_puts(
			"PBKDF2-AES-CMAC-PRF-128 differs from the host vector\r\n");
	}
	ok = ok && cmac_ok;

	ok = password_key_guarded() && ok;

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_pbkdf2") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_pbkdf2"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
