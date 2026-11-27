/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_x25519.
 *
 * X25519 key agreement on Curve25519, checked against the RFC 7748 sec. 6.1
 * known-answer vector. psa_ecdh runs the same primitive on secp256r1 and proves
 * it by self-consistency - both sides reach the same secret - which cannot catch
 * an encoding that is wrong in the same way at both ends. Here the two public
 * keys and the shared secret are all pinned to the RFC, so the byte order and
 * the encoding are checked too. Those are the parts of this curve most easily
 * got wrong, because Curve25519 differs from secp256r1 in four ways:
 *
 *   - the key is 255 bits, not 256: psa_set_key_bits() names the scalar field,
 *     while every buffer here is 32 bytes (the same trap as Ed25519).
 *   - the public key is the bare 32-byte u-coordinate in little-endian order,
 *     with no SEC1 0x04 prefix - not the 65-byte 0x04||X||Y point secp256r1
 *     exports. Code ported from psa_ecdh has to resize its buffers.
 *   - the shared secret is likewise 32 little-endian bytes.
 *   - the private scalar must be imported already clamped; see g_priv_a.
 *
 * The sample then pins the reach of the family: PSA_ECC_FAMILY_MONTGOMERY also
 * covers Curve448, which UROT does not build, so a 448-bit key cannot be used.
 *
 * A Montgomery key does key agreement and nothing else - PSA lists no signature
 * algorithm as compatible with this curve family. Give a device that needs to
 * sign a separate Ed25519 key (psa_ed25519); never reuse an agreement scalar for
 * signatures.
 *
 * The raw form is used throughout because it is what makes the RFC vector
 * checkable - psa_raw_key_agreement() hands the shared secret to the App. In
 * production prefer the combined agreement+KDF form (psa_ecdh_kdf), which keeps
 * the secret inside UROT; X25519 works there exactly as secp256r1 does, since
 * its 32-byte shared secret is a valid AES-256 key for the SP 800-108 PRF.
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

/* Curve25519's scalar field is 2^255-19, so PSA calls the key 255 bits even
 * though the encoded scalar and the public key are both 32 bytes. */
#define X25519_KEY_BITS 255
#define X25519_BYTES    32

/*
 * RFC 7748 sec. 6.1, with the sec. 5 clamping already applied to the two private
 * scalars: PSA requires a Montgomery private key to be imported in clamped form
 * (the forced bits set to zero or one), while the RFC prints the raw scalars.
 * Clamping is what X25519 would apply internally anyway, so the public keys and
 * the shared secret below are the RFC's values unchanged - a & 248 on the first
 * byte and & 127 | 64 on the last is the whole difference (0x77->0x70 and
 * 0x2a->0x6a for Alice).
 */
static const uint8_t g_priv_a[X25519_BYTES] = {
	0x70, 0x07, 0x6d, 0x0a, 0x73, 0x18, 0xa5, 0x7d, 0x3c, 0x16, 0xc1,
	0x72, 0x51, 0xb2, 0x66, 0x45, 0xdf, 0x4c, 0x2f, 0x87, 0xeb, 0xc0,
	0x99, 0x2a, 0xb1, 0x77, 0xfb, 0xa5, 0x1d, 0xb9, 0x2c, 0x6a,
};
static const uint8_t g_pub_a[X25519_BYTES] = {
	0x85, 0x20, 0xf0, 0x09, 0x89, 0x30, 0xa7, 0x54, 0x74, 0x8b, 0x7d,
	0xdc, 0xb4, 0x3e, 0xf7, 0x5a, 0x0d, 0xbf, 0x3a, 0x0d, 0x26, 0x38,
	0x1a, 0xf4, 0xeb, 0xa4, 0xa9, 0x8e, 0xaa, 0x9b, 0x4e, 0x6a,
};
static const uint8_t g_priv_b[X25519_BYTES] = {
	0x58, 0xab, 0x08, 0x7e, 0x62, 0x4a, 0x8a, 0x4b, 0x79, 0xe1, 0x7f,
	0x8b, 0x83, 0x80, 0x0e, 0xe6, 0x6f, 0x3b, 0xb1, 0x29, 0x26, 0x18,
	0xb6, 0xfd, 0x1c, 0x2f, 0x8b, 0x27, 0xff, 0x88, 0xe0, 0x6b,
};
static const uint8_t g_pub_b[X25519_BYTES] = {
	0xde, 0x9e, 0xdb, 0x7d, 0x7b, 0x7d, 0xc1, 0xb4, 0xd3, 0x5b, 0x61,
	0xc2, 0xec, 0xe4, 0x35, 0x37, 0x3f, 0x83, 0x43, 0xc8, 0x5b, 0x78,
	0x67, 0x4d, 0xad, 0xfc, 0x7e, 0x14, 0x6f, 0x88, 0x2b, 0x4f,
};
static const uint8_t g_shared[X25519_BYTES] = {
	0x4a, 0x5d, 0x9d, 0x5b, 0xa4, 0xce, 0x2d, 0xe1, 0x72, 0x8e, 0x3b,
	0xf4, 0x80, 0x35, 0x0f, 0x25, 0xe0, 0x7e, 0x21, 0xc9, 0x47, 0xd1,
	0x9e, 0x33, 0x76, 0xf0, 0x9b, 0x3c, 0x1e, 0x16, 0x17, 0x42,
};

