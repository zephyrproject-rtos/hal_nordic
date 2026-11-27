/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - PSA Crypto values.
 *
 */

#ifndef IRONSIDE_CLIENT_PSA_CRYPTO_VALUES_H
#define IRONSIDE_CLIENT_PSA_CRYPTO_VALUES_H

#include <ironside_client/psa/crypto_types.h>

/* ---- Key types and ECC families ------------------------------------------ */

#define PSA_KEY_TYPE_NONE                ((psa_key_type_t)0x0000)
#define PSA_KEY_TYPE_CATEGORY_MASK       ((psa_key_type_t)0x7000)
#define PSA_KEY_TYPE_CATEGORY_RAW        ((psa_key_type_t)0x1000)
#define PSA_KEY_TYPE_CATEGORY_SYMMETRIC  ((psa_key_type_t)0x2000)
#define PSA_KEY_TYPE_CATEGORY_PUBLIC_KEY ((psa_key_type_t)0x4000)
#define PSA_KEY_TYPE_CATEGORY_KEY_PAIR   ((psa_key_type_t)0x7000)
#define PSA_KEY_TYPE_CATEGORY_FLAG_PAIR  ((psa_key_type_t)0x3000)

#define PSA_KEY_TYPE_IS_UNSTRUCTURED(type)                                     \
	(((type) & PSA_KEY_TYPE_CATEGORY_MASK) == PSA_KEY_TYPE_CATEGORY_RAW || \
	 ((type) & PSA_KEY_TYPE_CATEGORY_MASK) == PSA_KEY_TYPE_CATEGORY_SYMMETRIC)
#define PSA_KEY_TYPE_IS_ASYMMETRIC(type)                                             \
	(((type) & PSA_KEY_TYPE_CATEGORY_MASK & ~PSA_KEY_TYPE_CATEGORY_FLAG_PAIR) == \
	 PSA_KEY_TYPE_CATEGORY_PUBLIC_KEY)
#define PSA_KEY_TYPE_IS_PUBLIC_KEY(type) \
	(((type) & PSA_KEY_TYPE_CATEGORY_MASK) == PSA_KEY_TYPE_CATEGORY_PUBLIC_KEY)
#define PSA_KEY_TYPE_IS_KEY_PAIR(type) \
	(((type) & PSA_KEY_TYPE_CATEGORY_MASK) == PSA_KEY_TYPE_CATEGORY_KEY_PAIR)
#define PSA_KEY_TYPE_PUBLIC_KEY_OF_KEY_PAIR(type) \
	((psa_key_type_t)((type) & ~PSA_KEY_TYPE_CATEGORY_FLAG_PAIR))
#define PSA_KEY_TYPE_KEY_PAIR_OF_PUBLIC_KEY(type) \
	((psa_key_type_t)((type) | PSA_KEY_TYPE_CATEGORY_FLAG_PAIR))

#define PSA_KEY_TYPE_RAW_DATA                ((psa_key_type_t)0x1001)
#define PSA_KEY_TYPE_HMAC                    ((psa_key_type_t)0x1100)
#define PSA_KEY_TYPE_AES                     ((psa_key_type_t)0x2400)
#define PSA_KEY_TYPE_CHACHA20                ((psa_key_type_t)0x2004)
#define PSA_KEY_TYPE_DERIVE                  ((psa_key_type_t)0x1200)
#define PSA_KEY_TYPE_PASSWORD                ((psa_key_type_t)0x1203)
#define PSA_KEY_TYPE_SPAKE2P_PUBLIC_KEY_BASE ((psa_key_type_t)0x4400)
#define PSA_KEY_TYPE_SPAKE2P_KEY_PAIR_BASE   ((psa_key_type_t)0x7400)
#define PSA_KEY_TYPE_SPAKE2P_PUBLIC_KEY(curve) \
	((psa_key_type_t)(PSA_KEY_TYPE_SPAKE2P_PUBLIC_KEY_BASE | (curve)))
#define PSA_KEY_TYPE_SPAKE2P_KEY_PAIR(curve) \
	((psa_key_type_t)(PSA_KEY_TYPE_SPAKE2P_KEY_PAIR_BASE | (curve)))

#define PSA_ECC_FAMILY_SECP_R1         ((psa_ecc_family_t)0x12)
#define PSA_ECC_FAMILY_MONTGOMERY      ((psa_ecc_family_t)0x41)
#define PSA_ECC_FAMILY_TWISTED_EDWARDS ((psa_ecc_family_t)0x42)

