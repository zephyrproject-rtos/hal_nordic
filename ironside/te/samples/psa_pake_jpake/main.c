/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_pake_jpake.
 *
 * EC-JPAKE over secp256r1 with SHA-256, the exchange Thread commissioning runs, through
 * the standard psa_pake_* API. Both parties run in this one App against UROT's single
 * PAKE slot at a time per side: each party outputs a round, the other inputs it, and the
 * second round follows.
 *   1. with one password on both sides the exchange completes and the two shared keys
 *      agree, which is judged by running HKDF from each key and comparing the outputs
 *   2. with different passwords the exchange still completes, there being no key
 *      confirmation, but the two shared keys differ
 *   3. a SPAKE2+ cipher suite that UROT does not admit (the HMAC variant) is refused at
 *      setup
 *   4. with one password the TLS 1.2 premaster secrets that PSA_ALG_TLS12_ECJPAKE_TO_PMS
 *      derives from the two shared keys agree, and so do the master secrets the TLS 1.2
 *      PRF derives from them, as the DTLS handshake of Thread commissioning derives them
 *   5. the TLS 1.2 PRF with SHA-256 gives the published known answer
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

#define ALG_JPAKE         PSA_ALG_JPAKE(PSA_ALG_SHA_256)
#define MSG_MAX           100
#define ROUND1_MESSAGES   6
#define ROUND2_MESSAGES   3
#define PMS_LEN           32
#define MASTER_SECRET_LEN 48

/* PSA_ALG_SPAKE2P_HMAC(PSA_ALG_SHA_256): the Matter variant is the one UROT admits. */
#define ALG_SPAKE2P_HMAC_UNSUPPORTED ((psa_algorithm_t)0x0a000409)

struct party {
	psa_pake_operation_t op;
	psa_key_id_t password;
	uint8_t msg[ROUND1_MESSAGES + ROUND2_MESSAGES][MSG_MAX];
	size_t msg_len[ROUND1_MESSAGES + ROUND2_MESSAGES];
};

static const psa_pake_step_t g_steps[3] = { PSA_PAKE_STEP_KEY_SHARE, PSA_PAKE_STEP_ZK_PUBLIC,
					    PSA_PAKE_STEP_ZK_PROOF };

static void fill_suite(psa_pake_cipher_suite_t *suite, psa_algorithm_t alg)
{
	*suite = PSA_PAKE_CIPHER_SUITE_INIT;
	psa_pake_cs_set_algorithm(suite, alg);
	psa_pake_cs_set_primitive(suite, PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_ECC,
							    PSA_ECC_FAMILY_SECP_R1, 256));
	psa_pake_cs_set_key_confirmation(suite, PSA_PAKE_UNCONFIRMED_KEY);
}

static bool import_password(const char *password, psa_key_id_t *key)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_type(&attr, PSA_KEY_TYPE_PASSWORD);
	psa_set_key_bits(&attr, 8 * strlen(password));
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, ALG_JPAKE);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);

	return psa_import_key(&attr, (const uint8_t *)password, strlen(password), key) ==
	       PSA_SUCCESS;
}

static bool start_party(struct party *p, const char *password, psa_pake_role_t role,
			const char *user, const char *peer)
{
	psa_pake_cipher_suite_t suite;

	fill_suite(&suite, ALG_JPAKE);
	p->op = PSA_PAKE_OPERATION_INIT;
	if (!import_password(password, &p->password)) {
		integration_kit_uart_puts("import of the password key failed\r\n");
		return false;
	}
	psa_status_t st = psa_pake_setup(&p->op, p->password, &suite);

	if (st == PSA_SUCCESS) {
		st = psa_pake_set_role(&p->op, role);
	}
	if (st == PSA_SUCCESS) {
		st = psa_pake_set_user(&p->op, (const uint8_t *)user, strlen(user));
	}
	if (st == PSA_SUCCESS) {
		st = psa_pake_set_peer(&p->op, (const uint8_t *)peer, strlen(peer));
	}
	if (st != PSA_SUCCESS) {
		integration_kit_uart_printf("PAKE party setup failed: %ld\r\n", (long)st);
	}
	return st == PSA_SUCCESS;
}

/* Output messages [first, first + count) of one round, three steps per share. */
static bool output_round(struct party *p, size_t first, size_t count)
{
	for (size_t i = first; i < first + count; i++) {
		psa_status_t st =
			psa_pake_output(&p->op, g_steps[i % 3], p->msg[i], MSG_MAX, &p->msg_len[i]);

		if (st != PSA_SUCCESS) {
			integration_kit_uart_printf("output %lu failed: %ld\r\n", (unsigned long)i,
						    (long)st);
			return false;
		}
	}
	return true;
}

