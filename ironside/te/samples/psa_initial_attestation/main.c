/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide Integration Kit - psa_initial_attestation.
 *
 * A non-secure, bare-metal App that asks UROT for a signed Initial Attestation
 * token over the PSA API (PSA Certified Attestation API 2.0, whose token
 * format is RFC 9783 - full scope: all
 * RFC-9783-MUST claims plus the device-uuid vendor claim, and exactly one
 * supported challenge size (32 bytes) - deliberately not a certifiable
 * attestation report).
 *
 * This sample is the readable demo: a challenge, get_token_size() then
 * get_token(), a report naming every claim in the token, and a hex dump of the
 * whole token so a human (or an off-target cbor2/cryptography script) can
 * inspect and verify it. The automated, on-target correctness gate - signature
 * verification, tamper detection, negative-argument cases - lives in Nordic's
 * own test suite.
 *
 * The report is produced by DECODING the token with a CBOR parser, looking each
 * claim up by its label. That parser is attest_cbor.c, in this sample
 * directory: about 200 lines, no allocation, and no dependency beyond the C
 * library, so the sample is complete on a bare-metal build with no CBOR module
 * available. Replace it with QCBOR, zcbor, TinyCBOR or your RTOS's own decoder
 * - the token is ordinary CBOR and any of them reads it. The format is
 * ../../client/psa_attest_token.cddl.
 *
 * Do not read a claim at a byte offset. A claim's position depends on the claim
 * set, and every IronSide release that has added a claim moved every byte
 * behind it, so hardcoded offsets break on an update - silently, returning a
 * well-formed value from the wrong claim rather than an error. Searching for a
 * label survives that, which is why the client library publishes the labels and
 * no offsets. Reading a claim this App cares about - its own lifecycle, its
 * measurements, the boot seed - is a few lines either way.
 *
 * A remote verifier decodes the token the same way, and additionally verifies
 * the signature; this is only the device's view of what it sent.
 *
 * The challenge is the token's only freshness input. In a real
 * deployment the remote verifier generates it, and the verifier must check
 * that the token's nonce claim echoes the value it sent - a token whose nonce
 * is not checked proves nothing about when it was produced, and replays. This
 * sample has no verifier to talk to, so it draws the challenge from
 * psa_generate_random() rather than hardcoding one: a fixed challenge in
 * copied code is how a replayable deployment starts.
 *
 * Console: UARTE20, TX = P1.16, 115200 8N1 (same as the other psa_* samples).
 */

#include "attest_cbor.h"

#include <integration_kit_uart.h>
#include <integration_kit_test.h>
#include <ironside_client/psa/crypto.h>
#include <ironside_client/psa/initial_attestation.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* The challenge a remote verifier would have supplied. Stands in for one here;
 * see the note above. */
static uint8_t g_challenge[PSA_INITIAL_ATTEST_CHALLENGE_SIZE_32];

static uint8_t g_token[PSA_INITIAL_ATTEST_MAX_TOKEN_SIZE];

/* Hex-dump @data over UART in fixed-width lines. Needs no buffer of its own:
 * integration_kit_uart_printf() streams as it formats, so a 500-byte token costs
 * nothing in RAM here. */
static void uart_hex_dump(const uint8_t *data, size_t len)
{
	enum { BYTES_PER_LINE = 32 };

	for (size_t off = 0; off < len; off += BYTES_PER_LINE) {
		size_t n = len - off < BYTES_PER_LINE ? len - off : BYTES_PER_LINE;

		integration_kit_uart_hexdump(&data[off], n);
		integration_kit_uart_puts("\r\n");
	}
}

/* "<label><hex of @data>" as one line. integration_kit_uart_hexdump() writes no prefix
 * or newline of its own, which is what lets one helper serve every claim
 * value the report prints. */
static void report_hex(const char *label, const uint8_t *data, size_t len)
{
	integration_kit_uart_puts(label);
	integration_kit_uart_hexdump(data, len);
	integration_kit_uart_puts("\r\n");
}

/* A claim's text value, quoted. Written with integration_kit_uart_write() because
 * these strings are fixed-length spans inside the token and carry no NUL. */
