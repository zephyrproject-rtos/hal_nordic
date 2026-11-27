/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_pake_spake2p.
 *
 * SPAKE2+, the Matter variant, over secp256r1, the exchange Matter's PASE runs, through the
 * standard psa_pake_* API. Both parties run in this one App:
 *   1. the passcode is a PSA_KEY_TYPE_PASSWORD key and PBKDF2 derives the prover's key pair
 *      from it; the verifier holds only the public verifier key exported from that pair
 *   2. with one passcode the exchange completes, key confirmation passes both ways and the
 *      two shared keys agree, which is judged by running HKDF from each key
 *   3. with a different passcode on the verifier key confirmation fails
 *   4. the other SPAKE2+ variants and a SPAKE2+ key on another curve are refused
 *   5. the verifier is set up the way Matter's PSASpake2p::BeginVerifier() does it: a verifier
 *      key with no size or lifetime stated, a cipher suite with no key confirmation stated,
 *      empty identities and a 32-byte context
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

#define MSG_MAX 100

/* PSA_ALG_SPAKE2P_CMAC(PSA_ALG_SHA_256), a variant UROT does not admit. */
#define ALG_SPAKE2P_CMAC_UNSUPPORTED ((psa_algorithm_t)0x0a000509)
#define ECC_FAMILY_SECP_K1           ((psa_ecc_family_t)0x17)

#define PBKDF2_ITERATIONS 1000
static const uint8_t g_salt[] = "SPAKE2P Key Salt";
static const uint8_t g_context[] = "CHIP PAKE V1 Commissioning";

struct party {
	psa_pake_operation_t op;
	psa_key_id_t key;
	uint8_t share[MSG_MAX];
	size_t share_len;
	uint8_t confirm[MSG_MAX];
	size_t confirm_len;
};

static struct party g_prover;
static struct party g_verifier;

static void fill_suite(psa_pake_cipher_suite_t *suite, psa_algorithm_t alg)
{
	*suite = PSA_PAKE_CIPHER_SUITE_INIT;
	psa_pake_cs_set_algorithm(suite, alg);
	psa_pake_cs_set_primitive(suite, PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_ECC,
							    PSA_ECC_FAMILY_SECP_R1, 256));
	psa_pake_cs_set_key_confirmation(suite, PSA_PAKE_CONFIRMED_KEY);
}

/* Derive the prover's SPAKE2+ key pair from a passcode, as Matter's PASE does. */
static bool derive_key_pair(const char *passcode, psa_key_id_t *pair)
{
	psa_key_attributes_t pw_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_attributes_t pair_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_derivation_operation_t kd = PSA_KEY_DERIVATION_OPERATION_INIT;
	psa_key_id_t pw = 0;
	psa_status_t st = PSA_ERROR_GENERIC_ERROR;

	psa_set_key_type(&pw_attr, PSA_KEY_TYPE_PASSWORD);
	psa_set_key_usage_flags(&pw_attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&pw_attr, PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_256));
	psa_set_key_lifetime(&pw_attr, PSA_KEY_LIFETIME_VOLATILE);
	st = psa_import_key(&pw_attr, (const uint8_t *)passcode, strlen(passcode), &pw);
	if (st != PSA_SUCCESS) {
		integration_kit_uart_printf("import of the passcode failed: %ld\r\n", (long)st);
		return false;
	}

	psa_set_key_type(&pair_attr, PSA_KEY_TYPE_SPAKE2P_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&pair_attr, 256);
	psa_set_key_usage_flags(&pair_attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&pair_attr, PSA_ALG_SPAKE2P_MATTER);
	psa_set_key_lifetime(&pair_attr, PSA_KEY_LIFETIME_VOLATILE);

	st = psa_key_derivation_setup(&kd, PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_256));
	if (st == PSA_SUCCESS) {
		st = psa_key_derivation_input_integer(&kd, PSA_KEY_DERIVATION_INPUT_COST,
						      PBKDF2_ITERATIONS);
	}
	if (st == PSA_SUCCESS) {
		st = psa_key_derivation_input_bytes(&kd, PSA_KEY_DERIVATION_INPUT_SALT, g_salt,
						    sizeof(g_salt) - 1);
	}
	if (st == PSA_SUCCESS) {
		st = psa_key_derivation_input_key(&kd, PSA_KEY_DERIVATION_INPUT_PASSWORD, pw);
	}
	if (st == PSA_SUCCESS) {
		st = psa_key_derivation_output_key(&pair_attr, &kd, pair);
	}
	if (st != PSA_SUCCESS) {
		integration_kit_uart_printf("derivation of the SPAKE2+ key pair failed: %ld\r\n",
					    (long)st);
	}
	(void)psa_key_derivation_abort(&kd);
	(void)psa_destroy_key(pw);
	return st == PSA_SUCCESS;
}