#define PSA_KEY_TYPE_ECC_PUBLIC_KEY_BASE ((psa_key_type_t)0x4100)
#define PSA_KEY_TYPE_ECC_KEY_PAIR_BASE   ((psa_key_type_t)0x7100)
#define PSA_KEY_TYPE_ECC_CURVE_MASK      ((psa_key_type_t)0x00ff)

#define PSA_KEY_TYPE_ECC_KEY_PAIR(curve) \
	((psa_key_type_t)(PSA_KEY_TYPE_ECC_KEY_PAIR_BASE | (curve)))
#define PSA_KEY_TYPE_ECC_PUBLIC_KEY(curve) \
	((psa_key_type_t)(PSA_KEY_TYPE_ECC_PUBLIC_KEY_BASE | (curve)))

#define PSA_KEY_TYPE_IS_ECC(type)                                                      \
	((PSA_KEY_TYPE_PUBLIC_KEY_OF_KEY_PAIR(type) & ~PSA_KEY_TYPE_ECC_CURVE_MASK) == \
	 PSA_KEY_TYPE_ECC_PUBLIC_KEY_BASE)
#define PSA_KEY_TYPE_IS_ECC_KEY_PAIR(type) \
	(((type) & ~PSA_KEY_TYPE_ECC_CURVE_MASK) == PSA_KEY_TYPE_ECC_KEY_PAIR_BASE)
#define PSA_KEY_TYPE_IS_ECC_PUBLIC_KEY(type) \
	(((type) & ~PSA_KEY_TYPE_ECC_CURVE_MASK) == PSA_KEY_TYPE_ECC_PUBLIC_KEY_BASE)
#define PSA_KEY_TYPE_ECC_GET_FAMILY(type) \
	((psa_ecc_family_t)(PSA_KEY_TYPE_IS_ECC(type) ? ((type) & PSA_KEY_TYPE_ECC_CURVE_MASK) : 0))

#define PSA_BLOCK_CIPHER_BLOCK_LENGTH(type)                                         \
	(((type) & PSA_KEY_TYPE_CATEGORY_MASK) == PSA_KEY_TYPE_CATEGORY_SYMMETRIC ? \
		 1u << (((type) >> 8) & 7) :                                        \
		 0u)

/* ---- Key lifetimes ------------------------------------------------------- */

#define PSA_KEY_LIFETIME_VOLATILE   ((psa_key_lifetime_t)0x00000000)
#define PSA_KEY_LIFETIME_PERSISTENT ((psa_key_lifetime_t)0x00000001)

#define PSA_KEY_PERSISTENCE_VOLATILE  ((psa_key_persistence_t)0x00)
#define PSA_KEY_PERSISTENCE_DEFAULT   ((psa_key_persistence_t)0x01)
#define PSA_KEY_PERSISTENCE_READ_ONLY ((psa_key_persistence_t)0xff)

#define PSA_KEY_LOCATION_LOCAL_STORAGE ((psa_key_location_t)0x000000)

#define PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(persistence, location) \
	((psa_key_lifetime_t)(((uint32_t)(location) << 8) | (persistence)))
#define PSA_KEY_LIFETIME_GET_PERSISTENCE(lifetime) \
	((psa_key_persistence_t)((uint32_t)(lifetime) & 0xffu))
#define PSA_KEY_LIFETIME_GET_LOCATION(lifetime) ((psa_key_location_t)((uint32_t)(lifetime) >> 8))
#define PSA_KEY_LIFETIME_IS_VOLATILE(lifetime) \
	(PSA_KEY_LIFETIME_GET_PERSISTENCE(lifetime) == PSA_KEY_PERSISTENCE_VOLATILE)

/* ---- Key identifiers ----------------------------------------------------- */

#define PSA_KEY_ID_NULL       ((psa_key_id_t)0x00000000)
#define PSA_KEY_ID_USER_MIN   ((psa_key_id_t)0x00000001)
#define PSA_KEY_ID_USER_MAX   ((psa_key_id_t)0x3fffffff)
#define PSA_KEY_ID_VENDOR_MIN ((psa_key_id_t)0x40000000)
#define PSA_KEY_ID_VENDOR_MAX ((psa_key_id_t)0x7fffffff)

/* ---- Algorithm encoding -------------------------------------------------- */

#define PSA_ALG_NONE ((psa_algorithm_t)0)

