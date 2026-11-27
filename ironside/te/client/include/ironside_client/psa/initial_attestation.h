/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - PSA Initial Attestation API.
 *
 * Standard PSA Initial Attestation entry points for a non-secure App, per PSA
 * Certified Attestation API 2.0 - the function signatures are unchanged from
 * 1.0 (only the token format changed, delegated to RFC 9783).
 *
 * This implementation supports all three PSA-legal challenge sizes (32/48/64
 * bytes).
 *
 * Reading a token: decode it with a CBOR parser, looking claims up by label.
 * The labels, the claim value lengths and the claim count are published below;
 * the token's full format is ../../psa_attest_token.cddl, and the
 * psa_initial_attestation sample decodes a whole token with a ~200-line parser
 * of its own that you can copy or replace with QCBOR, zcbor, TinyCBOR or your
 * RTOS's decoder.
 *
 * This header publishes no byte offsets into a token, and earlier releases'
 * PSA_INITIAL_ATTEST_*_OFFSET macros are gone. A claim's position depends on
 * the claim set, and the claim set grows: every release that has added a claim
 * moved every byte behind it. An offset therefore breaks on an update in the
 * worst possible way - it keeps returning a well-formed value, taken from
 * whichever claim moved into that position, and nothing reports an error. A
 * label search costs a few lines more and survives.
 */

#ifndef IRONSIDE_CLIENT_PSA_INITIAL_ATTESTATION_H
#define IRONSIDE_CLIENT_PSA_INITIAL_ATTESTATION_H

#include <stddef.h>
#include <stdint.h>
#include <ironside_client/psa/error.h>

/* Challenge sizes the PSA API permits (PSA Certified Attestation API 2.0,
 * "Challenge sizes") - all three are usable end-to-end with this API. */
#define PSA_INITIAL_ATTEST_CHALLENGE_SIZE_32 32u
#define PSA_INITIAL_ATTEST_CHALLENGE_SIZE_48 48u
#define PSA_INITIAL_ATTEST_CHALLENGE_SIZE_64 64u

#define PSA_INITIAL_ATTEST_API_VERSION_MAJOR 2
#define PSA_INITIAL_ATTEST_API_VERSION_MINOR 0
#define PSA_INITIAL_ATTEST_MAX_TOKEN_SIZE    620u

/*
 * COSE constants for verifying a token's signature.
 *
 * The signature is ECDSA P-256 over the COSE Sig_structure
 *   ["Signature1", protected_bstr, external_aad, payload_bstr]
 * with a zero-length external_aad. A verifier concatenates SIG1_PREAMBLE (the
 * CBOR encoding of that 4-element array's head plus the "Signature1" text
 * string), the PROTECTED byte string as encoded, EMPTY_AAD_BYTE (a zero-length
 * bstr), and the PAYLOAD byte string as encoded - all three of those spans are
 * what a CBOR parser returns for a bstr, so no byte arithmetic over the token is
 * involved. The signature itself is SIGNATURE_LEN bytes, raw r || s rather than
 * DER.
 *
 * Verification needs the device's Identity Attestation Key public part, which is
 * extracted and registered at manufacturing time, so it is a remote verifier's
 * job rather than an application's.
 */
#define PSA_INITIAL_ATTEST_SIG1_PREAMBLE \
	{ 0x84, 0x6A, 'S', 'i', 'g', 'n', 'a', 't', 'u', 'r', 'e', '1' }
#define PSA_INITIAL_ATTEST_SIG1_PREAMBLE_LEN 12U
#define PSA_INITIAL_ATTEST_EMPTY_AAD_BYTE    0x40U
#define PSA_INITIAL_ATTEST_SIGNATURE_LEN     64U

/* Claim labels, as the token's CBOR claims map spells them: what to search for,
 * and what to call a claim when reporting it the way RFC 9783 does. The map
 * holds exactly CLAIM_COUNT entries with no duplicates - the seven claims RFC
 * 9783 marks MUST, the one it marks MAY that IronSide TE carries (boot-seed), and
 * one vendor claim (device-uuid).
 *
 * Map entry order is not part of the token's contract: CBOR requires no
 * canonical key ordering and RFC 9783 imposes none. Search by label.
 */
#define PSA_INITIAL_ATTEST_CLAIM_COUNT             9U
#define PSA_INITIAL_ATTEST_CLAIM_NONCE             10
#define PSA_INITIAL_ATTEST_CLAIM_INSTANCE_ID       256
#define PSA_INITIAL_ATTEST_CLAIM_PROFILE           265
#define PSA_INITIAL_ATTEST_CLAIM_BOOT_SEED         268
#define PSA_INITIAL_ATTEST_CLAIM_CLIENT_ID         2394
#define PSA_INITIAL_ATTEST_CLAIM_LIFECYCLE         2395
#define PSA_INITIAL_ATTEST_CLAIM_IMPLEMENTATION_ID 2396
#define PSA_INITIAL_ATTEST_CLAIM_SW_COMPONENTS     2399
#define PSA_INITIAL_ATTEST_CLAIM_DEVICE_UUID       (-1000000)

