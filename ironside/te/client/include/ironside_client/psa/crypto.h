/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - PSA Crypto API.
 *
 * Standard PSA Crypto entry points for a non-secure App. Including this one
 * header gives the whole API; it pulls in the standard set beside it, which an
 * App may also include individually:
 *
 *     crypto_types.h   the scalar types and psa_key_attributes_t
 *     crypto_values.h  status codes, key types, algorithms, usage flags
 *     crypto_struct.h  the multipart operation objects
 *     crypto_sizes.h   compile-time output buffer sizes
 *     crypto_extra.h   IronSide TE's own IRONSIDE_CLIENT_* extensions
 *
 * The PSA Crypto API is a part of the IronSide TE Client and just
 * like the client, is highly portable C as it does not depend on any
 * external libraries or RTOS/SDK functions.
 *
 * Detailed documentation for using IronSide TE's PSA Crypto can be seen in the
 * user guide, section "PSA Crypto".
 */

#ifndef IRONSIDE_CLIENT_PSA_CRYPTO_H
#define IRONSIDE_CLIENT_PSA_CRYPTO_H

#include <stddef.h>
#include <stdint.h>

#include <ironside_client/psa/crypto_extra.h>
#include <ironside_client/psa/crypto_sizes.h>
#include <ironside_client/psa/crypto_struct.h>
#include <ironside_client/psa/crypto_types.h>
#include <ironside_client/psa/crypto_values.h>
#include <ironside_client/psa/error.h>

#define PSA_CRYPTO_API_VERSION_MAJOR 1
#define PSA_CRYPTO_API_VERSION_MINOR 4

/**
 * @brief PSA Crypto: initialize the library.
 *
 * IronSide TE initializes the PSA core itself before booting the App, so on the
 * non-secure side this is a no-op kept for API parity with standard PSA code.
 */
psa_status_t psa_crypto_init(void);

/* ---- Key management ------------------------------------------------------ */

static inline psa_key_attributes_t psa_key_attributes_init(void)
{
	return PSA_KEY_ATTRIBUTES_INIT;
}

psa_status_t psa_get_key_attributes(psa_key_id_t key, psa_key_attributes_t *attributes);

void psa_reset_key_attributes(psa_key_attributes_t *attributes);

static inline void psa_set_key_type(psa_key_attributes_t *attr, psa_key_type_t type)
{
	attr->type = type;
}

static inline psa_key_type_t psa_get_key_type(const psa_key_attributes_t *attr)
{
	return attr->type;
}

static inline void psa_set_key_bits(psa_key_attributes_t *attr, size_t bits)
{
	attr->bits = (uint16_t)bits;
}

static inline size_t psa_get_key_bits(const psa_key_attributes_t *attr)
{
	return attr->bits;
}

static inline void psa_set_key_lifetime(psa_key_attributes_t *attr, psa_key_lifetime_t lifetime)
{
	attr->lifetime = lifetime;
}

static inline psa_key_lifetime_t psa_get_key_lifetime(const psa_key_attributes_t *attr)
{
	return attr->lifetime;
}

static inline void psa_set_key_id(psa_key_attributes_t *attr, psa_key_id_t id)
{
	attr->id = id;
}

static inline psa_key_id_t psa_get_key_id(const psa_key_attributes_t *attr)
{
	return attr->id;
}

static inline void psa_set_key_algorithm(psa_key_attributes_t *attr, psa_algorithm_t alg)
{
	attr->alg = alg;
}

static inline psa_algorithm_t psa_get_key_algorithm(const psa_key_attributes_t *attr)
{
	return attr->alg;
}

static inline void psa_set_key_usage_flags(psa_key_attributes_t *attr, psa_key_usage_t usage_flags)
{
	attr->usage = usage_flags;
}

static inline psa_key_usage_t psa_get_key_usage_flags(const psa_key_attributes_t *attr)
{
	return attr->usage;
}

psa_status_t psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			    size_t data_length, psa_key_id_t *key);

psa_status_t psa_generate_key(const psa_key_attributes_t *attributes, psa_key_id_t *key);

psa_status_t psa_copy_key(psa_key_id_t source_key, const psa_key_attributes_t *attributes,
			  psa_key_id_t *target_key);