/* Encoding of the algorithm namespace, as used by the predicates below. */
#define PSA_ALG_CATEGORY_MASK           ((psa_algorithm_t)0x7f000000)
#define PSA_ALG_CATEGORY_HASH           ((psa_algorithm_t)0x02000000)
#define PSA_ALG_CATEGORY_MAC            ((psa_algorithm_t)0x03000000)
#define PSA_ALG_CATEGORY_CIPHER         ((psa_algorithm_t)0x04000000)
#define PSA_ALG_CATEGORY_AEAD           ((psa_algorithm_t)0x05000000)
#define PSA_ALG_CATEGORY_SIGN           ((psa_algorithm_t)0x06000000)
#define PSA_ALG_CATEGORY_KEY_DERIVATION ((psa_algorithm_t)0x08000000)
#define PSA_ALG_CATEGORY_KEY_AGREEMENT  ((psa_algorithm_t)0x09000000)
#define PSA_ALG_CATEGORY_KEY_WRAP       ((psa_algorithm_t)0x0b000000)
#define PSA_ALG_HASH_MASK               ((psa_algorithm_t)0x000000ff)

#define PSA_ALG_IS_HASH(alg)     (((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_HASH)
#define PSA_ALG_IS_MAC(alg)      (((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_MAC)
#define PSA_ALG_IS_CIPHER(alg)   (((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_CIPHER)
#define PSA_ALG_IS_AEAD(alg)     (((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_AEAD)
#define PSA_ALG_IS_SIGN(alg)     (((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_SIGN)
#define PSA_ALG_IS_KEY_WRAP(alg) (((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_KEY_WRAP)
#define PSA_ALG_IS_KEY_DERIVATION(alg) \
	(((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_KEY_DERIVATION)
#define PSA_ALG_IS_KEY_AGREEMENT(alg) \
	(((alg) & PSA_ALG_CATEGORY_MASK) == PSA_ALG_CATEGORY_KEY_AGREEMENT)

/* The hash an algorithm is built on: the hash itself for a bare hash algorithm,
 * the underlying hash for an HMAC / hash-and-sign one, PSA_ALG_NONE otherwise. */
#define PSA_ALG_GET_HASH(alg)               \
	(((alg) & PSA_ALG_HASH_MASK) == 0 ? \
		 PSA_ALG_NONE :             \
		 ((psa_algorithm_t)(PSA_ALG_CATEGORY_HASH | ((alg) & PSA_ALG_HASH_MASK))))

/* ---- Key usage flags ----------------------------------------------------- */

#define PSA_KEY_USAGE_EXPORT         ((psa_key_usage_t)0x00000001)
#define PSA_KEY_USAGE_COPY           ((psa_key_usage_t)0x00000002)
#define PSA_KEY_USAGE_ENCRYPT        ((psa_key_usage_t)0x00000100)
#define PSA_KEY_USAGE_DECRYPT        ((psa_key_usage_t)0x00000200)
#define PSA_KEY_USAGE_SIGN_MESSAGE   ((psa_key_usage_t)0x00000400)
#define PSA_KEY_USAGE_VERIFY_MESSAGE ((psa_key_usage_t)0x00000800)
#define PSA_KEY_USAGE_SIGN_HASH      ((psa_key_usage_t)0x00001000)
#define PSA_KEY_USAGE_VERIFY_HASH    ((psa_key_usage_t)0x00002000)
#define PSA_KEY_USAGE_DERIVE         ((psa_key_usage_t)0x00004000)
#define PSA_KEY_USAGE_UNWRAP         ((psa_key_usage_t)0x00020000)

/* ---- Hash algorithms ----------------------------------------------------- */

#define PSA_ALG_SHA_1    ((psa_algorithm_t)0x02000005)
#define PSA_ALG_SHA_256  ((psa_algorithm_t)0x02000009)
#define PSA_ALG_SHA_384  ((psa_algorithm_t)0x0200000a)
#define PSA_ALG_SHA_512  ((psa_algorithm_t)0x0200000b)
#define PSA_ALG_SHA3_224 ((psa_algorithm_t)0x02000010)
#define PSA_ALG_SHA3_256 ((psa_algorithm_t)0x02000011)
#define PSA_ALG_SHA3_384 ((psa_algorithm_t)0x02000012)
#define PSA_ALG_SHA3_512 ((psa_algorithm_t)0x02000013)

/* ---- MAC algorithms ------------------------------------------------------ */

#define PSA_ALG_MAC_SUBCATEGORY_MASK ((psa_algorithm_t)0x00c00000)
#define PSA_ALG_HMAC_BASE            ((psa_algorithm_t)0x03800000)
#define PSA_ALG_CIPHER_MAC_BASE      ((psa_algorithm_t)0x03c00000)
#define PSA_ALG_MAC_TRUNCATION_MASK  ((psa_algorithm_t)0x003f0000)
#define PSA_MAC_TRUNCATION_OFFSET    16

#define PSA_ALG_HMAC(hash_alg) \
	((psa_algorithm_t)(PSA_ALG_HMAC_BASE | ((hash_alg) & PSA_ALG_HASH_MASK)))
#define PSA_ALG_CMAC ((psa_algorithm_t)0x03c00200)

#define PSA_ALG_IS_HMAC(alg) \
	(((alg) & (PSA_ALG_CATEGORY_MASK | PSA_ALG_MAC_SUBCATEGORY_MASK)) == PSA_ALG_HMAC_BASE)
#define PSA_ALG_IS_BLOCK_CIPHER_MAC(alg)                                     \
	(((alg) & (PSA_ALG_CATEGORY_MASK | PSA_ALG_MAC_SUBCATEGORY_MASK)) == \
	 PSA_ALG_CIPHER_MAC_BASE)
#define PSA_MAC_TRUNCATED_LENGTH(mac_alg) \
	(((mac_alg) & PSA_ALG_MAC_TRUNCATION_MASK) >> PSA_MAC_TRUNCATION_OFFSET)

/* ---- Cipher algorithms --------------------------------------------------- */

#define PSA_ALG_CTR            ((psa_algorithm_t)0x04c01000)
#define PSA_ALG_ECB_NO_PADDING ((psa_algorithm_t)0x04404400)
#define PSA_ALG_CBC_NO_PADDING ((psa_algorithm_t)0x04404000)
#define PSA_ALG_CBC_PKCS7      ((psa_algorithm_t)0x04404100)

/* ---- AEAD algorithms ----------------------------------------------------- */

#define PSA_ALG_AEAD_FROM_BLOCK_FLAG ((psa_algorithm_t)0x00400000)

#define PSA_ALG_CCM               ((psa_algorithm_t)0x05500100)
#define PSA_ALG_GCM               ((psa_algorithm_t)0x05500200)
#define PSA_ALG_CHACHA20_POLY1305 ((psa_algorithm_t)0x05100500)

#define PSA_ALG_IS_AEAD_ON_BLOCK_CIPHER(alg)                                 \
	(((alg) & (PSA_ALG_CATEGORY_MASK | PSA_ALG_AEAD_FROM_BLOCK_FLAG)) == \
	 (PSA_ALG_CATEGORY_AEAD | PSA_ALG_AEAD_FROM_BLOCK_FLAG))

#define PSA_ALG_AEAD_TAG_LENGTH_MASK   ((psa_algorithm_t)0x003f0000)
#define PSA_ALG_AEAD_TAG_LENGTH_OFFSET 16
#define PSA_ALG_AEAD_GET_TAG_LENGTH(aead_alg) \
	(((aead_alg) & PSA_ALG_AEAD_TAG_LENGTH_MASK) >> PSA_ALG_AEAD_TAG_LENGTH_OFFSET)

#define PSA_ALG_AEAD_WITH_SHORTENED_TAG(aead_alg, tag_length)                  \
	((psa_algorithm_t)(((aead_alg) & ~PSA_ALG_AEAD_TAG_LENGTH_MASK) |      \
			   (((tag_length) << PSA_ALG_AEAD_TAG_LENGTH_OFFSET) & \
			    PSA_ALG_AEAD_TAG_LENGTH_MASK)))

#define PSA_ALG_AEAD_WITH_DEFAULT_LENGTH_TAG(aead_alg)                                   \
	(PSA_ALG_AEAD_WITH_SHORTENED_TAG(aead_alg, 0) ==                                 \
			 PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CCM, 0) ?               \
		 PSA_ALG_CCM :                                                           \
	 PSA_ALG_AEAD_WITH_SHORTENED_TAG(aead_alg, 0) ==                                 \
			 PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, 0) ?               \
		 PSA_ALG_GCM :                                                           \
	 PSA_ALG_AEAD_WITH_SHORTENED_TAG(aead_alg, 0) ==                                 \
			 PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_CHACHA20_POLY1305, 0) ? \
		 PSA_ALG_CHACHA20_POLY1305 :                                             \
		 PSA_ALG_NONE)