static psa_status_t input_round(struct party *p, const struct party *from, size_t first,
				size_t count)
{
	for (size_t i = first; i < first + count; i++) {
		psa_status_t st =
			psa_pake_input(&p->op, g_steps[i % 3], from->msg[i], from->msg_len[i]);

		if (st != PSA_SUCCESS) {
			integration_kit_uart_printf("input %lu failed: %ld\r\n", (unsigned long)i,
						    (long)st);
			return st;
		}
	}
	return PSA_SUCCESS;
}

/* Run both rounds; returns the status of the first input that failed, or success. */
static psa_status_t run_exchange(struct party *a, struct party *b)
{
	psa_status_t st = PSA_ERROR_GENERIC_ERROR;

	if (!output_round(a, 0, ROUND1_MESSAGES) || !output_round(b, 0, ROUND1_MESSAGES)) {
		return PSA_ERROR_GENERIC_ERROR;
	}
	st = input_round(a, b, 0, ROUND1_MESSAGES);
	if (st != PSA_SUCCESS) {
		return st;
	}
	st = input_round(b, a, 0, ROUND1_MESSAGES);
	if (st != PSA_SUCCESS) {
		return st;
	}
	if (!output_round(a, ROUND1_MESSAGES, ROUND2_MESSAGES) ||
	    !output_round(b, ROUND1_MESSAGES, ROUND2_MESSAGES)) {
		return PSA_ERROR_GENERIC_ERROR;
	}
	st = input_round(a, b, ROUND1_MESSAGES, ROUND2_MESSAGES);
	if (st != PSA_SUCCESS) {
		return st;
	}
	return input_round(b, a, ROUND1_MESSAGES, ROUND2_MESSAGES);
}

/* The shared key of @p p, for use with the key derivation algorithm @p kdf. */
static bool get_shared_key(struct party *p, psa_algorithm_t kdf, psa_key_id_t *key)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t st = PSA_ERROR_GENERIC_ERROR;

	psa_set_key_type(&attr, PSA_KEY_TYPE_DERIVE);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, kdf);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);
	st = psa_pake_get_shared_key(&p->op, &attr, key);
	if (st != PSA_SUCCESS) {
		integration_kit_uart_printf("psa_pake_get_shared_key failed: %ld\r\n", (long)st);
	}
	return st == PSA_SUCCESS;
}

static bool expand_key(psa_key_id_t key, uint8_t *out, size_t out_len)
{
	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;
	bool ok = psa_key_derivation_setup(&op, PSA_ALG_HKDF(PSA_ALG_SHA_256)) == PSA_SUCCESS &&
		  psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_SALT,
						 (const uint8_t *)"psa_pake_jpake",
						 14) == PSA_SUCCESS &&
		  psa_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET, key) ==
			  PSA_SUCCESS &&
		  psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_INFO,
						 (const uint8_t *)"agree", 5) == PSA_SUCCESS &&
		  psa_key_derivation_output_bytes(&op, out, out_len) == PSA_SUCCESS;

	(void)psa_key_derivation_abort(&op);
	return ok;
}

static bool derive_pms(psa_key_id_t key, uint8_t *pms, size_t pms_len)
{
	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;
	psa_status_t st = psa_key_derivation_setup(&op, PSA_ALG_TLS12_ECJPAKE_TO_PMS);

	if (st == PSA_SUCCESS) {
		st = psa_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET, key);
	}
	if (st == PSA_SUCCESS) {
		st = psa_key_derivation_output_bytes(&op, pms, pms_len);
	}
	if (st != PSA_SUCCESS) {
		integration_kit_uart_printf("TLS 1.2 premaster secret derivation failed: %ld\r\n",
					    (long)st);
	}
	(void)psa_key_derivation_abort(&op);
	return st == PSA_SUCCESS;
}

/* PRF(secret, label, seed), the TLS 1.2 PRF over SHA-256, with the secret as bytes. */
static bool tls12_prf(const uint8_t *secret, size_t secret_len, const char *label,
		      const uint8_t *seed, size_t seed_len, uint8_t *out, size_t out_len)
{
	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;
	psa_status_t st = psa_key_derivation_setup(&op, PSA_ALG_TLS12_PRF(PSA_ALG_SHA_256));

	if (st == PSA_SUCCESS) {
		st = psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_SEED, seed,
						    seed_len);
	}
	if (st == PSA_SUCCESS) {
		st = psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_SECRET, secret,
						    secret_len);
	}
	if (st == PSA_SUCCESS) {
		st = psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_LABEL,
						    (const uint8_t *)label, strlen(label));
	}
	if (st == PSA_SUCCESS) {
		st = psa_key_derivation_output_bytes(&op, out, out_len);
	}
	if (st != PSA_SUCCESS) {
		integration_kit_uart_printf("TLS 1.2 PRF failed: %ld\r\n", (long)st);
	}
	(void)psa_key_derivation_abort(&op);
	return st == PSA_SUCCESS;
}