/* The verifier key: what a device that never sees the passcode stores. */
static bool derive_verifier_key(const char *passcode, psa_key_id_t *verifier)
{
	uint8_t pub[MSG_MAX + 60];
	size_t pub_len = 0;
	psa_key_id_t pair = 0;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t st = PSA_ERROR_GENERIC_ERROR;

	if (!derive_key_pair(passcode, &pair)) {
		return false;
	}
	st = psa_export_public_key(pair, pub, sizeof(pub), &pub_len);
	(void)psa_destroy_key(pair);
	if (st != PSA_SUCCESS) {
		integration_kit_uart_printf("export of the verifier key failed: %ld\r\n", (long)st);
		return false;
	}

	psa_set_key_type(&attr, PSA_KEY_TYPE_SPAKE2P_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_SPAKE2P_MATTER);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);
	st = psa_import_key(&attr, pub, pub_len, verifier);
	if (st != PSA_SUCCESS) {
		integration_kit_uart_printf(
			"import of the verifier key (%lu bytes) failed: %ld\r\n",
			(unsigned long)pub_len, (long)st);
	}
	return st == PSA_SUCCESS;
}

static bool start_party(struct party *p, psa_pake_role_t role)
{
	psa_pake_cipher_suite_t suite;
	psa_status_t st = PSA_ERROR_GENERIC_ERROR;

	fill_suite(&suite, PSA_ALG_SPAKE2P_MATTER);
	p->op = PSA_PAKE_OPERATION_INIT;
	st = psa_pake_setup(&p->op, p->key, &suite);
	if (st == PSA_SUCCESS) {
		st = psa_pake_set_role(&p->op, role);
	}
	if (st == PSA_SUCCESS) {
		st = psa_pake_set_context(&p->op, g_context, sizeof(g_context) - 1);
	}
	if (st != PSA_SUCCESS) {
		integration_kit_uart_printf("PAKE party setup failed: %ld\r\n", (long)st);
	}
	return st == PSA_SUCCESS;
}

static psa_status_t step_output(const char *what, struct party *p, psa_pake_step_t step,
				uint8_t *buf, size_t *len)
{
	psa_status_t st = psa_pake_output(&p->op, step, buf, MSG_MAX, len);

	if (st != PSA_SUCCESS) {
		integration_kit_uart_printf("output %s failed: %ld\r\n", what, (long)st);
	}
	return st;
}

static psa_status_t step_input(const char *what, struct party *p, psa_pake_step_t step,
			       const uint8_t *buf, size_t len)
{
	psa_status_t st = psa_pake_input(&p->op, step, buf, len);

	if (st != PSA_SUCCESS) {
		integration_kit_uart_printf("input %s failed: %ld\r\n", what, (long)st);
	}
	return st;
}

/* The prover opens, the verifier answers with its share and confirmation, and the prover
 * confirms last. Returns the first failing status. */
static psa_status_t run_exchange(struct party *prover, struct party *verifier)
{
	psa_status_t st = PSA_ERROR_GENERIC_ERROR;

	st = step_output("shareP", prover, PSA_PAKE_STEP_KEY_SHARE, prover->share,
			 &prover->share_len);
	if (st == PSA_SUCCESS) {
		st = step_input("shareP", verifier, PSA_PAKE_STEP_KEY_SHARE, prover->share,
				prover->share_len);
	}
	if (st == PSA_SUCCESS) {
		st = step_output("shareV", verifier, PSA_PAKE_STEP_KEY_SHARE, verifier->share,
				 &verifier->share_len);
	}
	if (st == PSA_SUCCESS) {
		st = step_output("confirmV", verifier, PSA_PAKE_STEP_CONFIRM, verifier->confirm,
				 &verifier->confirm_len);
	}
	if (st == PSA_SUCCESS) {
		st = step_input("shareV", prover, PSA_PAKE_STEP_KEY_SHARE, verifier->share,
				verifier->share_len);
	}
	if (st == PSA_SUCCESS) {
		st = step_input("confirmV", prover, PSA_PAKE_STEP_CONFIRM, verifier->confirm,
				verifier->confirm_len);
	}
	if (st == PSA_SUCCESS) {
		st = step_output("confirmP", prover, PSA_PAKE_STEP_CONFIRM, prover->confirm,
				 &prover->confirm_len);
	}
	if (st == PSA_SUCCESS) {
		st = step_input("confirmP", verifier, PSA_PAKE_STEP_CONFIRM, prover->confirm,
				prover->confirm_len);
	}
	return st;
}