/* ---- Key-wrap algorithms ------------------------------------------------- */

#define PSA_ALG_KW ((psa_algorithm_t)0x0b400100)

/* ---- Key-derivation algorithms and steps --------------------------------- */

#define PSA_ALG_SP800_108_COUNTER_CMAC ((psa_algorithm_t)0x08000800)

#define PSA_ALG_PBKDF2_HMAC_BASE ((psa_algorithm_t)0x08800100)
#define PSA_ALG_PBKDF2_HMAC(hash_alg) \
	((psa_algorithm_t)(PSA_ALG_PBKDF2_HMAC_BASE | ((hash_alg) & PSA_ALG_HASH_MASK)))
#define PSA_ALG_PBKDF2_AES_CMAC_PRF_128 ((psa_algorithm_t)0x08800200)

#define PSA_ALG_TLS12_PRF_BASE ((psa_algorithm_t)0x08000200)
#define PSA_ALG_TLS12_PRF(hash_alg) \
	((psa_algorithm_t)(PSA_ALG_TLS12_PRF_BASE | ((hash_alg) & PSA_ALG_HASH_MASK)))
#define PSA_ALG_TLS12_ECJPAKE_TO_PMS ((psa_algorithm_t)0x08000609)

#define PSA_ALG_HKDF_BASE         ((psa_algorithm_t)0x08000100)
#define PSA_ALG_HKDF_EXTRACT_BASE ((psa_algorithm_t)0x08000400)
#define PSA_ALG_HKDF_EXPAND_BASE  ((psa_algorithm_t)0x08000500)
#define PSA_ALG_HKDF(hash_alg) \
	((psa_algorithm_t)(PSA_ALG_HKDF_BASE | ((hash_alg) & PSA_ALG_HASH_MASK)))