static void end_party(struct party *p)
{
	(void)psa_pake_abort(&p->op);
	(void)psa_destroy_key(p->password);
}

static struct party g_client;
static struct party g_server;

static bool same_password_agrees(void)
{
	uint8_t out_client[32];
	uint8_t out_server[32];
	psa_key_id_t key_client = 0;
	psa_key_id_t key_server = 0;
	bool ok = false;

	if (start_party(&g_client, "threadjpaketest", PSA_PAKE_ROLE_NONE, "client", "server") &&
	    start_party(&g_server, "threadjpaketest", PSA_PAKE_ROLE_NONE, "server", "client") &&
	    run_exchange(&g_client, &g_server) == PSA_SUCCESS &&
	    get_shared_key(&g_client, PSA_ALG_HKDF(PSA_ALG_SHA_256), &key_client) &&
	    get_shared_key(&g_server, PSA_ALG_HKDF(PSA_ALG_SHA_256), &key_server) &&
	    expand_key(key_client, out_client, sizeof(out_client)) &&
	    expand_key(key_server, out_server, sizeof(out_server))) {
		ok = memcmp(out_client, out_server, sizeof(out_client)) == 0;
		if (!ok) {
			integration_kit_uart_puts("the two shared keys differ\r\n");
		}
	}
	end_party(&g_client);
	end_party(&g_server);
	(void)psa_destroy_key(key_client);
	(void)psa_destroy_key(key_server);
	return ok;
}

/* EC-JPAKE has no key confirmation, so a password mismatch is not an error of the exchange:
 * it ends in two different shared keys, which is what the parties find out when they use them. */
static bool different_password_disagrees(void)
{
	uint8_t out_client[32];
	uint8_t out_server[32];
	psa_key_id_t key_client = 0;
	psa_key_id_t key_server = 0;
	bool ok = false;

	if (start_party(&g_client, "threadjpaketest", PSA_PAKE_ROLE_NONE, "client", "server") &&
	    start_party(&g_server, "otherpassword!!!", PSA_PAKE_ROLE_NONE, "server", "client") &&
	    run_exchange(&g_client, &g_server) == PSA_SUCCESS &&
	    get_shared_key(&g_client, PSA_ALG_HKDF(PSA_ALG_SHA_256), &key_client) &&
	    get_shared_key(&g_server, PSA_ALG_HKDF(PSA_ALG_SHA_256), &key_server) &&
	    expand_key(key_client, out_client, sizeof(out_client)) &&
	    expand_key(key_server, out_server, sizeof(out_server))) {
		ok = memcmp(out_client, out_server, sizeof(out_client)) != 0;
		if (!ok) {
			integration_kit_uart_puts(
				"different passwords gave the same shared key\r\n");
		}
	}
	end_party(&g_client);
	end_party(&g_server);
	(void)psa_destroy_key(key_client);
	(void)psa_destroy_key(key_server);
	return ok;
}

/* The TLS 1.2 premaster secret is 32 bytes, the X coordinate of the shared point, and the
 * master secret is PRF(premaster secret, "master secret", client random + server random). */
static bool premaster_secrets_agree(void)
{
	static const uint8_t randoms[64] = { 0x01, 0x02, 0x03, 0x04 };
	uint8_t pms_client[PMS_LEN];
	uint8_t pms_server[PMS_LEN];
	uint8_t master_client[MASTER_SECRET_LEN];
	uint8_t master_server[MASTER_SECRET_LEN];
	psa_key_id_t key_client = 0;
	psa_key_id_t key_server = 0;
	bool ok = false;

	if (start_party(&g_client, "threadjpaketest", PSA_PAKE_ROLE_NONE, "client", "server") &&
	    start_party(&g_server, "threadjpaketest", PSA_PAKE_ROLE_NONE, "server", "client") &&
	    run_exchange(&g_client, &g_server) == PSA_SUCCESS &&
	    get_shared_key(&g_client, PSA_ALG_TLS12_ECJPAKE_TO_PMS, &key_client) &&
	    get_shared_key(&g_server, PSA_ALG_TLS12_ECJPAKE_TO_PMS, &key_server) &&
	    derive_pms(key_client, pms_client, sizeof(pms_client)) &&
	    derive_pms(key_server, pms_server, sizeof(pms_server)) &&
	    tls12_prf(pms_client, sizeof(pms_client), "master secret", randoms, sizeof(randoms),
		      master_client, sizeof(master_client)) &&
	    tls12_prf(pms_server, sizeof(pms_server), "master secret", randoms, sizeof(randoms),
		      master_server, sizeof(master_server))) {
		ok = memcmp(pms_client, pms_server, sizeof(pms_client)) == 0 &&
		     memcmp(master_client, master_server, sizeof(master_client)) == 0;
		if (!ok) {
			integration_kit_uart_puts("the two premaster or master secrets differ\r\n");
		}
	}
	end_party(&g_client);
	end_party(&g_server);
	(void)psa_destroy_key(key_client);
	(void)psa_destroy_key(key_server);
	return ok;
}

