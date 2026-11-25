/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - PSA Crypto extensions.
 *
 */

#ifndef IRONSIDE_CLIENT_PSA_CRYPTO_EXTRA_H
#define IRONSIDE_CLIENT_PSA_CRYPTO_EXTRA_H

#include <ironside_client/psa/key_persistence.h>

/*
 * Persistence levels of a persistent key. Destroying a REVOKABLE key revokes its
 * ID: a later create under the ID fails with PSA_ERROR_ALREADY_EXISTS and a later
 * use with PSA_ERROR_NOT_PERMITTED. A LOCKED key cannot be destroyed.
 */
#define IRONSIDE_CLIENT_KEY_PERSISTENCE_ROTATING PSA_KEY_PERSISTENCE_DEFAULT
#define IRONSIDE_CLIENT_KEY_PERSISTENCE_REVOKABLE \
	((psa_key_persistence_t)IRONSIDE_CLIENT_KEY_PERSISTENCE_REVOKABLE_VALUE)
#define IRONSIDE_CLIENT_KEY_PERSISTENCE_LOCKED \
	((psa_key_persistence_t)IRONSIDE_CLIENT_KEY_PERSISTENCE_LOCKED_VALUE)

/*
 * Persistent key IDs reserved for the IronSide DFU image's keys. Documented but
 * not enforced: an App may create a key in this range and collide with a
 * provisioned DFU image key.
 */
#define IRONSIDE_CLIENT_RESERVED_KEY_ID_MIN 228u
#define IRONSIDE_CLIENT_RESERVED_KEY_ID_MAX 239u

#define IRONSIDE_CLIENT_MEXT_ID ((psa_key_id_t)0x7fffc003) /* AES-256 */

/*
 * The multi-part operations IronSide TE holds in progress at once, of all kinds together. A
 * setup past this number fails with PSA_ERROR_NOT_PERMITTED until an operation is finished or
 * aborted.
 */
#define IRONSIDE_CLIENT_PSA_CRYPTO_MAX_OPERATIONS 4u

#endif /* IRONSIDE_CLIENT_PSA_CRYPTO_EXTRA_H */
