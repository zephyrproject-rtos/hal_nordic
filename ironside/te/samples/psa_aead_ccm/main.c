/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_aead_ccm.
 *
 * Multi-part AES-256-CCM over UROT's PSA Crypto service: how to authenticate-
 * encrypt and decrypt a message that arrives in pieces - a stream of radio
 * packets, a firmware image read one page at a time - without holding all of
 * it in RAM at once.
 *
 * Nothing secret is hardcoded. The key is generated inside UROT with
 * psa_generate_key(), so the App only ever holds a key id, and the nonce comes
 * from psa_aead_generate_nonce(), which is the safe way to get one: CCM is
 * broken outright by reusing a nonce with the same key.
 *
 * The one rule CCM adds to the generic AEAD flow is psa_aead_set_lengths():
 * CCM encodes the associated-data and payload lengths into its first MAC
 * block, so it has to be told both *before* any data is fed in. AES-GCM has no
 * such requirement, which is why psa_aead does not call it.
 *
 * For the single-shot form (psa_aead_encrypt / psa_aead_decrypt in one call)
 * see psa_aead (AES-GCM) and psa_aead_chachapoly (CCM and ChaCha20-Poly1305).
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

#define AEAD_ALG PSA_ALG_CCM
#define KEY_BITS 256u

/* The payload is fed in chunks of this size. 40 bytes of message over 16-byte
 * chunks leaves a short final chunk, which is what a real stream looks like. */
#define CHUNK_LEN 16u

/* Associated data: authenticated but not encrypted - a header the receiver
 * needs in the clear, yet must not be able to alter. */
static const uint8_t g_aad[8] = { 'c', 'c', 'm', '-', 'h', 'd', 'r', '1' };
static const uint8_t g_msg[40] = {
	'I', 'r', 'o', 'n', 'S', 'i', 'd', 'e', ' ', 'm', 'u', 'l', 't', 'i',
	'-', 'p', 'a', 'r', 't', ' ', 'C', 'C', 'M', ' ', 'p', 'a', 'y', 'l',
	'o', 'a', 'd', ' ', 'b', 'l', 'o', 'c', 'k', ' ', '0', '1',
};

/* Sized from the PSA macros rather than from constants, so the buffers stay
 * right if AEAD_ALG or the message length changes. psa_aead_update() may emit
 * up to a whole block more than it was handed, hence the round-up. */
static uint8_t g_nonce[PSA_AEAD_NONCE_LENGTH(PSA_KEY_TYPE_AES, AEAD_ALG)];
static uint8_t g_tag[PSA_AEAD_TAG_LENGTH(PSA_KEY_TYPE_AES, KEY_BITS, AEAD_ALG)];
static uint8_t g_ct[PSA_AEAD_UPDATE_OUTPUT_SIZE(PSA_KEY_TYPE_AES, AEAD_ALG, sizeof(g_msg))];
static uint8_t g_pt[sizeof(g_ct)];
static size_t g_nonce_len;

/* Static rather than automatic buffers above keep the App's stack small. */

static size_t chunk_at(size_t off, size_t total)
{
	return (total - off < CHUNK_LEN) ? total - off : CHUNK_LEN;
}

/*
 * A volatile AES-256 key made by the CRACEN DRBG inside UROT. The material
 * never exists in non-secure memory; a real product would instead name a key
 * provisioned into the CRACEN KMU by id (see psa_persistent_key).
 */
static psa_key_id_t generate_key(void)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key = 0;

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, KEY_BITS);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attr, AEAD_ALG);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	if (psa_generate_key(&attr, &key) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_generate_key failed\r\n");
		return 0;
	}
	return key;
}

