/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_key_rotation.
 *
 * Rotating an App-owned *persistent* key in place: destroy the current key
 * material and re-provision the same key id with new material, without ever
 * moving to a different id. The differences versus a plain persistent key
 * (see psa_persistent_key) are in the *sequence*, not the attributes:
 *
 *   1. Use whatever key is currently stored at the fixed id (generate it
 *      once, on a device without one).
 *   2. psa_destroy_key() it. Because the key was created with
 *      IRONSIDE_CLIENT_KEY_PERSISTENCE_ROTATING (the default persistence
 *      level), the destroy erases it and leaves the id free to create again.
 *   3. psa_generate_key() fresh material into that same id. No new id, no
 *      bookkeeping of "which generation is where" - the id never changes.
 *
 * This is deliberately the other persistence level from the vendor
 * firmware-decryption KEK's rotation (a generation sweep across reserved key
 * IDs, IRONSIDE_CLIENT_KEY_PERSISTENCE_REVOKABLE - destroying such a key
 * revokes its id for good, so "rotation" there means moving to the next
 * generation's id instead). This sample is the pattern for an App's own
 * persistent key: rotate the material, keep the id.
 *
 * Each rotation's new material is freshly generated in place with
 * psa_generate_key() rather than a second fixed constant - a fixed target
 * would rotate back onto the same bytes on a re-run once one rotation has
 * happened, silently defeating the very check meant to catch that (the
 * pre-rotation ciphertext would then decrypt "successfully" simply because
 * the "old" and "new" keys happened to be identical, not because rotation was
 * skipped). psa_generate_key() is also the right tool for the job on its own
 * merits, not just as a source of freshness: CRACEN generates the key bytes
 * behind the TrustZone boundary and hands back only a key id - the material
 * never exists in non-secure RAM, not even transiently, unlike drawing bytes
 * with psa_generate_random() and passing them to psa_import_key(). The proof
 * that the rotation is real: a ciphertext produced before the rotation,
 * decrypted with the key after the rotation, must be rejected - the new key
 * really is different key bytes, and the old ones are gone. A round trip
 * under the new key must then succeed.
 *
 * The sample destroys the key before it reports, so a run leaves no key
 * behind. A key left by an interrupted run is used as the current generation.
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
#define USER_KEY_ID ((psa_key_id_t)130)

static const uint8_t g_nonce_before[12] = {
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab,
};
static const uint8_t g_nonce_after[12] = {
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb,
};
static const uint8_t g_msg_before[16] = { 'g', 'e', 'n', '-', 'A', ' ', 'r', 'e',
					  'c', 'o', 'r', 'd', ' ', '#', '0', '1' };
static const uint8_t g_msg_after[16] = { 'g', 'e', 'n', '-', 'B', ' ', 'r', 'e',
					 'c', 'o', 'r', 'd', ' ', '#', '0', '1' };

/* Generate fresh AES-256 key material directly into the fixed id (standard
 * PSA psa_generate_key(), not psa_generate_random() + psa_import_key()): the
 * key bytes are produced by CRACEN behind the TrustZone boundary and never
 * exist in this App's own RAM. ROTATING persistence (the default) so a later
 * psa_destroy_key() frees the id for reprovisioning instead of revoking it
 * forever. */
static bool generate_generation(void)
{
	psa_key_id_t out_id = 0;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attr, PSA_ALG_GCM);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_PERSISTENT);
	psa_set_key_id(&attr, USER_KEY_ID);

	if (psa_generate_key(&attr, &out_id) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_generate_key failed\r\n");
		return false;
	}
	return true;
}

/* Make sure some generation is stored at the fixed id before the rotation.
 * With no key at the id a fresh generation is generated; a key an interrupted
 * run left behind is left exactly as it is - whichever material is there now
 * is simply "current". */
static bool ensure_current_generation(void)
{
	psa_key_attributes_t existing = PSA_KEY_ATTRIBUTES_INIT;

	if (psa_get_key_attributes(USER_KEY_ID, &existing) == PSA_SUCCESS) {
		integration_kit_uart_puts("current generation already stored, used by id\r\n");
		return true;
	}

	integration_kit_uart_puts("no key at this id -> generating a fresh generation\r\n");
	return generate_generation();
}

