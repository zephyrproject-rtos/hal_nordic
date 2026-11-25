/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_ed25519.
 *
 * PureEdDSA over Ed25519 (RFC 8032) - the third signature scheme on UROT's NSC
 * surface, alongside psa_import_ecc (ECDSA P256) and psa_ecdsa_p384. Imports a
 * *user* Ed25519 key pair into UROT, signs and verifies with PSA_ALG_PURE_EDDSA,
 * then repeats the verify with the public key imported on its own - the shape a
 * device uses to check a vendor's signature, and the same primitive IROT/UROT
 * verify images with.
 *
 * Both keys are *volatile* (default lifetime): RAM key slots, gone on reset.
 *
 * Unlike ECDSA, EdDSA is deterministic: one message and one key give exactly one
 * signature. So this is a known-answer test against RFC 8032 sec. 7.1 TEST 2, not
 * just a round-trip - the public key CRACEN derives from the seed and the
 * signature it produces are both compared byte-for-byte with the RFC, which a
 * round-trip alone would not catch if CRACEN and the standard disagreed.
 *
 *   secret key  4ccd089b 28ff96da 9db6c346 ec114e0f 5b8a319f 35aba624 da8cf6ed 4fb8a6fb
 *   public key  3d4017c3 e843895a 92b70aa7 4d1b7ebc 9c982ccf 2ec4968c c0cd55f1 2af4660c
 *   message     72
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

/* RFC 8032 sec. 7.1 TEST 2 - the 32-byte seed is what PSA imports as the key pair. */
static const uint8_t g_priv[32] = {
	0x4c, 0xcd, 0x08, 0x9b, 0x28, 0xff, 0x96, 0xda, 0x9d, 0xb6, 0xc3,
	0x46, 0xec, 0x11, 0x4e, 0x0f, 0x5b, 0x8a, 0x31, 0x9f, 0x35, 0xab,
	0xa6, 0x24, 0xda, 0x8c, 0xf6, 0xed, 0x4f, 0xb8, 0xa6, 0xfb,
};
static const uint8_t g_pub[32] = {
	0x3d, 0x40, 0x17, 0xc3, 0xe8, 0x43, 0x89, 0x5a, 0x92, 0xb7, 0x0a,
	0xa7, 0x4d, 0x1b, 0x7e, 0xbc, 0x9c, 0x98, 0x2c, 0xcf, 0x2e, 0xc4,
	0x96, 0x8c, 0xc0, 0xcd, 0x55, 0xf1, 0x2a, 0xf4, 0x66, 0x0c,
};
static const uint8_t g_msg[1] = { 0x72 };
static const uint8_t g_sig[64] = {
	0x92, 0xa0, 0x09, 0xa9, 0xf0, 0xd4, 0xca, 0xb8, 0x72, 0x0e, 0x82, 0x0b, 0x5f,
	0x64, 0x25, 0x40, 0xa2, 0xb2, 0x7b, 0x54, 0x16, 0x50, 0x3f, 0x8f, 0xb3, 0x76,
	0x22, 0x23, 0xeb, 0xdb, 0x69, 0xda, 0x08, 0x5a, 0xc1, 0xe4, 0x3e, 0x15, 0x99,
	0x6e, 0x45, 0x8f, 0x36, 0x13, 0xd0, 0xf1, 0x1d, 0x8c, 0x38, 0x7b, 0x2e, 0xae,
	0xb4, 0x30, 0x2a, 0xee, 0xb0, 0x0d, 0x29, 0x16, 0x12, 0xbb, 0x0c, 0x00,
};

/*
 * Ed25519's curve size in PSA is 255 bits, not 256: the encoded key is 32 bytes,
 * but the scalar field is 2^255-19 and psa_set_key_bits() names the field.
 */
#define ED25519_KEY_BITS 255

