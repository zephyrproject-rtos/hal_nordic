/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * A small CBOR decoder for PSA Initial Attestation tokens, for the
 * psa_initial_attestation sample. The client library returns the token as
 * bytes; decoding it is the application's job. Copy this file, or replace it
 * with your own CBOR decoder.
 *
 * Claims are found by label, not by position: a claim's position shifts as
 * later releases add claims, but its label does not.
 */

#ifndef ATTEST_CBOR_H
#define ATTEST_CBOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* What a claim's value turned out to be. */
enum attest_claim_type {
	ATTEST_CLAIM_UINT,  /* value in .number; e.g. security-lifecycle */
	ATTEST_CLAIM_INT,   /* value in .number, negative; e.g. client-id */
	ATTEST_CLAIM_BYTES, /* value in .bytes/.len; e.g. instance-id */
	ATTEST_CLAIM_TEXT,  /* value in .bytes/.len, not NUL-terminated */
	ATTEST_CLAIM_ARRAY, /* .bytes/.len span the encoded array */
};

struct attest_claim {
	int32_t label;
	enum attest_claim_type type;
	int32_t number;
	const uint8_t *bytes;
	size_t len;
};

/*
 * A token, decoded down to its parts.
 *
 * The three spans are the token's own bytes, in place - nothing is copied, so
 * the token buffer must outlive this struct.
 *
 * protected_hdr and payload are handed back as encoded byte strings, not
 * decoded contents, because a COSE verifier needs them that way: the
 * Sig_structure is ["Signature1", protected_hdr_as_a_bstr, h'', payload_bstr].
 */
struct attest_token {
	const uint8_t *protected_hdr; /* the bstr's content: {1: -7}, ES256 */
	size_t protected_hdr_len;

	const uint8_t *payload; /* the bstr's content: the encoded claims map */
	size_t payload_len;

	const uint8_t *signature; /* ECDSA P-256, raw r || s */
	size_t signature_len;

	uint32_t claim_count; /* entries in the claims map */
};

/*
 * Decode @token's COSE_Sign1 envelope into @out.
 *
 * Checks the envelope as it goes - the COSE_Sign1 tag, the four-element array,
 * an empty unprotected header, and that the payload and signature lie inside
 * the buffer - so a caller that gets `true` holds a token, not merely some
 * bytes. It does not verify the signature: that needs the device's public
 * Identity Attestation Key, registered off-device at manufacturing time.
 */
bool attest_token_parse(struct attest_token *out, const uint8_t *token, size_t len);

/*
 * Find the claim labelled @label and decode its value into @out.
 *
 * False if the claim is absent, appears twice, or the map is malformed.
 */
bool attest_token_claim(const struct attest_token *token, int32_t label, struct attest_claim *out);

/* One record of the software-components claim: what IronSide measured of one
 * image, the version it read from that image's header, and the verification key
 * the signature was checked against. Every span is in place in the token. */
struct attest_sw_component {
	const uint8_t *type; /* measurement-type: "UROT", "APP" or "DFU" */
	size_t type_len;
	const uint8_t *measurement; /* SHA-256 over the image's signed region */
	size_t measurement_len;
	const uint8_t *version; /* "NNN.NNN.NNN+NNN", zero-padded */
	size_t version_len;
	const uint8_t *signer_id; /* SHA-256 of the verification key record */
	size_t signer_id_len;
};

/*
 * Decode record @index of the software-components claim @sw (as returned by
 * attest_token_claim() for label 2399) into @out.
 *
 * The records are in a fixed order today - UROT, the App, the DFU image - but
 * read the measurement-type rather than relying on the index if you care which
 * image a record describes.
 */
bool attest_sw_component(const struct attest_claim *sw, size_t index,
			 struct attest_sw_component *out);

#endif /* ATTEST_CBOR_H */
