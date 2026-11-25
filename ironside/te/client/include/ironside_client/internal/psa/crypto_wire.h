/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - PSA Crypto NSC wire layout.
 *
 * Internal to the Client: used by src/psa_crypto.c, not part of the
 * App-facing API.
 */

#ifndef IRONSIDE_CLIENT_INTERNAL_PSA_CRYPTO_WIRE_H
#define IRONSIDE_CLIENT_INTERNAL_PSA_CRYPTO_WIRE_H

#include <stddef.h>
#include <stdint.h>

#include <ironside_client/internal/psa/crypto_sids.h>

struct ironside_client_pack_iovec {
	uint32_t key_id;           /* 0  */
	uint32_t alg;              /* 4  */
	uint32_t op_handle;        /* 8  */
	uint32_t ad_length;        /* 12 */
	uint32_t plaintext_length; /* 16 */
	uint8_t nonce[16];         /* 20 (maximum nonce length) */
	uint32_t nonce_length;     /* 36 */
	uint16_t function_id;      /* 40 */
	uint16_t step;             /* 42 */
	union {
		uint32_t capacity; /* 48 (4 bytes pad at 44 to 8-align the union) */
		uint64_t value;
	} u;
	uint8_t role; /* 56 */
};

_Static_assert(sizeof(struct ironside_client_pack_iovec) == 64, "pack_iovec ABI size");
_Static_assert(offsetof(struct ironside_client_pack_iovec, alg) == 4, "pack_iovec alg offset");
_Static_assert(offsetof(struct ironside_client_pack_iovec, function_id) == 40,
	       "pack_iovec function_id offset");

#endif /* IRONSIDE_CLIENT_INTERNAL_PSA_CRYPTO_WIRE_H */
