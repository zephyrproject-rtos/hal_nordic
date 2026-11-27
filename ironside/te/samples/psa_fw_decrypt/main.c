/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_fw_decrypt.
 *
 * The IronSide DFU firmware-decrypt scenario, end to end.
 * A single persistent vendor KEK is **shared by all three image roles**
 * (UROT / App / DFU) - matching the KEK in the IronSide TE specification's "Firmware
 * decryption" design. Critically, the KEK never touches firmware bytes directly: its
 * only job is to **AES-KW unwrap** (psa_unwrap_key, RFC 3394/NIST SP800-38F, the same
 * primitive MCUboot uses for encrypted-image support) a small per-release DEK into a
 * ready-to-use, **volatile** AES-256-CTR key handle. That transient DEK - never
 * persistent - is what actually decrypts the firmware body with AES-CTR:
 *
 *   DFU production sequence (once per session, any routed image role):
 *     psa_unwrap_key(DFU_OTA_KEK_KEY_ID(gen), PSA_ALG_KW, wrapped_dek, &dek);
 *     psa_cipher_decrypt(dek, PSA_ALG_CTR, iv_and_ciphertext, len, plaintext, ...);
 *     psa_destroy_key(dek); -- end of session
 *
 * AES-CTR is confidentiality only - image *integrity* comes from the MCUboot-format
 * signature that IROT (UROT image) and UROT (App/DFU images) already verify, so no AEAD
 * tag is needed here. RFC 3394's own integrity check rejects a tampered wrap
 * (PSA_ERROR_INVALID_SIGNATURE) before the DEK is ever created, so a corrupted header
 * can't turn into a chosen-ciphertext oracle against the long-lived KEK.
 *
 * This sample provisions its own standalone demo KEK at a dedicated USER key ID
 * (160 - below the real vendor KEK's reserved key IDs, 228-239) rather than a real
 * DFU_OTA_KEK_KEY_ID() id: those keys are provisioned with real, unknown-
 * to-this-sample key material by the standard install flow, so a fixed known-answer vector
 * could never unwrap against whatever is actually stored there. Using its own key ID keeps
 * this sample a self-contained KAT independent of board provisioning history, while still
 * demonstrating the exact mechanism production uses:
 *
 *   DFU production sequence (once per session, any routed image role):
 *     psa_unwrap_key(DFU_OTA_KEK_KEY_ID(gen), PSA_ALG_KW, wrapped_dek, &dek);
 *     psa_cipher_decrypt(dek, PSA_ALG_CTR, iv_and_ciphertext, len, plaintext, ...);
 *     psa_destroy_key(dek); -- end of session
 *
 * (This sample calls the same three functions against its own demo key id, not the
 * production one above.) It then unwraps one DEK and decrypts three role payload blocks
 * with it (counter block = the block index from zero, big-endian - the MCUboot
 * encrypted-image counter the IronSide DFU image uses), showing the same unwrapped session
 * key handles all three roles: there is no per-role key selection anywhere in this flow.
 * Every input/expected-output byte was generated together by `scripts/lib/mcuboot_encrypt.py`
 * (RFC 3394 AES-KW + AES-CTR, both via Python's `cryptography`), so this is a real
 * cross-implementation known-answer test: a disagreement between CRACEN's AES-KW/CTR and
 * the host encoder fails this sample even though both implement the same standard. Since
 * psa_unwrap_key() never exposes the unwrapped DEK in the clear, the check is indirect but
 * airtight - the CTR-decrypted body must match the known plaintext, which only happens if
 * the unwrap recovered the exact right key. The demo KEK is a persistent key in the URoT
 * storage (encrypted at rest under a key derived from the device MKEK, as the production
 * KEKs are). The sample destroys it before it reports, so a run leaves no key behind.
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

#define AES_CTR_IV_LEN 16

/* A dedicated USER key ID for this sample's own demo KEK - not the production vendor
 * KEK (DFU_OTA_KEK_KEY_ID(), key IDs 228-239, reserved for import-fw-kek).
 * Keeps this known-answer test independent of whatever the board's real OTA KEKs hold. */
#define DEMO_KEK_ID ((psa_key_id_t)160)

/* --- Fixed cross-implementation test vectors (see header comment) --------- */

static const uint8_t g_kek[32] = {
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,
	0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75,
	0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
};
/* RFC 3394 AES-KW(g_kek, dek) where dek = 0xa0..0xbf -- 40 bytes (32 B key + 8 B
 * overhead). dek itself is never referenced below -- psa_unwrap_key() never exposes it
 * in the clear, only as the key handle each role_roundtrip_ok() call unwraps to. */
static const uint8_t g_wrapped_dek[40] = {
	0xb7, 0x48, 0x8d, 0xc2, 0x66, 0x91, 0x7a, 0x5e, 0xad, 0x24, 0xf9, 0x26, 0x5c, 0xfb,
	0x6d, 0x32, 0xea, 0xdc, 0xa7, 0xf4, 0x83, 0xb2, 0xfa, 0xd7, 0xa8, 0xdd, 0x55, 0xc9,
	0xd7, 0x5e, 0xb6, 0xd0, 0x00, 0xc7, 0x63, 0xc0, 0x89, 0x25, 0x40, 0x01,
};
/* One image role's payload block, decrypted with the same session DEK at block index
 * `idx` (counter block = idx, big-endian, as for an MCUboot encrypted-image body) --
 * there is no per-role key selection, only a per-block counter. */
struct fw_role {
	const char *name;
	uint8_t idx;
	uint8_t ciphertext[32];
	uint8_t plaintext[32];
};

static const struct fw_role g_roles[] = {
	{ "UROT",
	  0,
	  { 0x16, 0x7d, 0xc8, 0x42, 0x32, 0x70, 0xe4, 0x12, 0xee, 0x11, 0xbc,
	    0x7e, 0x2d, 0x1c, 0x7a, 0x2d, 0x23, 0x5e, 0xbe, 0xac, 0x18, 0xa7,
	    0x6c, 0xd2, 0xe3, 0x16, 0xae, 0x84, 0x3e, 0x42, 0xa8, 0xd0 },
	  { 'I', 'r', 'o', 'n', 'S', 'i', 'd', 'e', ' ', 'U', 'R', 'O', 'T', ' ', 'p', 'a',
	    'y', 'l', 'o', 'a', 'd', ' ', 'b', 'l', 'o', 'c', 'k', ' ', '#', '0', '1', ' ' } },
	{ "APP",
	  1,
	  { 0x13, 0x40, 0xbe, 0xa3, 0x2f, 0xee, 0x6a, 0xdb, 0xac, 0x34, 0x95,
	    0xf4, 0x3d, 0x02, 0xf8, 0x89, 0xd2, 0xce, 0x71, 0x13, 0xf5, 0xd2,
	    0x48, 0x1d, 0xc1, 0xd4, 0xd6, 0xba, 0x97, 0x59, 0x0d, 0xd3 },
	  { 'I', 'r', 'o', 'n', 'S', 'i', 'd', 'e', ' ', 'A', 'P', 'P', ' ', 'p', 'a', 'y',
	    'l', 'o', 'a', 'd', ' ', 'b', 'l', 'o', 'c', 'k', ' ', '#', '0', '1', ' ', ' ' } },
	{ "DFU",
	  2,
	  { 0xf7, 0xd3, 0x7f, 0x19, 0x86, 0xd9, 0x40, 0x17, 0x82, 0xfb, 0xb0,
	    0xcc, 0x87, 0x18, 0x4c, 0x8a, 0xe5, 0x14, 0x00, 0x47, 0x7d, 0x8f,
	    0xa3, 0x43, 0x5e, 0x3d, 0x13, 0x4e, 0xa3, 0xca, 0x59, 0x43 },
	  { 'I', 'r', 'o', 'n', 'S', 'i', 'd', 'e', ' ', 'D', 'F', 'U', ' ', 'p', 'a', 'y',
	    'l', 'o', 'a', 'd', ' ', 'b', 'l', 'o', 'c', 'k', ' ', '#', '0', '1', ' ', ' ' } },
};

/* Import this sample's own demo KEK. UNWRAP-only, AES-KW-bound -- the same policy
 * `import-fw-kek` applies in production, just at a different key ID. A run that stopped
 * early leaves the key behind, so it is destroyed first. */
static bool import_demo_kek(void)
{
	(void)psa_destroy_key(DEMO_KEK_ID);

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t out_id = 0;

	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_UNWRAP);
	psa_set_key_algorithm(&attr, PSA_ALG_KW);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_PERSISTENT);
	psa_set_key_id(&attr, DEMO_KEK_ID);

	return psa_import_key(&attr, g_kek, sizeof(g_kek), &out_id) == PSA_SUCCESS;
}