static void report_text(const char *text, size_t len)
{
	integration_kit_uart_puts("\"");
	integration_kit_uart_write(text, len);
	integration_kit_uart_puts("\"");
}

static uint32_t be16(const uint8_t *p)
{
	return ((uint32_t)p[0] << 8) | (uint32_t)p[1];
}

static uint32_t be32(const uint8_t *p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) |
	       (uint32_t)p[3];
}

/* The PSA lifecycle spelled for a reader; NULL for a value that is none of
 * them. */
static const char *lifecycle_name(uint32_t lifecycle)
{
	switch (lifecycle) {
	case PSA_LIFECYCLE_UNKNOWN:
		return "UNKNOWN";
	case PSA_LIFECYCLE_ASSEMBLY_AND_TEST:
		return "ASSEMBLY_AND_TEST";
	case PSA_LIFECYCLE_PSA_ROT_PROVISIONING:
		return "PSA_ROT_PROVISIONING";
	case PSA_LIFECYCLE_SECURED:
		return "SECURED";
	case PSA_LIFECYCLE_NON_PSA_ROT_DEBUG:
		return "NON_PSA_ROT_DEBUG";
	case PSA_LIFECYCLE_DECOMMISSIONED:
		return "DECOMMISSIONED";
	default:
		return NULL;
	}
}

/*
 * Print one implementation-id word as its ASCII reading when it has one and as
 * hex when it does not. A part whose VARIANT is not printable must not put raw
 * bytes in a console log, where they read as a defect rather than as an
 * unprintable VARIANT.
 */
static void report_word_text(const uint8_t *word)
{
	for (unsigned int i = 0U; i < 4U; i++) {
		if (word[i] < 0x20U || word[i] >= 0x7FU) {
			integration_kit_uart_printf("0x%08lx", (unsigned long)be32(word));
			return;
		}
	}

	integration_kit_uart_printf("\"%c%c%c%c\"", word[0], word[1], word[2], word[3]);
}

/*
 * Decode the implementation-id claim: which part this is, and which version of
 * IronSide's immutable boot stage it runs. The claim is transparent by design,
 * so a bench can hold this line against the value an endorsement was published
 * against.
 *
 * A scheme version this code does not know means the 32 bytes follow a later
 * layout (or a certification scheme's own), so they are reported as opaque
 * rather than read as fields that may have moved.
 *
 * The PSA_INITIAL_ATTEST_IMPL_ID_* offsets used below are the one kind of
 * offset that survives: they are offsets into this claim's value, not into the
 * token. A CBOR parser hands back the 32 bytes and does not resolve what is
 * inside them, so the client library publishes their layout - and the scheme
 * version in the first byte is what makes a later layout safe to add.
 */
static void report_implementation_id(const uint8_t *id, size_t len)
{
	report_hex("                               ", id, len);

	if (len != PSA_INITIAL_ATTEST_IMPLEMENTATION_ID_LEN) {
		integration_kit_uart_printf("      opaque: %u bytes, expected %u\r\n",
					    (unsigned int)len,
					    (unsigned int)PSA_INITIAL_ATTEST_IMPLEMENTATION_ID_LEN);
		return;
	}

	if (id[PSA_INITIAL_ATTEST_IMPL_ID_SCHEME_OFFSET] != PSA_INITIAL_ATTEST_IMPL_ID_SCHEME_V1) {
		integration_kit_uart_printf(
			"      opaque: scheme version %u\r\n",
			(unsigned int)id[PSA_INITIAL_ATTEST_IMPL_ID_SCHEME_OFFSET]);
		return;
	}

	const uint8_t *irot = &id[PSA_INITIAL_ATTEST_IMPL_ID_IROT_VERSION_OFFSET];

	integration_kit_uart_printf(
		"      scheme version %u, PART 0x%08lx, VARIANT ",
		(unsigned int)PSA_INITIAL_ATTEST_IMPL_ID_SCHEME_V1,
		(unsigned long)be32(&id[PSA_INITIAL_ATTEST_IMPL_ID_PART_OFFSET]));
	report_word_text(&id[PSA_INITIAL_ATTEST_IMPL_ID_VARIANT_OFFSET]);
	integration_kit_uart_printf(
		", CONFIGID 0x%08lx\r\n",
		(unsigned long)be32(&id[PSA_INITIAL_ATTEST_IMPL_ID_CONFIGID_OFFSET]));
	integration_kit_uart_printf(
		"      RAM %lu kB, RRAM %lu kB, IROT %u.%u.%u+%u\r\n",
		(unsigned long)be16(&id[PSA_INITIAL_ATTEST_IMPL_ID_RAM_KB_OFFSET]),
		(unsigned long)be16(&id[PSA_INITIAL_ATTEST_IMPL_ID_RRAM_KB_OFFSET]),
		(unsigned int)irot[0], (unsigned int)irot[1], (unsigned int)irot[2],
		(unsigned int)irot[3]);
}

