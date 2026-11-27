/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_mext.
 *
 * Vendor-key scenario: the fixed MEXT key
 * (IRONSIDE_CLIENT_MEXT_ID) - an IKG-generated, per-device AES-256
 * key-derivation root the silicon provisions. It is referenced by a fixed id,
 * never imported, never exportable, and is bound to NIST SP 800-108 counter-mode
 * CMAC. The App uses it the only way it can be used: as the SECRET input of a
 * key-derivation operation, to derive a *device-unique, diversified* application
 * key whose material never leaves UROT.
 *
 * This is the "encrypt NVM data at rest with a device-unique key" use case: the
 * derived AES-256 key is used for AES-GCM, so data written to NVM is confidential
 * + authenticated and readable only on this device. Both algorithms this needs
 * over the NSC - SP 800-108 (to derive) and AES-GCM (to protect) - are on UROT's
 * NSC allowlist (SP 800-108 is admitted as a deliberate exception because the
 * MEXT/MKEK roots are bound to it).
 *
 * This is the recommended division of labour for the two AES-256 IKG roots:
 *   - MKEK is the platform's internal confidentiality root (UROT derives the
 *     key that protects each persistent key record from it), so leave it to the
 *     platform.
 *   - MEXT is left free as the *application* diversification root. Deriving from
 *     it gives the App a HUK-style per-device key without any provisioning and
 *     without an ITS/Protected-Storage backend (which IronSide TE does not
 *     provide). A flaw in the App's derivations cannot touch the root that
 *     protects the stored persistent keys.
 *
 * What this sample does (the canonical pattern):
 *   1. psa_key_derivation_setup(PSA_ALG_SP800_108_COUNTER_CMAC)
 *   2. input MEXT as the SECRET step (the root never crosses the NSC boundary)
 *   3. input a fixed LABEL + per-purpose CONTEXT (the diversifier - change the
 *      CONTEXT and you get an independent key bound to the same device root)
 *   4. psa_key_derivation_output_key(... volatile AES-256 for GCM ...) -> key id
 *   5. prove the derived key protects data: AES-GCM encrypt a record, decrypt it
 *      back, and confirm a tampered ciphertext is rejected
 *   6. derive the same key again and decrypt the record with the re-derived key
 *      (determinism - a SP 800-108 KDF is a pure function of {root, label,
 *      context}; this is the real at-rest flow: re-derive after reboot, read back)
 * The derived key material is never exported; only its id and the ciphertext of
 * app data leave UROT.
 *
 * Prerequisites (both in place): UROT provisions the CRACEN IKG seed at boot
 * (src/main.c, so MEXT loads like the IAK) and the KEY_DERIVATION module is
 * enabled (CRYPTO_KEY_DERIVATION_MODULE_ENABLED=1 + PSA_WANT_KEY_TYPE_DERIVE; the
 * CRACEN SP800-108 driver is built in).
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

/* Domain-separation inputs for the KDF. The LABEL names the key family; the
 * CONTEXT diversifies a specific purpose - a different CONTEXT yields a fully
 * independent key derived from the same per-device MEXT root. */
static const uint8_t g_label[] = "ironside-app-storage-key/v1";
static const uint8_t g_context[] = "data-at-rest";

/* A record of application data to protect at rest, its authenticated-but-clear
 * header (AAD), and a fixed nonce (one encryption per record here). */
static const uint8_t g_record[32] = {
	'I', 'r', 'o', 'n', 'S', 'i', 'd', 'e', ' ', 'N', 'V', 'M', ' ', 'r', 'e', 'c',
	'o', 'r', 'd', ' ', 'a', 't', ' ', 'r', 'e', 's', 't', ' ', ' ', ' ', '#', '1',
};
static const uint8_t g_aad[8] = { 'r', 'e', 'c', 'h', 'd', 'r', '0', '1' };
static const uint8_t g_nonce[12] = {
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab,
};

/*
 * Derive a volatile AES-256 key from MEXT via SP 800-108 counter-mode CMAC, bound
 * to AES-GCM. Returns PSA_SUCCESS and *key on success.
 */
