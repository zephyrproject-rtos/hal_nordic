/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - psa_key_attributes_t wire layout.
 *
 * Internal to the Client: included by <ironside_client/psa/crypto_types.h>, which is what an
 * App includes - psa_key_attributes_t itself is App-facing, its layout is not.
 */

#ifndef IRONSIDE_CLIENT_INTERNAL_PSA_CRYPTO_KEY_ATTRIBUTES_ABI_H
#define IRONSIDE_CLIENT_INTERNAL_PSA_CRYPTO_KEY_ATTRIBUTES_ABI_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint16_t type;     /* 0  */
	uint16_t bits;     /* 2  */
	uint32_t lifetime; /* 4  */
	uint32_t usage;    /* 8  (policy.usage) */
	uint32_t alg;      /* 12 (policy.alg)   */
	uint32_t alg2;     /* 16 (policy.alg2)  */
	uint32_t id;       /* 20 */
} psa_key_attributes_t;

_Static_assert(sizeof(psa_key_attributes_t) == 24, "key_attributes ABI size");
_Static_assert(offsetof(psa_key_attributes_t, usage) == 8, "key_attributes usage offset");
_Static_assert(offsetof(psa_key_attributes_t, id) == 20, "key_attributes id offset");

#endif /* IRONSIDE_CLIENT_INTERNAL_PSA_CRYPTO_KEY_ATTRIBUTES_ABI_H */