/*
 * Print the software-components claim: what IronSide measured of each image it
 * booted or would boot, in the order the claim carries them. The measurement
 * is over the image's signed region and the signer-id names the verification
 * key its signature was checked against, so the pair says both what ran and
 * who was trusted to say it could.
 */
static void report_sw_components(const struct attest_claim *sw)
{
	for (size_t i = 0U; i < PSA_INITIAL_ATTEST_SW_COMPONENT_COUNT; i++) {
		struct attest_sw_component rec;

		if (!attest_sw_component(sw, i, &rec)) {
			integration_kit_uart_printf("      [%u] could not be decoded\r\n",
						    (unsigned int)i);
			continue;
		}

		integration_kit_uart_printf("      [%u] measurement-type ", (unsigned int)i);
		report_text((const char *)rec.type, rec.type_len);
		integration_kit_uart_puts(", version ");
		report_text((const char *)rec.version, rec.version_len);
		integration_kit_uart_puts("\r\n");
		report_hex("          measurement-value    ", rec.measurement, rec.measurement_len);
		report_hex("          signer-id            ", rec.signer_id, rec.signer_id_len);
	}
}

/* Print one claim's header line and hand back its value, or say it is missing.
 * NULL if the claim is absent or malformed, which is a finding rather than a
 * shrug: every claim this sample prints is one the format promises. */
static const struct attest_claim *claim_of(const struct attest_token *token, int32_t label,
					   const char *name, struct attest_claim *out)
{
	if (!attest_token_claim(token, label, out)) {
		/* No field width: integration_kit_uart_printf() implements no '-' flag and
		 * ignores a width on %s (samples/lib/integration_kit_uart.c). */
		integration_kit_uart_printf("    %s (%ld) MISSING or malformed\r\n", name,
					    (long)label);
		return NULL;
	}
	return out;
}

/*
 * Print every claim in @token, in ascending claim-label order rather than in
 * the order the map happens to carry them, so two runs - or two devices, or
 * the same device across a firmware update - read against each other line by
 * line. The vendor claim goes last, where its negative label would otherwise
 * put it first: the RFC's claims are the ones a reader compares against the
 * spec.
 *
 * Claim order in the token is not part of its contract, which is exactly why
 * this can choose its own: every claim is fetched by label
 * (attest_token_claim()), so the printing order is a presentation choice here
 * and not a fact about the encoding.
 */
