#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Encrypt a signed image for an IronSide firmware update.

This is the only update step that uses the OTA KEK. Separating encryption from
upload keeps the KEK off the upload host. The SMP transport accepts only
encrypted updates.

The output is an MCUboot encrypted image: the header and TLVs stay plaintext,
the body is encrypted with AES-256-CTR, and the body key travels AES-KW wrapped
under the KEK in the unprotected IMAGE_TLV_ENC_KW TLV. `dfu_upload.py` sends
that TLV to the device before it streams the image, so IronSide DFU can decrypt
the image as it arrives.

A new body key is drawn for every invocation, so the same input never encrypts
to the same artifact twice. Reusing a CTR keystream is unsafe, and an artifact
cannot be reproduced byte-for-byte later.

`--keyset` selects a keyring or directory key source, and `--kek-gen` selects
the generation. The device checks provisioned generations in descending order.
A successful upload permanently revokes generations below the one that unwraps
the body key.

`--out` should use the `.ota.bin` suffix: `nrfutil mcu-manager serial image-upload`
rejects plain `.ota` files.
"""

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import mcuboot_encrypt
from lib import mcuboot_image as mci


def _iron_keychain():
    """Import the key-access layer only when --keyset is used."""
    import iron_keychain as kc

    return kc


def load_kek(args) -> bytes:
    """Load the OTA KEK."""
    if args.kek_file is not None:
        if not args.kek_file.is_file():
            sys.exit(f"KEK file not found: {args.kek_file}")
        return args.kek_file.read_bytes()

    kc = _iron_keychain()
    try:
        source = kc.KeySource.parse(args.keyset or kc.sole_keyset_spec())
        key_id = kc.kek_key_id(args.kek_gen)
        kek = source.get_bytes(key_id)
    except kc.KeySourceError as exc:
        sys.exit(f"dfu_encrypt.py: {exc}")
    if kek is None:
        sys.exit(
            f"dfu_encrypt.py: {key_id} not found in {source.describe()}. "
            "Run iron_keychain.py new or choose another --keyset."
        )
    return kek


def read_signed_image(path: Path) -> bytes:
    """Read and validate a signed image."""
    if not path.is_file():
        sys.exit(f"Image not found: {path}")
    data = path.read_bytes()

    # sign.py selects its output format from the extension.
    if data[:1] == b":":
        sys.exit(f"{path} is an Intel HEX file. Sign it to a .bin file before encryption.")
    if len(data) < 4 or int.from_bytes(data[:4], "little") != mci.IMAGE_MAGIC:
        sys.exit(
            f"{path} is not a signed MCUboot binary. Sign it to a .bin file before encryption."
        )
    try:
        _region, signature = mci.split_signed(data)
    except mci.SignError as exc:
        sys.exit(f"{path}: {exc}")
    if signature is None:
        sys.exit(f"{path} is not signed. Sign it with sign.py before encryption.")
    return data


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Encrypt a signed image for an IronSide firmware update."
    )
    ap.add_argument("image", type=Path, help="signed binary image")
    ap.add_argument("-o", "--out", type=Path, required=True, help="output encrypted update")
    ap.add_argument(
        "--keyset",
        dest="keyset",
        metavar="SPEC",
        help="key source as keyring:NAME or dir:PATH (default: only keyset in the OS keyring)",
    )
    ap.add_argument(
        "--kek-gen",
        type=int,
        default=None,
        choices=(0, 1, 2),
        help="OTA KEK generation (default: 0). A successful upload revokes older generations.",
    )
    ap.add_argument("--kek-file", type=Path, help="raw 32-byte binary KEK file instead of --keyset")
    args = ap.parse_args()

    if args.kek_file is not None and (args.keyset is not None or args.kek_gen is not None):
        sys.exit("dfu_encrypt.py: use --kek-file without --keyset or --kek-gen")
    if args.kek_gen is None:
        args.kek_gen = 0

    img = read_signed_image(args.image)
    kek = load_kek(args)

    try:
        artifact = mcuboot_encrypt.encrypt_image(kek, img)
    except mcuboot_encrypt.EncryptError as exc:
        sys.exit(f"dfu_encrypt.py: {args.image}: {exc}")
    args.out.write_bytes(artifact)

    print(f"wrote {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