/* instance-id / EAT ueid claim (256) - 0x01 (UEID type: RAND) ||
 * SHA-256(IAK public key). A per-token identity binding, and since a device has
 * one IAK for its lifetime, also the verifier's registry-lookup key: strip the
 * leading type byte to get the hash. A device re-keyed at RMA attests under a
 * different instance-id while remaining the same die - the device-uuid claim is
 * what names the die. */
#define PSA_INITIAL_ATTEST_INSTANCE_ID_LEN 33U

/* boot-seed / EAT label 268 claim - the value IronSide TE created at boot. The
 * same in every token this boot session emits, and different in every other
 * boot session's, so a caller can tell whether a token it holds describes the
 * run it is in.
 *
 * It is public, not confidential. It travels in the clear in every token, and
 * RFC 9711 section 4.2.13 states that a boot seed "is not a secret and MUST NOT
 * be used for any purpose where a secret seed is needed, such as seeding a
 * random number generator". It labels a boot session. It is not key material.
 */
#define PSA_INITIAL_ATTEST_BOOT_SEED_LEN 32U

/* device-uuid claim (-1000000) - the 128-bit unique identifier of the die this
 * token came off, as 16 raw bytes. A vendor claim (CWT label in the registry's
 * Private Use range), so a verifier that only knows RFC 9783 skips it. Each of
 * the four FICR words is little-endian, which makes the value byte-for-byte
 * this device's ADAC soc_id.
 */
#define PSA_INITIAL_ATTEST_DEVICE_UUID_LEN 16U

/* client-id claim (2394) - a CBOR negative integer, so a parser returns the
 * value itself. It is CLIENT_ID_NONSECURE for every caller of this API: RFC
 * 9783 gives negative ids to callers outside the secure processing environment,
 * and there is one non-secure security domain here, so the claim does not say
 * which non-secure image asked.
 */
#define PSA_INITIAL_ATTEST_CLIENT_ID_NONSECURE (-1)

/* implementation-id claim (2396) - 32 bytes naming the PSA RoT implementation
 * this device runs: which part it is, and the version of IronSide TE's immutable
 * boot stage (IROT). It identifies the implementation and not the instance, so
 * no per-die register is an input; the die is named by the device-uuid claim.
 *
 * The value is transparent by design, and the IMPL_ID offsets below are offsets
 * into the claim value rather than into the token - the one kind of offset this
 * header still publishes, because a CBOR parser hands back the 32 bytes and
 * does not resolve what is inside them.
 *
 * Read the scheme version first: a later layout takes a new number rather than
 * reinterpreting this one, so a reader that does not know the number it finds
 * must treat the 32 bytes as opaque. PART, VARIANT and CONFIGID are the
 * device-information words in big-endian order, so VARIANT reads as four ASCII
 * characters on a part whose VARIANT is printable. RAM and RRAM are sizes in
 * kB, big-endian. The IROT version is four bytes: major, minor, patch, tweak.
 * The bytes from RESERVED_OFFSET on are reserved for future fields, so ignore
 * them rather than requiring a value. scripts/lib/impl_id.py composes and
 * decodes the same 32 bytes host side.
 */
#define PSA_INITIAL_ATTEST_IMPLEMENTATION_ID_LEN       32U
#define PSA_INITIAL_ATTEST_IMPL_ID_SCHEME_OFFSET       0U
#define PSA_INITIAL_ATTEST_IMPL_ID_SCHEME_V1           1U
#define PSA_INITIAL_ATTEST_IMPL_ID_PART_OFFSET         1U
#define PSA_INITIAL_ATTEST_IMPL_ID_VARIANT_OFFSET      5U
#define PSA_INITIAL_ATTEST_IMPL_ID_CONFIGID_OFFSET     9U
#define PSA_INITIAL_ATTEST_IMPL_ID_RAM_KB_OFFSET       13U
#define PSA_INITIAL_ATTEST_IMPL_ID_RRAM_KB_OFFSET      15U
#define PSA_INITIAL_ATTEST_IMPL_ID_IROT_VERSION_OFFSET 17U
#define PSA_INITIAL_ATTEST_IMPL_ID_RESERVED_OFFSET     21U

/* profile-definition claim (265) - the claim-set profile this token follows, as
 * text under the PSA tag URI a verification service dispatches on. Its length
 * is fixed, trailing padding included; that padding is what keeps the token's
 * SIZE constant for a given challenge size.
 */
