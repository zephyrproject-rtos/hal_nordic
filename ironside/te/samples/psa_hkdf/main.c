/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_hkdf.
 *
 * HKDF (RFC 5869) with SHA-256 through the standard PSA key-derivation API,
 * checked against RFC 5869 test case 1:
 *   1. the one-shot PSA_ALG_HKDF: salt, secret and info in, 42 bytes out
 *   2. the same derivation as its two halves: PSA_ALG_HKDF_EXTRACT yields the
 *      pseudorandom key, PSA_ALG_HKDF_EXPAND turns it into the same 42 bytes
 *   3. HKDF over a hash outside UROT's allowlist (SHA-1) and the TLS 1.2
 *      PSK-to-master-secret KDF are refused at setup
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

#define OKM_LEN 42
#define PRK_LEN 32

/* Not in the supported surface, so spelled out here rather than taken from the
 * client's PSA replica: PSA_ALG_HKDF(PSA_ALG_SHA_1) and
 * PSA_ALG_TLS12_PSK_TO_MS(PSA_ALG_SHA_256). */
#define ALG_HKDF_SHA1_UNSUPPORTED              ((psa_algorithm_t)0x08000105)
#define ALG_TLS12_PSK_TO_MS_SHA256_UNSUPPORTED ((psa_algorithm_t)0x08000309)

static const uint8_t g_ikm[22] = {
	0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
	0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
};
static const uint8_t g_salt[13] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
};
static const uint8_t g_info[10] = {
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
};
static const uint8_t g_expected_prk[PRK_LEN] = {
	0x07, 0x77, 0x09, 0x36, 0x2c, 0x2e, 0x32, 0xdf, 0x0d, 0xdc, 0x3f,
	0x0d, 0xc4, 0x7b, 0xba, 0x63, 0x90, 0xb6, 0xc7, 0x3b, 0xb5, 0x0f,
	0x9c, 0x31, 0x22, 0xec, 0x84, 0x4a, 0xd7, 0xc2, 0xb3, 0xe5,
};
static const uint8_t g_expected_okm[OKM_LEN] = {
	0x3c, 0xb2, 0x5f, 0x25, 0xfa, 0xac, 0xd5, 0x7a, 0x90, 0x43, 0x4f, 0x64, 0xd0, 0x36,
	0x2f, 0x2a, 0x2d, 0x2d, 0x0a, 0x90, 0xcf, 0x1a, 0x5a, 0x4c, 0x5d, 0xb0, 0x2d, 0x56,
	0xec, 0xc4, 0xc5, 0xbf, 0x34, 0x00, 0x72, 0x08, 0xd5, 0xb8, 0x87, 0x18, 0x58, 0x65,
};

static bool import_derive_key(const uint8_t *data, size_t len, psa_algorithm_t alg,
			      psa_key_id_t *key)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_DERIVE);
	psa_set_key_bits(&attr, 8 * len);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, alg);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	return psa_import_key(&attr, data, len, key) == PSA_SUCCESS;
}

static bool hkdf_one_shot_ok(void)
{
	static uint8_t okm[OKM_LEN];
	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;
	const psa_algorithm_t alg = PSA_ALG_HKDF(PSA_ALG_SHA_256);
	psa_key_id_t ikm = 0;
	bool ok = false;

	if (!import_derive_key(g_ikm, sizeof(g_ikm), alg, &ikm)) {
		integration_kit_uart_puts("import of the HKDF input key failed\r\n");
		return false;
	}

	if (psa_key_derivation_setup(&op, alg) == PSA_SUCCESS &&
	    psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_SALT, g_salt,
					   sizeof(g_salt)) == PSA_SUCCESS &&
	    psa_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET, ikm) ==
		    PSA_SUCCESS &&
	    psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_INFO, g_info,
					   sizeof(g_info)) == PSA_SUCCESS &&
	    psa_key_derivation_output_bytes(&op, okm, sizeof(okm)) == PSA_SUCCESS) {
		ok = memcmp(okm, g_expected_okm, sizeof(okm)) == 0;
	}
	if (!ok) {
		integration_kit_uart_puts(
			"one-shot HKDF failed or differs from RFC 5869 case 1\r\n");
	}

	(void)psa_key_derivation_abort(&op);
	(void)psa_destroy_key(ikm);
	return ok;
}

static bool hkdf_extract_expand_ok(void)
{
	static uint8_t prk[PRK_LEN];
	static uint8_t okm[OKM_LEN];
	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;
	const psa_algorithm_t extract = PSA_ALG_HKDF_EXTRACT(PSA_ALG_SHA_256);
	const psa_algorithm_t expand = PSA_ALG_HKDF_EXPAND(PSA_ALG_SHA_256);
	psa_key_id_t ikm = 0;
	psa_key_id_t prk_key = 0;
	bool ok = false;

	if (!import_derive_key(g_ikm, sizeof(g_ikm), extract, &ikm)) {
		integration_kit_uart_puts("import of the HKDF-Extract input key failed\r\n");
		return false;
	}

	if (psa_key_derivation_setup(&op, extract) == PSA_SUCCESS &&
	    psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_SALT, g_salt,
					   sizeof(g_salt)) == PSA_SUCCESS &&
	    psa_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET, ikm) ==
		    PSA_SUCCESS &&
	    psa_key_derivation_output_bytes(&op, prk, sizeof(prk)) == PSA_SUCCESS) {
		ok = memcmp(prk, g_expected_prk, sizeof(prk)) == 0;
	}
	(void)psa_key_derivation_abort(&op);
	(void)psa_destroy_key(ikm);
	if (!ok) {
		integration_kit_uart_puts(
			"HKDF-Extract failed or differs from the RFC 5869 PRK\r\n");
		return false;
	}

	ok = false;
	if (import_derive_key(prk, sizeof(prk), expand, &prk_key) &&
	    psa_key_derivation_setup(&op, expand) == PSA_SUCCESS &&
	    psa_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET, prk_key) ==
		    PSA_SUCCESS &&
	    psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_INFO, g_info,
					   sizeof(g_info)) == PSA_SUCCESS &&
	    psa_key_derivation_output_bytes(&op, okm, sizeof(okm)) == PSA_SUCCESS) {
		ok = memcmp(okm, g_expected_okm, sizeof(okm)) == 0;
	}
	if (!ok) {
		integration_kit_uart_puts("HKDF-Expand failed or differs from RFC 5869 case 1\r\n");
	}

	(void)psa_key_derivation_abort(&op);
	(void)psa_destroy_key(prk_key);
	return ok;
}

static bool unsupported_kdfs_refused(void)
{
	static const psa_algorithm_t refuse[] = {
		ALG_HKDF_SHA1_UNSUPPORTED,
		ALG_TLS12_PSK_TO_MS_SHA256_UNSUPPORTED,
	};
	bool ok = true;

	for (size_t i = 0; i < sizeof(refuse) / sizeof(refuse[0]); i++) {
		psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;

		if (psa_key_derivation_setup(&op, refuse[i]) != PSA_ERROR_NOT_SUPPORTED) {
			integration_kit_uart_puts("out-of-set KDF wrongly admitted\r\n");
			ok = false;
		}
		(void)psa_key_derivation_abort(&op);
	}
	return ok;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts("psa_hkdf: HKDF-SHA-256 via UROT PSA service\r\n");

	(void)psa_crypto_init();

	bool ok = hkdf_one_shot_ok();

	ok = hkdf_extract_expand_ok() && ok;
	ok = unsupported_kdfs_refused() && ok;

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_hkdf") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_hkdf"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