/* Encrypt g_msg chunk by chunk into g_ct, with the tag in g_tag. */
static bool encrypt_stream(psa_key_id_t key, size_t *ct_len)
{
	psa_aead_operation_t op = PSA_AEAD_OPERATION_INIT;
	size_t total = 0;
	size_t out_len = 0;
	size_t tag_len = 0;
	bool ok = false;

	ok = psa_aead_encrypt_setup(&op, key, AEAD_ALG) == PSA_SUCCESS;
	/* CCM only: both lengths, before any data. */
	ok = ok && psa_aead_set_lengths(&op, sizeof(g_aad), sizeof(g_msg)) == PSA_SUCCESS;
	/* The nonce is not secret and travels with the ciphertext, but it must be
	 * fresh for every message under this key. */
	ok = ok &&
	     psa_aead_generate_nonce(&op, g_nonce, sizeof(g_nonce), &g_nonce_len) == PSA_SUCCESS;
	ok = ok && psa_aead_update_ad(&op, g_aad, sizeof(g_aad)) == PSA_SUCCESS;

	for (size_t off = 0; ok && off < sizeof(g_msg); off += CHUNK_LEN) {
		ok = psa_aead_update(&op, g_msg + off, chunk_at(off, sizeof(g_msg)), g_ct + total,
				     sizeof(g_ct) - total, &out_len) == PSA_SUCCESS;
		total += out_len;
	}

	/* finish() flushes whatever the last update() still held back and emits
	 * the authentication tag. */
	ok = ok && psa_aead_finish(&op, g_ct + total, sizeof(g_ct) - total, &out_len, g_tag,
				   sizeof(g_tag), &tag_len) == PSA_SUCCESS;
	total += out_len;

	if (!ok) {
		/* Any failure leaves the operation live - abort frees it. */
		(void)psa_aead_abort(&op);
		integration_kit_uart_puts("multi-part encrypt failed\r\n");
		return false;
	}
	if (total != sizeof(g_msg) || tag_len != sizeof(g_tag)) {
		integration_kit_uart_puts("unexpected ciphertext or tag length\r\n");
		return false;
	}

	*ct_len = total;
	return true;
}

/* Decrypt g_ct chunk by chunk into g_pt and check the tag. */
static bool decrypt_stream(psa_key_id_t key, size_t ct_len)
{
	psa_aead_operation_t op = PSA_AEAD_OPERATION_INIT;
	size_t total = 0;
	size_t out_len = 0;
	bool ok = false;

	ok = psa_aead_decrypt_setup(&op, key, AEAD_ALG) == PSA_SUCCESS;
	ok = ok && psa_aead_set_lengths(&op, sizeof(g_aad), ct_len) == PSA_SUCCESS;
	/* The nonce the sender used, and the same associated data - a different
	 * header would fail the tag check below. */
	ok = ok && psa_aead_set_nonce(&op, g_nonce, g_nonce_len) == PSA_SUCCESS;
	ok = ok && psa_aead_update_ad(&op, g_aad, sizeof(g_aad)) == PSA_SUCCESS;

	for (size_t off = 0; ok && off < ct_len; off += CHUNK_LEN) {
		ok = psa_aead_update(&op, g_ct + off, chunk_at(off, ct_len), g_pt + total,
				     sizeof(g_pt) - total, &out_len) == PSA_SUCCESS;
		total += out_len;
	}

	/* verify() is what authenticates the message. Plaintext handed out by
	 * update() before this point is unverified: do not act on it until
	 * verify() has returned PSA_SUCCESS. */
	ok = ok && psa_aead_verify(&op, g_pt + total, sizeof(g_pt) - total, &out_len, g_tag,
				   sizeof(g_tag)) == PSA_SUCCESS;
	total += out_len;

	if (!ok) {
		(void)psa_aead_abort(&op);
		integration_kit_uart_puts("multi-part decrypt/verify failed\r\n");
		return false;
	}
	if (total != sizeof(g_msg) || memcmp(g_pt, g_msg, sizeof(g_msg)) != 0) {
		integration_kit_uart_puts("plaintext mismatch\r\n");
		return false;
	}
	return true;
}

int main(void)
{
	size_t ct_len = 0;
	psa_key_id_t key = PSA_KEY_ID_NULL;
	bool ok = false;

	integration_kit_uart_init();
	integration_kit_uart_puts("psa_aead_ccm: multi-part AES-256-CCM via UROT\r\n");

	(void)psa_crypto_init();

	key = generate_key();
	ok = key != 0;

	ok = ok && encrypt_stream(key, &ct_len);
	if (ok) {
		integration_kit_uart_puts("nonce: ");
		integration_kit_uart_hexdump(g_nonce, g_nonce_len);
		integration_kit_uart_puts("\r\ntag:   ");
		integration_kit_uart_hexdump(g_tag, sizeof(g_tag));
		integration_kit_uart_puts("\r\n");
	}

	ok = ok && decrypt_stream(key, ct_len);
	if (ok) {
		integration_kit_uart_puts("multi-part round-trip: ok\r\n");
	}

	if (key != 0 && psa_destroy_key(key) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_destroy_key failed\r\n");
		ok = false;
	}

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_aead_ccm") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_aead_ccm"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