static psa_status_t derive_app_key(psa_key_id_t *key)
{
	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;

	status = psa_key_derivation_setup(&op, PSA_ALG_SP800_108_COUNTER_CMAC);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/* The per-device root - supplied by id; its material never leaves UROT. */
	status = psa_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET,
					      IRONSIDE_CLIENT_MEXT_ID);
	if (status == PSA_SUCCESS) {
		status = psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_LABEL,
							g_label, sizeof(g_label) - 1);
	}
	if (status == PSA_SUCCESS) {
		status = psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_CONTEXT,
							g_context, sizeof(g_context) - 1);
	}
	if (status == PSA_SUCCESS) {
		psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

		psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
		psa_set_key_bits(&attr, 256);
		psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
		psa_set_key_algorithm(&attr, PSA_ALG_GCM);
		psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

		status = psa_key_derivation_output_key(&attr, &op, key);
	}

	(void)psa_key_derivation_abort(&op);
	return status;
}

static bool mext_ok(void)
{
	static uint8_t ct[sizeof(g_record) + 16]; /* ciphertext || 16-byte GCM tag */
	static uint8_t pt[sizeof(g_record)];
	size_t ct_len = 0;
	size_t pt_len = 0;
	psa_key_id_t key_a = 0;
	psa_key_id_t key_b = 0;
	bool ok = false;

	/* (1) Derive a device-unique app key from MEXT. */
	if (derive_app_key(&key_a) != PSA_SUCCESS) {
		integration_kit_uart_puts("derive from MEXT failed\r\n");
		return false;
	}
	integration_kit_uart_puts("derived device-unique app key from MEXT\r\n");

	/* (2) Protect an NVM record at rest: AES-GCM encrypt, decrypt, round-trip. */
	if (psa_aead_encrypt(key_a, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), g_aad, sizeof(g_aad),
			     g_record, sizeof(g_record), ct, sizeof(ct), &ct_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("GCM encrypt with derived key failed\r\n");
		goto out_a;
	}
	if (psa_aead_decrypt(key_a, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), g_aad, sizeof(g_aad), ct,
			     ct_len, pt, sizeof(pt), &pt_len) != PSA_SUCCESS ||
	    pt_len != sizeof(g_record) || memcmp(pt, g_record, sizeof(g_record)) != 0) {
		integration_kit_uart_puts("GCM round-trip with derived key failed\r\n");
		goto out_a;
	}
	integration_kit_uart_puts("encrypted + decrypted an NVM record with the derived key\r\n");

	/* Tamper one ciphertext byte - GCM authentication must reject it. */
	ct[0] ^= 0xff;
	if (psa_aead_decrypt(key_a, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), g_aad, sizeof(g_aad), ct,
			     ct_len, pt, sizeof(pt), &pt_len) == PSA_SUCCESS) {
		integration_kit_uart_puts("tampered record wrongly accepted\r\n");
		ct[0] ^= 0xff;
		goto out_a;
	}
	ct[0] ^= 0xff; /* restore for the re-derive check below */

	/* (3) Re-derive with the same {root,label,context}: a SP 800-108 KDF is
	 * deterministic, so the re-derived key decrypts the record written under the
	 * first key - the real at-rest flow (re-derive after reboot, read back). */
	if (derive_app_key(&key_b) != PSA_SUCCESS) {
		integration_kit_uart_puts("re-derive from MEXT failed\r\n");
		goto out_a;
	}
	if (psa_aead_decrypt(key_b, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), g_aad, sizeof(g_aad), ct,
			     ct_len, pt, sizeof(pt), &pt_len) == PSA_SUCCESS &&
	    pt_len == sizeof(g_record) && memcmp(pt, g_record, sizeof(g_record)) == 0) {
		ok = true;
		integration_kit_uart_puts("re-derived key decrypts the record: deterministic\r\n");
	} else {
		integration_kit_uart_puts(
			"re-derived key failed to decrypt: not deterministic\r\n");
	}

	(void)psa_destroy_key(key_b);
out_a:
	(void)psa_destroy_key(key_a);
	return ok;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts("psa_mext: derive an app key from the MEXT root\r\n");

	(void)psa_crypto_init();

	bool ok = mext_ok();

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_mext") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_mext"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
