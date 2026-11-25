#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Encrypt a signed image in the MCUboot encrypted-image format.

The 32-byte image header and both TLV areas stay plaintext. The body, the
ih_img_size bytes after the header, is encrypted with AES-256-CTR under
a random 32-byte key, with the counter block starting at zero at the first body
byte, as MCUboot decrypts it. The key travels in the unprotected TLV
IMAGE_TLV_ENC_KW (0x31), wrapped under the OTA Key Encryption Key with RFC 3394
AES Key Wrap.

The header must already carry IMAGE_F_ENCRYPTED_AES256, which sign.py sets,
because the flag is inside the signed region. The signature covers the
plaintext, so it verifies only after decryption.
"""

import os
import struct
from pathlib import Path

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.keywrap import aes_key_unwrap, aes_key_wrap

try:
    from lib import mcuboot_image as mci
except ImportError:  # run directly: this file's own dir is sys.path[0]
    import mcuboot_image as mci

KEK_LEN = 32
BODY_KEY_LEN = 32
ENC_KW_LEN = BODY_KEY_LEN + 8  # RFC 3394 adds eight bytes.
AES_BLOCK_LEN = 16

# nrfutil mcu-manager serial image-upload rejects the .ota extension; use .ota.bin.
OTA_ARTIFACT_SUFFIX = ".ota.bin"


class EncryptError(ValueError):
    pass


def is_ota_artifact(path) -> bool:
    """Return whether @path uses the on-disk OTA artifact suffix."""
    return "".join(Path(path).suffixes) == OTA_ARTIFACT_SUFFIX


def encrypt_image(kek: bytes, signed: bytes) -> bytes:
    """Return signed image @signed with its body encrypted and an ENC_KW TLV added.

    A new body key is drawn for every call, so the same input never encrypts to
    the same bytes twice.
    """
    if len(kek) != KEK_LEN:
        raise EncryptError(f"KEK must be {KEK_LEN} bytes (AES-256), got {len(kek)}")
    start, end = _body_range(signed)
    if enc_kw_of(signed) is not None:
        raise EncryptError("image already carries an ENC_KW TLV, so it is already encrypted")
    body_key = os.urandom(BODY_KEY_LEN)
    out = signed[:start] + ctr_crypt(body_key, signed[start:end]) + signed[end:]
    return mci.append_unprotected_tlv(out, mci.TLV_ENC_KW, aes_key_wrap(kek, body_key))


def decrypt_image(kek: bytes, image: bytes) -> bytes:
    """Return encrypted image @image with its body decrypted; the ENC_KW TLV stays."""
    wrapped = enc_kw_of(image)
    if wrapped is None:
        raise EncryptError("image carries no ENC_KW TLV")
    start, end = _body_range(image)
    body_key = aes_key_unwrap(kek, wrapped)
    return image[:start] + ctr_crypt(body_key, image[start:end]) + image[end:]


def enc_kw_of(image: bytes) -> bytes | None:
    """Return the wrapped body key from the ENC_KW TLV, or None when there is none."""
    try:
        _region, tlvs = mci.split_unprotected_tlvs(image)
    except mci.SignError as exc:
        raise EncryptError(str(exc)) from exc
    wrapped = tlvs.get(mci.TLV_ENC_KW)
    if wrapped is not None and len(wrapped) != ENC_KW_LEN:
        raise EncryptError(f"ENC_KW TLV is {len(wrapped)} bytes, expected {ENC_KW_LEN}")
    return wrapped


def ctr_crypt(body_key: bytes, data: bytes) -> bytes:
    """Encrypt or decrypt an image body with AES-256-CTR from a zero counter block."""
    cipher = Cipher(algorithms.AES(body_key), modes.CTR(bytes(AES_BLOCK_LEN))).encryptor()
    return cipher.update(data) + cipher.finalize()


def _body_range(image: bytes) -> tuple[int, int]:
    """Return the body's start and end offset in @image, which must be flagged AES-256."""
    if len(image) < mci.IMAGE_HEADER_SIZE:
        raise EncryptError("file is smaller than an image header")
    magic, _load, header_size, _prot_size, img_size, flags = struct.unpack("<IIHHII", image[:20])
    if magic != mci.IMAGE_MAGIC:
        raise EncryptError(f"not an MCUboot image (magic 0x{magic:08X})")
    if not flags & mci.IMAGE_F_ENCRYPTED_AES256:
        raise EncryptError(
            "image header does not set IMAGE_F_ENCRYPTED_AES256. Sign it again with sign.py."
        )
    if header_size < mci.IMAGE_HEADER_SIZE or header_size + img_size > len(image):
        raise EncryptError("image header describes more than the file holds")
    return header_size, header_size + img_size


def _self_test() -> None:
    """Check the round trip, the refusals and MCUboot's counter layout."""
    import tempfile

    key = mci.CURVE.load_private_key(mci.CURVE.generate_private_pem())
    kek = os.urandom(KEK_LEN)
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / "fixture.bin"
        src.write_bytes(bytes(0x800) + os.urandom(1000))
        signed, _ = mci.sign_image(src, key, slot_size=0, image_id=1)

    encrypted = encrypt_image(kek, signed)
    start, end = _body_range(signed)
    assert encrypted[:start] == signed[:start]
    assert encrypted[start:end] != signed[start:end]
    signed_end = len(mci.split_unprotected_tlvs(signed)[0])
    assert encrypted[end:signed_end] == signed[end:signed_end]
    assert decrypt_image(kek, encrypted)[:signed_end] == signed[:signed_end]
    assert len(enc_kw_of(encrypted)) == ENC_KW_LEN
    # The hash TLV keeps the plaintext hash, as imgtool's does for an encrypted image.
    region, tlvs = mci.split_unprotected_tlvs(signed)
    assert mci.split_unprotected_tlvs(encrypted)[1][mci.TLV_SHA256] == tlvs[mci.TLV_SHA256]
    for image, verifies in ((encrypted, False), (decrypt_image(kek, encrypted), True)):
        region, tlvs = mci.split_unprotected_tlvs(image)
        public, signature = tlvs[mci.TLV_ECDSA_PUBKEY], tlvs[mci.TLV_ECDSA_SIG]
        assert bool(mci.CURVE.verify(public, region, signature)) is verifies

    for bad, why in (
        (encrypted, "an ENC_KW TLV"),
        (signed[:16] + bytes(4) + signed[20:], "the flag cleared"),
    ):
        try:
            encrypt_image(kek, bad)
        except EncryptError:
            continue
        raise AssertionError(f"encrypt_image must refuse an image with {why}")

    # MCUboot's boot_enc_decrypt() keystream: AES(key, BE128(block index)) from the
    # body start, built here independently of the CTR mode used above.
    body_key = bytes(range(BODY_KEY_LEN))
    plaintext = bytes(range(256)) * 3
    ecb = Cipher(algorithms.AES(body_key), modes.ECB()).encryptor()
    stream = b"".join(
        ecb.update(i.to_bytes(AES_BLOCK_LEN, "big"))
        for i in range(-(-len(plaintext) // AES_BLOCK_LEN))
    )
    expected = bytes(p ^ k for p, k in zip(plaintext, stream, strict=False))
    assert ctr_crypt(body_key, plaintext) == expected

    print("mcuboot_encrypt self-test: OK")


if __name__ == "__main__":
    _self_test()
