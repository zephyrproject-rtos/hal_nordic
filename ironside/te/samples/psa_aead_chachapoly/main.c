/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_aead_chachapoly.
 *
 * Exercises the two AEAD modes added to UROT's PSA Crypto service beyond the
 * AES-GCM the psa_aead sample already covers: ChaCha20-Poly1305 and
 * AES-256-CCM. Both are driven from the non-secure side over the NSC veneer
 * and pass UROT's NSC algorithm allowlist (both AEAD modes are in-set;
 * AES-CTR, by contrast, is not and is rejected).
 *
 * For each mode: import a volatile key, authenticated-encrypt a message with
 * associated data, decrypt + check the round-trip, then flip a ciphertext byte
 * and confirm the AEAD tag check rejects it. Console: UARTE20, TX = P1.16.
 */

#include <integration_kit_uart.h>
#include <integration_kit_test.h>
#include <ironside_client/psa/crypto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* A 256-bit key (reused for both ChaCha20 and AES - distinct key *types*). */
static const uint8_t g_key[32] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
	0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
	0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};
/* 12-byte nonce - valid for both ChaCha20-Poly1305 and AES-CCM (L=3). */
static const uint8_t g_nonce[12] = {
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab,
};
static const uint8_t g_aad[8] = { 'a', 'e', 'a', 'd', '-', 'a', 'd', '0' };
static const uint8_t g_msg[32] = {
	'I', 'r', 'o', 'n', 'S', 'i', 'd', 'e', ' ', 'T', 'Z', ' ', 'A', 'E', 'A', 'D',
	' ', 't', 'e', 's', 't', ' ', 'm', 'e', 's', 's', 'a', 'g', 'e', ' ', '#', '1',
};

static bool aead_roundtrip_ok(const char *name, psa_key_type_t type, psa_algorithm_t alg)
{
	static uint8_t ct[sizeof(g_msg) + 16]; /* ciphertext || 16-byte tag */
	static uint8_t pt[sizeof(g_msg)];
	size_t ct_len = 0;
	size_t pt_len = 0;
	psa_key_id_t key = 0;
	bool ok = true;

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, type);
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attr, alg);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	integration_kit_uart_puts(name);
	if (psa_import_key(&attr, g_key, sizeof(g_key), &key) != PSA_SUCCESS) {
		integration_kit_uart_puts(": psa_import_key failed\r\n");
		return false;
	}

	if (psa_aead_encrypt(key, alg, g_nonce, sizeof(g_nonce), g_aad, sizeof(g_aad), g_msg,
			     sizeof(g_msg), ct, sizeof(ct), &ct_len) != PSA_SUCCESS) {
		integration_kit_uart_puts(": encrypt failed\r\n");
		ok = false;
	}
	if (ok && psa_aead_decrypt(key, alg, g_nonce, sizeof(g_nonce), g_aad, sizeof(g_aad), ct,
				   ct_len, pt, sizeof(pt), &pt_len) != PSA_SUCCESS) {
		integration_kit_uart_puts(": decrypt failed\r\n");
		ok = false;
	}
	if (ok && (pt_len != sizeof(g_msg) || memcmp(pt, g_msg, sizeof(g_msg)) != 0)) {
		integration_kit_uart_puts(": round-trip mismatch\r\n");
		ok = false;
	}

	/* Tamper one ciphertext byte - the AEAD tag check must reject it. */
	if (ok) {
		ct[0] ^= 0xff;
		if (psa_aead_decrypt(key, alg, g_nonce, sizeof(g_nonce), g_aad, sizeof(g_aad), ct,
				     ct_len, pt, sizeof(pt), &pt_len) == PSA_SUCCESS) {
			integration_kit_uart_puts(": tamper wrongly accepted\r\n");
			ok = false;
		}
	}

	if (psa_destroy_key(key) != PSA_SUCCESS) {
		ok = false;
	}
	if (ok) {
		integration_kit_uart_puts(": ok (round-trip + tamper-reject)\r\n");
	}
	return ok;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts(
		"psa_aead_chachapoly: ChaCha20-Poly1305 + AES-256-CCM via UROT\r\n");

	(void)psa_crypto_init();

	bool ok = aead_roundtrip_ok("ChaCha20-Poly1305", PSA_KEY_TYPE_CHACHA20,
				    PSA_ALG_CHACHA20_POLY1305);
	ok = aead_roundtrip_ok("AES-256-CCM", PSA_KEY_TYPE_AES, PSA_ALG_CCM) && ok;

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_aead_chachapoly") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_aead_chachapoly"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