/* The TLS 1.2 PRF SHA-256 test vector published for RFC 5246 implementations. */
static bool tls12_prf_known_answer(void)
{
	static const uint8_t secret[] = { 0x9b, 0xbe, 0x43, 0x6b, 0xa9, 0x40, 0xf0, 0x17,
					  0xb1, 0x76, 0x52, 0x84, 0x9a, 0x71, 0xdb, 0x35 };
	static const uint8_t seed[] = { 0xa0, 0xba, 0x9f, 0x93, 0x6c, 0xda, 0x31, 0x18,
					0x27, 0xa6, 0xf7, 0x96, 0xff, 0xd5, 0x19, 0x8c };
	static const uint8_t expected[] = { 0xe3, 0xf2, 0x29, 0xba, 0x72, 0x7b, 0xe1, 0x7b,
					    0x8d, 0x12, 0x26, 0x20, 0x55, 0x7c, 0xd4, 0x53,
					    0xc2, 0xaa, 0xb2, 0x1d, 0x07, 0xc3, 0xd4, 0x95,
					    0x32, 0x9b, 0x52, 0xd4, 0xe6, 0x1e, 0xdb, 0x5a };
	uint8_t out[sizeof(expected)];

	if (!tls12_prf(secret, sizeof(secret), "test label", seed, sizeof(seed), out,
		       sizeof(out))) {
		return false;
	}
	if (memcmp(out, expected, sizeof(out)) != 0) {
		integration_kit_uart_puts("the TLS 1.2 PRF output is not the known answer\r\n");
		return false;
	}
	return true;
}

static bool unadmitted_variant_refused(void)
{
	psa_pake_operation_t op = PSA_PAKE_OPERATION_INIT;
	psa_pake_cipher_suite_t suite;
	psa_key_id_t key = 0;
	psa_status_t st = PSA_ERROR_GENERIC_ERROR;

	fill_suite(&suite, ALG_SPAKE2P_HMAC_UNSUPPORTED);
	if (!import_password("any", &key)) {
		return false;
	}
	st = psa_pake_setup(&op, key, &suite);
	(void)psa_pake_abort(&op);
	(void)psa_destroy_key(key);
	if (st != PSA_ERROR_NOT_SUPPORTED) {
		integration_kit_uart_printf(
			"SPAKE2+ HMAC setup returned %ld, not NOT_SUPPORTED\r\n", (long)st);
	}
	return st == PSA_ERROR_NOT_SUPPORTED;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts("psa_pake_jpake: EC-JPAKE via UROT PSA service\r\n");

	(void)psa_crypto_init();

	bool ok = same_password_agrees();

	integration_kit_uart_puts(ok ? "same password: keys agree\r\n" :
				       "same password: FAILED\r\n");
	bool mismatch_ok = different_password_disagrees();

	integration_kit_uart_puts(mismatch_ok ? "different password: keys differ\r\n" :
						"different password: FAILED\r\n");
	bool variant_ok = unadmitted_variant_refused();

	integration_kit_uart_puts(variant_ok ? "unadmitted variant: refused\r\n" :
					       "unadmitted variant: FAILED\r\n");
	bool pms_ok = premaster_secrets_agree();

	integration_kit_uart_puts(pms_ok ? "TLS 1.2 premaster and master secrets: agree\r\n" :
					   "TLS 1.2 premaster and master secrets: FAILED\r\n");
	bool prf_ok = tls12_prf_known_answer();

	integration_kit_uart_puts(prf_ok ? "TLS 1.2 PRF: known answer\r\n" :
					   "TLS 1.2 PRF: FAILED\r\n");
	ok = ok && mismatch_ok && variant_ok && pms_ok && prf_ok;

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_pake_jpake") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_pake_jpake"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