#define PSA_ALG_HKDF_EXTRACT(hash_alg) \
	((psa_algorithm_t)(PSA_ALG_HKDF_EXTRACT_BASE | ((hash_alg) & PSA_ALG_HASH_MASK)))
#define PSA_ALG_HKDF_EXPAND(hash_alg) \
	((psa_algorithm_t)(PSA_ALG_HKDF_EXPAND_BASE | ((hash_alg) & PSA_ALG_HASH_MASK)))

#define PSA_KEY_DERIVATION_INPUT_SECRET   ((psa_key_derivation_step_t)0x0101)
#define PSA_KEY_DERIVATION_INPUT_PASSWORD ((psa_key_derivation_step_t)0x0102)
#define PSA_KEY_DERIVATION_INPUT_LABEL    ((psa_key_derivation_step_t)0x0201)
#define PSA_KEY_DERIVATION_INPUT_SALT     ((psa_key_derivation_step_t)0x0202)
#define PSA_KEY_DERIVATION_INPUT_INFO     ((psa_key_derivation_step_t)0x0203)
#define PSA_KEY_DERIVATION_INPUT_SEED     ((psa_key_derivation_step_t)0x0204)
#define PSA_KEY_DERIVATION_INPUT_COST     ((psa_key_derivation_step_t)0x0205)
#define PSA_KEY_DERIVATION_INPUT_CONTEXT  ((psa_key_derivation_step_t)0x0206)

#define PSA_KEY_DERIVATION_UNLIMITED_CAPACITY ((size_t)(-1))

/* ---- Asymmetric signature algorithms ------------------------------------- */

#define PSA_ALG_ECDSA_BASE               ((psa_algorithm_t)0x06000600)
#define PSA_ALG_ECDSA_DETERMINISTIC_FLAG ((psa_algorithm_t)0x00000100)

#define PSA_ALG_ECDSA(hash_alg) \
	((psa_algorithm_t)(PSA_ALG_ECDSA_BASE | ((hash_alg) & PSA_ALG_HASH_MASK)))
#define PSA_ALG_DETERMINISTIC_ECDSA(hash_alg)                                      \
	((psa_algorithm_t)(PSA_ALG_ECDSA_BASE | PSA_ALG_ECDSA_DETERMINISTIC_FLAG | \
			   ((hash_alg) & PSA_ALG_HASH_MASK)))
#define PSA_ALG_ECDSA_ANY PSA_ALG_ECDSA_BASE
#define PSA_ALG_IS_ECDSA(alg) \
	(((alg) & ~PSA_ALG_HASH_MASK & ~PSA_ALG_ECDSA_DETERMINISTIC_FLAG) == PSA_ALG_ECDSA_BASE)

