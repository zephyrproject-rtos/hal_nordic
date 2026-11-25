# Secure services and boot failure codes

Secure services are the PSA Crypto, PSA Attestation, and platform and boot-control functions that IronSide TE exposes to the non-secure images over a Non-Secure-Callable (NSC) gateway.

IronSide Integration Kit has C source files that the application can use to ease integration with IronSide TE.
Unless the system integrator or the RTOS SDK has already done so, the application must add all sources in `client/src/` to its C sources and `client/include/` to its include path.

After that, secure services are available to the application.
The following example calls `psa_hash_compute`, which is declared in `client/include/ironside_client/psa/crypto.h`:

```c
#include <ironside_client/psa/crypto.h>

int main(void)
{
    static uint8_t digest[32];   /* static -> RAM */
    size_t len = 0;
    psa_status_t status = psa_hash_compute(PSA_ALG_SHA_256, (const uint8_t *)"abc", 3,
      digest, sizeof(digest), &len);
}
```

You can call secure services from any non-secure context, thread or interrupt, but only one call can be in flight at a time.
For how to keep to one call at a time, see [One call at a time](#one-call-at-a-time).

The following sections describe the secure services that IronSide TE offers, how the services interact with the threads and interrupts of the application, and the codes that IronSide TE reports when it halts during boot:

* [PSA Crypto](#psa-crypto)
* [PSA Attestation](#psa-attestation)
* [Platform](#platform)
* [DFU boot selection](#dfu-boot-selection)
* [Boot report](#boot-report)
* [Executing code from RAM](#executing-code-from-ram)
* [Interrupts, concurrency and latency](#interrupts-concurrency-and-latency)
* [Boot failure codes](#boot-failure-codes)

## PSA Crypto

IronSide TE implements PSA Crypto 1.4.1.
For detailed documentation about this API, see the [PSA Crypto API 1.4 specification](https://arm-software.github.io/psa-api/crypto/1.4/).

The following sections describe the implementation-defined behavior and which parts of the API are implemented, that is, which algorithms, functions, and keys are available.

### Algorithm support

The following table lists the supported algorithms.

| **Category** | **Description** | **PSA Algorithm** | **Notes** |
| --- | --- | --- | --- |
| Hashes | SHA-256<br>SHA-384<br>SHA-512 | PSA_ALG_SHA_256<br>PSA_ALG_SHA_384<br>PSA_ALG_SHA_512 | FIPS 180-4 |
| Hashes | SHA-1 | PSA_ALG_SHA_1 | FIPS 180-4<br><br>Only as a plain hash. IronSide TE computes no HMAC, ECDSA signature or key derivation over SHA-1. PSA_ALG_ECDSA_ANY signs any digest that the caller supplies, including a SHA-1 digest. |
| Hashes | SHA3-224<br>SHA3-256<br>SHA3-384<br>SHA3-512 | PSA_ALG_SHA3_224 PSA_ALG_SHA3_256 PSA_ALG_SHA3_384 PSA_ALG_SHA3_512 | FIPS 202 |
| Message authentication codes | HMAC | PSA_ALG_HMAC(hash) | RFC 2104<br><br>Can be used with any supported hash algorithm except SHA-1. |
| Message authentication codes | AES-CMAC | PSA_ALG_CMAC | SP 800-38B<br>RFC 4493<br><br>Only the full-length 16-byte MAC is supported. Truncated CMAC is not available. |
| Unauthenticated ciphers | AES-CBC with and without padding. | PSA_ALG_CBC_NO_PADDING<br>PSA_ALG_CBC_PKCS7 | SP 800-38A<br>and<br>PKCS#7 |
| Unauthenticated ciphers | AES-CTR | PSA_ALG_CTR | SP 800-38A |
| Unauthenticated ciphers | AES-ECB without padding | PSA_ALG_ECB_NO_PADDING | SP 800-38A<br><br>The input length must be a whole number of 16-byte blocks. |
| AEAD | AES-GCM<br>AES-CCM | PSA_ALG_GCM<br>PSA_ALG_CCM | SP 800-38D<br>SP 800-38C |
| AEAD | ChaCha20-Poly1305 | PSA_ALG_CHACHA20_POLY1305 | RFC 8439<br><br>ChaCha20 is not available as an unauthenticated stream cipher. It is only for use in AEAD. |
| Asymmetric signature | ECDSA | PSA_ALG_ECDSA(hash)<br>PSA_ALG_DETERMINISTIC_ECDSA(hash)<br>PSA_ALG_ECDSA_ANY | FIPS 186-5<br>RFC 6979<br><br>Can be used with any supported hash algorithm except SHA-1. PSA_ALG_ECDSA_ANY signs a digest that the caller computes, with any hash algorithm.<br><br>Curves secp256r1 and secp384r1 can be used. |
| Asymmetric signature | PureEdDSA / Ed25519 | PSA_ALG_PURE_EDDSA | RFC 8032<br><br>Ed25519ph is not supported. |
| Key agreement | ECDH | PSA_ALG_ECDH | SP 800-56A<br><br>Curves secp256r1, secp384r1, and Curve25519 (X25519) can be used. |
| Key derivation | NIST SP 800-108r1 counter-mode KDF, CMAC PRF | PSA_ALG_SP800_108_COUNTER_CMAC | This algorithm is intended to be used with the Master External Storage Key as a secret input, but any other valid key can also be used. |
| Key derivation | HKDF, HKDF-Extract and HKDF-Expand | PSA_ALG_HKDF(hash)<br>PSA_ALG_HKDF_EXTRACT(hash)<br>PSA_ALG_HKDF_EXPAND(hash) | RFC 5869<br><br>Can be used with any supported hash algorithm except SHA-1. |
| Key derivation | PBKDF2 with HMAC | PSA_ALG_PBKDF2_HMAC(hash) | RFC 8018<br><br>Can be used with any supported hash algorithm except SHA-1.<br><br>The password is a key of type PSA_KEY_TYPE_PASSWORD. |
| Key derivation | PBKDF2 with AES-CMAC-PRF-128 | PSA_ALG_PBKDF2_AES_CMAC_PRF_128 | RFC 8018<br>RFC 4615<br><br>The password must be exactly 16 bytes. |
| Key derivation | TLS 1.2 PRF | PSA_ALG_TLS12_PRF(hash) | RFC 5246<br><br>Can be used with SHA-256 and SHA-384. |
| Key derivation | TLS 1.2 premaster secret from EC-JPAKE | PSA_ALG_TLS12_ECJPAKE_TO_PMS | The secret input is the shared key of an EC-JPAKE exchange, and the output is the 32-byte premaster secret of a TLS 1.2 or DTLS 1.2 handshake that uses EC-JPAKE. |
| Password-authenticated key exchange | EC-JPAKE | PSA_ALG_JPAKE(PSA_ALG_SHA_256) | Curve secp256r1 only.<br><br>The password is a key of type PSA_KEY_TYPE_PASSWORD, and the only role is PSA_PAKE_ROLE_NONE. The exchange has no key confirmation, so a different password shows only as a different shared key. |
| Password-authenticated key exchange | SPAKE2+, Matter variant | PSA_ALG_SPAKE2P_MATTER | Curve secp256r1 only, with key confirmation.<br><br>The prover holds a key of type PSA_KEY_TYPE_SPAKE2P_KEY_PAIR and the verifier one of type PSA_KEY_TYPE_SPAKE2P_PUBLIC_KEY. The other SPAKE2+ variants are not supported. |
| Key unwrapping | AES Key unwrap | PSA_ALG_KW | RFC 3394<br>SP 800-38F<br><br>Only unwrapping is supported. Wrapping is not supported. |
| Key generation | CTR-DRBG | N/A | Keys can be generated randomly by a cryptographically secure pseudorandom number generator (CSPRNG) with `psa_generate_key`. |
| Random generation | CSPRNG seeded with a TRNG | N/A | The CSPRNG is available through the `psa_generate_random` function.<br><br>It uses CTR-DRBG, and its TRNG comes from CRACEN. |

### Key support

The following table lists the supported key types.

| **Category** | **Description** | **PSA Key Type** | **Bits** | **Notes** |
| --- | --- | --- | --- | --- |
| Symmetric keys | AES | PSA_KEY_TYPE_AES | 128<br>256 |  |
| Symmetric keys | ChaCha20 | PSA_KEY_TYPE_CHACHA20 | 256 |  |
| Symmetric keys | HMAC key | PSA_KEY_TYPE_HMAC | 128<br>192<br>256<br>384 |  |
| Symmetric keys | Key-derivation secret | PSA_KEY_TYPE_DERIVE | 128<br>192<br>256<br>384 |  |
| Symmetric keys | Password | PSA_KEY_TYPE_PASSWORD | Up to the key size limit | Input to PBKDF2. |
| Symmetric keys | Master External Storage Key | PSA_KEY_TYPE_AES | 256 | A special key that is not a key type per se, but is always available to the application for the purposes of key derivation.<br><br>See [MEXT](#encrypted-filesystems-and-derivation-of-aes-keys) for details. |
| Asymmetric keys | SPAKE2+ key pair and verifier key<br>secp256r1 | PSA_KEY_TYPE_SPAKE2P_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)<br>PSA_KEY_TYPE_SPAKE2P_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1) | 256 | For PSA_ALG_SPAKE2P_MATTER. A key pair can be imported, or derived from a password with PBKDF2, and its public key is the verifier key. |
| Asymmetric keys | ECC key pair<br>secp256r1<br>secp384r1 | PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1) | 256<br>384 | Key pairs.<br><br>NIST P-256<br>NIST P-384 |
| Asymmetric keys | ECC public key | PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1) | 256<br>384 |  |
| Asymmetric keys | Ed25519 | PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS)<br><br>PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS) | 255<br>255 | PureEdDSA.<br><br>Both key pairs and public keys are supported. |
| Asymmetric keys | X25519 | PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY)<br><br>PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_MONTGOMERY) | 255<br>255 | X25519.<br><br>Both key pairs and public keys are supported.<br><br>Only key agreement is supported. Signing and signature verification are not supported. |

### Volatile keys

You can use all supported key types as volatile keys.

Volatile keys are useful for short-lived keys that do not need to persist across a device reset.
For details on creating and using these keys, see [Volatile keys](https://arm-software.github.io/psa-api/crypto/1.4/api/keys/lifetimes.html#volatile-keys) in the PSA Crypto API specification.

The keys are stored transiently in secure key slots of IronSide TE, at 192 bytes per slot.
Every device has 8 slots, and the base configuration can make room for more with `--key-slot-storage-size-bytes`, up to 221.
For details, see [Base configuration](configuring-ironside-te.md#base-configuration).
`IRONSIDE_PSA_KEY_SLOTS` in `ironside_client/layout.h` gives the count that the key slot storage of the bundle holds.
The number of supported keys could also be calculated at runtime based on the amount of RAM allocated, which is reported in the boot report.

Certain IronSide TE services use a volatile key themselves, so keep one slot free.

### Persistent keys

You can store all supported key types persistently.

Persistent keys are stored in non-volatile memory and therefore persist across power loss and device reset.
However, the authenticated hardware operation ERASEALL erases persistent keys.
This operation is also known as an authenticated backwards lifecycle transition to the Empty LCS.
For details on creating and using these keys, see [Persistent keys](https://arm-software.github.io/psa-api/crypto/1.4/api/keys/lifetimes.html#persistent-keys) in the PSA Crypto API specification.

IronSide TE supports PSA Crypto persistent keys with the standard key attributes, where the `psa_key_persistence_t` level is `PSA_KEY_PERSISTENCE_DEFAULT` and the `psa_key_location_t` is `PSA_KEY_LOCATION_LOCAL_STORAGE`.
Any other location returns `PSA_ERROR_NOT_PERMITTED`.

### Persistent key identifiers

IronSide TE has implementation-defined behavior for persistent PSA key identifiers.
Take this behavior into account when you port code that used another PSA Crypto implementation.

Persistent keys can use any key ID in the PSA user range `0x00000001` to `0x3fffffff`.
Any other key ID returns `PSA_ERROR_INVALID_ARGUMENT`.
Key IDs 228 to 239 are reserved for the keys of IronSide DFU.
IronSide TE does not refuse an application key in the reserved range, but such a key collides with a key provisioned for IronSide DFU.

A persistent key holds at most 145 bytes of key data, the largest key IronSide TE holds, so any key the application can create can also be persistent.
Persistent keys are stored in the UROTSTORAGE partition together with the records of IronSide TE, so the partition size limits how many keys fit.
If a new persistent key would leave less space than those records need, IronSide TE refuses to create it and returns `PSA_ERROR_INSUFFICIENT_STORAGE`.
For details, see [Sizing the UROTSTORAGE partition](configuring-ironside-te.md#sizing-the-urotstorage-partition).

### Persistence levels

For keys that are even more persistent than the default PSA Crypto persistence level allows, configure the key attributes according to the following table.

| **PSA persistence level** | **Action taken for psa_destroy_key** |
| --- | --- |
| PSA_KEY_PERSISTENCE_DEFAULT (0x01) | The key is erased, and the key ID can be reused. |
| IRONSIDE_CLIENT_KEY_PERSISTENCE_REVOKABLE (0x80) | The key is erased, and the key ID cannot be reused. Creating a key with the ID returns `PSA_ERROR_ALREADY_EXISTS`, and any other use returns `PSA_ERROR_NOT_PERMITTED`. |
| IRONSIDE_CLIENT_KEY_PERSISTENCE_LOCKED (0x81) | No action. `psa_destroy_key` returns `PSA_ERROR_NOT_PERMITTED`. |

### Encrypted filesystems and derivation of AES keys

You can store keys and other small secrets securely with PSA Crypto.
However, IronSide TE does not provide any services that implement the Internal Trusted Storage or Protected Storage of the [PSA Secure Storage API](https://arm-software.github.io/psa-api/storage/1.0/overview/intro.html#about-platform-security-architecture).

To store large amounts of data securely in internal non-volatile memory or in external off-chip memory, use the device-unique Master External Storage Key (MEXT), which is always available for this purpose.

The MEXT, with key ID `(psa_key_id_t)0x7fffc003`, is a 256-bit symmetric AES key that only the Application uses.

The Application can derive 128-bit or 256-bit AES keys from the MEXT with `PSA_ALG_SP800_108_COUNTER_CMAC`.
You can use these derived keys to store each file in the filesystem securely with AEAD.

To prevent accidental reuse of the MEXT, IronSide TE blocks any use of it except for key derivation.

### Multi-part operations and concurrent operations

Some cryptographic operations require data to be fed piecewise in several service calls instead of in a single PSA Crypto function call.
These operations are known as multi-part operations.
IronSide TE supports at most 4 multi-part operations in progress simultaneously, which the client declares as `IRONSIDE_CLIENT_PSA_CRYPTO_MAX_OPERATIONS`.
Concurrent calls are not supported.
For details, see [One call at a time](#one-call-at-a-time).

### Supported PSA Crypto functions

IronSide TE supports both the single-part and multi-part functions for the algorithms listed in [Algorithm support](#algorithm-support).
The following list groups all supported functions by category:

* Initialization
  * `psa_crypto_init`
* Hashes
  * `psa_hash_compute`
  * `psa_hash_setup`
  * `psa_hash_update`
  * `psa_hash_finish`
  * `psa_hash_compare`
  * `psa_hash_verify`
  * `psa_hash_clone`
  * `psa_hash_abort`
* Random generation
  * `psa_generate_random`
* Key management
  * `psa_import_key`
  * `psa_generate_key`
  * `psa_copy_key`
  * `psa_destroy_key`
  * `psa_export_key`
  * `psa_export_public_key`
  * `psa_get_key_attributes`
  * `psa_unwrap_key`
* Key agreement
  * `psa_raw_key_agreement`
* Password-authenticated key exchange
  * `psa_pake_setup`
  * `psa_pake_set_role`
  * `psa_pake_set_user`
  * `psa_pake_set_peer`
  * `psa_pake_set_context`
  * `psa_pake_output`
  * `psa_pake_input`
  * `psa_pake_get_shared_key`
  * `psa_pake_abort`
* Message authentication codes
  * `psa_mac_compute`
  * `psa_mac_verify`
  * `psa_mac_sign_setup`
  * `psa_mac_verify_setup`
  * `psa_mac_update`
  * `psa_mac_sign_finish`
  * `psa_mac_verify_finish`
  * `psa_mac_abort`
* Asymmetric signature
  * `psa_sign_message`
  * `psa_sign_hash`
  * `psa_verify_message`
  * `psa_verify_hash`
* Authenticated encryption with associated data (AEAD)
  * `psa_aead_encrypt`
  * `psa_aead_decrypt`
  * `psa_aead_encrypt_setup`
  * `psa_aead_decrypt_setup`
  * `psa_aead_generate_nonce`
  * `psa_aead_set_nonce`
  * `psa_aead_set_lengths`
  * `psa_aead_update_ad`
  * `psa_aead_update`
  * `psa_aead_finish`
  * `psa_aead_verify`
  * `psa_aead_abort`
* Unauthenticated ciphers
  * `psa_cipher_encrypt`
  * `psa_cipher_decrypt`
  * `psa_cipher_encrypt_setup`
  * `psa_cipher_decrypt_setup`
  * `psa_cipher_generate_iv`
  * `psa_cipher_set_iv`
  * `psa_cipher_update`
  * `psa_cipher_finish`
  * `psa_cipher_abort`
* Key derivation
  * `psa_key_derivation_setup`
  * `psa_key_derivation_get_capacity`
  * `psa_key_derivation_set_capacity`
  * `psa_key_derivation_input_key`
  * `psa_key_derivation_input_bytes`
  * `psa_key_derivation_input_integer`
  * `psa_key_derivation_key_agreement`
  * `psa_key_derivation_output_key`
  * `psa_key_derivation_output_bytes`
  * `psa_key_derivation_abort`

## PSA Attestation

IronSide TE implements PSA Attestation 2.0, which produces the PSA Attestation Token defined by RFC 9783.
For detailed documentation about this API, see the [PSA Attestation API 2.0 specification](https://arm-software.github.io/psa-api/attestation/2.0/).
For the token format, see [RFC 9783](https://datatracker.ietf.org/doc/html/rfc9783).

The attestation service proves that a device is authentic and pre-enrolled in an online service.
The token also contains claims about the installed software versions, so you can also use it as part of secure fleet management.

Before you use the attestation service, extract the public part of the Identity Attestation Key (IAK) from the device during device manufacturing and register it with the online service.

To decode the token, use a Concise Binary Object Representation (CBOR) parser and look up claims by their label.
`client/psa_attest_token.cddl` describes the complete token format in Concise Data Definition Language (CDDL, RFC 8610).
`client/include/ironside_client/psa/initial_attestation.h` declares the claim labels and value lengths as `PSA_INITIAL_ATTEST_*` constants.
The `psa_initial_attestation` sample decodes and prints a whole token.
It includes a small, self-contained CBOR parser (`attest_cbor.c`, roughly 200 lines, no allocation) that you can copy directly or replace with QCBOR, zcbor, TinyCBOR, or the decoder of your RTOS.

### Claims

The attestation token carries its information as claims, and each claim has a label.
RFC 9783 describes which claims the attestation token makes, but it leaves some choices to the implementation.
The following sections describe the choices that IronSide TE makes for each claim.

Look up each claim by its label, not at a hardcoded byte offset, for the following reasons:

* The position of a claim in the token depends on the claim set, and the claim set grows.
  Every release that added a claim moved every byte behind it.
  An offset therefore breaks on a firmware update without reporting an error.
  It keeps returning a well-formed value, taken from whichever claim moved into that position.
* The claim order within the map is not part of the format.
  CBOR requires no canonical key ordering, and RFC 9783 imposes none.

Token *sizes* are stable, and `PSA_INITIAL_ATTEST_MAX_TOKEN_SIZE` remains the buffer size to use.

### Nonce

Size: 32, 48, or 64 bytes.

The nonce is also known as a challenge object, and the caller of the service supplies it.
The caller can use a cryptographic nonce or a hash of some replay-protected data.
Using hashed data lets the caller convey application-specific information to any consumer of the token.

### Client ID

Size: 4 bytes.

The client ID usually distinguishes between different callers of the service.
In IronSide TE, the only caller is the non-secure domain, so the value is always -1.

### Instance ID

Size: 33 bytes.

The instance ID identifies the IAK.
In IronSide TE, each device has only one IAK, so you can also use the instance ID to identify the device.

The first byte has the value 1, and the following 32 bytes are the SHA-256 hash of the public part of the IAK.

### Implementation ID

Size: 32 bytes.

The implementation ID uniquely identifies the hardware version and the immutable bootloader version.
You need these immutable versions for fleet management and to assess the certification status.
The following table describes the bytes of the implementation ID.

| **Implementation ID bytes** | **Field** | **Description** |
| --- | --- | --- |
| 0 | Implementation ID encoding/scheme version | Currently always 1, but will be incremented if the encoding changes. |
| 1-4 | `FICR.INFO.PART` | See datasheet. |
| 5-8 | `FICR.INFO.VARIANT` | See datasheet. |
| 9-12 | `FICR.INFO.CONFIGID` | See datasheet. |
| 13-14 | `FICR.INFO.RAM` | See datasheet. |
| 15-16 | `FICR.INFO.RRAM` | See datasheet. |
| 17 | IROT major version | |
| 18 | IROT minor version | |
| 19 | IROT patch version | |
| 20 | IROT tweak version | |
| 21-31 | Reserved for future use | Always zero. |

All multi-byte fields are big-endian.

### Lifecycle

Size: 2 bytes.

The lifecycle claim holds the PSA lifecycle state of the device.
The following table lists the values that a token can contain:

| **Value** | **PSA lifecycle state** |
| --- | --- |
| `0x1000` | `PSA_LIFECYCLE_ASSEMBLY_AND_TEST` |
| `0x2000` | `PSA_LIFECYCLE_PSA_ROT_PROVISIONING` |
| `0x3000` | `PSA_LIFECYCLE_SECURED` |
| `0x4000` | `PSA_LIFECYCLE_NON_PSA_ROT_DEBUG` |

For the IronSide TE lifecycle states behind these values and how a device transitions between them, see [Limiting debugger access](debugger-access.md).

### Boot seed

Size: 32 bytes.

The boot seed is 32 bytes of CSPRNG data that is unique for each boot.
You can use it to differentiate tokens from different boot sessions.

### Device UUID

Size: 16 bytes.

The device UUID is an IronSide TE vendor extension that is not present in RFC 9783.
It is read from `FICR.INFO.UUID`.

### Profile definition

Size: 43 bytes.

The profile definition has the value `tag:psacertified.org,2023:psa#ironside-----`.
You can use it to interpret the rest of the claims in the token.

### Software components

The software components claim is a list of the three updatable software components in IronSide TE: UROT, the application, and the DFU image.
You can use this claim to identify the software versions installed on the system.

Each of the three software component entries has the attributes listed in the following table.

| **Attribute** | **Size** | **Description** |
| --- | --- | --- |
| Measurement type | 3-4 bytes | Either `UROT`, `APP`, or `DFU`. |
| Version | 15 bytes | The version encoded as `%03u.%03u.%03u+%03u`, for example `001.002.000+004`. |
| Measurement value | 32 bytes | SHA-256 hash, calculated by the attestation service, of the signed memory region where the software component is installed. |
| Signer ID | 32 bytes | SHA-256 of the key record used to verify the component during secure boot. The record is itself a digest: for App and DFU the SHA-256 of the ECDSA P-384 public key, and for UROT the SHA-256 of its ECDSA P-384 and ML-DSA-65 public keys. |

A vendor can create two firmware releases with the same version, although this is not recommended.
To identify a firmware release uniquely, the measurement value is therefore the more reliable claim.

## Platform

The platform services execute operations behind the secure boundary on peripherals that the non-secure code cannot use directly, but that are relevant to the SoC system state.
The platform services allow filtered access to these peripherals.

There are two classes of platform services:

* Low-level memory read and write functions, which are slim wrappers that check access rights.
  They are intended mostly for setup, where a range of registers is configured according to the needs of the application and not changed later, such as regulator and clock configurations.
* Higher-level, feature-specific services, which are mostly tailored to a particular operation.
  They are intended mostly for runtime, where low overhead and specific functionality are needed, such as selecting the resistive RAM (RRAM) power mode.

The following platform service functions are available:

* `ironside_platform_mem_read`
* `ironside_platform_mem_write`
* `ironside_platform_nvm_write`
* `ironside_platform_rram_power_mode_set`
* `ironside_platform_system_off`
* `ironside_platform_system_reset`

### Register read and write

`ironside_platform_mem_read()` and `ironside_platform_mem_write()` operate on individual 32-bit-aligned registers, batched up to 16 per call.
Every address is checked independently against a register-granular allow-list before it is read or written.

#### `ironside_platform_mem_read()`

`ironside_platform_mem_read()` can read the registers listed in the following table:

| Register | Notes |
| --- | --- |
| `OSCILLATORS.PLL.FREQ` | |
| `OSCILLATORS.XOSC32KI.BYPASS` | |
| `OSCILLATORS.XOSC32KI.INTCAP` | |
| `OSCILLATORS.XOSC32M.CONFIG.INTCAP` | |
| `REGULATORS.VREGMAIN.DCDCEN` | |
| `MEMCONF.POWER[0].RET` | |
| `MEMCONF.POWER[0].RET2` | nRF54L15 only |
| `MEMCONF.POWER[1].RET` | |
| `MEMCONF.POWER[1].RET2` | nRF54L15 only |
| `GPIO.PIN_CNF[0..31]` | on every GPIO port present on the SoC |
| `UICR.OTP[0..319]` | non-secure code cannot read the UICR without this function |

#### `ironside_platform_mem_write()`

`ironside_platform_mem_write()` can write the registers listed in the following table, each restricted to a field mask:

| Register | Writable field | Notes |
| --- | --- | --- |
| `OSCILLATORS.XOSC32KI.INTCAP` | `INTCAP.VAL` | |
| `OSCILLATORS.XOSC32KI.BYPASS` | `BYPASS.BYPASS` | |
| `OSCILLATORS.XOSC32M.CONFIG.INTCAP` | `INTCAP.VAL` | |
| `REGULATORS.VREGMAIN.DCDCEN` | `DCDCEN.VAL` | |
| `MEMCONF.POWER[0].RET` | `MEMn` of each 32 KB RAM section that does not overlap secure RAM | RAM retention in System OFF; clear unneeded sections. |
| `MEMCONF.POWER[0].RET2` | same bits as `RET` | nRF54L15 only. Retention of the second bank of each section. |
| `MEMCONF.POWER[1].RET` | all bits | `MEM0` retains the VPR context. System OFF does not change it. |
| `MEMCONF.POWER[1].RET2` | all bits | nRF54L15 only |
| `GPIO.PIN_CNF[0..31]` | `CTRLSEL` | on every GPIO port present on the SoC |
| `UICR.OTP[0..319]` | all bits | write-once, only while the word is erased |

For `MEMCONF.POWER[0].RET`, the writable bits select which 32 KB RAM sections stay powered in System OFF.
`RET` resets with every bit set, and System OFF does not change it.
Clear the bits of the sections the application does not need to retain.

Pairs that target `UICR.OTP` follow these rules in addition to the table:

* `ironside_platform_mem_write()` writes a `UICR.OTP` word only while it is erased, because each word can be written only once.
* A call that targets a word that is not erased, or targets the same word twice, returns `PSA_ERROR_ALREADY_EXISTS` and writes nothing.
* A value of `0xFFFFFFFF` is not written, so the word stays erased and can be written later.
* In a call that also writes registers, IronSide TE writes the `UICR.OTP` words first and then the registers.
* If a word does not read back as the requested value, the call returns `PSA_ERROR_HARDWARE_FAILURE` and writes no register.
  The other `UICR.OTP` words from the same call might already be written.

### Non-volatile memory write

Non-volatile memory is written through a designated service that checks the permissions for the writes and configures the underlying non-volatile memory technology accordingly.
You cannot write to non-volatile memory without the service, because the storage controller peripheral is secure-only.

### System reset

`ironside_platform_system_reset()` resets the device.
It is the only way for the Application or the DFU image to reset the device in software.
IronSide TE makes `SYSRESETREQ` secure-only, so `NVIC_SystemReset()` from non-secure code does not reset the device and hangs.
For details, see [Peripheral isolation](architecture.md#peripheral-isolation).
Replace every call to `NVIC_SystemReset()`, including any inside an RTOS reboot routine, with this call.

The call does not return on success.
Its argument selects the reset, and the next boot reports its reason in the [boot report](#boot-report):

| Reset type | Reset | Reset reason |
| --- | --- | --- |
| `IRONSIDE_PLATFORM_RESET_SYSTEM` | Soft reset through `SYSRESETREQ` | SREQ |
| `IRONSIDE_PLATFORM_RESET_CTRLAP_SOFT` | CTRL-AP soft reset | CTRLAPSOFT |
| `IRONSIDE_PLATFORM_RESET_CTRLAP_PIN` | CTRL-AP pin reset | CTRLAPPIN |

There is no CTRL-AP hard reset type, because that reset is the request of a debugger to enter ADAC mode.
IronSide TE applies the errata 63 workaround before every reset type, as listed in [Applied system configuration](architecture.md#applied-system-configuration).
While another secure service call is in flight, the call is refused with `IRONSIDE_ERROR_CALL_IN_FLIGHT`, as described in [One call at a time](#one-call-at-a-time).

## DFU boot selection

To designate the DFU image explicitly as the next boot target, call `ironside_dfu_request_set()` and reset the device through `ironside_platform_system_reset()`.
The DFU image then boots on the following boot.
You can use this to delegate DFU transport handling to the DFU image, so that the App image needs no further handling.

## Boot report

The IronSide TE boot report provides information about the past boot sequence.
For example, it includes the versions of the components, which image is booted and why, and the address and size of each non-secure partition as provisioned.
When the DFU image was booted, the `dfu_reason` field records which [DFU entry condition](architecture.md#dfu-entry-conditions) decided the boot.
The field names only that condition, even when several conditions are met.
For example, a configured trigger might also be met on a boot that reports an application image failure, because UROT checks the triggers only when the application image is valid.
To see whether a reset reason trigger was also met, read the `reset_reason` field.
After a fault, the `fault` field holds the fault record, and the `fault_image` field names the image that left it.
On a boot that the fault trigger decided, read `fault_image` to tell a fault of the application from a fault of the DFU image.
To read the boot report, call `ironside_boot_report_get()` from the client library.
`client/include/ironside_client/services/boot_report.h` declares the report and describes each field and its values.

### Reset reason

The `reset_reason` field holds the reasons for the reset that started the current boot.
At the start of every boot, IronSide TE copies the `RESET.RESETREAS` register into the boot report and then clears the register.
Read the reset reason from the boot report, not from the register.

`RESET.RESETREAS` is cumulative: the hardware sets a bit for each reset reason and keeps it until software clears it.
Without the clear, the register would also hold the reasons for earlier resets.
IronSide TE bases its own boot decisions on the reset reason, for example the [DFU entry conditions](architecture.md#dfu-entry-conditions) configured with `--dfu-trigger-resetreas`.
Clearing the register on every boot keeps these decisions from acting on a bit that an earlier reset set.

The field is a copy of `RESET.RESETREAS` and follows its description in the product specification of the SoC.

## Executing code from RAM

Non-secure RAM is readable and writable, and outside the Open and PRoT Provisioning states it is not executable.
For details, see [Memory isolation](architecture.md#memory-isolation).
You can allocate a single executable RAM region at the end of the non-secure RAM at runtime.
All code to be executed from RAM must be placed within this region.

The following figure shows the non-secure RAM before and after the region is defined.

![Two RAM maps. Before the call: Reserved, then APP-RAM with RW and, in brackets, RWX, then VPRCTX-RAM with RW and, in brackets, RWX, and Reserved at the end of RAM. After ironside_devmgmt_xram_region_define(...): the same areas, with APP-XRAM marked RX taking the end of APP-RAM, directly below VPRCTX-RAM.](images/xram-region.svg)

To initialize the executable RAM region, complete the following steps:

1. Copy all required code from non-volatile memory to RAM.
2. Call `ironside_devmgmt_xram_region_define(size)`, where `size` is the size in bytes of the executable region.
   The size must be non-zero, a multiple of 4 kB, and smaller than the non-secure RAM, so that ordinary RAM remains below the region.

This configures the final `size` bytes of the non-secure RAM with read and execute permissions and removes write permissions until the next system reset.
The region can be defined once per boot.
A second call returns `PSA_ERROR_NOT_PERMITTED`.
A call with an invalid size returns `PSA_ERROR_INVALID_ARGUMENT` and leaves the region undefined, so it can be retried.
Do not access the region while the call is in progress.

The nRF54L15 does not support the executable RAM region.
On that part, the whole non-secure RAM is readable, writable and executable, and `ironside_devmgmt_xram_region_define()` returns `PSA_ERROR_NOT_SUPPORTED`.

## Interrupts, concurrency and latency

IronSide TE has no scheduler, no timer, and no periodic task.
After it boots the Application or the DFU image, it runs only when that image calls a secure service, or when an interrupt or fault is taken to the secure side.

This section describes how IronSide TE interacts with the threads and interrupts of the application, how its secure services affect interrupt latency, and how it uses secure interrupts.

### One call at a time

A service call attempted while another call is already in flight is refused with `IRONSIDE_ERROR_CALL_IN_FLIGHT`.
This happens, for example, when an interrupt handler preempts a call in progress and makes another call, or when a thread makes a call while another thread that was switched out inside its call has not returned yet.
The exception is `ironside_platform_rram_power_mode_set()`, which selects the RRAM power mode.
This call is never refused, so an interrupt handler can set the mode while another call is in flight.

An application or OS integrator can implement the locking hooks `ironside_client_lock_acquire()` and `ironside_client_lock_release()`, so that a thread waits for a call in flight to return instead of being refused.
The client library declares them in `integration.h`, together with the calling contract and rules for the hooks to follow.
The default hooks do nothing, which is only correct when a single thread makes secure calls.

If a thread can be switched out in the middle of a call, then the locking hooks are mandatory.
In this case, the OS must save and restore the exception return value (`EXC_RETURN`) of that thread.
Otherwise, the switch faults.
In addition, the lock should not let other threads keep its holder off the CPU.
While the holder is off the CPU, its call stays in flight, and every other caller waits or is refused, as described in [Calls that the application keeps from returning](#calls-that-the-application-keeps-from-returning) below.

Consider the following options for the lock implementation:

* A semaphore does not change the holder's priority.
  Any thread with a higher priority than the holder can keep it from running for as long as that thread runs.
* A mutex with priority inheritance raises the holder to the priority of a thread that waits for the lock, but only while one waits.
  When no thread waits, the holder runs at its own priority, and a higher-priority thread can still keep it from running for as long as that thread runs.
* A mutex combined with a priority ceiling raises the holder, for the whole call, to a fixed priority above every thread that can run for a long time.
  To implement the ceiling, raise the priority of the calling thread in `ironside_client_lock_acquire()` after taking the mutex, and restore it in `ironside_client_lock_release()` before releasing the mutex.

Only the priority ceiling rules out starving the holder.
The cost is that no thread below the ceiling runs while a call is in flight, which for the longest calls is several seconds.
It is recommended to use a mutex without a ceiling and to keep every higher-priority thread short.

A thread that holds the lock is never refused, so a thread that receives `IRONSIDE_ERROR_CALL_IN_FLIGHT` made its call without holding the lock.
This is only possible when the application does not call the API as documented.
If a thread does this while another thread's call is in flight, and it gets switched out inside the refusal path, then resuming the two threads out of order corrupts non-secure state.

The lock can only make one thread wait for another.
It cannot make an interrupt handler wait for the thread it preempted, because that thread cannot release the lock until the handler returns.
Therefore, bypass the lock in interrupt context and before the scheduler starts, where the caller cannot block.
Bypass it in both hooks on the same condition, because releasing a mutex that the caller did not take fails even where the acquire was bypassed.
A caller that bypasses the lock must be prepared to handle `IRONSIDE_ERROR_CALL_IN_FLIGHT`.
It can defer the work to a thread or retry another time, but it must never retry before the handler returns, because the preempted call cannot make progress until then.

### Calls that the application keeps from returning

Every service call returns unless the application keeps it from returning.
The exceptions are `ironside_platform_system_off()` and `ironside_platform_system_reset()`, which do not return on success.
For example, the application keeps a call from returning in the following cases:

* A non-secure interrupt handler that preempted the call never returns.
* The non-secure scheduler switches the calling thread out and never schedules it again.
* Non-secure code that preempts the call sets an interrupt mask and leaves it set when the call resumes.

IronSide TE does not limit how long a call can be kept from returning, and it does not reset the device to end the call.

The call then stays in flight, and every later call except the RRAMC low-power mode calls, including calls from interrupts, is refused with `IRONSIDE_ERROR_CALL_IN_FLIGHT`.
Only a reset ends this state: the watchdog of the application (WDT31), a reset requested by a debugger, or a power cycle.
The [system reset](#system-reset) service is itself a call and is refused while the call is in flight.

If the application must recover from such a call, start WDT31 from the [image configuration](configuring-ironside-te.md#enabling-automatic-watchdog-timer-start) and feed it from code that still runs.

### Where IronSide TE masks interrupts

IronSide TE masks interrupts during the following windows:

* For a few instructions when it accepts a call and when the call returns.
* For each 16-byte write line during `ironside_platform_nvm_write()`.
  Interrupts are unmasked between lines, so the added latency is bounded by the write of one line.
* While `ironside_platform_mem_write()` writes the `UICR.OTP` words of a call, at most 16 words.
* From the start of `ironside_platform_system_off()` and `ironside_platform_system_reset()`, which do not return.

Most of the time, interrupts are enabled during service calls, so non-secure interrupts can preempt them.
The latency added by the masking of IronSide TE is therefore bounded by the windows listed above.

### Secure interrupts and terminal conditions

Every interrupt line targets the non-secure side, except the ones listed in the following table.
IronSide TE sets `AIRCR.PRIS`, which maps every non-secure priority onto the less urgent half of the priority range, secure levels 4 to 7, so no non-secure interrupt can preempt the most urgent secure events.

| Source | Class | Priority | Behavior |
| --- | --- | --- | --- |
| CRACEN | Service-scoped | One step more urgent than the calling context, for the duration of the call. For details, see [CRACEN priority during a call](#cracen-priority-during-a-call). | Completes cryptographic operations during an accepted service call. IronSide TE boosts its priority when it accepts the call and restores it when the call returns, so it preempts the context that made the call, except in the cases listed in the linked section. |
| MPC00 | Terminal | Most urgent | A bus access that the MPC refused, for example DMA into secure memory, resets the device and leaves a fault record. For details, see [Handling faults](architecture.md#handling-faults). |
| Application CPU faults taken in the secure world | Terminal | Most urgent | Device reset. For details, see [Handling faults](architecture.md#handling-faults). |
| SPU00, SPU10, SPU20, SPU30 | Reserved | - | Secure and unused. The application cannot use them. |

A terminal condition always ends in a device reset, and IronSide TE does not return to the interrupted code.
Glitch and tamper detection is also terminal, but it does not raise an interrupt, and it leaves no fault record after the reset.

#### CRACEN priority during a call

The device implements 3 priority bits, so priority levels run from 0 to 7 and 0 is the most urgent.
A level in this section is the argument of the CMSIS `NVIC_SetPriority()` function, which the NVIC priority register holds in bits [7:5].
An OS interrupt API can add an offset to the value you give it.
This section always uses levels as defined above, not the value passed to such an API.

IronSide TE sets CRACEN to the secure level one step more urgent than the level at which the caller arbitrates.
The caller's level is the more urgent of the following levels:

* The level of the current exception, or 8 in Thread mode.
* The level of the caller's interrupt mask: 0 if `PRIMASK` or `FAULTMASK` is set, otherwise the level held in bits [7:5] of `BASEPRI` if it is nonzero, otherwise 8.

A Thread mode caller whose `BASEPRI` holds level `b` is therefore treated like a handler at level `b`.
Both levels count after the non-secure `AIRCR.PRIGROUP` rounds them to their group priority.
`PRIGROUP` is 0 after reset, and values up to 5 leave every level unchanged.
With `PRIGROUP` 6, levels 0 to 3 and levels 4 to 7 each form one group, and with 7 all levels form one group.

The following figure shows the secure level of CRACEN for each caller level, and how CRACEN's handler affects each non-secure level.
CRACEN's handler holds off the non-secure levels that arbitrate as the same secure level as CRACEN, which are the pair one step more urgent than the caller's pair, if there is one.

![Five ladders of secure levels 0 to 7 and Thread mode, one for each caller level. Non-secure levels 0 and 1 arbitrate as secure level 4, levels 2 and 3 as 5, levels 4 and 5 as 6, and levels 6 and 7 as 7. For a caller in Thread mode, CRACEN is at secure level 7, levels 0 to 5 preempt CRACEN's handler, and levels 6 and 7 are held off. For a caller at level 6 or 7, CRACEN is at 6, levels 0 to 3 preempt CRACEN's handler, levels 4 and 5 are held off, and levels 6 and 7 are preempted. For a caller at level 4 or 5, CRACEN is at 5, levels 0 and 1 preempt CRACEN's handler, levels 2 and 3 are held off, and levels 4 to 7 are preempted. For a caller at level 2 or 3, CRACEN is at 4, levels 0 and 1 are held off, and levels 2 to 7 are preempted. For a caller at level 0 or 1, or with PRIMASK or FAULTMASK set, CRACEN is at 3 and levels 0 to 7 are preempted. Thread mode is preempted in every case.](images/cracen-priority-ladders.svg)

As the figure shows, each pair of adjacent non-secure levels arbitrates as one secure level in the range 4 to 7, and secure levels 0 to 3 are more urgent than every non-secure level.
Interrupts that arbitrate as the same level do not preempt each other, whether they are secure or non-secure.
A non-secure handler at level 1 is not preempted by a non-secure interrupt at level 0, or by a secure interrupt at level 4.
The non-secure side therefore has 4 preemption levels, not 8.
The Armv8-M Architecture Reference Manual and the Cortex-M33 Devices Generic User Guide describe a finer mapping, in which each non-secure level arbitrates on its own and the odd levels fall between two secure levels.
The device does not follow that mapping.

The boosted level applies only while CRACEN's handler runs.
The rest of the call runs at the caller's execution priority, so a non-secure interrupt that arbitrates more urgently than the caller still preempts the call.
An interrupt that CRACEN's handler holds off runs as soon as that handler returns, and an interrupt at the caller's level or a less urgent one waits for the caller as usual.
The following figure shows a call from a handler at level 6:

![Timeline of a call from a non-secure handler at level 6, during which CRACEN is at secure level 6. Interrupts at levels 2, 4 and 7 become pending while CRACEN's handler runs. The level 2 interrupt preempts CRACEN's handler. The level 4 interrupt is held off until CRACEN's handler returns, and then preempts the call. The level 7 interrupt waits until the caller's handler returns. When the call returns, IronSide TE restores the priority of CRACEN.](images/cracen-priority-timeline.svg)

IronSide TE reads the caller's level once, when it accepts the call, from the current exception and the interrupt mask registers.
If the caller arbitrates more urgently than that level while the call waits for CRACEN, the interrupt of CRACEN is never taken and the call does not return.
This cannot happen while the application follows these rules:

* Change the priority of an active interrupt only from its own handler, and make it less urgent only if that handler has not preempted another handler.
* Set the non-secure `AIRCR.PRIGROUP` before interrupts are enabled, and do not change it afterwards.
* Return from every exception handler to the code that it preempted, and do not set or clear the active bit of an exception in `SHCSR`.

For details, see [calls that the application keeps from returning](#calls-that-the-application-keeps-from-returning).

## Boot failure codes

When IronSide TE halts during boot, it writes one 32-bit BOOTFAIL word to the CTRL-AP mailbox.
The word says which stage halted and why.
The device stays halted, so the word stays readable until the device is reset.
Nothing is written on a boot that does not halt.
For how IronSide TE handles faults that do not halt the boot, see [Handling faults](architecture.md#handling-faults).

To read the word, run the following command:

```sh
scripts/read_boot_error.py --serial-number <serial-number>
```

The `--json` option prints each field by value and by name.
The script reads only the CTRL-AP, so it neither resets nor halts the target, and it works in every lifecycle state.
The following table describes the fields of the word.

| Bits | Field | Meaning |
| --- | --- | --- |
| 31..28 | MAGIC | Always `0xB`. ADAC protocol words start with `0xA`. |
| 27 | STAGE | 0 = IROT, 1 = UROT. The same ERROR value means different things in the two stages. |
| 26..8 | reserved | 0 |
| 7..0 | ERROR | One error code, from the following tables. |

A mailbox value of 0 means that no stage has halted since the mailbox was last cleared.
A CTRL-AP soft reset does not clear the mailbox, so a word read after one can come from an earlier boot.

The following table lists the IROT errors (STAGE 0):

| Code | Name | Meaning |
| --- | --- | --- |
| 0x80 | `CRYPTO_INIT` | Crypto initialization failed. |
| 0x81 | `BASE_CONFIG_VERSION` | The IROT base configuration (UICR) version is unsupported. |
| 0x82 | `LAYOUT_INVALID` | The UICR partition layout is invalid or not provisioned. |
| 0x83 | `UROT_UPDATE` | A UROT update failed. |
| 0x84 | `RRAMC_LOCK` | The RRAMC region lock did not read back as written. |
| 0x85 | `UROT_SIGNATURE` | The UROT signature is invalid. |
| 0x86 | `UROT_SEC_CNT_MISSING` | The UROT security counter cannot be read. |
| 0x87 | `UROT_ROLLBACK` | The UROT failed the rollback counter check. |
| 0x89 | `UROT_BOOT_RETURNED` | The UROT's reset handler returned. |

The following table lists the UROT errors (STAGE 1):

| Code | Name | Meaning |
| --- | --- | --- |
| 0x80 | `IROT_BASE_CONFIG_FORMAT` | The IROT base configuration format is unsupported. |
| 0x81 | `UROT_BASE_CONFIG_FORMAT` | The UROT base configuration format is unsupported. |
| 0x82 | `LAYOUT_INVALID` | The partition layout is invalid. |
| 0x83 | `CRYPTO_INIT` | Crypto initialization failed. |
| 0x84 | `HANDOVER` | The handover from the IROT is invalid. |
| 0x85 | `DECOMMISSIONED` | The device is decommissioned. This is its normal end state, not a failure. |
| 0x88 | `NO_BOOT_IMAGE` | Neither the Application nor the IronSide DFU image is valid. |
| 0x89 | `ADAC_EXIT` | The ADAC session ended unexpectedly. |
| 0x8A | `CONFIG_ASSERT` | The exception configuration check before the non-secure boot failed. |
| 0x8B | `IKG_SEED_INVALID` | The base configuration selects an IKG SEED entry that is not Locked. |
| 0x8D | `IKG_SEED_SELECTION_INVALID` | The base configuration selects no IKG SEED entry. |
| 0x8E | `BOARD_UNSET` | The image configuration cannot be read, or lacks a board value. See [Board crystals and regulator](configuring-ironside-te.md#board-crystals-and-regulator). |
| 0x8F | `UROT_STORAGE_MOUNT` | The UROT storage does not mount. |
| 0x90 | `UROT_STORAGE_NO_FORMAT` | The UROT storage is not initialized: provisioning did not finish. |
| 0x91 | `UROT_STORAGE_UNAUTHENTIC` | The UROT storage does not authenticate, for example after a SEED switch. |
| 0x92 | `UROT_STORAGE_VERSION` | The UROT storage layout version is unsupported. |
| 0x93 | `UROT_STORAGE_READ` | The UROT storage cannot be read. |
| 0x94 | `UROT_STORAGE_BLOCK` | The UROT storage access protection cannot be configured. |
| 0x95 | `NS_IMAGE_RETURNED` | The reset handler of the Application or the IronSide DFU image returned. |
| 0x96 | `AUTHOP_REVOKE` | An authenticated operation (AUTHOP) key generation that the image configuration revokes is still valid after its revocation write. UROT halts without entering ADAC mode. |
| 0x97 | `IKG_SEED_BLOCK` | The KMU slots of the IKG SEED entry that is not selected cannot be blocked. |

The following table lists the CPU faults, which only IROT reports:

| Code | Name |
| --- | --- |
| 0xF0 | `HARDFAULT` |
| 0xF1 | `MEMMANAGE` |
| 0xF2 | `BUSFAULT` |
| 0xF3 | `USAGEFAULT` |
| 0xF4 | `SECUREFAULT` |
| 0xF5 | `NMI` |
| 0xF6 | `SW_FATAL` (a fatal error raised by software) |
| 0xFE | `WATCHDOG` (reserved, because a watchdog reset leaves no time to publish it) |
