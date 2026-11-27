# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""ECDSA as IronSide TE verifies it: a raw r || s signature and the public key as
the uncompressed point 0x04 || X || Y, on a NIST curve with its matching SHA-2 hash.

A device holds a SHA-256 of the public key per key generation, so a key presented
with an image or a certificate is trusted only when its digest is one the device
holds.

Signatures are deterministic (RFC 6979), so signing the same bytes with the same key
gives the same signature.
"""

import hashlib

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import (
    decode_dss_signature,
    encode_dss_signature,
)

KEY_DIGEST_LEN = 32


class Curve:
    """One curve and its hash: P256 (SHA-256) or P384 (SHA-384)."""

    def __init__(self, name: str, curve: type[ec.EllipticCurve], hash_algorithm, coordinate_len):
        self.name = name
        self._curve = curve
        self._algorithm = ec.ECDSA(hash_algorithm(), deterministic_signing=True)
        self._coordinate_len = coordinate_len
        self.sig_len = 2 * coordinate_len
        self.pubkey_len = 1 + 2 * coordinate_len

    def generate_private_pem(self) -> bytes:
        """A fresh private key as unencrypted PKCS#8 PEM."""
        return ec.generate_private_key(self._curve()).private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.PKCS8,
            encryption_algorithm=serialization.NoEncryption(),
        )

    def load_private_key(self, pem_bytes: bytes, label: str = "key") -> ec.EllipticCurvePrivateKey:
        """Load a private key from PEM. Raises ValueError for a key on any other curve."""
        key = serialization.load_pem_private_key(pem_bytes, password=None)
        if not self.is_key(key):
            raise ValueError(f"{label}: not an ECDSA {self.name} private key")
        return key

    def load_public_key(self, pem_bytes: bytes, label: str = "key") -> ec.EllipticCurvePublicKey:
        """Load a public key from PEM. Raises ValueError for a key on any other curve."""
        key = serialization.load_pem_public_key(pem_bytes)
        if not self.is_key(key):
            raise ValueError(f"{label}: not an ECDSA {self.name} public key")
        return key

    def is_key(self, key) -> bool:
        """Whether @key is a private or public key on this curve."""
        return (
            isinstance(key, ec.EllipticCurvePrivateKey | ec.EllipticCurvePublicKey)
            and key.curve.name == self._curve.name
        )

    def key_digest(self, public: bytes) -> bytes:
        """The value a device stores for one key generation: SHA-256 of the point."""
        if len(public) != self.pubkey_len:
            raise ValueError(
                f"ECDSA {self.name} public key is {len(public)} bytes, expected {self.pubkey_len}"
            )
        return hashlib.sha256(public).digest()

    def sign(self, key: ec.EllipticCurvePrivateKey, data: bytes) -> bytes:
        """Sign @data, returning the r || s signature."""
        r, s = decode_dss_signature(key.sign(data, self._algorithm))
        return r.to_bytes(self._coordinate_len, "big") + s.to_bytes(self._coordinate_len, "big")

    def verify(self, public: bytes, data: bytes, signature: bytes) -> bool:
        """Whether @signature (r || s) is valid for @data under the point @public."""
        if len(public) != self.pubkey_len or len(signature) != self.sig_len:
            return False
        r = int.from_bytes(signature[: self._coordinate_len], "big")
        s = int.from_bytes(signature[self._coordinate_len :], "big")
        try:
            key = ec.EllipticCurvePublicKey.from_encoded_point(self._curve(), public)
            key.verify(encode_dss_signature(r, s), data, self._algorithm)
        except (InvalidSignature, ValueError):
            return False
        return True


def public_bytes(key) -> bytes:
    """The uncompressed point 0x04 || X || Y of a private or public key."""
    public = key.public_key() if isinstance(key, ec.EllipticCurvePrivateKey) else key
    return public.public_bytes(
        serialization.Encoding.X962, serialization.PublicFormat.UncompressedPoint
    )


P256 = Curve("P-256", ec.SECP256R1, hashes.SHA256, 32)
P384 = Curve("P-384", ec.SECP384R1, hashes.SHA384, 48)
