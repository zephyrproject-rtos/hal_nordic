/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_aead.
 *
 * AES-256-GCM authenticated encrypt + decrypt over UROT's PSA Crypto service.
 * This is the operation the IronSide DFU image will use to **decrypt an
 * encrypted firmware image**: it hands UROT the ciphertext (+ nonce / associated
 * data) and a *key id*, and gets plaintext back. The decryption key never leaves
 * UROT.
 *
 *   DFU production path (not this sample - DFU code is untouched):
 *     psa_aead_decrypt(VENDOR_KEY_ID, PSA_ALG_GCM, nonce, ..., ct, ..., pt, ...)
 *   where VENDOR_KEY_ID names a persistent key provisioned into UROT. The DFU
 *   image only ever sees plaintext; the key is opaque to it.
 *
 * This sample proves the same call path on HW with a *volatile* AES key.
 * It encrypts a "firmware" blob, decrypts it back and
 * checks the round-trip, then flips one ciphertext byte and confirms decryption
 * is rejected (GCM authenticity) - exactly the protection a FW-decrypt step
 * relies on.
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

/* A 256-bit AES key. In production this is a persistent vendor key in UROT,
 * referenced by id; here it is imported volatile so the path is HW-testable. */
static const uint8_t g_key[32] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
	0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
	0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};
static const uint8_t g_nonce[12] = {
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab,
};
/* "Associated data" - e.g. a firmware header authenticated but not encrypted. */
static const uint8_t g_aad[8] = { 'F', 'W', 'h', 'd', 'r', 'v', '0', '1' };
/* The "firmware image" plaintext. */
static const uint8_t g_firmware[32] = {
	'I', 'r', 'o', 'n', 'S', 'i', 'd', 'e', ' ', 'T', 'Z', ' ', 'F', 'W', ' ', 'p',
	'a', 'y', 'l', 'o', 'a', 'd', ' ', 'b', 'l', 'o', 'c', 'k', ' ', '#', '0', '1',
};

static bool aead_roundtrip_ok(void)
{
	static uint8_t ct[sizeof(g_firmware) + 16]; /* ciphertext || 16-byte GCM tag */
	static uint8_t pt[sizeof(g_firmware)];
	size_t ct_len = 0;
	size_t pt_len = 0;
	psa_key_id_t key = 0;
	bool ok = true;

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attr, PSA_ALG_GCM);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	if (psa_import_key(&attr, g_key, sizeof(g_key), &key) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_import_key (AES-256) failed\r\n");
		return false;
	}

	/* Produce the encrypted "firmware" (stands in for what the vendor ships). */
	if (psa_aead_encrypt(key, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), g_aad, sizeof(g_aad),
			     g_firmware, sizeof(g_firmware), ct, sizeof(ct),
			     &ct_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_aead_encrypt failed\r\n");
		ok = false;
	}

	/* The DFU-equivalent step: decrypt the firmware. */
	if (ok && psa_aead_decrypt(key, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), g_aad, sizeof(g_aad),
				   ct, ct_len, pt, sizeof(pt), &pt_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_aead_decrypt (genuine) failed\r\n");
		ok = false;
	}
	if (ok &&
	    (pt_len != sizeof(g_firmware) || memcmp(pt, g_firmware, sizeof(g_firmware)) != 0)) {
		integration_kit_uart_puts("decrypted firmware mismatch\r\n");
		ok = false;
	}
	if (ok) {
		integration_kit_uart_puts("decrypt round-trip: ok\r\n");
	}

	/* Tamper one ciphertext byte - GCM authentication must reject it. */
	if (ok) {
		uint8_t saved = ct[0];

		ct[0] ^= 0xff;
		if (psa_aead_decrypt(key, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), g_aad,
				     sizeof(g_aad), ct, ct_len, pt, sizeof(pt),
				     &pt_len) == PSA_SUCCESS) {
			integration_kit_uart_puts("tampered ciphertext wrongly accepted\r\n");
			ok = false;
		} else {
			integration_kit_uart_puts("tampered ciphertext: rejected\r\n");
		}
		ct[0] = saved;
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
	integration_kit_uart_puts("psa_aead: AES-256-GCM FW decrypt via UROT PSA service\r\n");

	(void)psa_crypto_init();

	bool ok = aead_roundtrip_ok();

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_aead") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_aead"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