/* A valid Curve448 scalar (RFC 7748 sec. 5 clamping: & 252 on the first byte, | 128
 * on the last), so the refusal checked below is the curve being unavailable and
 * not the value being rejected. */
static const uint8_t g_priv_c448[56] = {
	0x30, 0x37, 0x3e, 0x45, 0x4c, 0x53, 0x5a, 0x61, 0x68, 0x6f, 0x76, 0x7d, 0x84, 0x8b,
	0x92, 0x99, 0xa0, 0xa7, 0xae, 0xb5, 0xbc, 0xc3, 0xca, 0xd1, 0xd8, 0xdf, 0xe6, 0xed,
	0xf4, 0xfb, 0x02, 0x09, 0x10, 0x17, 0x1e, 0x25, 0x2c, 0x33, 0x3a, 0x41, 0x48, 0x4f,
	0x56, 0x5d, 0x64, 0x6b, 0x72, 0x79, 0x80, 0x87, 0x8e, 0x95, 0x9c, 0xa3, 0xaa, 0xb1,
};

/*
 * Export buffers are deliberately 65 bytes - the size a secp256r1 point needs -
 * rather than the 32 X25519 uses. An implementation that wrongly emitted a
 * prefixed or doubled point would then still fit, and the length check below
 * would report the real length instead of a buffer-too-small error that hides it.
 */
#define PUB_BUF_BYTES 65

/* Import an X25519 private key for agreement; returns its id, or 0 on failure. */
static psa_key_id_t import_x25519_pair(const uint8_t *scalar)
{
	psa_key_id_t key = 0;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
	psa_set_key_bits(&attr, X25519_KEY_BITS);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	if (psa_import_key(&attr, scalar, X25519_BYTES, &key) != PSA_SUCCESS) {
		return 0;
	}
	return key;
}

/* Export a public key and require it to be exactly the 32-byte u-coordinate. */
static bool export_pub_32(psa_key_id_t key, uint8_t *out)
{
	static uint8_t buf[PUB_BUF_BYTES];
	size_t len = 0;

	if (psa_export_public_key(key, buf, sizeof(buf), &len) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_export_public_key failed\r\n");
		return false;
	}
	if (len != X25519_BYTES) {
		integration_kit_uart_puts("public key is not the bare 32-byte u-coordinate\r\n");
		return false;
	}
	memcpy(out, buf, X25519_BYTES);
	return true;
}

/*
 * The RFC 7748 sec. 6.1 vector: derive both public keys from the private scalars and
 * agree in both directions, checking each against the standard.
 */
static bool known_answer_ok(void)
{
	static uint8_t pub_a[X25519_BYTES];
	static uint8_t pub_b[X25519_BYTES];
	static uint8_t z_ab[X25519_BYTES];
	static uint8_t z_ba[X25519_BYTES];
	size_t z_ab_len = 0, z_ba_len = 0;
	psa_key_id_t key_a = import_x25519_pair(g_priv_a);
	psa_key_id_t key_b = import_x25519_pair(g_priv_b);
	bool ok = false;

	if (key_a == 0 || key_b == 0) {
		integration_kit_uart_puts("psa_import_key (x25519 pair) failed\r\n");
		goto out;
	}

	if (!export_pub_32(key_a, pub_a) || !export_pub_32(key_b, pub_b)) {
		goto out;
	}
	if (memcmp(pub_a, g_pub_a, X25519_BYTES) != 0 ||
	    memcmp(pub_b, g_pub_b, X25519_BYTES) != 0) {
		integration_kit_uart_puts("derived public key does not match RFC 7748\r\n");
		goto out;
	}
	integration_kit_uart_puts("public keys match RFC 7748 (32-byte u, little-endian)\r\n");

	if (psa_raw_key_agreement(PSA_ALG_ECDH, key_a, pub_b, X25519_BYTES, z_ab, sizeof(z_ab),
				  &z_ab_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("X25519(privA, pubB) failed\r\n");
		goto out;
	}
	if (psa_raw_key_agreement(PSA_ALG_ECDH, key_b, pub_a, X25519_BYTES, z_ba, sizeof(z_ba),
				  &z_ba_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("X25519(privB, pubA) failed\r\n");
		goto out;
	}
	if (z_ab_len != X25519_BYTES || z_ba_len != X25519_BYTES) {
		integration_kit_uart_puts("shared secret is not 32 bytes\r\n");
		goto out;
	}
	if (memcmp(z_ab, g_shared, X25519_BYTES) != 0 ||
	    memcmp(z_ba, g_shared, X25519_BYTES) != 0) {
		integration_kit_uart_puts("shared secret does not match RFC 7748\r\n");
		goto out;
	}
	integration_kit_uart_puts("shared secret matches RFC 7748, both directions\r\n");
	ok = true;

out:
	(void)psa_destroy_key(key_a);
	(void)psa_destroy_key(key_b);
	return ok;
}

/*
 * A peer's public key can also be held as a key object in its own right, which
 * is the shape an App uses when it stores a counterparty's identity rather than
 * receiving it on the wire. Import Bob's public key alone and read it back.
 */
static bool public_key_round_trips(void)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key = 0;
	static uint8_t pub[X25519_BYTES];
	bool ok = false;

	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_MONTGOMERY));
	psa_set_key_bits(&attr, X25519_KEY_BITS);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	if (psa_import_key(&attr, g_pub_b, sizeof(g_pub_b), &key) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_import_key (x25519 public) failed\r\n");
		return false;
	}
	ok = export_pub_32(key, pub) && memcmp(pub, g_pub_b, X25519_BYTES) == 0;
	if (!ok) {
		integration_kit_uart_puts("public-key import did not round-trip\r\n");
	}
	(void)psa_destroy_key(key);
	return ok;
}

