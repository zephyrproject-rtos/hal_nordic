/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_ecdsa_p384.
 *
 * The P384 counterpart of psa_import_ecc: imports a *user* ECC secp384r1 key
 * pair into UROT with psa_import_key() and exercises the asymmetric-signature
 * path on the larger curve - sign a message with ECDSA-SHA-384
 * (psa_sign_message), verify it back (psa_verify_message), and export the
 * public key (psa_export_public_key). Proves the ECDSA P384 addition (secp384r1
 * curve + SHA-384) end-to-end on HW over the NSC veneer.
 *
 * The key is *volatile* (default lifetime): a RAM key slot, gone on reset.
 *
 * ECDSA is randomized, so there is no fixed signature vector - the test is a
 * sign/verify round-trip (which validates both directions) plus a negative
 * check that a tampered message fails verification.
 *
 * Private key (RFC 6979 A.2.6, P-384, a valid secp384r1 scalar):
 *   6B9D3DAD 2E1B8C1C 05B19875 B6659F4D E23C3B66 7BF297BA
 *   9AA47740 787137D8 96D5724E 4C70A825 F872C9EA 60D2EDF5
 *
 * Console: UARTE20, TX = P1.16, 115200 8N1 (same as hello_world).
 */

#include <integration_kit_uart.h>
#include <integration_kit_test.h>
#include <ironside_client/psa/crypto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* secp384r1 private scalar (48 bytes, big-endian) - RFC 6979 A.2.6. */
static const uint8_t g_priv[48] = {
	0x6B, 0x9D, 0x3D, 0xAD, 0x2E, 0x1B, 0x8C, 0x1C, 0x05, 0xB1, 0x98, 0x75,
	0xB6, 0x65, 0x9F, 0x4D, 0xE2, 0x3C, 0x3B, 0x66, 0x7B, 0xF2, 0x97, 0xBA,
	0x9A, 0xA4, 0x77, 0x40, 0x78, 0x71, 0x37, 0xD8, 0x96, 0xD5, 0x72, 0x4E,
	0x4C, 0x70, 0xA8, 0x25, 0xF8, 0x72, 0xC9, 0xEA, 0x60, 0xD2, 0xED, 0xF5,
};
static const char g_msg[] = "IronSide TE psa_ecdsa_p384";

static bool import_sign_verify_ok(void)
{
	static uint8_t sig[96]; /* P-384 ECDSA r||s (48 + 48) */
	static uint8_t pub[97]; /* uncompressed SEC1 point 0x04||X||Y (1 + 48 + 48) */
	size_t sig_len = 0;
	size_t pub_len = 0;
	psa_key_id_t key = 0;
	bool ok = true;
	const psa_algorithm_t alg = PSA_ALG_ECDSA(PSA_ALG_SHA_384);
	const size_t msg_len = sizeof(g_msg) - 1;

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attr, 384);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_algorithm(&attr, alg);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	if (psa_import_key(&attr, g_priv, sizeof(g_priv), &key) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_import_key (ECC P384) failed\r\n");
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
		"psa_ecdsa_p384: import secp384r1 + ECDSA-SHA-384 sign/verify via UROT PSA service\r\n");

	(void)psa_crypto_init();

	bool ok = import_sign_verify_ok();

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_ecdsa_p384") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_ecdsa_p384"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
