/*
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * IronSide TE Client - persistence levels of a persistent key.
 *
 * Besides the default level (PSA_KEY_PERSISTENCE_DEFAULT, aliased as
 * IRONSIDE_CLIENT_KEY_PERSISTENCE_ROTATING), a persistent key can be REVOKABLE or
 * LOCKED. PSA Crypto reserves persistence 0x02..0x7f for the specification, so
 * both sit in the implementation-defined range 0x80..0xfe. psa/crypto_extra.h
 * publishes them as IRONSIDE_CLIENT_KEY_PERSISTENCE_*.
 */

#ifndef IRONSIDE_CLIENT_PSA_KEY_PERSISTENCE_H
#define IRONSIDE_CLIENT_PSA_KEY_PERSISTENCE_H

#define IRONSIDE_CLIENT_KEY_PERSISTENCE_REVOKABLE_VALUE 0x80u
#define IRONSIDE_CLIENT_KEY_PERSISTENCE_LOCKED_VALUE    0x81u

#endif /* IRONSIDE_CLIENT_PSA_KEY_PERSISTENCE_H */
