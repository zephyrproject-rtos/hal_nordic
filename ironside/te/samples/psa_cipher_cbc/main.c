/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_cipher_cbc.
 *
 * Exercises AES-256-CBC (PKCS#7 padding), the one cipher not covered
 * by the other PSA samples (GCM -> psa_aead, CCM/ChaCha20-Poly1305 ->
 * psa_aead_chachapoly). Imports a volatile AES-256 key bound to PSA_ALG_CBC_PKCS7
 * and does a psa_cipher_encrypt -> psa_cipher_decrypt round-trip over the NSC
 * veneer, confirming CBC is reachable from non-secure and admitted by UROT's
 * allowlist.
 *
 * Note: CBC is a confidentiality-only mode with no authentication - the standard
 * single-shot PSA API generates a random IV and prepends it to the ciphertext on
 * encrypt, and consumes it on decrypt. A non-block-aligned (20-byte) plaintext is
 * used so PKCS#7 padding is exercised. For authenticated data-at-rest use an AEAD
 * (see psa_aead / psa_mext); this sample just proves the CBC path.
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

static const uint8_t g_key[32] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
	0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
	0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};
/* 20 bytes - deliberately not a block multiple, so PKCS#7 padding is exercised. */
static const uint8_t g_msg[20] = {
	'I', 'r', 'o', 'n', 'S', 'i', 'd', 'e', '-', 'T',
	'Z', ' ', 'C', 'B', 'C', ' ', 't', 'e', 's', 't',
};

static bool cbc_roundtrip_ok(void)
{
	/* Single-shot CBC output = 16-byte IV || ciphertext padded up to the next
	 * 16-byte block; a 20-byte message pads to 32, so at most 16 + 32 bytes. */
	static uint8_t ct[16 + sizeof(g_msg) + 16];
	static uint8_t pt[sizeof(g_msg) + 16];
	size_t ct_len = 0;
	size_t pt_len = 0;
	psa_key_id_t key = 0;
	bool ok = true;

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attr, PSA_ALG_CBC_PKCS7);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	if (psa_import_key(&attr, g_key, sizeof(g_key), &key) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_import_key (AES-256 CBC) failed\r\n");
		return false;
	}

	if (psa_cipher_encrypt(key, PSA_ALG_CBC_PKCS7, g_msg, sizeof(g_msg), ct, sizeof(ct),
			       &ct_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_cipher_encrypt failed\r\n");
		ok = false;
	}
	if (ok && psa_cipher_decrypt(key, PSA_ALG_CBC_PKCS7, ct, ct_len, pt, sizeof(pt), &pt_len) !=
			  PSA_SUCCESS) {
		integration_kit_uart_puts("psa_cipher_decrypt failed\r\n");
		ok = false;
	}
	if (ok && (pt_len != sizeof(g_msg) || memcmp(pt, g_msg, sizeof(g_msg)) != 0)) {
		integration_kit_uart_puts("CBC round-trip mismatch\r\n");
		ok = false;
	}
	if (ok) {
		integration_kit_uart_puts("AES-256-CBC (PKCS7) round-trip: ok\r\n");
	}

	if (psa_destroy_key(key) != PSA_SUCCESS) {
		ok = false;
	}
	return ok;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts("psa_cipher_cbc: AES-256-CBC (PKCS7) via UROT PSA service\r\n");

	(void)psa_crypto_init();

	bool ok = cbc_roundtrip_ok();

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_cipher_cbc") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_cipher_cbc"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
