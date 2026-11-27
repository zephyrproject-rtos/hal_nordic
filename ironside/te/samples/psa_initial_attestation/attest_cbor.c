/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * A small CBOR decoder for PSA Initial Attestation tokens.
 */

#include "attest_cbor.h"

/* CBOR major types (RFC 8949 section 3.1), the seven a token uses. */
#define MT_UINT   0U
#define MT_NEGINT 1U
#define MT_BSTR   2U
#define MT_TSTR   3U
#define MT_ARRAY  4U
#define MT_MAP    5U
#define MT_TAG    6U

#define COSE_SIGN1_TAG 18U

/* A read position in the token. `err` latches, so a run of reads can be
 * chained and tested once at the end. */
struct reader {
	const uint8_t *buf;
	size_t len;
	size_t pos;
	bool err;
};

/*
 * Read one CBOR head: its major type into @mt, its argument into @arg.
 *
 * The argument is a uint32_t, so an 8-byte argument is REFUSED rather than
 * truncated - nothing in a token needs one, and narrowing it silently would
 * turn a malformed token into a plausible-looking claim. Additional
 * information 31 is an indefinite length, which RFC 9783 section 5.1.1 forbids
 * in an attestation token outright, and 28-30 are reserved; refusing all three
 * here is what makes definite-length encoding a checked property rather than an
 * assumption.
 */
static bool head(struct reader *r, uint8_t *mt, uint32_t *arg)
{
	if (r->err || r->pos >= r->len) {
		r->err = true;
		return false;
	}

	uint8_t ib = r->buf[r->pos++];
	uint8_t ai = (uint8_t)(ib & 0x1FU);
	size_t n = 0U;

	*mt = (uint8_t)(ib >> 5);
	if (ai < 24U) {
		*arg = ai;
		return true;
	}
	switch (ai) {
	case 24U:
		n = 1U;
		break;
	case 25U:
		n = 2U;
		break;
	case 26U:
		n = 4U;
		break;
	default:
		r->err = true;
		return false;
	}
	if (r->len - r->pos < n) {
		r->err = true;
		return false;
	}

	uint32_t v = 0;

	for (size_t i = 0; i < n; i++) {
		v = (v << 8) | r->buf[r->pos++];
	}
	*arg = v;
	return true;
}

/* Read one head and require its major type. */
static bool expect(struct reader *r, uint8_t want_mt, uint32_t *arg)
{
	uint8_t mt = 0xFFU;

	if (!head(r, &mt, arg) || mt != want_mt) {
		r->err = true;
		return false;
	}
	return true;
}

/* Consume @n bytes and return them in place. */
static const uint8_t *take(struct reader *r, uint32_t n)
{
	if (r->err || (r->len - r->pos) < n) {
		r->err = true;
		return NULL;
	}

	const uint8_t *p = &r->buf[r->pos];

	r->pos += n;
	return p;
}

/*
 * Skip the item at the read position, including a container and its contents.
 *
 * This is what makes a label search possible: walk the claims map and step over
 * the value of every claim that is not the wanted one, without having to know
 * how to decode it. Iterative rather than recursive, with a pending-item count
 * - a malformed token must not get to choose how deep this recurses.
 */
static bool skip(struct reader *r)
{
	size_t pending = 1U;

	while (pending > 0U) {
		uint8_t mt = 0xFFU;
		uint32_t arg = 0;

		if (!head(r, &mt, &arg)) {
			return false;
		}
		pending--;

		switch (mt) {
		case MT_BSTR:
		case MT_TSTR:
			if (take(r, arg) == NULL) {
				return false;
			}
			break;
		case MT_ARRAY:
			pending += arg;
			break;
		case MT_MAP:
			if (arg > (0xFFFFFFFFU / 2U)) {
				r->err = true;
				return false;
			}
			pending += (size_t)arg * 2U; /* a key and a value each */
			break;
		case MT_TAG:
			pending++; /* the tagged item still follows */
			break;
		default:
			break; /* integers are complete in their head */
		}
		if (pending > r->len) {
			/* Every pending item costs at least one more byte, so a
			 * count past the buffer's length is a malformed header
			 * rather than a container these bytes could hold. */
			r->err = true;
			return false;
		}
	}
	return true;
}

/*
 * Read one map key as a claim label.
 *
 * Both integer major types are accepted: RFC 9783's own claims carry unsigned
 * labels, and the device-uuid vendor claim carries a negative one. A decoder
 * that read only unsigned keys would report that claim as malformed.
 */
static bool label(struct reader *r, int32_t *out)
{
	uint8_t mt = 0xFFU;
	uint32_t arg = 0;

	if (!head(r, &mt, &arg) || arg > 0x7FFFFFFFU) {
		r->err = true;
		return false;
	}
	if (mt == MT_UINT) {
		*out = (int32_t)arg;
		return true;
	}
	if (mt == MT_NEGINT) {
		*out = -(int32_t)arg - 1; /* CBOR encodes -(n + 1) as n */
		return true;
	}
	r->err = true;
	return false;
}

