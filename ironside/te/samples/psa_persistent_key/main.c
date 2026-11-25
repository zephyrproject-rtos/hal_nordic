/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_persistent_key.
 *
 * User-key scenario: a *persistent* key that survives reset, kept by UROT in
 * the URoT storage instead of a RAM slot. The only difference from a volatile
 * import is the key's lifetime + id:
 *   - lifetime = PSA_KEY_LIFETIME_PERSISTENT: local storage, the only location
 *                UROT lets an App create a key at, and the default (ROTATING)
 *                persistence level
 *   - id       = a USER key ID the application chooses
 * After the one-time import, later boots use the key by that same id with no
 * re-import (the secret is in the URoT storage, not in the image).
 *
 * The sample destroys the key before it reports, so a run leaves no key in the
 * URoT storage. A product keeps the key; a key left by an interrupted run is
 * used by id with no re-import.
 *
 * The key is used for AES-GCM, the natural way to protect data with a
 * persistent device key.
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

/* This sample's focus is the *persistence* idiom (use by id, no re-import
 * needed after the first boot); the confidentiality properties and the
 * tamper-reject check are covered by psa_data_at_rest_key. The key is bound to
 * AES-GCM, which is the natural way to protect data with a persistent device
 * key. The ID is a USER key ID (PSA sec. 9.7) outside the reserved range
 * 228..239; each persistent-key sample uses its own ID, so they can share a
 * device. */
#define USER_KEY_ID ((psa_key_id_t)90)

static const uint8_t g_key[32] = {
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a,
	0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45,
	0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
};
static const uint8_t g_nonce[12] = {
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb,
};
static const uint8_t g_msg[16] = { 'f', 'i', 'r', 'm', 'w', 'a', 'r', 'e',
				   '-', 'r', 'e', 'c', 'o', 'r', 'd', '1' };

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

/* Use the persistent key for an AES-GCM round-trip; returns true if the key works. */
static bool gcm_roundtrip(psa_key_id_t kid)
{
	uint8_t ct[sizeof(g_msg) + 16];
	uint8_t pt[sizeof(g_msg)];
	size_t ct_len = 0;
	size_t pt_len = 0;

	if (psa_aead_encrypt(kid, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), NULL, 0, g_msg,
			     sizeof(g_msg), ct, sizeof(ct), &ct_len) != PSA_SUCCESS) {
		return false;
	}
	return psa_aead_decrypt(kid, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), NULL, 0, ct, ct_len, pt,
				sizeof(pt), &pt_len) == PSA_SUCCESS &&
	       pt_len == sizeof(g_msg) && memcmp(pt, g_msg, sizeof(g_msg)) == 0;
}

static bool persistent_key_ok(void)
{
	const psa_key_id_t kid = USER_KEY_ID;

	/* Correct persistent-key idiom: just use the key by its id. If it is already
	 * provisioned (a prior boot did so - the secret lives in the URoT storage,
	 * not in this image), this succeeds with no import: that is the persistence proof. */
	if (gcm_roundtrip(kid)) {
		integration_kit_uart_puts(
			"persistent key already provisioned, used by id (persisted): ok\r\n");
		return true;
	}

	/* Not provisioned yet (e.g. first boot after a full erase): do it once, then use. */
	integration_kit_uart_puts("no key at this id -> provisioning once\r\n");
	if (!provision(kid)) {
		integration_kit_uart_puts("psa_import_key (persistent) failed\r\n");
		return false;
	}
	if (!gcm_roundtrip(kid)) {
		integration_kit_uart_puts("AES-GCM with the persistent key failed\r\n");
		return false;
	}
	integration_kit_uart_puts("persistent key provisioned + used: ok\r\n");
	return true;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts(
		"psa_persistent_key: persistent key in the URoT storage via UROT PSA service\r\n");

	(void)psa_crypto_init();

	bool ok = persistent_key_ok();

	/* The key shares the URoT storage with every later run, so it may not outlive this one. */
	if (psa_destroy_key(USER_KEY_ID) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_destroy_key (cleanup) failed\r\n");
		ok = false;
	}

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_persistent_key") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_persistent_key"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
