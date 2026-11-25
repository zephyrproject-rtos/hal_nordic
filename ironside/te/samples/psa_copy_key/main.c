/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_copy_key.
 *
 * psa_copy_key() on an AES-GCM key, the way Matter and Aliro use it to turn a key
 * that proved itself in a session into a stored one:
 *   1. a volatile source with the COPY usage is copied to a volatile key whose
 *      usage is narrower; the copy encrypts to the same ciphertext as the source
 *      and refuses to decrypt
 *   2. the same source is copied to a persistent user key, which then works by id
 *      and is destroyed again
 *   3. a source without the COPY usage cannot be copied
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

/* A USER key id outside the range other samples and tests use. */
#define STORED_KEY_ID ((psa_key_id_t)91)

static const uint8_t g_key[16] = { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
				   0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f };
static const uint8_t g_nonce[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
static const uint8_t g_msg[16] = { 'c', 'o', 'p', 'y', '-', 'k', 'e', 'y',
				   '-', 'm', 'e', 's', 's', 'a', 'g', 'e' };

static void set_attributes(psa_key_attributes_t *attr, psa_key_usage_t usage)
{
	psa_set_key_type(attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(attr, 8 * sizeof(g_key));
	psa_set_key_usage_flags(attr, usage);
	psa_set_key_algorithm(attr, PSA_ALG_GCM);
}

static psa_status_t encrypt(psa_key_id_t key, uint8_t *ct, size_t *ct_len)
{
	return psa_aead_encrypt(key, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), NULL, 0, g_msg,
				sizeof(g_msg), ct, sizeof(g_msg) + 16, ct_len);
}

static bool copy_narrows_usage_ok(psa_key_id_t source)
{
	uint8_t ct_source[sizeof(g_msg) + 16];
	uint8_t ct_copy[sizeof(g_msg) + 16];
	uint8_t pt[sizeof(g_msg)];
	size_t ct_source_len = 0;
	size_t ct_copy_len = 0;
	size_t pt_len = 0;
	psa_key_id_t copy = 0;
	bool ok = false;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	set_attributes(&attr, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	if (psa_copy_key(source, &attr, &copy) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_copy_key to a volatile key failed\r\n");
		return false;
	}

	if (encrypt(source, ct_source, &ct_source_len) != PSA_SUCCESS ||
	    encrypt(copy, ct_copy, &ct_copy_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("encrypt with the source or the copy failed\r\n");
	} else if (ct_source_len != ct_copy_len || memcmp(ct_source, ct_copy, ct_copy_len) != 0) {
		integration_kit_uart_puts("copy encrypts differently from its source\r\n");
	} else if (psa_aead_decrypt(copy, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), NULL, 0, ct_copy,
				    ct_copy_len, pt, sizeof(pt), &pt_len) == PSA_SUCCESS) {
		integration_kit_uart_puts("copy decrypted despite its narrower usage\r\n");
	} else {
		ok = true;
	}

	(void)psa_destroy_key(copy);
	return ok;
}

static bool copy_to_persistent_ok(psa_key_id_t source)
{
	uint8_t ct_source[sizeof(g_msg) + 16];
	uint8_t ct_stored[sizeof(g_msg) + 16];
	size_t ct_source_len = 0;
	size_t ct_stored_len = 0;
	psa_key_id_t stored = 0;
	bool ok = false;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	set_attributes(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_PERSISTENT);
	psa_set_key_id(&attr, STORED_KEY_ID);

	if (psa_copy_key(source, &attr, &stored) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_copy_key to a persistent key failed\r\n");
		return false;
	}

	if (encrypt(source, ct_source, &ct_source_len) != PSA_SUCCESS ||
	    encrypt(STORED_KEY_ID, ct_stored, &ct_stored_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("encrypt with the source or the stored key failed\r\n");
	} else {
		ok = ct_source_len == ct_stored_len &&
		     memcmp(ct_source, ct_stored, ct_stored_len) == 0;
	}

	/* The key shares the URoT storage with every later run, so it may not outlive this one. */
	if (psa_destroy_key(STORED_KEY_ID) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_destroy_key (stored key) failed\r\n");
		ok = false;
	}
	return ok;
}

static bool copy_needs_usage_ok(void)
{
	psa_key_id_t source = 0;
	psa_key_id_t copy = 0;
	bool ok = true;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	set_attributes(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);
	if (psa_import_key(&attr, g_key, sizeof(g_key), &source) != PSA_SUCCESS) {
		integration_kit_uart_puts("import of the no-COPY key failed\r\n");
		return false;
	}

	psa_key_attributes_t target = PSA_KEY_ATTRIBUTES_INIT;

	set_attributes(&target, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_lifetime(&target, PSA_KEY_LIFETIME_VOLATILE);
	if (psa_copy_key(source, &target, &copy) != PSA_ERROR_NOT_PERMITTED) {
		integration_kit_uart_puts("a key without the COPY usage was copied\r\n");
		(void)psa_destroy_key(copy);
		ok = false;
	}
	(void)psa_destroy_key(source);
	return ok;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts("psa_copy_key: key copy via UROT PSA service\r\n");

	(void)psa_crypto_init();

	psa_key_id_t source = 0;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	bool ok = false;

	set_attributes(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT | PSA_KEY_USAGE_COPY);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);
	if (psa_import_key(&attr, g_key, sizeof(g_key), &source) != PSA_SUCCESS) {
		integration_kit_uart_puts("import of the source key failed\r\n");
	} else {
		ok = copy_narrows_usage_ok(source);
		ok = copy_to_persistent_ok(source) && ok;
		(void)psa_destroy_key(source);
	}
	ok = copy_needs_usage_ok() && ok;

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_copy_key") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_copy_key"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