/* Decode the value at the read position into @out, whatever type it is. */
static bool value(struct reader *r, struct attest_claim *out)
{
	uint8_t mt = 0xFFU;
	uint32_t arg = 0;
	size_t start = r->pos; /* before the head, for the array span below */

	if (!head(r, &mt, &arg)) {
		return false;
	}
	switch (mt) {
	case MT_UINT:
		if (arg > 0x7FFFFFFFU) {
			r->err = true;
			return false;
		}
		out->type = ATTEST_CLAIM_UINT;
		out->number = (int32_t)arg;
		return true;
	case MT_NEGINT:
		if (arg > 0x7FFFFFFFU) {
			r->err = true;
			return false;
		}
		out->type = ATTEST_CLAIM_INT;
		out->number = -(int32_t)arg - 1;
		return true;
	case MT_BSTR:
	case MT_TSTR:
		out->type = (mt == MT_BSTR) ? ATTEST_CLAIM_BYTES : ATTEST_CLAIM_TEXT;
		out->bytes = take(r, arg);
		out->len = arg;
		return out->bytes != NULL;
	case MT_ARRAY:
		/* Hand back the encoded array so a caller can walk it - which
		 * is what attest_sw_component() does. The span starts at the
		 * array's HEAD, not after it: the walk reads that head to learn
		 * how many records follow. Rewind to just before it, then let
		 * skip() find the end. */
		r->pos = start;
		out->type = ATTEST_CLAIM_ARRAY;
		if (!skip(r)) {
			return false;
		}
		out->bytes = &r->buf[start];
		out->len = r->pos - start;
		return true;
	default:
		r->err = true;
		return false;
	}
}

bool attest_token_parse(struct attest_token *out, const uint8_t *token, size_t len)
{
	struct reader r = { .buf = token, .len = len, .pos = 0U, .err = false };
	uint32_t arg = 0;

	/* Tag 18: this is a COSE_Sign1. */
	if (!expect(&r, MT_TAG, &arg) || arg != COSE_SIGN1_TAG) {
		return false;
	}
	/* [protected, unprotected, payload, signature] */
	if (!expect(&r, MT_ARRAY, &arg) || arg != 4U) {
		return false;
	}
	if (!expect(&r, MT_BSTR, &arg)) {
		return false;
	}
	out->protected_hdr = take(&r, arg);
	out->protected_hdr_len = arg;
	if (out->protected_hdr == NULL) {
		return false;
	}
	if (!expect(&r, MT_MAP, &arg) || arg != 0U) {
		return false;
	}
	if (!expect(&r, MT_BSTR, &arg)) {
		return false;
	}
	out->payload = take(&r, arg);
	out->payload_len = arg;
	if (out->payload == NULL) {
		return false;
	}
	if (!expect(&r, MT_BSTR, &arg)) {
		return false;
	}
	out->signature = take(&r, arg);
	out->signature_len = arg;
	if (out->signature == NULL) {
		return false;
	}
	if (r.pos != len) {
		return false; /* trailing bytes after the envelope */
	}

	/* The claims map's entry count, read out of the payload. */
	struct reader claims = {
		.buf = out->payload, .len = out->payload_len, .pos = 0U, .err = false
	};

	if (!expect(&claims, MT_MAP, &out->claim_count)) {
		return false;
	}
	return true;
}

bool attest_token_claim(const struct attest_token *token, int32_t want, struct attest_claim *out)
{
	struct reader r = {
		.buf = token->payload, .len = token->payload_len, .pos = 0U, .err = false
	};
	uint32_t entries = 0;
	bool found = false;
	size_t found_at = 0U;

	if (!expect(&r, MT_MAP, &entries)) {
		return false;
	}
	for (uint32_t i = 0; i < entries; i++) {
		int32_t got = 0;

		if (!label(&r, &got)) {
			return false;
		}
		if (got == want) {
			if (found) {
				return false; /* duplicated: value is ambiguous */
			}
			found = true;
			found_at = r.pos;
		}
		if (!skip(&r)) {
			return false;
		}
	}
	if (!found) {
		return false;
	}

	r.pos = found_at;
	r.err = false;
	out->label = want;
	return value(&r, out);
}

/* Sub-claim keys RFC 9783 section 4.4.1 defines for a software-component record. */
enum sw_component_key {
	SWC_TYPE = 1,
	SWC_MEASUREMENT = 2,
	SWC_VERSION = 4,
	SWC_SIGNER_ID = 5,
};

bool attest_sw_component(const struct attest_claim *sw, size_t index,
			 struct attest_sw_component *out)
{
	if (sw->type != ATTEST_CLAIM_ARRAY) {
		return false;
	}

	struct reader r = { .buf = sw->bytes, .len = sw->len, .pos = 0U, .err = false };
	uint32_t records = 0;

	if (!expect(&r, MT_ARRAY, &records) || index >= records) {
		return false;
	}
	for (size_t i = 0; i < index; i++) {
		if (!skip(&r)) {
			return false;
		}
	}

	uint32_t fields = 0;

	if (!expect(&r, MT_MAP, &fields)) {
		return false;
	}

	out->type = NULL;
	out->measurement = NULL;
	out->version = NULL;
	out->signer_id = NULL;
	out->type_len = 0U;
	out->measurement_len = 0U;
	out->version_len = 0U;
	out->signer_id_len = 0U;

	for (uint32_t i = 0; i < fields; i++) {
		int32_t key = 0;
		struct attest_claim v = { 0 };

		if (!label(&r, &key) || !value(&r, &v)) {
			return false;
		}
		switch (key) {
		case SWC_TYPE:
			out->type = v.bytes;
			out->type_len = v.len;
			break;
		case SWC_MEASUREMENT:
			out->measurement = v.bytes;
			out->measurement_len = v.len;
			break;
		case SWC_VERSION:
			out->version = v.bytes;
			out->version_len = v.len;
			break;
		case SWC_SIGNER_ID:
			out->signer_id = v.bytes;
			out->signer_id_len = v.len;
			break;
		default:
			break; /* a sub-claim a later release added */
		}
	}
	return out->type != NULL && out->measurement != NULL && out->version != NULL &&
	       out->signer_id != NULL;
}
