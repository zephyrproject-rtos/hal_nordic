/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - PSA Crypto buffer sizes.
 *
 */

#ifndef IRONSIDE_CLIENT_PSA_CRYPTO_SIZES_H
#define IRONSIDE_CLIENT_PSA_CRYPTO_SIZES_H

#include <ironside_client/psa/crypto_values.h>

#define PSA_BITS_TO_BYTES(bits) (((bits) + 7u) / 8u)
#define PSA_ROUND_UP_TO_MULTIPLE(block_size, length) \
	(((length) + (block_size) - 1) / (block_size) * (block_size))

/* ---- Key export ---------------------------------------------------------- */

#define PSA_KEY_EXPORT_ECC_KEY_PAIR_MAX_SIZE(key_bits)   (PSA_BITS_TO_BYTES(key_bits))
#define PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(key_bits) (2u * PSA_BITS_TO_BYTES(key_bits) + 1u)

#define PSA_EXPORT_KEY_OUTPUT_SIZE(key_type, key_bits)                             \
	(PSA_KEY_TYPE_ECC_GET_FAMILY(key_type) == PSA_ECC_FAMILY_TWISTED_EDWARDS ? \
		 PSA_BITS_TO_BYTES((key_bits) + 1u) :                              \
	 PSA_KEY_TYPE_ECC_GET_FAMILY(key_type) == PSA_ECC_FAMILY_MONTGOMERY ?      \
		 PSA_BITS_TO_BYTES(key_bits) :                                     \
	 PSA_KEY_TYPE_IS_ECC_KEY_PAIR(key_type) ?                                  \
		 PSA_KEY_EXPORT_ECC_KEY_PAIR_MAX_SIZE(key_bits) :                  \
	 PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(key_type) ?                                \
		 PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(key_bits) :                \
	 PSA_KEY_TYPE_IS_UNSTRUCTURED(key_type) ? PSA_BITS_TO_BYTES(key_bits) :    \
						  0u)

#define PSA_EXPORT_PUBLIC_KEY_OUTPUT_SIZE(key_type, key_bits)                               \
	(PSA_KEY_TYPE_ECC_GET_FAMILY(key_type) == PSA_ECC_FAMILY_TWISTED_EDWARDS ?          \
		 PSA_BITS_TO_BYTES((key_bits) + 1u) :                                       \
	 PSA_KEY_TYPE_ECC_GET_FAMILY(key_type) == PSA_ECC_FAMILY_MONTGOMERY ?               \
		 PSA_BITS_TO_BYTES(key_bits) :                                              \
	 PSA_KEY_TYPE_IS_ECC(key_type) ? PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(key_bits) : \
					 0u)

#define PSA_EXPORT_KEY_PAIR_MAX_SIZE       PSA_KEY_EXPORT_ECC_KEY_PAIR_MAX_SIZE(384)
#define PSA_EXPORT_PUBLIC_KEY_MAX_SIZE     PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(384)
#define PSA_EXPORT_ASYMMETRIC_KEY_MAX_SIZE PSA_EXPORT_PUBLIC_KEY_MAX_SIZE

/* ---- Hash ---------------------------------------------------------------- */

#define PSA_HASH_LENGTH(alg)                               \
	(PSA_ALG_GET_HASH(alg) == PSA_ALG_SHA_256  ? 32u : \
	 PSA_ALG_GET_HASH(alg) == PSA_ALG_SHA_384  ? 48u : \
	 PSA_ALG_GET_HASH(alg) == PSA_ALG_SHA_512  ? 64u : \
	 PSA_ALG_GET_HASH(alg) == PSA_ALG_SHA3_224 ? 28u : \
	 PSA_ALG_GET_HASH(alg) == PSA_ALG_SHA3_256 ? 32u : \
	 PSA_ALG_GET_HASH(alg) == PSA_ALG_SHA3_384 ? 48u : \
	 PSA_ALG_GET_HASH(alg) == PSA_ALG_SHA3_512 ? 64u : \
						     0u)

#define PSA_HASH_MAX_SIZE 64u

#define PSA_HASH_BLOCK_LENGTH(alg)                          \
	(PSA_ALG_GET_HASH(alg) == PSA_ALG_SHA_256  ? 64u :  \
	 PSA_ALG_GET_HASH(alg) == PSA_ALG_SHA_384  ? 128u : \
	 PSA_ALG_GET_HASH(alg) == PSA_ALG_SHA_512  ? 128u : \
	 PSA_ALG_GET_HASH(alg) == PSA_ALG_SHA3_224 ? 144u : \
	 PSA_ALG_GET_HASH(alg) == PSA_ALG_SHA3_256 ? 136u : \
	 PSA_ALG_GET_HASH(alg) == PSA_ALG_SHA3_384 ? 104u : \
	 PSA_ALG_GET_HASH(alg) == PSA_ALG_SHA3_512 ? 72u :  \
						     0u)