psa_status_t psa_destroy_key(psa_key_id_t key);

psa_status_t psa_export_key(psa_key_id_t key, uint8_t *data, size_t data_size, size_t *data_length);

psa_status_t psa_export_public_key(psa_key_id_t key, uint8_t *data, size_t data_size,
				   size_t *data_length);

/* ---- Hash ---------------------------------------------------------------- */

psa_status_t psa_hash_compute(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
			      uint8_t *hash, size_t hash_size, size_t *hash_length);

psa_status_t psa_hash_compare(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
			      const uint8_t *hash, size_t hash_length);

static inline psa_hash_operation_t psa_hash_operation_init(void)
{
	return PSA_HASH_OPERATION_INIT;
}

psa_status_t psa_hash_setup(psa_hash_operation_t *operation, psa_algorithm_t alg);

psa_status_t psa_hash_update(psa_hash_operation_t *operation, const uint8_t *input,
			     size_t input_length);

psa_status_t psa_hash_finish(psa_hash_operation_t *operation, uint8_t *hash, size_t hash_size,
			     size_t *hash_length);

psa_status_t psa_hash_verify(psa_hash_operation_t *operation, const uint8_t *hash,
			     size_t hash_length);

psa_status_t psa_hash_abort(psa_hash_operation_t *operation);

psa_status_t psa_hash_clone(const psa_hash_operation_t *source_operation,
			    psa_hash_operation_t *target_operation);

/* ---- MAC ----------------------------------------------------------------- */

psa_status_t psa_mac_compute(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
			     size_t input_length, uint8_t *mac, size_t mac_size,
			     size_t *mac_length);

psa_status_t psa_mac_verify(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
			    size_t input_length, const uint8_t *mac, size_t mac_length);

static inline psa_mac_operation_t psa_mac_operation_init(void)
{
	return PSA_MAC_OPERATION_INIT;
}

psa_status_t psa_mac_sign_setup(psa_mac_operation_t *operation, psa_key_id_t key,
				psa_algorithm_t alg);

psa_status_t psa_mac_verify_setup(psa_mac_operation_t *operation, psa_key_id_t key,
				  psa_algorithm_t alg);

psa_status_t psa_mac_update(psa_mac_operation_t *operation, const uint8_t *input,
			    size_t input_length);

psa_status_t psa_mac_sign_finish(psa_mac_operation_t *operation, uint8_t *mac, size_t mac_size,
				 size_t *mac_length);

psa_status_t psa_mac_verify_finish(psa_mac_operation_t *operation, const uint8_t *mac,
				   size_t mac_length);

psa_status_t psa_mac_abort(psa_mac_operation_t *operation);

/* ---- Cipher -------------------------------------------------------------- */

psa_status_t psa_cipher_encrypt(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, uint8_t *output, size_t output_size,
				size_t *output_length);

psa_status_t psa_cipher_decrypt(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, uint8_t *output, size_t output_size,
				size_t *output_length);

static inline psa_cipher_operation_t psa_cipher_operation_init(void)
{
	return PSA_CIPHER_OPERATION_INIT;
}

psa_status_t psa_cipher_encrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key,
				      psa_algorithm_t alg);

psa_status_t psa_cipher_decrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key,
				      psa_algorithm_t alg);

psa_status_t psa_cipher_generate_iv(psa_cipher_operation_t *operation, uint8_t *iv, size_t iv_size,
				    size_t *iv_length);

psa_status_t psa_cipher_set_iv(psa_cipher_operation_t *operation, const uint8_t *iv,
			       size_t iv_length);

psa_status_t psa_cipher_update(psa_cipher_operation_t *operation, const uint8_t *input,
			       size_t input_length, uint8_t *output, size_t output_size,
			       size_t *output_length);

psa_status_t psa_cipher_finish(psa_cipher_operation_t *operation, uint8_t *output,
			       size_t output_size, size_t *output_length);

psa_status_t psa_cipher_abort(psa_cipher_operation_t *operation);

/* ---- AEAD ---------------------------------------------------------------- */

