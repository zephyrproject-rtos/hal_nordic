/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - PSA Crypto types.
 *
 */

#ifndef IRONSIDE_CLIENT_PSA_CRYPTO_TYPES_H
#define IRONSIDE_CLIENT_PSA_CRYPTO_TYPES_H

#include <stddef.h>
#include <stdint.h>

#include <ironside_client/internal/psa/crypto_key_attributes_abi.h>
#include <ironside_client/psa/error.h>

typedef uint32_t psa_key_id_t;

typedef uint16_t psa_key_type_t;
typedef uint8_t psa_ecc_family_t;

typedef uint32_t psa_key_lifetime_t;
typedef uint8_t psa_key_persistence_t;
typedef uint32_t psa_key_location_t;

typedef uint32_t psa_algorithm_t;

typedef uint32_t psa_key_usage_t;

typedef uint16_t psa_key_derivation_step_t;

typedef uint8_t psa_pake_primitive_type_t;
typedef uint8_t psa_pake_family_t;
typedef uint32_t psa_pake_primitive_t;
typedef uint8_t psa_pake_role_t;
typedef uint8_t psa_pake_step_t;

/* The cipher suite is passed to UROT as these bytes, laid out as TF-PSA-Crypto lays out its own,
 * which is what an application's PSA API is built from (crypto_abi.h pins the offsets). */
typedef struct {
	psa_algorithm_t algorithm;
	psa_pake_primitive_type_t type;
	psa_pake_family_t family;
	uint16_t bits;
	uint32_t key_confirmation;
} psa_pake_cipher_suite_t;

#define PSA_PAKE_CIPHER_SUITE_INIT ((psa_pake_cipher_suite_t){ 0, 0, 0, 0, 0 })

#endif /* IRONSIDE_CLIENT_PSA_CRYPTO_TYPES_H */
