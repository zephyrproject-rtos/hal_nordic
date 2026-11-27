/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_data_at_rest_key.
 *
 * User-key scenario: a *persistent application encryption key* that survives
 * reset, never leaves UROT, and is **never stored in plaintext** - UROT keeps it
 * in the URoT storage, where every record is AES-GCM encrypted under a key
 * derived from the device MKEK, so an RRAM readout yields only ciphertext. This
 * is the companion to the two neighbouring persistent-key samples:
 *   - psa_persistent_key  - the same kind of key, but focused on the
 *                           persistence idiom (use by id, no re-import) rather
 *                           than the confidentiality/tamper-reject properties.
 *   - psa_fw_decrypt  - a persistent key too, but for the *vendor*
 *                           firmware-decryption KEK (the reserved key IDs
 *                           228..239, shared by all three image roles).
 * This sample covers an *application's own* data-at-rest key, at a USER key ID
 * outside the reserved range.
 *
 * The deciding question vs. deriving from MEXT (psa_mext): use a persistent key
 * when the key's *value* must be preserved (e.g. a key shared with a backend);
 * derive from MEXT when you only need a device-unique secret and do not care
 * what its bytes are.
 *
 * The key's lifetime + id are the only things that differ from a volatile import:
 *   - lifetime = PSA_KEY_LIFETIME_PERSISTENT: local storage, the only location
 *                UROT lets an App create a key at, and the default (ROTATING)
 *                persistence level
 *   - id       = a USER key ID the application chooses
 * After the one-time provisioning, later boots use the key by that same id with
 * no re-import - the (encrypted) secret is in the URoT storage, not in this
 * image.
 *
 * It then proves the key works: AES-256-GCM encrypt a record of application data,
 * decrypt it back and check the round-trip, then flip one ciphertext byte and
 * confirm decryption is rejected (GCM authenticity) - the at-rest tamper check a
 * real data-at-rest store relies on.
 *
 * The sample destroys the key before it reports, so a run leaves no key in the
 * URoT storage. A product keeps the key; a key left by an interrupted run takes
 * the "already provisioned, used by id" branch.
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

/* A USER key ID (PSA sec. 9.7) outside the reserved range 228..239. Each
 * persistent-key sample uses its own ID, so they can share a device. */
#define USER_KEY_ID ((psa_key_id_t)48)

/* The application data-at-rest key. In a real product its value comes from
 * wherever the secret must match (a backend handshake, a provisioning step); here
 * a fixed AES-256 key stands in. It is encrypted the moment it enters the URoT
 * storage. */
static const uint8_t g_key[32] = {
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,
	0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75,
	0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
};

/* A record of application data to protect at rest (stands in for a real payload). */
static const uint8_t g_record[32] = {
	'a', 'p', 'p', ' ', 'd', 'a', 't', 'a', '-', 'a', 't', '-', 'r', 'e', 's', 't',
	' ', 'r', 'e', 'c', 'o', 'r', 'd', ' ', 'n', 'u', 'm', 'b', 'e', 'r', '0', '1',
};

/* Associated data bound to the ciphertext but not encrypted (e.g. a record id). */
static const uint8_t g_aad[8] = { 'r', 'e', 'c', 'o', 'r', 'd', '0', '1' };

/* A single fixed GCM nonce is fine for this self-contained round-trip. In a real
 * data-at-rest store you must use a unique nonce per encryption with the same key
 * (a counter or random 96-bit value) - reusing a (key, nonce) pair breaks GCM. */
static const uint8_t g_nonce[12] = {
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b,
};

/* Provision the AES key into the URoT storage (the one-time step). */
static bool provision(psa_key_id_t kid)
{
	psa_key_id_t out_id = 0;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attr, PSA_ALG_GCM);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_PERSISTENT);
	psa_set_key_id(&attr, kid);

	return psa_import_key(&attr, g_key, sizeof(g_key), &out_id) == PSA_SUCCESS;
}

/* Ensure the key exists: if a prior boot provisioned it (the encrypted secret
 * lives in the URoT storage, not in this image) use it as-is - that is the
 * persistence proof. */
static bool ensure_key(psa_key_id_t kid)
{
	psa_key_attributes_t existing = PSA_KEY_ATTRIBUTES_INIT;

	if (psa_get_key_attributes(kid, &existing) == PSA_SUCCESS) {
		integration_kit_uart_puts(
			"persistent key already provisioned, using by id (persisted)\r\n");
		return true;
	}

	integration_kit_uart_puts("no key at this id -> provisioning it once\r\n");
	if (!provision(kid)) {
		integration_kit_uart_puts("psa_import_key (persistent) failed\r\n");
		return false;
	}
	return true;
}

static bool encrypted_key_ok(void)
{
	uint8_t ct[sizeof(g_record) + 16]; /* ciphertext || 16-byte GCM tag */
	uint8_t pt[sizeof(g_record)];
	size_t ct_len = 0;
	size_t pt_len = 0;
	const psa_key_id_t kid = USER_KEY_ID;

	if (!ensure_key(kid)) {
		return false;
	}

	/* Encrypt-at-rest with the persistent key (loaded from the URoT storage
	 * secure-side; the key never reaches this image). */
	if (psa_aead_encrypt(kid, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), g_aad, sizeof(g_aad),
			     g_record, sizeof(g_record), ct, sizeof(ct), &ct_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_aead_encrypt (persistent key) failed\r\n");
		return false;
	}

	/* Decrypt it back and check the round-trip. */
	if (psa_aead_decrypt(kid, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), g_aad, sizeof(g_aad), ct,
			     ct_len, pt, sizeof(pt), &pt_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_aead_decrypt (genuine) failed\r\n");
		return false;
	}
	if (pt_len != sizeof(g_record) || memcmp(pt, g_record, sizeof(g_record)) != 0) {
		integration_kit_uart_puts("decrypted record mismatch\r\n");
		return false;
	}
	integration_kit_uart_puts("encrypt -> decrypt round-trip with the persistent key: ok\r\n");

	/* Tamper check: flip one ciphertext byte; GCM must REJECT it (no plaintext). */
	ct[0] ^= 0x01;
	if (psa_aead_decrypt(kid, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), g_aad, sizeof(g_aad), ct,
			     ct_len, pt, sizeof(pt), &pt_len) == PSA_SUCCESS) {
		integration_kit_uart_puts("tampered ciphertext was not rejected (fail)\r\n");
		return false;
	}
	integration_kit_uart_puts("tampered ciphertext rejected (GCM authenticity): ok\r\n");
	return true;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts(
		"psa_data_at_rest_key: persistent key in the URoT storage via UROT\r\n");

	(void)psa_crypto_init();

	bool ok = encrypted_key_ok();

	/* The key shares the URoT storage with every later run, so it may not outlive this one. */
	if (psa_destroy_key(USER_KEY_ID) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_destroy_key (cleanup) failed\r\n");
		ok = false;
	}

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_data_at_rest_key") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_data_at_rest_key"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