static bool get_shared_key(struct party *p, psa_key_id_t *key)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t st = PSA_ERROR_GENERIC_ERROR;

	psa_set_key_type(&attr, PSA_KEY_TYPE_DERIVE);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_HKDF(PSA_ALG_SHA_256));
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
						 (const uint8_t *)"psa_pake_spake2p",
						 16) == PSA_SUCCESS &&
		  psa_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET, key) ==
			  PSA_SUCCESS &&
		  psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_INFO,
						 (const uint8_t *)"agree", 5) == PSA_SUCCESS &&
		  psa_key_derivation_output_bytes(&op, out, out_len) == PSA_SUCCESS;

	(void)psa_key_derivation_abort(&op);
	return ok;
}

static void end_party(struct party *p)
{
	(void)psa_pake_abort(&p->op);
	(void)psa_destroy_key(p->key);
	p->key = 0;
}

/* Run an exchange where the prover derived its pair from @prover_passcode and the verifier
 * holds the verifier key of @verifier_passcode; return the status of the first failure. */
static psa_status_t exchange_for(const char *prover_passcode, const char *verifier_passcode,
				 uint8_t *out_prover, uint8_t *out_verifier, size_t out_len)
{
	psa_key_id_t key_prover = 0;
	psa_key_id_t key_verifier = 0;
	psa_status_t st = PSA_ERROR_GENERIC_ERROR;

	if (derive_key_pair(prover_passcode, &g_prover.key) &&
	    derive_verifier_key(verifier_passcode, &g_verifier.key) &&
	    start_party(&g_prover, PSA_PAKE_ROLE_CLIENT) &&
	    start_party(&g_verifier, PSA_PAKE_ROLE_SERVER)) {
		st = run_exchange(&g_prover, &g_verifier);
		if (st == PSA_SUCCESS) {
			st = (get_shared_key(&g_prover, &key_prover) &&
			      get_shared_key(&g_verifier, &key_verifier) &&
			      expand_key(key_prover, out_prover, out_len) &&
			      expand_key(key_verifier, out_verifier, out_len)) ?
				     PSA_SUCCESS :
				     PSA_ERROR_GENERIC_ERROR;
		}
	}
	end_party(&g_prover);
	end_party(&g_verifier);
	(void)psa_destroy_key(key_prover);
	(void)psa_destroy_key(key_verifier);
	return st;
}

static bool same_passcode_agrees(void)
{
	uint8_t out_prover[32] = { 0 };
	uint8_t out_verifier[32] = { 1 };

	if (exchange_for("20202021", "20202021", out_prover, out_verifier, sizeof(out_prover)) !=
	    PSA_SUCCESS) {
		return false;
	}
	if (memcmp(out_prover, out_verifier, sizeof(out_prover)) != 0) {
		integration_kit_uart_puts("the two shared keys differ\r\n");
		return false;
	}
	return true;
}

static bool different_passcode_caught(void)
{
	uint8_t out_prover[32];
	uint8_t out_verifier[32];
	psa_status_t st =
		exchange_for("20202021", "20202022", out_prover, out_verifier, sizeof(out_prover));

	if (st == PSA_SUCCESS) {
		integration_kit_uart_puts(
			"a passcode mismatch was not caught by key confirmation\r\n");
	}
	return st != PSA_SUCCESS;
}

static bool unadmitted_refused(void)
{
	psa_pake_operation_t op = PSA_PAKE_OPERATION_INIT;
	psa_pake_cipher_suite_t suite;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t material[97] = { 0 };
	psa_key_id_t key = 0;
	psa_status_t st_variant = PSA_ERROR_GENERIC_ERROR;
	psa_status_t st_curve = PSA_ERROR_GENERIC_ERROR;

	/* A password key is enough to reach the suite check, which comes first. */
	psa_set_key_type(&attr, PSA_KEY_TYPE_PASSWORD);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_SPAKE2P_MATTER);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);
	if (psa_import_key(&attr, (const uint8_t *)"any", 3, &key) != PSA_SUCCESS) {
		return false;
	}
	fill_suite(&suite, ALG_SPAKE2P_CMAC_UNSUPPORTED);
	st_variant = psa_pake_setup(&op, key, &suite);
	(void)psa_pake_abort(&op);
	(void)psa_destroy_key(key);

	psa_reset_key_attributes(&attr);
	psa_set_key_type(&attr, PSA_KEY_TYPE_SPAKE2P_PUBLIC_KEY(ECC_FAMILY_SECP_K1));
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_SPAKE2P_MATTER);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_VOLATILE);
	st_curve = psa_import_key(&attr, material, sizeof(material), &key);
	if (st_curve == PSA_SUCCESS) {
		(void)psa_destroy_key(key);
	}

	if (st_variant != PSA_ERROR_NOT_SUPPORTED || st_curve != PSA_ERROR_NOT_SUPPORTED) {
		integration_kit_uart_printf(
			"variant setup %ld, other-curve key import %ld, want %ld\r\n",
			(long)st_variant, (long)st_curve, (long)PSA_ERROR_NOT_SUPPORTED);
	}
	return st_variant == PSA_ERROR_NOT_SUPPORTED && st_curve == PSA_ERROR_NOT_SUPPORTED;
}