psa_status_t psa_aead_encrypt(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *nonce,
			      size_t nonce_length, const uint8_t *additional_data,
			      size_t additional_data_length, const uint8_t *plaintext,
			      size_t plaintext_length, uint8_t *ciphertext, size_t ciphertext_size,
			      size_t *ciphertext_length);

psa_status_t psa_aead_decrypt(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *nonce,
			      size_t nonce_length, const uint8_t *additional_data,
			      size_t additional_data_length, const uint8_t *ciphertext,
			      size_t ciphertext_length, uint8_t *plaintext, size_t plaintext_size,
			      size_t *plaintext_length);

static inline psa_aead_operation_t psa_aead_operation_init(void)
{
	return PSA_AEAD_OPERATION_INIT;
}

psa_status_t psa_aead_encrypt_setup(psa_aead_operation_t *operation, psa_key_id_t key,
				    psa_algorithm_t alg);

psa_status_t psa_aead_decrypt_setup(psa_aead_operation_t *operation, psa_key_id_t key,
				    psa_algorithm_t alg);

psa_status_t psa_aead_set_lengths(psa_aead_operation_t *operation, size_t ad_length,
				  size_t plaintext_length);

psa_status_t psa_aead_generate_nonce(psa_aead_operation_t *operation, uint8_t *nonce,
				     size_t nonce_size, size_t *nonce_length);

psa_status_t psa_aead_set_nonce(psa_aead_operation_t *operation, const uint8_t *nonce,
				size_t nonce_length);

psa_status_t psa_aead_update_ad(psa_aead_operation_t *operation, const uint8_t *input,
				size_t input_length);

psa_status_t psa_aead_update(psa_aead_operation_t *operation, const uint8_t *input,
			     size_t input_length, uint8_t *output, size_t output_size,
			     size_t *output_length);

psa_status_t psa_aead_finish(psa_aead_operation_t *operation, uint8_t *ciphertext,
			     size_t ciphertext_size, size_t *ciphertext_length, uint8_t *tag,
			     size_t tag_size, size_t *tag_length);

psa_status_t psa_aead_verify(psa_aead_operation_t *operation, uint8_t *plaintext,
			     size_t plaintext_size, size_t *plaintext_length, const uint8_t *tag,
			     size_t tag_length);

psa_status_t psa_aead_abort(psa_aead_operation_t *operation);

/* ---- Key unwrapping ------------------------------------------------------ */

psa_status_t psa_unwrap_key(const psa_key_attributes_t *attributes, psa_key_id_t wrapping_key,
			    psa_algorithm_t alg, const uint8_t *data, size_t data_length,
			    psa_key_id_t *key);

/* ---- Key derivation ------------------------------------------------------ */

static inline psa_key_derivation_operation_t psa_key_derivation_operation_init(void)
{
	return PSA_KEY_DERIVATION_OPERATION_INIT;
}

psa_status_t psa_key_derivation_setup(psa_key_derivation_operation_t *operation,
				      psa_algorithm_t alg);

psa_status_t psa_key_derivation_get_capacity(const psa_key_derivation_operation_t *operation,
					     size_t *capacity);

psa_status_t psa_key_derivation_set_capacity(psa_key_derivation_operation_t *operation,
					     size_t capacity);

psa_status_t psa_key_derivation_input_bytes(psa_key_derivation_operation_t *operation,
					    psa_key_derivation_step_t step, const uint8_t *data,
					    size_t data_length);

psa_status_t psa_key_derivation_input_integer(psa_key_derivation_operation_t *operation,
					      psa_key_derivation_step_t step, uint64_t value);

psa_status_t psa_key_derivation_input_key(psa_key_derivation_operation_t *operation,
					  psa_key_derivation_step_t step, psa_key_id_t key);

psa_status_t psa_key_derivation_output_bytes(psa_key_derivation_operation_t *operation,
					     uint8_t *output, size_t output_length);

psa_status_t psa_key_derivation_output_key(const psa_key_attributes_t *attributes,
					   psa_key_derivation_operation_t *operation,
					   psa_key_id_t *key);

psa_status_t psa_key_derivation_abort(psa_key_derivation_operation_t *operation);

/* ---- Asymmetric signature ------------------------------------------------ */

