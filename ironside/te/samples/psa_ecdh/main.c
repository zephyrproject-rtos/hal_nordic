/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_ecdh.
 *
 * Exercises ECDH key agreement, added to UROT's PSA Crypto service. Key
 * agreement had no non-secure path before this: it needs the client's new
 * psa_raw_key_agreement() wrapper, and UROT's NSC allowlist admits raw ECDH (a
 * combined agreement+KDF would be rejected - the KDF is out of set).
 *
 * Two secp256r1 key pairs (A, B) are imported volatile with USAGE_DERIVE; their
 * public points are exported, then the raw shared secret is computed both ways:
 * ECDH(privA, pubB) and ECDH(privB, pubA). The defining ECDH property is that
 * the two agree - so a matching 32-byte secret proves the operation end-to-end
 * without needing a hardcoded external Z vector. Console: UARTE20, TX = P1.16.
 */

#include <integration_kit_uart.h>
#include <integration_kit_test.h>
#include <ironside_client/psa/crypto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * Two secp256r1 private scalars. Both lead with a byte well below the curve
 * order's leading 0xFF, so each is unambiguously in [1, n-1] and a valid key.
 */
static const uint8_t priv_a[32] = {
	0x39, 0x4f, 0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x70, 0x81, 0x92,
	0xa3, 0xb4, 0xc5, 0xd6, 0xe7, 0xf8, 0x09, 0x1a, 0x2b, 0x3c, 0x4d,
	0x5e, 0x6f, 0x70, 0x81, 0x92, 0xa3, 0xb4, 0xc5, 0xd6, 0xe7,
};
static const uint8_t priv_b[32] = {
	0x4a, 0x5b, 0x6c, 0x7d, 0x8e, 0x9f, 0xa0, 0xb1, 0xc2, 0xd3, 0xe4,
	0xf5, 0x06, 0x17, 0x28, 0x39, 0x4a, 0x5b, 0x6c, 0x7d, 0x8e, 0x9f,
	0xa0, 0xb1, 0xc2, 0xd3, 0xe4, 0xf5, 0x06, 0x17, 0x28, 0x39,
};

/* Import a secp256r1 key-agreement private key; return its id (0 on failure). */
static psa_key_id_t import_ecdh_key(const uint8_t *scalar)
{
	psa_key_id_t key = 0;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attr, 256);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	if (psa_import_key(&attr, scalar, 32, &key) != PSA_SUCCESS) {
		return 0;
	}
	return key;
}

static bool ecdh_ok(void)
{
	/* secp256r1 public point: 0x04 || X(32) || Y(32) = 65 bytes. */
	static uint8_t pub_a[65];
	static uint8_t pub_b[65];
	static uint8_t z_ab[32]; /* ECDH(privA, pubB) */
	static uint8_t z_ba[32]; /* ECDH(privB, pubA) */
	size_t pub_a_len = 0, pub_b_len = 0, z_ab_len = 0, z_ba_len = 0;
	psa_key_id_t key_a = import_ecdh_key(priv_a);
	psa_key_id_t key_b = import_ecdh_key(priv_b);
	bool ok = true;

	if (key_a == 0 || key_b == 0) {
		integration_kit_uart_puts("psa_import_key (secp256r1) failed\r\n");
		return false;
	}

	if (psa_export_public_key(key_a, pub_a, sizeof(pub_a), &pub_a_len) != PSA_SUCCESS ||
	    psa_export_public_key(key_b, pub_b, sizeof(pub_b), &pub_b_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_export_public_key failed\r\n");
		ok = false;
	}

	if (ok && psa_raw_key_agreement(PSA_ALG_ECDH, key_a, pub_b, pub_b_len, z_ab, sizeof(z_ab),
					&z_ab_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("ECDH(privA, pubB) failed\r\n");
		ok = false;
	}
	if (ok && psa_raw_key_agreement(PSA_ALG_ECDH, key_b, pub_a, pub_a_len, z_ba, sizeof(z_ba),
					&z_ba_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("ECDH(privB, pubA) failed\r\n");
		ok = false;
	}

	if (ok && (z_ab_len != 32 || z_ba_len != 32 || memcmp(z_ab, z_ba, 32) != 0)) {
		integration_kit_uart_puts("shared secrets differ\r\n");
		ok = false;
	}
	if (ok) {
		integration_kit_uart_puts("ECDH P-256: shared secret agrees both ways\r\n");
	}

	(void)psa_destroy_key(key_a);
	(void)psa_destroy_key(key_b);
	return ok;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts("psa_ecdh: secp256r1 ECDH key agreement via UROT\r\n");

	(void)psa_crypto_init();

	bool ok = ecdh_ok();

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_ecdh") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_ecdh"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
