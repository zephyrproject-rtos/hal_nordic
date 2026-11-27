/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_ecdsa_deterministic.
 *
 * Deterministic ECDSA (RFC 6979) on secp256r1 with SHA-256, checked against the
 * RFC 6979 A.2.5 signature for the message "sample": the signature must match
 * byte for byte, which randomized ECDSA cannot do, and must verify.
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

/* secp256r1 private scalar - RFC 6979 A.2.5. */
static const uint8_t g_priv[32] = {
	0xC9, 0xAF, 0xA9, 0xD8, 0x45, 0xBA, 0x75, 0x16, 0x6B, 0x5C, 0x21,
	0x57, 0x67, 0xB1, 0xD6, 0x93, 0x4E, 0x50, 0xC3, 0xDB, 0x36, 0xE8,
	0x9B, 0x12, 0x7B, 0x8A, 0x62, 0x2B, 0x12, 0x0F, 0x67, 0x21,
};
/* r || s for SHA-256 and the message "sample" - RFC 6979 A.2.5. */
static const uint8_t g_expected_sig[64] = {
	0xEF, 0xD4, 0x8B, 0x2A, 0xAC, 0xB6, 0xA8, 0xFD, 0x11, 0x40, 0xDD, 0x9C, 0xD4,
	0x5E, 0x81, 0xD6, 0x9D, 0x2C, 0x87, 0x7B, 0x56, 0xAA, 0xF9, 0x91, 0xC3, 0x4D,
	0x0E, 0xA8, 0x4E, 0xAF, 0x37, 0x16, 0xF7, 0xCB, 0x1C, 0x94, 0x2D, 0x65, 0x7C,
	0x41, 0xD4, 0x36, 0xC7, 0xA1, 0xB6, 0xE2, 0x9F, 0x65, 0xF3, 0xE9, 0x00, 0xDB,
	0xB9, 0xAF, 0xF4, 0x06, 0x4D, 0xC4, 0xAB, 0x2F, 0x84, 0x3A, 0xCD, 0xA8,
};

static bool deterministic_sign_ok(void)
{
	uint8_t sig[64];
	size_t sig_len = 0;
	psa_key_id_t key = 0;
	bool ok = false;
	const psa_algorithm_t alg = PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256);
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_algorithm(&attr, alg);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	if (psa_import_key(&attr, g_priv, sizeof(g_priv), &key) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_import_key (P-256) failed\r\n");
		return false;
	}

	if (psa_sign_message(key, alg, (const uint8_t *)"sample", 6, sig, sizeof(sig), &sig_len) !=
	    PSA_SUCCESS) {
		integration_kit_uart_puts("deterministic psa_sign_message failed\r\n");
	} else if (sig_len != sizeof(g_expected_sig) ||
		   memcmp(sig, g_expected_sig, sizeof(sig)) != 0) {
		integration_kit_uart_puts("signature differs from RFC 6979 A.2.5: ");
		integration_kit_uart_hexdump(sig, sig_len);
		integration_kit_uart_puts("\r\n");
	} else if (psa_verify_message(key, alg, (const uint8_t *)"sample", 6, sig, sig_len) !=
		   PSA_SUCCESS) {
		integration_kit_uart_puts("verify of the deterministic signature failed\r\n");
	} else {
		ok = true;
	}

	(void)psa_destroy_key(key);
	return ok;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts(
		"psa_ecdsa_deterministic: RFC 6979 ECDSA P-256 via UROT PSA service\r\n");

	(void)psa_crypto_init();

	bool ok = deterministic_sign_ok();

	integration_kit_uart_puts(
		ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_ecdsa_deterministic") :
		     INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_ecdsa_deterministic"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
