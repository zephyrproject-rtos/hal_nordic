# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Hybrid ECDSA P-384 + ML-DSA-65 signing of the UROT image.

IROT accepts a UROT image only when an ECDSA P-384 signature and an ML-DSA-65
signature both verify over the image's signed region (header, body and protected
TLVs). The image carries both public keys. The device holds, per key generation,
one SHA-256 over the two public keys concatenated (ECDSA first), in its 32-byte
key record. The two keys therefore revoke and rotate as one pair.

The unprotected TLV area of the UROT image:

  ECDSA_SIG(0x17E2)       96 bytes, r || s
  MLDSA_SIG(0x17E3)       3309 bytes
  ECDSA_PUBKEY(0x17E4)    97 bytes, 0x04 || X || Y
  MLDSA_PUBKEY(0x17E5)    1952 bytes

ECDSA signs with SHA-384, and ML-DSA in pure mode with an empty context.

Needs a `cryptography` release that provides ML-DSA (see the IronSide Integration Kit
requirements). The ML-DSA module is imported on first use, so a tool that never
touches a UROT key, such as App signing, runs without it.
"""

import hashlib
from pathlib import Path

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import serialization

try:
    from lib import ecdsa  # noqa: I001  (a try/except is not a sortable block)
    from lib import mcuboot_image as mci
except ImportError:  # run directly: this file's own dir is sys.path[0]
    import ecdsa
    import mcuboot_image as mci

ECDSA_CURVE = ecdsa.P384

TLV_ECDSA_SIG = mci.TLV_ECDSA_SIG
TLV_MLDSA_SIG = 0x17E3
TLV_ECDSA_PUBKEY = mci.TLV_ECDSA_PUBKEY
TLV_MLDSA_PUBKEY = 0x17E5

MLDSA65_SIG_LEN = 3309
MLDSA65_PUBKEY_LEN = 1952
PAIR_DIGEST_LEN = 32

# Unprotected TLV area: info header, the hash TLV and four signature TLVs.
SIGNATURE_AREA_SIZE = (
    mci.TLV_INFO_SIZE
    + mci.HASH_TLV_SIZE
    + 4 * mci.TLV_HDR_SIZE
    + ECDSA_CURVE.sig_len
    + MLDSA65_SIG_LEN
    + ECDSA_CURVE.pubkey_len
    + MLDSA65_PUBKEY_LEN
)

# Only the UROT image is verified by IROT, so only it is ever signed this way.
ROLE = "urot"
GENERATIONS = (0, 1, 2)


class HybridUnavailable(mci.SignError):
    """The installed cryptography package cannot do ML-DSA."""


def _mldsa():
    try:
        from cryptography.hazmat.primitives.asymmetric import mldsa
    except ImportError as exc:
        raise HybridUnavailable(
            "hybrid signing needs a cryptography release with ML-DSA support "
            "(cryptography.hazmat.primitives.asymmetric.mldsa); install cryptography 48 "
            "or later"
        ) from exc
    return mldsa


def _pem(key) -> bytes:
    return key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption(),
    )


def generate_ecdsa_pem() -> bytes:
    """A fresh ECDSA private key as unencrypted PKCS#8 PEM."""
    return ECDSA_CURVE.generate_private_pem()


def generate_mldsa_pem() -> bytes:
    """A fresh ML-DSA-65 private key as unencrypted PKCS#8 PEM."""
    return _pem(_mldsa().MLDSA65PrivateKey.generate())


def load_ecdsa_key(pem_bytes: bytes, label: str = "key"):
    try:
        return ECDSA_CURVE.load_private_key(pem_bytes, label)
    except ValueError as exc:
        raise mci.SignError(str(exc)) from exc


def load_mldsa_key(pem_bytes: bytes, label: str = "key"):
    key = serialization.load_pem_private_key(pem_bytes, password=None)
    if not isinstance(key, _mldsa().MLDSA65PrivateKey):
        raise mci.SignError(f"{label}: not an ML-DSA-65 private key")
    return key


def ecdsa_public_bytes(key) -> bytes:
    """The uncompressed point 0x04 || X || Y, as the image carries it."""
    return ecdsa.public_bytes(key)


def mldsa_public_bytes(key) -> bytes:
    return key.public_key().public_bytes(serialization.Encoding.Raw, serialization.PublicFormat.Raw)


def pair_digest(ecdsa_public: bytes, mldsa_public: bytes) -> bytes:
    """The value a device stores for one generation: SHA-256(ECDSA key || ML-DSA key)."""
    if len(ecdsa_public) != ECDSA_CURVE.pubkey_len:
        raise mci.SignError(
            f"ECDSA public key is {len(ecdsa_public)} bytes, expected {ECDSA_CURVE.pubkey_len}"
        )
    if len(mldsa_public) != MLDSA65_PUBKEY_LEN:
        raise mci.SignError(
            f"ML-DSA public key is {len(mldsa_public)} bytes, expected {MLDSA65_PUBKEY_LEN}"
        )
    return hashlib.sha256(ecdsa_public + mldsa_public).digest()