psa_status_t psa_sign_message(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
			      size_t input_length, uint8_t *signature, size_t signature_size,
			      size_t *signature_length);

psa_status_t psa_verify_message(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *input,
				size_t input_length, const uint8_t *signature,
				size_t signature_length);

psa_status_t psa_sign_hash(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *hash,
			   size_t hash_length, uint8_t *signature, size_t signature_size,
			   size_t *signature_length);

psa_status_t psa_verify_hash(psa_key_id_t key, psa_algorithm_t alg, const uint8_t *hash,
			     size_t hash_length, const uint8_t *signature, size_t signature_length);

/* ---- Key agreement ------------------------------------------------------- */

psa_status_t psa_raw_key_agreement(psa_algorithm_t alg, psa_key_id_t private_key,
				   const uint8_t *peer_key, size_t peer_key_length, uint8_t *output,
				   size_t output_size, size_t *output_length);

psa_status_t psa_key_derivation_key_agreement(psa_key_derivation_operation_t *operation,
					      psa_key_derivation_step_t step,
					      psa_key_id_t private_key, const uint8_t *peer_key,
					      size_t peer_key_length);

/* ---- Password-authenticated key exchange --------------------------------- */

static inline void psa_pake_cs_set_algorithm(psa_pake_cipher_suite_t *cipher_suite,
					     psa_algorithm_t alg)
{
	cipher_suite->algorithm = alg;
}

static inline psa_algorithm_t psa_pake_cs_get_algorithm(const psa_pake_cipher_suite_t *cipher_suite)
{
	return cipher_suite->algorithm;
}

static inline void psa_pake_cs_set_primitive(psa_pake_cipher_suite_t *cipher_suite,
					     psa_pake_primitive_t primitive)
{
	cipher_suite->type = PSA_PAKE_PRIMITIVE_GET_TYPE(primitive);
	cipher_suite->family = PSA_PAKE_PRIMITIVE_GET_FAMILY(primitive);
	cipher_suite->bits = (uint16_t)PSA_PAKE_PRIMITIVE_GET_BITS(primitive);
}

static inline psa_pake_primitive_t
psa_pake_cs_get_primitive(const psa_pake_cipher_suite_t *cipher_suite)
{
	return PSA_PAKE_PRIMITIVE(cipher_suite->type, cipher_suite->family, cipher_suite->bits);
}

static inline void psa_pake_cs_set_key_confirmation(psa_pake_cipher_suite_t *cipher_suite,
						    uint32_t key_confirmation)
{
	cipher_suite->key_confirmation = key_confirmation;
}

static inline uint32_t psa_pake_cs_get_key_confirmation(const psa_pake_cipher_suite_t *cipher_suite)
{
	return cipher_suite->key_confirmation;
}

psa_status_t psa_pake_setup(psa_pake_operation_t *operation, psa_key_id_t password_key,
			    const psa_pake_cipher_suite_t *cipher_suite);

psa_status_t psa_pake_set_role(psa_pake_operation_t *operation, psa_pake_role_t role);

psa_status_t psa_pake_set_user(psa_pake_operation_t *operation, const uint8_t *user_id,
			       size_t user_id_len);

psa_status_t psa_pake_set_peer(psa_pake_operation_t *operation, const uint8_t *peer_id,
			       size_t peer_id_len);

psa_status_t psa_pake_set_context(psa_pake_operation_t *operation, const uint8_t *context,
				  size_t context_len);

psa_status_t psa_pake_output(psa_pake_operation_t *operation, psa_pake_step_t step, uint8_t *output,
			     size_t output_size, size_t *output_length);

psa_status_t psa_pake_input(psa_pake_operation_t *operation, psa_pake_step_t step,
			    const uint8_t *input, size_t input_length);

psa_status_t psa_pake_get_shared_key(psa_pake_operation_t *operation,
				     const psa_key_attributes_t *attributes, psa_key_id_t *key);

psa_status_t psa_pake_abort(psa_pake_operation_t *operation);

/* ---- Random generation --------------------------------------------------- */

psa_status_t psa_generate_random(uint8_t *output, size_t output_size);

#endif /* IRONSIDE_CLIENT_PSA_CRYPTO_H */