#define PSA_INITIAL_ATTEST_PROFILE_LEN 43U

/* software-components claim (2399) - one record per image IronSide TE measures,
 * always all three and always in this order: UROT, the App, the DFU image. Read
 * a record's measurement-type (RFC 9783 sub-claim key 1) rather than relying on
 * its position if you care which image you are reading.
 *
 * Each record carries the measurement (SHA-256 over the image's signed region,
 * key 2), the version from the image header (key 4), and the SHA-256 of the
 * verification key the signature was checked against (key 5). The measurement
 * algorithm is not a per-record field: it is SHA-256 for the whole token, fixed
 * by the ES256 COSE algorithm in the protected header.
 *
 * The version is text, emitted three characters per field
 * ("001.002.003+004"). Fixed width is what keeps the token's size a property of
 * the token format rather than of the installed firmware; strip the zero
 * padding to compare it against a version printed the ordinary way.
 */
#define PSA_INITIAL_ATTEST_SW_COMPONENT_COUNT 3U
#define PSA_INITIAL_ATTEST_SW_MEASUREMENT_LEN 32U
#define PSA_INITIAL_ATTEST_SW_SIGNER_ID_LEN   32U
#define PSA_INITIAL_ATTEST_SW_VERSION_LEN     15U

/* The PSA security-lifecycle major states IronSide TE knows.
 *
 * Only the four deployable ones can appear in the security-lifecycle claim of a
 * token: a device that resolves to UNKNOWN or DECOMMISSIONED is issued no token
 * at all, and psa_initial_attest_get_token() reports PSA_ERROR_NOT_PERMITTED.
 * The other two are defined here because they do appear on the channels that
 * report device state rather than attest it - ADAC Discovery's psa_lifecycle
 * TLV and the SMP boot report's lifecycle field - and a decoder shared with
 * those needs to spell them.
 */
#define PSA_LIFECYCLE_UNKNOWN              0x0000U /* never in a token */
#define PSA_LIFECYCLE_ASSEMBLY_AND_TEST    0x1000U
#define PSA_LIFECYCLE_PSA_ROT_PROVISIONING 0x2000U
#define PSA_LIFECYCLE_SECURED              0x3000U
#define PSA_LIFECYCLE_NON_PSA_ROT_DEBUG    0x4000U
#define PSA_LIFECYCLE_DECOMMISSIONED       0x6000U /* never in a token */

/**
 * @brief PSA Initial Attestation: query the token size for a challenge size.
 *
 * @param challenge_size One of PSA_INITIAL_ATTEST_CHALLENGE_SIZE_{32,48,64}.
 * @param token_size     Receives the exact buffer size psa_initial_attest_get_token()
 *                        will need, on PSA_SUCCESS.
 *
 * @retval 0     PSA_SUCCESS; @p token_size was written.
 * @retval -135  (PSA_ERROR_INVALID_ARGUMENT) @p challenge_size is not one of 32/48/64.
 * @retval -249  (IRONSIDE_ERROR_CALL_IN_FLIGHT) another call is in flight; see integration.h.
 */
psa_status_t psa_initial_attest_get_token_size(size_t challenge_size, size_t *token_size);

/**
 * @brief PSA Initial Attestation: get a signed attestation token (standard PSA API).
 *
 * @param auth_challenge  Caller-supplied nonce/challenge (non-secure RAM) -
 *                         carried verbatim into the token's nonce claim.
 * @param challenge_size  Length of @p auth_challenge; one of
 *                         PSA_INITIAL_ATTEST_CHALLENGE_SIZE_{32,48,64}.
 * @param token_buf       Output buffer for the token (non-secure RAM).
 * @param token_buf_size  Size of @p token_buf.
 * @param token_size      Receives the number of bytes written, on PSA_SUCCESS.
 *
 * @retval 0     PSA_SUCCESS; @p token_buf holds the token and @p token_size its length.
 * @retval -135  (PSA_ERROR_INVALID_ARGUMENT) @p challenge_size is not one of 32/48/64.
 * @retval -138  (PSA_ERROR_BUFFER_TOO_SMALL) @p token_buf_size is too small.
 * @retval -133  (PSA_ERROR_NOT_PERMITTED) the device's life-cycle state does not resolve
 *               to one of the four deployable states, so there is no authentic, deployed
 *               device for a token to attest (see the PSA_LIFECYCLE_* values above).
 * @retval -249  (IRONSIDE_ERROR_CALL_IN_FLIGHT) another call is in flight; see integration.h.
 */
psa_status_t psa_initial_attest_get_token(const uint8_t *auth_challenge, size_t challenge_size,
					  uint8_t *token_buf, size_t token_buf_size,
					  size_t *token_size);

#endif /* IRONSIDE_CLIENT_PSA_INITIAL_ATTESTATION_H */
