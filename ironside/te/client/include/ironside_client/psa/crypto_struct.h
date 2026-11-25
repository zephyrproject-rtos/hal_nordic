/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - PSA Crypto multipart operation objects.
 *
 */

#ifndef IRONSIDE_CLIENT_PSA_CRYPTO_STRUCT_H
#define IRONSIDE_CLIENT_PSA_CRYPTO_STRUCT_H

#include <ironside_client/psa/crypto_types.h>

#define PSA_KEY_ATTRIBUTES_INIT ((psa_key_attributes_t){ 0 })

typedef struct {
	uint32_t handle;
} psa_hash_operation_t;

#define PSA_HASH_OPERATION_INIT ((psa_hash_operation_t){ 0 })

typedef struct {
	uint32_t handle;
} psa_mac_operation_t;

#define PSA_MAC_OPERATION_INIT ((psa_mac_operation_t){ 0 })

typedef struct {
	uint32_t handle;
} psa_cipher_operation_t;

#define PSA_CIPHER_OPERATION_INIT ((psa_cipher_operation_t){ 0 })

typedef struct {
	uint32_t handle;
} psa_aead_operation_t;

#define PSA_AEAD_OPERATION_INIT ((psa_aead_operation_t){ 0 })

typedef struct {
	uint32_t handle;
} psa_key_derivation_operation_t;

#define PSA_KEY_DERIVATION_OPERATION_INIT ((psa_key_derivation_operation_t){ 0 })

typedef struct {
	uint32_t handle;
} psa_pake_operation_t;

#define PSA_PAKE_OPERATION_INIT ((psa_pake_operation_t){ 0 })

#endif /* IRONSIDE_CLIENT_PSA_CRYPTO_STRUCT_H */
