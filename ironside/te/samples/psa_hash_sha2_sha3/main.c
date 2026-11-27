/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_hash_sha2_sha3.
 *
 * Exercises the SHA-2 and SHA-3 hashes added to UROT's PSA Crypto service:
 * SHA-384, SHA-512 (SHA2-384/512) and SHA3-256. The original psa_sha256 sample already
 * covers SHA-256; this one proves the wider digest set is reachable from the
 * non-secure side over the same NSC veneer and passes UROT's NSC algorithm
 * allowlist (the SHA-2/3 family is in-set).
 *
 * Each digest is checked single-shot (psa_hash_compute) against the published
 * FIPS-180-4 / FIPS-202 "abc" vector. Console: UARTE20, TX = P1.16, 115200 8N1.
 */

#include <integration_kit_uart.h>
#include <integration_kit_test.h>
#include <ironside_client/psa/crypto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* FIPS-180-4 / FIPS-202 digests of the message "abc". */
static const uint8_t sha384_abc[48] = {
	0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b, 0xb5, 0xa0, 0x3d, 0x69,
	0x9a, 0xc6, 0x50, 0x07, 0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63,
	0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed, 0x80, 0x86, 0x07, 0x2b,
	0xa1, 0xe7, 0xcc, 0x23, 0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7,
};
static const uint8_t sha512_abc[64] = {
	0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba, 0xcc, 0x41, 0x73, 0x49, 0xae,
	0x20, 0x41, 0x31, 0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2, 0x0a, 0x9e,
	0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a, 0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1,
	0xa8, 0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd, 0x45, 0x4d, 0x44, 0x23,
	0x64, 0x3c, 0xe8, 0x0e, 0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f,
};
static const uint8_t sha3_256_abc[32] = {
	0x3a, 0x98, 0x5d, 0xa7, 0x4f, 0xe2, 0x25, 0xb2, 0x04, 0x5c, 0x17,
	0x2d, 0x6b, 0xd3, 0x90, 0xbd, 0x85, 0x5f, 0x08, 0x6e, 0x3e, 0x9d,
	0x52, 0x5b, 0x46, 0xbf, 0xe2, 0x45, 0x11, 0x43, 0x15, 0x32,
};

static bool digest_ok(const char *name, psa_algorithm_t alg, const uint8_t *expected,
		      size_t expected_len)
{
	static uint8_t digest[64];
	size_t digest_len = 0;
	psa_status_t status = psa_hash_compute(alg, (const uint8_t *)"abc", 3, digest,
					       sizeof(digest), &digest_len);

	integration_kit_uart_puts(name);
	if (status != PSA_SUCCESS) {
		integration_kit_uart_puts(": psa_hash_compute failed\r\n");
		return false;
	}
	if (digest_len != expected_len || memcmp(digest, expected, expected_len) != 0) {
		integration_kit_uart_puts(": digest mismatch\r\n");
		return false;
	}
	integration_kit_uart_puts(": ok\r\n");
	return true;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts(
		"psa_hash_sha2_sha3: SHA-384/512 + SHA3-256(\"abc\") via UROT\r\n");

	(void)psa_crypto_init();

	bool ok = digest_ok("SHA-384", PSA_ALG_SHA_384, sha384_abc, sizeof(sha384_abc));
	ok = digest_ok("SHA-512", PSA_ALG_SHA_512, sha512_abc, sizeof(sha512_abc)) && ok;
	ok = digest_ok("SHA3-256", PSA_ALG_SHA3_256, sha3_256_abc, sizeof(sha3_256_abc)) && ok;

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_hash_sha2_sha3") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_hash_sha2_sha3"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