/* ---- MAC ----------------------------------------------------------------- */

#define PSA_MAC_LENGTH(key_type, key_bits, alg)                                            \
	(((alg) & PSA_ALG_MAC_TRUNCATION_MASK) ? PSA_MAC_TRUNCATED_LENGTH(alg) :           \
	 PSA_ALG_IS_HMAC(alg)                  ? PSA_HASH_LENGTH(alg) :                    \
	 PSA_ALG_IS_BLOCK_CIPHER_MAC(alg)      ? PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) : \
						 0u)

#define PSA_MAC_MAX_SIZE PSA_HASH_MAX_SIZE

/* ---- Cipher -------------------------------------------------------------- */

#define PSA_BLOCK_CIPHER_BLOCK_MAX_SIZE 16u

#define PSA_CIPHER_IV_LENGTH(key_type, alg)                                          \
	(PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) > 1 &&                              \
			 ((alg) == PSA_ALG_CTR || (alg) == PSA_ALG_CBC_NO_PADDING || \
			  (alg) == PSA_ALG_CBC_PKCS7) ?                              \
		 PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) :                           \
		 0u)

#define PSA_CIPHER_IV_MAX_SIZE 16u

#define PSA_CIPHER_ENCRYPT_OUTPUT_SIZE(key_type, alg, input_length)                          \
	((alg) == PSA_ALG_CBC_PKCS7 ?                                                        \
		 (PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) != 0 ?                             \
			  PSA_ROUND_UP_TO_MULTIPLE(PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type),  \
						   (input_length) + 1u) +                    \
				  PSA_CIPHER_IV_LENGTH((key_type), (alg)) :                  \
			  0u) :                                                              \
	 PSA_ALG_IS_CIPHER(alg) ? (input_length) + PSA_CIPHER_IV_LENGTH((key_type), (alg)) : \
				  0u)

#define PSA_CIPHER_ENCRYPT_OUTPUT_MAX_SIZE(input_length)                                  \
	(PSA_ROUND_UP_TO_MULTIPLE(PSA_BLOCK_CIPHER_BLOCK_MAX_SIZE, (input_length) + 1u) + \
	 PSA_CIPHER_IV_MAX_SIZE)

#define PSA_CIPHER_DECRYPT_OUTPUT_SIZE(key_type, alg, input_length)             \
	(PSA_ALG_IS_CIPHER(alg) && ((key_type) & PSA_KEY_TYPE_CATEGORY_MASK) == \
					   PSA_KEY_TYPE_CATEGORY_SYMMETRIC ?    \
		 (input_length) :                                               \
		 0u)

#define PSA_CIPHER_DECRYPT_OUTPUT_MAX_SIZE(input_length) (input_length)

#define PSA_CIPHER_UPDATE_OUTPUT_SIZE(key_type, alg, input_length)                           \
	(PSA_ALG_IS_CIPHER(alg) ?                                                            \
		 (PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) != 0 ?                             \
			  (((alg) == PSA_ALG_CBC_PKCS7 || (alg) == PSA_ALG_CBC_NO_PADDING) ? \
				   PSA_ROUND_UP_TO_MULTIPLE(                                 \
					   PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type),          \
					   (input_length)) :                                 \
				   (input_length)) :                                         \
			  0u) :                                                              \
		 0u)

#define PSA_CIPHER_UPDATE_OUTPUT_MAX_SIZE(input_length) \
	PSA_ROUND_UP_TO_MULTIPLE(PSA_BLOCK_CIPHER_BLOCK_MAX_SIZE, (input_length))

#define PSA_CIPHER_FINISH_OUTPUT_SIZE(key_type, alg)                                           \
	(PSA_ALG_IS_CIPHER(alg) ?                                                              \
		 ((alg) == PSA_ALG_CBC_PKCS7 ? PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) : 0u) : \
		 0u)

#define PSA_CIPHER_FINISH_OUTPUT_MAX_SIZE PSA_BLOCK_CIPHER_BLOCK_MAX_SIZE

/* ---- AEAD ---------------------------------------------------------------- */

#define PSA_AEAD_NONCE_LENGTH(key_type, alg)                                                      \
	(PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) == 16 ?                                          \
		 (PSA_ALG_AEAD_WITH_DEFAULT_LENGTH_TAG(alg) == PSA_ALG_CCM ? 13u :                \
		  PSA_ALG_AEAD_WITH_DEFAULT_LENGTH_TAG(alg) == PSA_ALG_GCM ? 12u :                \
									     0u) :                \
	 (key_type) == PSA_KEY_TYPE_CHACHA20 &&                                                   \
			 PSA_ALG_AEAD_WITH_DEFAULT_LENGTH_TAG(alg) == PSA_ALG_CHACHA20_POLY1305 ? \
		 12u :                                                                            \
		 0u)