static void print_token_report(const struct attest_token *token, size_t token_size,
			       size_t challenge_size)
{
	struct attest_claim c;
	const struct attest_claim *v = NULL;

	integration_kit_uart_printf("token report: challenge_size=%u, token_size=%u\r\n",
				    (unsigned int)challenge_size, (unsigned int)token_size);

	/* The envelope. protected_hdr and payload are the two spans a COSE
	 * verifier reconstructs the Sig_structure from, which is the one thing
	 * a token's bytes are still needed for - and the parser hands them
	 * back, so even that needs no offset. */
	integration_kit_uart_puts("  envelope             COSE_Sign1, protected header ");
	integration_kit_uart_hexdump(token->protected_hdr, token->protected_hdr_len);
	integration_kit_uart_puts(" ({1: -7}, ES256)\r\n");
	integration_kit_uart_printf("  signature            %u B, ECDSA P-256\r\n",
				    (unsigned int)token->signature_len);
	integration_kit_uart_printf("  claims               %u, in a %u-byte payload\r\n",
				    (unsigned int)token->claim_count,
				    (unsigned int)token->payload_len);
	if (token->claim_count != PSA_INITIAL_ATTEST_CLAIM_COUNT) {
		integration_kit_uart_printf("    (expected %u -- a release may have added one)\r\n",
					    (unsigned int)PSA_INITIAL_ATTEST_CLAIM_COUNT);
	}

	v = claim_of(token, PSA_INITIAL_ATTEST_CLAIM_NONCE, "nonce", &c);
	if (v != NULL) {
		bool echoed = v->len == challenge_size &&
			      memcmp(v->bytes, g_challenge, challenge_size) == 0;

		integration_kit_uart_printf("    nonce               (%d)    %u B, %s\r\n",
					    PSA_INITIAL_ATTEST_CLAIM_NONCE, (unsigned int)v->len,
					    echoed ? "echoes the challenge" :
						     "does not echo the challenge");
		report_hex("                               ", v->bytes, v->len);
	}

	v = claim_of(token, PSA_INITIAL_ATTEST_CLAIM_INSTANCE_ID, "instance-id", &c);
	if (v != NULL && v->len == PSA_INITIAL_ATTEST_INSTANCE_ID_LEN) {
		integration_kit_uart_printf(
			"    instance-id         (%d)  %u B, UEID type 0x%02x\r\n",
			PSA_INITIAL_ATTEST_CLAIM_INSTANCE_ID, (unsigned int)v->len,
			(unsigned int)v->bytes[0]);
		report_hex("      SHA-256(IAK public key)  ", &v->bytes[1], v->len - 1U);
	}

	v = claim_of(token, PSA_INITIAL_ATTEST_CLAIM_PROFILE, "profile-definition", &c);
	if (v != NULL) {
		integration_kit_uart_printf("    profile-definition  (%d)  ",
					    PSA_INITIAL_ATTEST_CLAIM_PROFILE);
		report_text((const char *)v->bytes, v->len);
		integration_kit_uart_puts("\r\n");
	}

	v = claim_of(token, PSA_INITIAL_ATTEST_CLAIM_BOOT_SEED, "boot-seed", &c);
	if (v != NULL) {
		integration_kit_uart_printf(
			"    boot-seed           (%d)  %u B, this boot session\r\n",
			PSA_INITIAL_ATTEST_CLAIM_BOOT_SEED, (unsigned int)v->len);
		report_hex("                               ", v->bytes, v->len);
	}

	v = claim_of(token, PSA_INITIAL_ATTEST_CLAIM_CLIENT_ID, "client-id", &c);
	if (v != NULL) {
		/* A CBOR negative integer, so the parser returns the value
		 * itself - there is no sign convention to reimplement. */
		integration_kit_uart_printf("    client-id           (%d) %ld%s\r\n",
					    PSA_INITIAL_ATTEST_CLAIM_CLIENT_ID, (long)v->number,
					    v->number == PSA_INITIAL_ATTEST_CLIENT_ID_NONSECURE ?
						    " (a non-secure caller)" :
						    "");
	}

	v = claim_of(token, PSA_INITIAL_ATTEST_CLAIM_LIFECYCLE, "security-lifecycle", &c);
	if (v != NULL) {
		const char *lcs = lifecycle_name((uint32_t)v->number);

		integration_kit_uart_printf("    security-lifecycle  (%d) 0x%04lx %s\r\n",
					    PSA_INITIAL_ATTEST_CLAIM_LIFECYCLE,
					    (unsigned long)v->number,
					    lcs != NULL ? lcs : "(not a PSA lifecycle value)");
	}

	v = claim_of(token, PSA_INITIAL_ATTEST_CLAIM_IMPLEMENTATION_ID, "implementation-id", &c);
	if (v != NULL) {
		integration_kit_uart_printf("    implementation-id   (%d) %u B\r\n",
					    PSA_INITIAL_ATTEST_CLAIM_IMPLEMENTATION_ID,
					    (unsigned int)v->len);
		report_implementation_id(v->bytes, v->len);
	}

	v = claim_of(token, PSA_INITIAL_ATTEST_CLAIM_SW_COMPONENTS, "software-components", &c);
	if (v != NULL) {
		integration_kit_uart_printf("    software-components (%d) %u records\r\n",
					    PSA_INITIAL_ATTEST_CLAIM_SW_COMPONENTS,
					    (unsigned int)PSA_INITIAL_ATTEST_SW_COMPONENT_COUNT);
		report_sw_components(v);
	}

	v = claim_of(token, PSA_INITIAL_ATTEST_CLAIM_DEVICE_UUID, "device-uuid", &c);
	if (v != NULL) {
		integration_kit_uart_printf("    device-uuid    (%d) %u B\r\n",
					    PSA_INITIAL_ATTEST_CLAIM_DEVICE_UUID,
					    (unsigned int)v->len);
		report_hex("                               ", v->bytes, v->len);
	}
}

