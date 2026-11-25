/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_ecdh_kdf.
 *
 * ECDH chained onto a KDF, so the shared secret never crosses the NSC. This is
 * the counterpart to psa_ecdh: that sample uses psa_raw_key_agreement(), which
 * copies the raw x-coordinate into non-secure RAM and leaves the App to turn it
 * into a key. Here the agreement is an *input step* of a key-derivation
 * operation - UROT computes Z, feeds it straight into the KDF, and the App
 * receives only the id of the derived session key.
 *
 * Two things had to change for this path to exist. The client gained
 * psa_key_derivation_key_agreement() (SID 0x0907). And IronSide's NSC algorithm
 * exact-matching PSA_ALG_ECDH: it now decomposes a key-agreement algorithm and
 * checks the two halves independently, so ECDH chained onto any KDF that is
 * already reachable on its own is admitted. No KDF became reachable in the
 * process - SP 800-108 counter-CMAC (an allowlist exception, since the IKG
 * roots are bound to it) and HKDF are the ones reachable on their own.
 *
 * The scenario is a session-key handshake between two parties A and B:
 *   1. import two secp256r1 key-agreement pairs bound to PSA_ALG_ECDH (the
 *      operation carries the combined algorithm at setup time)
 *   2. exchange public points (psa_export_public_key)
 *   3. each side runs setup(combined) -> key_agreement(INPUT_SECRET) ->
 *      LABEL + CONTEXT -> output_key(volatile AES-256 for GCM)
 *   4. A encrypts a message under its derived key; B decrypts it under *its*
 *      derived key. Success means both sides landed on the same session key
 *      while neither ever held Z - the property the raw form cannot give you.
 *   5. a tampered ciphertext is rejected, and the API contract is checked: the
 *      operation must carry the combined algorithm, so feeding an agreement into
 *      a bare-KDF operation is refused
 *   6. the allowlist is still closed: an out-of-set agreement base (FFDH) and an
 *      out-of-set KDF (TLS 1.2 PSK-to-master-secret) are both refused, in either half of
 *      the pair
 *
 * secp256r1 is used because SP 800-108 counter-CMAC keys the PRF with the shared
 * secret, and P-256's 32-byte Z is a valid AES-256 key. P-384's 48-byte Z is not
 * a legal AES key size, so P-384 works with the raw form (psa_ecdh) but not
 * chained onto this particular KDF.
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

/* ECDH followed by the SP 800-108 counter-CMAC KDF, in one algorithm id. Both
 * the key policy and the derivation operation are bound to this combined form:
 * it is the operation's algorithm that records "a shared secret is expected
 * here", which is why a bare KDF cannot accept an agreement (checked below). */
#define ALG_ECDH_KDF PSA_ALG_KEY_AGREEMENT(PSA_ALG_ECDH, PSA_ALG_SP800_108_COUNTER_CMAC)

/*
 * Two algorithms UROT must still refuse, used by the last check below. Spelled
 * out here rather than added to the client's PSA replica precisely because they
 * are not part of the supported surface: FFDH is not an in-set agreement base,
 * and the TLS 1.2 PSK-to-master-secret KDF is not an in-set KDF (nor compiled into
 * UROT). Values from the PSA Crypto spec: PSA_ALG_FFDH, and
 * PSA_ALG_TLS12_PSK_TO_MS(PSA_ALG_SHA_256).
 */
#define ALG_FFDH_UNSUPPORTED                   ((psa_algorithm_t)0x09010000)
#define ALG_TLS12_PSK_TO_MS_SHA256_UNSUPPORTED ((psa_algorithm_t)0x08000309)

/*
 * Two secp256r1 private scalars, one per party. Both lead with a byte well below
 * the curve order's leading 0xFF, so each is unambiguously in [1, n-1].
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

/* Domain separation for the KDF. Both parties must use the same pair or they
 * derive independent keys from the same Z and the GCM decrypt below fails. */
static const uint8_t g_label[] = "ironside-ecdh-session-key/v1";
static const uint8_t g_context[] = "channel-0";

/* The message A sends to B under the derived session key, its clear-but-
 * authenticated header (AAD), and a fixed nonce (one message per session here). */