static bool role_decrypt_ok(psa_key_id_t dek_id, const struct fw_role *r)
{
	uint8_t iv_ct[AES_CTR_IV_LEN + sizeof(r->plaintext)]; /* IV || ciphertext */
	uint8_t pt[sizeof(r->plaintext)];
	size_t pt_len = 0;

	memset(iv_ct, 0, AES_CTR_IV_LEN);
	iv_ct[AES_CTR_IV_LEN - 1] = r->idx;
	memcpy(iv_ct + AES_CTR_IV_LEN, r->ciphertext, sizeof(r->ciphertext));

	if (psa_cipher_decrypt(dek_id, PSA_ALG_CTR, iv_ct, sizeof(iv_ct), pt, sizeof(pt),
			       &pt_len) != PSA_SUCCESS) {
		integration_kit_uart_puts(r->name);
		integration_kit_uart_puts(": decrypt failed\r\n");
		return false;
	}

	if (pt_len != sizeof(r->plaintext) || memcmp(pt, r->plaintext, sizeof(r->plaintext)) != 0) {
		integration_kit_uart_puts(r->name);
		integration_kit_uart_puts(": decrypted image mismatch\r\n");
		return false;
	}

	integration_kit_uart_puts(r->name);
	integration_kit_uart_puts(": AES-CTR decrypt with shared session DEK ok\r\n");
	return true;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts("psa_fw_decrypt: AES-KW-unwrap DEK from a demo KEK, "
				  "AES-CTR FW decrypt\r\n");

	(void)psa_crypto_init();

	bool ok = import_demo_kek();

	if (!ok) {
		integration_kit_uart_puts("demo KEK provisioning failed\r\n");
	}

	psa_key_attributes_t dek_attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&dek_attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&dek_attr, 256);
	psa_set_key_usage_flags(&dek_attr, PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&dek_attr, PSA_ALG_CTR);

	psa_key_id_t dek_id = 0;

	if (ok && psa_unwrap_key(&dek_attr, DEMO_KEK_ID, PSA_ALG_KW, g_wrapped_dek,
				 sizeof(g_wrapped_dek), &dek_id) != PSA_SUCCESS) {
		integration_kit_uart_puts(
			"psa_unwrap_key (DEK) failed -- CRACEN AES-KW rejected the known-good "
			"wrap\r\n");
		ok = false;
	}

	if (ok) {
		for (size_t i = 0; i < sizeof(g_roles) / sizeof(g_roles[0]); i++) {
			ok = role_decrypt_ok(dek_id, &g_roles[i]) && ok;
		}
		if (psa_destroy_key(dek_id) != PSA_SUCCESS) {
			integration_kit_uart_puts("psa_destroy_key (DEK) failed\r\n");
			ok = false;
		}
		if (psa_destroy_key(DEMO_KEK_ID) != PSA_SUCCESS) {
			integration_kit_uart_puts("psa_destroy_key (demo KEK) failed\r\n");
			ok = false;
		}
	}

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_fw_decrypt") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_fw_decrypt"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