static bool attestation_ok(void)
{
	size_t token_size = 0;
	size_t reported_size = 0;

	psa_status_t status = psa_generate_random(g_challenge, sizeof(g_challenge));

	if (status != PSA_SUCCESS) {
		integration_kit_uart_puts("psa_generate_random failed\r\n");
		return false;
	}

	/* Printed so a reader can match it against the token's nonce claim,
	 * which the report holds against it as well. */
	integration_kit_uart_puts("challenge: ");
	integration_kit_uart_hexdump(g_challenge, sizeof(g_challenge));
	integration_kit_uart_puts("\r\n");

	status = psa_initial_attest_get_token_size(PSA_INITIAL_ATTEST_CHALLENGE_SIZE_32,
						   &reported_size);
	if (status != PSA_SUCCESS) {
		integration_kit_uart_puts("get_token_size failed\r\n");
		return false;
	}
	if (reported_size > sizeof(g_token)) {
		integration_kit_uart_puts("bug: reported token size exceeds sample buffer\r\n");
		return false;
	}

	status = psa_initial_attest_get_token(g_challenge, sizeof(g_challenge), g_token,
					      sizeof(g_token), &token_size);
	if (status != PSA_SUCCESS) {
		integration_kit_uart_puts("get_token failed\r\n");
		return false;
	}
	if (token_size != reported_size) {
		integration_kit_uart_puts("bug: get_token size != get_token_size\r\n");
		return false;
	}

	/* Decode the envelope before reading a single claim. A token that does
	 * not parse is reported as such rather than half-read: this is the step
	 * that replaces trusting the buffer's shape. */
	struct attest_token token;

	if (!attest_token_parse(&token, g_token, token_size)) {
		integration_kit_uart_puts(
			"token is not a well-formed COSE_Sign1 attestation token\r\n");
		return false;
	}
	if (token.signature_len != PSA_INITIAL_ATTEST_SIGNATURE_LEN) {
		integration_kit_uart_printf("bug: signature is %u bytes, expected %u\r\n",
					    (unsigned int)token.signature_len,
					    (unsigned int)PSA_INITIAL_ATTEST_SIGNATURE_LEN);
		return false;
	}

	print_token_report(&token, token_size, sizeof(g_challenge));

	integration_kit_uart_printf("token (%u bytes):\r\n", (unsigned int)token_size);
	uart_hex_dump(g_token, token_size);

	return true;
}

int main(void)
{
	integration_kit_uart_init();
	integration_kit_uart_puts(
		"psa_initial_attestation: get a signed Initial Attestation token\r\n");

	(void)psa_crypto_init();

	bool ok = attestation_ok();

	integration_kit_uart_puts(
		ok ? INTEGRATION_KIT_TEST_SUCCESS_LINE("psa_initial_attestation") :
		     INTEGRATION_KIT_TEST_FAIL_LINE);
	integration_kit_uart_puts(INTEGRATION_KIT_TEST_FINISHED_LINE("psa_initial_attestation"));

	for (;;) {
		__asm__ volatile("wfi");
	}
}