/* BeginVerifier() of Matter's PSASpake2p: nothing is stated that the defaults can supply. */
static bool matter_verifier_sequence_ok(void)
{
	static const uint8_t context[32] = { 1,   8,   15,  22,  29,  36,  43,  50,  57,  64,  71,
					     78,  85,  92,  99,  106, 113, 120, 127, 134, 141, 148,
					     155, 162, 169, 176, 183, 190, 197, 204, 211, 218 };
	uint8_t verifier_key_data[MSG_MAX + 60];
	size_t verifier_key_data_len = 0;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_pake_cipher_suite_t suite = PSA_PAKE_CIPHER_SUITE_INIT;
	psa_pake_operation_t op = PSA_PAKE_OPERATION_INIT;
	psa_key_id_t pair = 0;
	psa_key_id_t key = 0;
	psa_status_t st = PSA_ERROR_GENERIC_ERROR;

	if (!derive_key_pair("20202021", &pair) ||
	    psa_export_public_key(pair, verifier_key_data, sizeof(verifier_key_data),
				  &verifier_key_data_len) != PSA_SUCCESS) {
		(void)psa_destroy_key(pair);
		return false;
	}
	(void)psa_destroy_key(pair);

	psa_pake_cs_set_algorithm(&suite, PSA_ALG_SPAKE2P_MATTER);
	psa_pake_cs_set_primitive(&suite, PSA_PAKE_PRIMITIVE(PSA_PAKE_PRIMITIVE_TYPE_ECC,
							     PSA_ECC_FAMILY_SECP_R1, 256));
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_SPAKE2P_MATTER);
	psa_set_key_type(&attr, PSA_KEY_TYPE_SPAKE2P_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
	st = psa_import_key(&attr, verifier_key_data, verifier_key_data_len, &key);
	if (st == PSA_SUCCESS) {
		st = psa_pake_setup(&op, key, &suite);
	}
	if (st == PSA_SUCCESS) {
		st = psa_pake_set_role(&op, PSA_PAKE_ROLE_SERVER);
	}
	if (st == PSA_SUCCESS) {
		st = psa_pake_set_peer(&op, NULL, 0);
	}
	if (st == PSA_SUCCESS) {
		st = psa_pake_set_user(&op, NULL, 0);
	}
	if (st == PSA_SUCCESS) {
		st = psa_pake_set_context(&op, context, sizeof(context));
	}
	if (st != PSA_SUCCESS) {
		integration_kit_uart_printf("Matter verifier sequence failed: %ld\r\n", (long)st);
	}
	(void)psa_pake_abort(&op);
	(void)psa_destroy_key(key);
	return st == PSA_SUCCESS;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts(
		"psa_pake_spake2p: SPAKE2+ Matter variant via UROT PSA service\r\n");

	(void)psa_crypto_init();

	bool agree_ok = same_passcode_agrees();

	integration_kit_uart_puts(agree_ok ? "same passcode: keys agree\r\n" :
					     "same passcode: FAILED\r\n");
	bool mismatch_ok = different_passcode_caught();

	integration_kit_uart_puts(mismatch_ok ? "different passcode: caught\r\n" :
						"different passcode: FAILED\r\n");
	bool refused_ok = unadmitted_refused();

	integration_kit_uart_puts(refused_ok ? "other variant and curve: refused\r\n" :
					       "other variant and curve: FAILED\r\n");
	bool matter_ok = matter_verifier_sequence_ok();

	integration_kit_uart_puts(matter_ok ? "Matter verifier sequence: ok\r\n" :
					      "Matter verifier sequence: FAILED\r\n");
	bool ok = agree_ok && mismatch_ok && refused_ok && matter_ok;

	integration_kit_uart_puts(ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_pake_spake2p") :
				       INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_pake_spake2p"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