#define PSA_ALG_PURE_EDDSA ((psa_algorithm_t)0x06000800)

/* ---- Password-authenticated key exchange --------------------------------- */

#define PSA_ALG_JPAKE_BASE ((psa_algorithm_t)0x0a000100)
#define PSA_ALG_JPAKE(hash_alg) \
	((psa_algorithm_t)(PSA_ALG_JPAKE_BASE | ((hash_alg) & PSA_ALG_HASH_MASK)))
#define PSA_ALG_SPAKE2P_MATTER ((psa_algorithm_t)0x0a000609)

#define PSA_PAKE_ROLE_NONE   ((psa_pake_role_t)0x00)
#define PSA_PAKE_ROLE_FIRST  ((psa_pake_role_t)0x01)
#define PSA_PAKE_ROLE_SECOND ((psa_pake_role_t)0x02)
#define PSA_PAKE_ROLE_CLIENT ((psa_pake_role_t)0x11)
#define PSA_PAKE_ROLE_SERVER ((psa_pake_role_t)0x12)

#define PSA_PAKE_PRIMITIVE_TYPE_ECC ((psa_pake_primitive_type_t)0x01)
#define PSA_PAKE_PRIMITIVE(pake_type, pake_family, pake_bits) \
	(((pake_bits) & 0xFFFF) != (pake_bits) ?              \
		 0 :                                          \
		 ((psa_pake_primitive_t)(((pake_type) << 24 | (pake_family) << 16) | (pake_bits))))
#define PSA_PAKE_PRIMITIVE_GET_BITS(pake_primitive) ((size_t)((pake_primitive) & 0xFFFF))
#define PSA_PAKE_PRIMITIVE_GET_FAMILY(pake_primitive) \
	((psa_pake_family_t)(((pake_primitive) >> 16) & 0xFF))
#define PSA_PAKE_PRIMITIVE_GET_TYPE(pake_primitive) \
	((psa_pake_primitive_type_t)(((pake_primitive) >> 24) & 0xFF))

#define PSA_PAKE_CONFIRMED_KEY   0
#define PSA_PAKE_UNCONFIRMED_KEY 1

#define PSA_PAKE_STEP_KEY_SHARE ((psa_pake_step_t)0x01)
#define PSA_PAKE_STEP_ZK_PUBLIC ((psa_pake_step_t)0x02)
#define PSA_PAKE_STEP_ZK_PROOF  ((psa_pake_step_t)0x03)
#define PSA_PAKE_STEP_CONFIRM   ((psa_pake_step_t)0x04)

/* ---- Key-agreement algorithms -------------------------------------------- */

#define PSA_ALG_KEY_DERIVATION_MASK ((psa_algorithm_t)0xfe00ffff)
#define PSA_ALG_KEY_AGREEMENT_MASK  ((psa_algorithm_t)0xffff0000)

#define PSA_ALG_ECDH ((psa_algorithm_t)0x09020000)

#define PSA_ALG_KEY_AGREEMENT(ka_alg, kdf_alg) ((psa_algorithm_t)((ka_alg) | (kdf_alg)))

#define PSA_ALG_KEY_AGREEMENT_GET_BASE(alg) \
	((psa_algorithm_t)(((alg) & PSA_ALG_KEY_AGREEMENT_MASK) | PSA_ALG_CATEGORY_KEY_AGREEMENT))
#define PSA_ALG_KEY_AGREEMENT_GET_KDF(alg) \
	((psa_algorithm_t)(((alg) & PSA_ALG_KEY_DERIVATION_MASK) | PSA_ALG_CATEGORY_KEY_DERIVATION))
#define PSA_ALG_IS_STANDALONE_KEY_AGREEMENT(alg)                            \
	(((alg) & (PSA_ALG_CATEGORY_MASK | ~PSA_ALG_KEY_AGREEMENT_MASK)) == \
	 PSA_ALG_CATEGORY_KEY_AGREEMENT)
#define PSA_ALG_IS_RAW_KEY_AGREEMENT(alg) PSA_ALG_IS_STANDALONE_KEY_AGREEMENT(alg)
#define PSA_ALG_IS_ECDH(alg)              (PSA_ALG_KEY_AGREEMENT_GET_BASE(alg) == PSA_ALG_ECDH)

#endif /* IRONSIDE_CLIENT_PSA_CRYPTO_VALUES_H */
