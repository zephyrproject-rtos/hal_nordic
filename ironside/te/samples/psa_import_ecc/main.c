/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_import_ecc.
 *
 * Imports a *user* ECC secp256r1 key pair into UROT with psa_import_key() and
 * exercises the asymmetric-signature path: sign a message with
 * ECDSA-SHA-256 (psa_sign_message), verify it back (psa_verify_message), and
 * export the public key (psa_export_public_key) - the only key material that may
 * leave UROT. Proves the KEY + ASYM_SIGN modules end-to-end on HW.
 *
 * The key is *volatile* (default lifetime): a RAM key slot, gone on reset.
 *
 * ECDSA is randomized, so there is no fixed signature vector - the test is a
 * sign/verify round-trip (which validates both directions) plus a negative
 * check that a tampered message fails verification.
 *
 * Private key (RFC 6979 A.2.5, P-256, a valid secp256r1 scalar):
 *   C9AFA9D8 45BA7516 6B5C2157 67B1D693 4E50C3DB 36E89B12 7B8A622B 120F6721
 *
 * Console: UARTE20, TX = P1.16, 115200 8N1 (same as hello_world).
 */

#include <integration_kit_uart.h>
#include <integration_kit_test.h>
#include <ironside_client/psa/crypto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* secp256r1 private scalar (32 bytes, big-endian) - RFC 6979 A.2.5. */
static const uint8_t g_priv[32] = {
	0xC9, 0xAF, 0xA9, 0xD8, 0x45, 0xBA, 0x75, 0x16, 0x6B, 0x5C, 0x21,
	0x57, 0x67, 0xB1, 0xD6, 0x93, 0x4E, 0x50, 0xC3, 0xDB, 0x36, 0xE8,
	0x9B, 0x12, 0x7B, 0x8A, 0x62, 0x2B, 0x12, 0x0F, 0x67, 0x21,
};
static const char g_msg[] = "IronSide TE psa_import_ecc";

static bool import_sign_verify_ok(void)
{
	static uint8_t sig[64]; /* P-256 ECDSA r||s */
	static uint8_t pub[65]; /* uncompressed SEC1 point 0x04||X||Y */
	size_t sig_len = 0;
	size_t pub_len = 0;
	psa_key_id_t key = 0;
	bool ok = true;
	const psa_algorithm_t alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256);
	const size_t msg_len = sizeof(g_msg) - 1;

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_algorithm(&attr, alg);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	if (psa_import_key(&attr, g_priv, sizeof(g_priv), &key) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_import_key (ECC) failed\r\n");
		return false;
	}

	/* Export the public key - public material is allowed to leave UROT. */
	if (psa_export_public_key(key, pub, sizeof(pub), &pub_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_export_public_key failed\r\n");
		ok = false;
	} else {
		integration_kit_uart_puts("public key: ");
		integration_kit_uart_hexdump(pub, pub_len);
		integration_kit_uart_puts("\r\n");
	}

	/* Sign the message. */
	if (psa_sign_message(key, alg, (const uint8_t *)g_msg, msg_len, sig, sizeof(sig),
			     &sig_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_sign_message failed\r\n");
		ok = false;
	} else {
		integration_kit_uart_puts("signature: ");
		integration_kit_uart_hexdump(sig, sig_len);
		integration_kit_uart_puts("\r\n");
	}

	/* Verify the genuine signature - must pass. */
	if (psa_verify_message(key, alg, (const uint8_t *)g_msg, msg_len, sig, sig_len) !=
	    PSA_SUCCESS) {
		integration_kit_uart_puts("psa_verify_message (genuine) failed\r\n");
		ok = false;
	} else {
		integration_kit_uart_puts("verify genuine: ok\r\n");
	}

	/* Verify a tampered message - must fail (negative check). */
	if (psa_verify_message(key, alg, (const uint8_t *)"tampered", 8, sig, sig_len) ==
	    PSA_SUCCESS) {
		integration_kit_uart_puts("psa_verify_message (tampered) wrongly succeeded\r\n");
		ok = false;
	} else {
		integration_kit_uart_puts("verify tampered: rejected\r\n");
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
		"psa_import_ecc: import secp256r1 + ECDSA sign/verify via UROT PSA service\r\n");

	(void)psa_crypto_init();

	bool ok = import_sign_verify_ok();

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_import_ecc") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_import_ecc"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