static bool import_key(const uint8_t *material, size_t material_len, psa_key_type_t type,
		       psa_key_usage_t usage, psa_key_id_t *key)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, type);
	psa_set_key_bits(&attr, ED25519_KEY_BITS);
	psa_set_key_usage_flags(&attr, usage);
	psa_set_key_algorithm(&attr, PSA_ALG_PURE_EDDSA);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	return psa_import_key(&attr, material, material_len, key) == PSA_SUCCESS;
}

static bool sign_verify_ok(void)
{
	static uint8_t sig[64]; /* Ed25519 R||S */
	static uint8_t pub[32]; /* compressed Edwards point - no SEC1 0x04 prefix */
	size_t sig_len = 0;
	size_t pub_len = 0;
	psa_key_id_t key = 0;
	psa_key_id_t pub_key = 0;
	bool ok = true;

	if (!import_key(g_priv, sizeof(g_priv),
			PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS),
			PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE, &key)) {
		integration_kit_uart_puts("psa_import_key (Ed25519 pair) failed\r\n");
		return false;
	}

	/* The public key CRACEN derives from the seed must be the RFC's. */
	if (psa_export_public_key(key, pub, sizeof(pub), &pub_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_export_public_key failed\r\n");
		ok = false;
	} else if (pub_len != sizeof(g_pub) || memcmp(pub, g_pub, sizeof(g_pub)) != 0) {
		integration_kit_uart_puts("public key: mismatch vs RFC 8032\r\n");
		ok = false;
	} else {
		integration_kit_uart_puts("public key: matches RFC 8032 TEST 2\r\n");
	}

	/* EdDSA is deterministic, so the signature must match the RFC's exactly. */
	if (psa_sign_message(key, PSA_ALG_PURE_EDDSA, g_msg, sizeof(g_msg), sig, sizeof(sig),
			     &sig_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_sign_message failed\r\n");
		ok = false;
	} else if (sig_len != sizeof(g_sig) || memcmp(sig, g_sig, sizeof(g_sig)) != 0) {
		integration_kit_uart_puts("signature: mismatch vs RFC 8032\r\n");
		ok = false;
	} else {
		integration_kit_uart_puts("signature: matches RFC 8032 TEST 2\r\n");
	}

	if (psa_verify_message(key, PSA_ALG_PURE_EDDSA, g_msg, sizeof(g_msg), g_sig,
			       sizeof(g_sig)) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_verify_message (genuine) failed\r\n");
		ok = false;
	} else {
		integration_kit_uart_puts("verify genuine: ok\r\n");
	}

	/* Negative check: a different message under the same signature must fail. */
	if (psa_verify_message(key, PSA_ALG_PURE_EDDSA, (const uint8_t *)"tampered", 8, g_sig,
			       sizeof(g_sig)) == PSA_SUCCESS) {
		integration_kit_uart_puts("psa_verify_message (tampered) wrongly succeeded\r\n");
		ok = false;
	} else {
		integration_kit_uart_puts("verify tampered: rejected\r\n");
	}

	/*
	 * Verify once more against a *public-key-only* import: the verifier's
	 * half of the scheme, where the device holds no secret at all.
	 */
	if (!import_key(g_pub, sizeof(g_pub),
			PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS),
			PSA_KEY_USAGE_VERIFY_MESSAGE, &pub_key)) {
		integration_kit_uart_puts("psa_import_key (Ed25519 public) failed\r\n");
		ok = false;
	} else {
		if (psa_verify_message(pub_key, PSA_ALG_PURE_EDDSA, g_msg, sizeof(g_msg), g_sig,
				       sizeof(g_sig)) != PSA_SUCCESS) {
			integration_kit_uart_puts("verify with public key: failed\r\n");
			ok = false;
		} else {
			integration_kit_uart_puts("verify with public key: ok\r\n");
		}
		if (psa_destroy_key(pub_key) != PSA_SUCCESS) {
			integration_kit_uart_puts("psa_destroy_key (public) failed\r\n");
			ok = false;
		}
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
		"psa_ed25519: Ed25519 sign/verify against RFC 8032 via UROT PSA service\r\n");

	(void)psa_crypto_init();

	bool ok = sign_verify_ok();

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_ed25519") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_ed25519"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
