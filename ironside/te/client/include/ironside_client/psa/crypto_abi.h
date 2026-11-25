/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - PSA Crypto client ABI.
 *
 * The sizes, alignments and offsets that src/psa_crypto.c assumes of the
 * objects an App passes to the PSA Crypto entry points. Only macros, so that
 * an integration providing its own PSA headers can check its definitions
 * against these in a translation unit that includes those headers.
 */

#ifndef IRONSIDE_CLIENT_PSA_CRYPTO_ABI_H
#define IRONSIDE_CLIENT_PSA_CRYPTO_ABI_H

#define IRONSIDE_CLIENT_PSA_ABI_STATUS_SIZE          4
#define IRONSIDE_CLIENT_PSA_ABI_KEY_ID_SIZE          4
#define IRONSIDE_CLIENT_PSA_ABI_KEY_TYPE_SIZE        2
#define IRONSIDE_CLIENT_PSA_ABI_KEY_BITS_SIZE        2
#define IRONSIDE_CLIENT_PSA_ABI_KEY_LIFETIME_SIZE    4
#define IRONSIDE_CLIENT_PSA_ABI_KEY_USAGE_SIZE       4
#define IRONSIDE_CLIENT_PSA_ABI_ALGORITHM_SIZE       4
#define IRONSIDE_CLIENT_PSA_ABI_DERIVATION_STEP_SIZE 2

#define IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_SIZE            24
#define IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_ALIGN           4
#define IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_TYPE_OFFSET     0
#define IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_BITS_OFFSET     2
#define IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_LIFETIME_OFFSET 4
#define IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_USAGE_OFFSET    8
#define IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_ALG_OFFSET      12
#define IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_ALG2_OFFSET     16
#define IRONSIDE_CLIENT_PSA_ABI_KEY_ATTRIBUTES_ID_OFFSET       20

/* The PAKE cipher suite is passed to UROT as it is, laid out as TF-PSA-Crypto's psa_pake_cipher_suite_t:
 * the algorithm, then the primitive as its type, family and size in bits, then the key
 * confirmation. An application whose PSA headers lay it out otherwise has to convert it first. */
#define IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_SIZE                    12
#define IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_ALIGN                   4
#define IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_ALGORITHM_OFFSET        0
#define IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_TYPE_OFFSET             4
#define IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_FAMILY_OFFSET           5
#define IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_BITS_OFFSET             6
#define IRONSIDE_CLIENT_PSA_ABI_PAKE_CIPHER_SUITE_KEY_CONFIRMATION_OFFSET 8

/* Every multipart operation object is a single handle. */
#define IRONSIDE_CLIENT_PSA_ABI_OPERATION_SIZE  4
#define IRONSIDE_CLIENT_PSA_ABI_OPERATION_ALIGN 4

#endif /* IRONSIDE_CLIENT_PSA_CRYPTO_ABI_H */