/*
 * Generating the pair inside UROT is the form real code should use: the private
 * scalar is created by the CRACEN DRBG and never exists in non-secure memory.
 * There is no vector to check it against, so agree with Alice both ways -
 * matching secrets prove the generated key is a usable, self-consistent pair.
 */
static bool generated_key_agrees(void)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_gen = 0;
	psa_key_id_t key_a = 0;
	static uint8_t pub_gen[X25519_BYTES];
	static uint8_t z_ga[X25519_BYTES];
	static uint8_t z_ag[X25519_BYTES];
	size_t z_ga_len = 0, z_ag_len = 0;
	bool ok = false;

	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
	psa_set_key_bits(&attr, X25519_KEY_BITS);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	if (psa_generate_key(&attr, &key_gen) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_generate_key (x25519) failed\r\n");
		goto out;
	}
	key_a = import_x25519_pair(g_priv_a);
	if (key_a == 0) {
		integration_kit_uart_puts("psa_import_key (x25519 pair) failed\r\n");
		goto out;
	}
	if (!export_pub_32(key_gen, pub_gen)) {
		goto out;
	}

	if (psa_raw_key_agreement(PSA_ALG_ECDH, key_gen, g_pub_a, sizeof(g_pub_a), z_ga,
				  sizeof(z_ga), &z_ga_len) != PSA_SUCCESS ||
	    psa_raw_key_agreement(PSA_ALG_ECDH, key_a, pub_gen, X25519_BYTES, z_ag, sizeof(z_ag),
				  &z_ag_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("agreement with the generated key failed\r\n");
		goto out;
	}
	if (z_ga_len != X25519_BYTES || z_ag_len != X25519_BYTES ||
	    memcmp(z_ga, z_ag, X25519_BYTES) != 0) {
		integration_kit_uart_puts("generated key: secrets differ\r\n");
		goto out;
	}
	integration_kit_uart_puts("generated pair agrees with Alice; scalar never left UROT\r\n");
	ok = true;

out:
	(void)psa_destroy_key(key_gen);
	(void)psa_destroy_key(key_a);
	return ok;
}

/*
 * PSA_ECC_FAMILY_MONTGOMERY covers Curve448 as well, but UROT builds only
 * Curve25519, so no Curve448 key can be put to work. The refusal lands at first
 * use rather than at import - psa_import_key() accepts the 448-bit attributes
 * and only psa_export_public_key() reports the curve is missing - so check that
 * the key is unusable rather than that any one call fails.
 */
static bool x448_unusable(void)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key = 0;
	static uint8_t pub[120];
	size_t pub_len = 0;
	bool unusable = true;

	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
	psa_set_key_bits(&attr, 448);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	if (psa_import_key(&attr, g_priv_c448, sizeof(g_priv_c448), &key) != PSA_SUCCESS) {
		return true;
	}
	unusable = psa_export_public_key(key, pub, sizeof(pub), &pub_len) != PSA_SUCCESS;
	(void)psa_destroy_key(key);
	return unusable;
}

static bool x25519_ok(void)
{
	if (!known_answer_ok()) {
		return false;
	}
	if (!public_key_round_trips()) {
		return false;
	}
	if (!generated_key_agrees()) {
		return false;
	}
	if (!x448_unusable()) {
		integration_kit_uart_puts("a Curve448 key was wrongly usable\r\n");
		return false;
	}
	integration_kit_uart_puts("Curve448 unusable: Curve25519 is the only curve built\r\n");
	return true;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts(
		"psa_x25519: X25519 key agreement via UROT, against RFC 7748\r\n");

	(void)psa_crypto_init();

	bool ok = x25519_ok();

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_x25519") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_x25519"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