static bool gcm_encrypt(psa_key_id_t kid, const uint8_t *nonce, const uint8_t *msg, size_t msg_len,
			uint8_t *ct, size_t ct_size, size_t *ct_len)
{
	return psa_aead_encrypt(kid, PSA_ALG_GCM, nonce, 12, NULL, 0, msg, msg_len, ct, ct_size,
				ct_len) == PSA_SUCCESS;
}

static psa_status_t gcm_decrypt(psa_key_id_t kid, const uint8_t *nonce, const uint8_t *ct,
				size_t ct_len, uint8_t *pt, size_t pt_size, size_t *pt_len)
{
	return psa_aead_decrypt(kid, PSA_ALG_GCM, nonce, 12, NULL, 0, ct, ct_len, pt, pt_size,
				pt_len);
}

static bool key_rotation_ok(void)
{
	const psa_key_id_t kid = USER_KEY_ID;
	uint8_t ct_before[sizeof(g_msg_before) + 16];
	uint8_t ct_after[sizeof(g_msg_after) + 16];
	uint8_t pt[16];
	size_t ct_before_len = 0, ct_after_len = 0, pt_len = 0;

	if (!ensure_current_generation()) {
		integration_kit_uart_puts("provisioning the current generation failed\r\n");
		return false;
	}

	/* Use the pre-rotation key: this ciphertext must become unreadable once
	 * the key is rotated. */
	if (!gcm_encrypt(kid, g_nonce_before, g_msg_before, sizeof(g_msg_before), ct_before,
			 sizeof(ct_before), &ct_before_len)) {
		integration_kit_uart_puts("pre-rotation encrypt failed\r\n");
		return false;
	}
	integration_kit_uart_puts("pre-rotation encrypt: ok\r\n");

	/* Rotate: destroy the current material, then generate fresh material
	 * straight into the same id. On a ROTATING key the destroy leaves the id
	 * free rather than revoked, so the generate that follows lands cleanly. */
	if (psa_destroy_key(kid) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_destroy_key (rotate) failed\r\n");
		return false;
	}
	if (!generate_generation()) {
		integration_kit_uart_puts("re-generating the rotated generation failed\r\n");
		return false;
	}
	integration_kit_uart_puts("rotated: same id, new material\r\n");

	/* The old ciphertext must now be rejected -- proof the material really
	 * changed, not that the App merely re-used the old key. */
	if (gcm_decrypt(kid, g_nonce_before, ct_before, ct_before_len, pt, sizeof(pt), &pt_len) ==
	    PSA_SUCCESS) {
		integration_kit_uart_puts(
			"pre-rotation ciphertext decrypted after rotation -- should have "
			"been rejected\r\n");
		return false;
	}
	integration_kit_uart_puts("pre-rotation ciphertext correctly rejected after rotation\r\n");

	/* The new generation must work end to end. */
	if (!gcm_encrypt(kid, g_nonce_after, g_msg_after, sizeof(g_msg_after), ct_after,
			 sizeof(ct_after), &ct_after_len)) {
		integration_kit_uart_puts("post-rotation encrypt failed\r\n");
		return false;
	}
	if (gcm_decrypt(kid, g_nonce_after, ct_after, ct_after_len, pt, sizeof(pt), &pt_len) !=
		    PSA_SUCCESS ||
	    pt_len != sizeof(g_msg_after) || memcmp(pt, g_msg_after, sizeof(g_msg_after)) != 0) {
		integration_kit_uart_puts("post-rotation round trip failed\r\n");
		return false;
	}
	integration_kit_uart_puts("post-rotation round trip: ok\r\n");
	return true;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts("psa_key_rotation: in-place rotation of a persistent key\r\n");

	(void)psa_crypto_init();

	bool ok = key_rotation_ok();

	psa_status_t status = psa_destroy_key(USER_KEY_ID);

	if (status != PSA_SUCCESS && status != PSA_ERROR_INVALID_HANDLE) {
		integration_kit_uart_puts("psa_destroy_key (cleanup) failed\r\n");
		ok = false;
	}

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_key_rotation") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_key_rotation"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