def key_pair_digest(ecdsa_key, mldsa_key) -> bytes:
    return pair_digest(ecdsa_public_bytes(ecdsa_key), mldsa_public_bytes(mldsa_key))


def sign_unsigned(data, ecdsa_key, mldsa_key, *, slot_size=0, pad=False):
    """Sign an unsigned image and append the hash TLV and the four hybrid TLVs."""
    mci.read_unsigned(data)  # reject a non-image, or an already-signed one
    region = bytes(data)

    ecdsa_sig = ECDSA_CURVE.sign(ecdsa_key, region)
    mldsa_sig = mldsa_key.sign(region)
    if len(mldsa_sig) != MLDSA65_SIG_LEN:
        raise mci.SignError(
            f"ML-DSA signature is {len(mldsa_sig)} bytes, expected {MLDSA65_SIG_LEN}"
        )

    tlvs = b"".join(
        (
            mci.hash_tlv(region),
            mci._tlv(TLV_ECDSA_SIG, ecdsa_sig),
            mci._tlv(TLV_MLDSA_SIG, mldsa_sig),
            mci._tlv(TLV_ECDSA_PUBKEY, ecdsa_public_bytes(ecdsa_key)),
            mci._tlv(TLV_MLDSA_PUBKEY, mldsa_public_bytes(mldsa_key)),
        )
    )
    out = region + mci._tlv_area(mci.TLV_INFO_MAGIC, tlvs)
    assert len(out) == len(region) + SIGNATURE_AREA_SIZE

    if slot_size and len(out) > slot_size:
        raise mci.SignError(f"signed image (0x{len(out):x}) exceeds slot 0x{slot_size:x}")
    if pad and slot_size:
        out += bytes([mci.ERASED_VAL]) * (slot_size - len(out))
    return out


def sign_image(image, ecdsa_key, mldsa_key, *, pad=False, **unsigned_args):
    """Build metadata if needed, apply overrides, and sign an image with both keys.

    Takes mcuboot_image.unsigned_image()'s arguments. Returns the signed bytes and
    the HEX slot base, or None for binary input.
    """
    unsigned, base = mci.unsigned_image(
        image, signature_area_size=SIGNATURE_AREA_SIZE, **unsigned_args
    )
    signed = sign_unsigned(
        unsigned, ecdsa_key, mldsa_key, slot_size=unsigned_args["slot_size"], pad=pad
    )
    return signed, base


def is_hybrid_signed(data) -> bool:
    """Whether the image bytes @data carry the hybrid key TLVs a UROT image is signed with."""
    try:
        return TLV_MLDSA_PUBKEY in mci.split_unprotected_tlvs(data)[1]
    except mci.SignError:
        return False


def verify_signed(image):
    """Check both signatures of a hybrid image against the keys it carries.

    Returns the pair digest of those keys when both signatures verify, or None.
    It establishes that the image matches the keys it carries, not trust in them:
    a device trusts the keys only if their digest is the one it holds.
    """
    data, _ = mci.read_image(image)
    region, tlvs = mci.split_unprotected_tlvs(data)
    wanted = {
        TLV_ECDSA_SIG: ECDSA_CURVE.sig_len,
        TLV_MLDSA_SIG: MLDSA65_SIG_LEN,
        TLV_ECDSA_PUBKEY: ECDSA_CURVE.pubkey_len,
        TLV_MLDSA_PUBKEY: MLDSA65_PUBKEY_LEN,
    }
    if any(tlvs.get(t) is None for t in wanted):
        return None
    for tlv_type, length in wanted.items():
        if len(tlvs[tlv_type]) != length:
            raise mci.SignError(
                f"hybrid TLV 0x{tlv_type:02X} is {len(tlvs[tlv_type])} bytes, expected {length}"
            )

    ecdsa_public = tlvs[TLV_ECDSA_PUBKEY]
    mldsa_public = tlvs[TLV_MLDSA_PUBKEY]
    if not ECDSA_CURVE.verify(ecdsa_public, region, tlvs[TLV_ECDSA_SIG]):
        return None
    try:
        _mldsa().MLDSA65PublicKey.from_public_bytes(mldsa_public).verify(
            tlvs[TLV_MLDSA_SIG], region
        )
    except InvalidSignature:
        return None
    return pair_digest(ecdsa_public, mldsa_public)


def load_keys(ecdsa_path: Path, mldsa_path: Path):
    """Load a key pair from two PEM files."""
    return (
        load_ecdsa_key(Path(ecdsa_path).read_bytes(), label=str(ecdsa_path)),
        load_mldsa_key(Path(mldsa_path).read_bytes(), label=str(mldsa_path)),
    )