static const uint8_t g_msg[32] = {
	'I', 'r', 'o', 'n', 'S', 'i', 'd', 'e', ' ', 'E', 'C', 'D', 'H', ' ', 's', 'e',
	's', 's', 'i', 'o', 'n', ' ', 'm', 'e', 's', 's', 'a', 'g', 'e', ' ', '#', '1',
};
static const uint8_t g_aad[8] = { 's', 'e', 's', 's', 'h', 'd', 'r', '0' };
static const uint8_t g_nonce[12] = {
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb,
};

/* Import a secp256r1 key-agreement private key bound to ALG_ECDH_KDF; 0 on failure. */
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

/*
 * One side of the handshake: agree with @peer_pub, run the shared secret through
 * the KDF, and land a volatile AES-256 GCM session key. The shared secret exists
 * only inside UROT for the duration - nothing in this function ever sees it.
 */
static psa_status_t derive_session_key(psa_key_id_t private_key, const uint8_t *peer_pub,
				       size_t peer_pub_len, psa_key_id_t *key)
{
	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;

	status = psa_key_derivation_setup(&op, ALG_ECDH_KDF);
	if (status == PSA_SUCCESS) {
		status = psa_key_derivation_key_agreement(&op, PSA_KEY_DERIVATION_INPUT_SECRET,
							  private_key, peer_pub, peer_pub_len);
	}
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

/*
 * The operation's algorithm is what makes an agreement a legal input step, so a
 * bare-KDF operation must refuse one. Worth pinning: the failure is otherwise a
 * confusing PSA_ERROR_INVALID_ARGUMENT from deep inside the core, and getting it
 * right is the difference between this path working and not.
 */
static bool bare_kdf_rejects_agreement(psa_key_id_t private_key, const uint8_t *peer_pub,
				       size_t peer_pub_len)
{
	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;
	bool refused = true;

	if (psa_key_derivation_setup(&op, PSA_ALG_SP800_108_COUNTER_CMAC) == PSA_SUCCESS) {
		refused = psa_key_derivation_key_agreement(&op, PSA_KEY_DERIVATION_INPUT_SECRET,
							   private_key, peer_pub,
							   peer_pub_len) != PSA_SUCCESS;
	}

	(void)psa_key_derivation_abort(&op);
	return refused;
}

/*
 * The allowlist now decomposes a key-agreement algorithm instead of matching it
 * exactly, so pin that it still refuses a half it should: an out-of-set base
 * (FFDH) chained onto an in-set KDF, and an in-set base chained onto an
 * out-of-set KDF (TLS 1.2 PSK-to-master-secret). Both are refused at setup, before the
 * call reaches the PSA Crypto core - widening the agreement gate must not have widened
 * the set.
 */
static bool allowlist_still_closed(void)
{
	static const psa_algorithm_t refuse[] = {
		PSA_ALG_KEY_AGREEMENT(ALG_FFDH_UNSUPPORTED, PSA_ALG_SP800_108_COUNTER_CMAC),
		PSA_ALG_KEY_AGREEMENT(PSA_ALG_ECDH, ALG_TLS12_PSK_TO_MS_SHA256_UNSUPPORTED),
	};
	bool ok = true;

	for (size_t i = 0; i < sizeof(refuse) / sizeof(refuse[0]); i++) {
		psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;

		if (psa_key_derivation_setup(&op, refuse[i]) != PSA_ERROR_NOT_SUPPORTED) {
			integration_kit_uart_puts("out-of-set agreement wrongly admitted\r\n");
			ok = false;
		}
		(void)psa_key_derivation_abort(&op);
	}
	return ok;
}

static bool ecdh_kdf_ok(void)
{
	/* secp256r1 public point: 0x04 || X(32) || Y(32) = 65 bytes. */
	static uint8_t pub_a[65];
	static uint8_t pub_b[65];
	static uint8_t ct[sizeof(g_msg) + 16]; /* ciphertext || 16-byte GCM tag */
	static uint8_t pt[sizeof(g_msg)];
	size_t pub_a_len = 0, pub_b_len = 0, ct_len = 0, pt_len = 0;
	psa_key_id_t priv_key_a = import_ecdh_key(priv_a);
	psa_key_id_t priv_key_b = import_ecdh_key(priv_b);
	psa_key_id_t sess_a = 0;
	psa_key_id_t sess_b = 0;
	bool ok = false;

	if (priv_key_a == 0 || priv_key_b == 0) {
		integration_kit_uart_puts("psa_import_key (secp256r1) failed\r\n");
		goto out_priv;
	}

	/* (2) Exchange public points. */
	if (psa_export_public_key(priv_key_a, pub_a, sizeof(pub_a), &pub_a_len) != PSA_SUCCESS ||
	    psa_export_public_key(priv_key_b, pub_b, sizeof(pub_b), &pub_b_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_export_public_key failed\r\n");
		goto out_priv;
	}

	/* (3) Each side derives its session key from the agreement, secret-side. */
	if (derive_session_key(priv_key_a, pub_b, pub_b_len, &sess_a) != PSA_SUCCESS) {
		integration_kit_uart_puts("A: ECDH+KDF derive failed\r\n");
		goto out_priv;
	}
	if (derive_session_key(priv_key_b, pub_a, pub_a_len, &sess_b) != PSA_SUCCESS) {
		integration_kit_uart_puts("B: ECDH+KDF derive failed\r\n");
		goto out_sess_a;
	}
	integration_kit_uart_puts(
		"both sides derived a session key; shared secret never left UROT\r\n");

	/* (4) A encrypts under its key, B decrypts under its own. Matching
	 * plaintext is the proof that the two derivations agree. */
	if (psa_aead_encrypt(sess_a, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), g_aad, sizeof(g_aad),
			     g_msg, sizeof(g_msg), ct, sizeof(ct), &ct_len) != PSA_SUCCESS) {
		integration_kit_uart_puts("A: GCM encrypt failed\r\n");
		goto out_sess_b;
	}
	if (psa_aead_decrypt(sess_b, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), g_aad, sizeof(g_aad),
			     ct, ct_len, pt, sizeof(pt), &pt_len) != PSA_SUCCESS ||
	    pt_len != sizeof(g_msg) || memcmp(pt, g_msg, sizeof(g_msg)) != 0) {
		integration_kit_uart_puts("B: could not decrypt A's message - keys differ\r\n");
		goto out_sess_b;
	}
	integration_kit_uart_puts("B decrypted A's message: session keys agree\r\n");

	/* (5a) Tamper one ciphertext byte - GCM must reject it. */
	ct[0] ^= 0xff;
	if (psa_aead_decrypt(sess_b, PSA_ALG_GCM, g_nonce, sizeof(g_nonce), g_aad, sizeof(g_aad),
			     ct, ct_len, pt, sizeof(pt), &pt_len) == PSA_SUCCESS) {
		integration_kit_uart_puts("tampered message wrongly accepted\r\n");
		goto out_sess_b;
	}

	/* (5b) A bare KDF operation must refuse an agreement input. */
	if (!bare_kdf_rejects_agreement(priv_key_a, pub_b, pub_b_len)) {
		integration_kit_uart_puts("bare KDF wrongly accepted an agreement input\r\n");
		goto out_sess_b;
	}
	integration_kit_uart_puts("tamper rejected; bare-KDF operation refuses an agreement\r\n");

	/* (6) The NSC allowlist is still closed around this new path. */
	if (!allowlist_still_closed()) {
		goto out_sess_b;
	}
	integration_kit_uart_puts("out-of-set agreement halves still refused\r\n");
	ok = true;

out_sess_b:
	(void)psa_destroy_key(sess_b);
out_sess_a:
	(void)psa_destroy_key(sess_a);
out_priv:
	(void)psa_destroy_key(priv_key_a);
	(void)psa_destroy_key(priv_key_b);
	return ok;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts(
		"psa_ecdh_kdf: ECDH into a KDF - session key without exposing Z\r\n");

	(void)psa_crypto_init();

	bool ok = ecdh_kdf_ok();

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_ecdh_kdf") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_ecdh_kdf"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