#define PSA_AEAD_NONCE_MAX_SIZE 13u

#define PSA_AEAD_TAG_LENGTH(key_type, key_bits, alg) \
	(PSA_AEAD_NONCE_LENGTH(key_type, alg) != 0 ? PSA_ALG_AEAD_GET_TAG_LENGTH(alg) : 0u)

#define PSA_AEAD_TAG_MAX_SIZE 16u

#define PSA_AEAD_ENCRYPT_OUTPUT_SIZE(key_type, alg, plaintext_length)    \
	(PSA_AEAD_NONCE_LENGTH(key_type, alg) != 0 ?                     \
		 (plaintext_length) + PSA_ALG_AEAD_GET_TAG_LENGTH(alg) : \
		 0u)

#define PSA_AEAD_ENCRYPT_OUTPUT_MAX_SIZE(plaintext_length) \
	((plaintext_length) + PSA_AEAD_TAG_MAX_SIZE)

#define PSA_AEAD_DECRYPT_OUTPUT_SIZE(key_type, alg, ciphertext_length)            \
	(PSA_AEAD_NONCE_LENGTH(key_type, alg) != 0 &&                             \
			 (ciphertext_length) > PSA_ALG_AEAD_GET_TAG_LENGTH(alg) ? \
		 (ciphertext_length) - PSA_ALG_AEAD_GET_TAG_LENGTH(alg) :         \
		 0u)

#define PSA_AEAD_DECRYPT_OUTPUT_MAX_SIZE(ciphertext_length) (ciphertext_length)

#define PSA_AEAD_UPDATE_OUTPUT_SIZE(key_type, alg, input_length)                            \
	(PSA_AEAD_NONCE_LENGTH(key_type, alg) != 0 ?                                        \
		 (PSA_ALG_IS_AEAD_ON_BLOCK_CIPHER(alg) ?                                    \
			  PSA_ROUND_UP_TO_MULTIPLE(PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type), \
						   (input_length)) :                        \
			  (input_length)) :                                                 \
		 0u)

#define PSA_AEAD_UPDATE_OUTPUT_MAX_SIZE(input_length) \
	PSA_ROUND_UP_TO_MULTIPLE(PSA_BLOCK_CIPHER_BLOCK_MAX_SIZE, (input_length))

#define PSA_AEAD_FINISH_OUTPUT_SIZE(key_type, alg)                                           \
	(PSA_AEAD_NONCE_LENGTH(key_type, alg) != 0 && PSA_ALG_IS_AEAD_ON_BLOCK_CIPHER(alg) ? \
		 PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) :                                   \
		 0u)

#define PSA_AEAD_FINISH_OUTPUT_MAX_SIZE PSA_BLOCK_CIPHER_BLOCK_MAX_SIZE

#define PSA_AEAD_VERIFY_OUTPUT_SIZE(key_type, alg)                                           \
	(PSA_AEAD_NONCE_LENGTH(key_type, alg) != 0 && PSA_ALG_IS_AEAD_ON_BLOCK_CIPHER(alg) ? \
		 PSA_BLOCK_CIPHER_BLOCK_LENGTH(key_type) :                                   \
		 0u)

#define PSA_AEAD_VERIFY_OUTPUT_MAX_SIZE PSA_BLOCK_CIPHER_BLOCK_MAX_SIZE

/* ---- Asymmetric signature ------------------------------------------------ */

#define PSA_ECDSA_SIGNATURE_SIZE(curve_bits) (PSA_BITS_TO_BYTES(curve_bits) * 2u)

#define PSA_SIGN_OUTPUT_SIZE(key_type, key_bits, alg)                                       \
	(PSA_KEY_TYPE_IS_ECC(key_type) ?                                                    \
		 ((alg) == PSA_ALG_PURE_EDDSA ? 64u : PSA_ECDSA_SIGNATURE_SIZE(key_bits)) : \
		 0u)

#define PSA_SIGNATURE_MAX_SIZE PSA_ECDSA_SIGNATURE_SIZE(384)

/* ---- Key agreement ------------------------------------------------------- */

#define PSA_RAW_KEY_AGREEMENT_OUTPUT_SIZE(key_type, key_bits) \
	(PSA_KEY_TYPE_IS_ECC_KEY_PAIR(key_type) ? PSA_BITS_TO_BYTES(key_bits) : 0u)

#define PSA_RAW_KEY_AGREEMENT_OUTPUT_MAX_SIZE PSA_BITS_TO_BYTES(384)

#endif /* IRONSIDE_CLIENT_PSA_CRYPTO_SIZES_H */
