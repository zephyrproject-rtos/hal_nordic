#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Add one of an application's own persistent keys to a keyset.

The key is stored as the packed PSA attributes and the key material, beside the
IronSide verification keys. `ironside_bundle.py update-keys` then provisions
whatever application keys the keyset holds.

--id is the PSA key ID the application uses at run time.
Only the keys IronSide TE can provision are offered; --list prints that table.
`iron_keychain.py delete <keyset> appkey_<id>` removes a key from the keyset.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import iron_keychain as kc  # noqa: E402
from lib import app_key_caps as caps  # noqa: E402
from lib import app_keys, parts, psa_key_attributes  # noqa: E402

TOOL = "add_application_key"


class MaterialError(Exception):
    """The key material is not the key the other arguments describe."""


def _parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)

    key = ap.add_argument_group("the key")
    key.add_argument("--type", choices=caps.key_type_names(), help="PSA key type")
    key.add_argument(
        "--key-bits",
        metavar="BITS",
        type=lambda s: int(s, 0),
        help="key size in bits, as the key type names it, for example 128 or 256. An "
        "ECC key is named by its curve size, not by the length of its encoded material",
    )
    key.add_argument(
        "--algorithm",
        choices=caps.algorithm_names(),
        help="the algorithm the key is bound to. The key is refused for every other "
        "algorithm",
    )
    key.add_argument(
        "--usage",
        action="append",
        default=[],
        choices=caps.usage_names(),
        help="what the key may be used for. Repeat for more than one. Each grants the "
        "whole PSA group: DECRYPT also unwraps, SIGN signs both a message and a hash",
    )
    key.add_argument(
        "--allow-usage-export",
        action="store_true",
        help="also let the application read this key out with psa_export_key",
    )
    source = key.add_mutually_exclusive_group()
    source.add_argument(
        "--key",
        metavar="HEX",
        help="key material as hex, with or without a leading 0x, for example "
        "101112131415161718191a1b1c1d1e1f for a 128-bit key",
    )
    source.add_argument(
        "--key-from-file",
        metavar="FILE",
        type=Path,
        help="read a symmetric key as raw bytes, or an EC key from an unencrypted PEM "
        "file. A secp-r1 public key becomes its uncompressed point; a private key gives "
        "its scalar, or its public point when --type names a public key",
    )

    where = ap.add_argument_group("placement")
    where.add_argument(
        "--id",
        metavar="KEY_ID",
        type=lambda s: int(s, 0),
        help=f"the PSA key ID, decimal or 0x hex, {app_keys.KEY_ID_MIN} to "
        f"0x{app_keys.KEY_ID_MAX:08X} except {app_keys.RESERVED_KEY_ID_MIN} to "
        f"{app_keys.RESERVED_KEY_ID_MAX}, for example 164",
    )
    where.add_argument(
        "--persistence",
        default=caps.DEFAULT_PERSISTENCE,
        choices=sorted(caps.PERSISTENCE),
        help="what psa_destroy_key does with the key ID: ROTATING frees it, "
        "REVOKABLE revokes it for the life of the part, LOCKED refuses the destroy "
        f"(default: {caps.DEFAULT_PERSISTENCE})",
    )
    out = ap.add_argument_group("output")
    out.add_argument(
        "--keyset",
        dest="keyset",
        metavar="SPEC",
        help="the keyset to store the key in, for example keyring:dev or dir:keys/ "
        "(default: the sole keyring keyset)",
    )
    out.add_argument(
        "--force",
        action="store_true",
        help="replace a key with the same key ID (default: keep it)",
    )
    out.add_argument(
        "--list",
        action="store_true",
        help="print the keys an SoC provisions, and exit",
    )
    out.add_argument(
        "--soc",
        choices=sorted(parts.PARTS),
        help="the SoC --list prints the table of (required with --list). Every SoC "
        "IronSide TE supports provisions the same keys, so it selects nothing else",
    )
    return ap


def main(argv=None) -> int:
    args = _parser().parse_args(argv)

    if args.list:
        if args.soc is None:
            return _fail("--list needs --soc to name the part whose table to print.", usage=True)
        profile = caps.profile(args.soc)
        print(f"{args.soc}: {profile.name}")
        print("\n".join(profile.describe()))
        return 0

    missing = [
        flag
        for flag, given in (
            ("--type", args.type),
            ("--key-bits", args.key_bits),
            ("--algorithm", args.algorithm),
            ("--usage", args.usage),
            ("--id", args.id is not None),
        )
        if not given
    ]
    if missing:
        return _fail(
            f"{', '.join(missing)} required. Run --list to see what can be provisioned.",
            usage=True,
        )
    if not args.key and not args.key_from_file:
        return _fail("no key material. Pass --key or --key-from-file.", usage=True)

    usage = set(args.usage)
    try:
        # A keyset is not per-part: one set prepares bundles for every SoC, so
        # the key must be one every part provisions, with the same metadata.
        per_part = {
            psa_key_attributes.attributes(
                soc,
                args.type,
                args.key_bits,
                args.algorithm,
                usage,
                args.id,
                args.persistence,
                allow_export=args.allow_usage_export,
            )
            for soc in sorted(parts.PARTS)
        }
        if len(per_part) != 1:
            raise caps.CapsError("the parts IronSide TE supports encode this key differently")
        (metadata,) = per_part
        material = _material(args)
        caps.check_material(args.type, args.key_bits, material)
    except (caps.CapsError, MaterialError) as exc:
        return _fail(exc)

    if args.allow_usage_export:
        print(
            f"{TOOL}: PSA_KEY_USAGE_EXPORT lets the application read this key out with "
            f"psa_export_key.",
            file=sys.stderr,
        )

    # No hard default: a bare keyring:dev would reach for a Secret Service on a
    # host that has none, where the sole indexed keyset is what the other tools
    # resolve to.
    try:
        target = kc.new_target(args.keyset or kc.sole_keyset_spec())
        kc.store_appkey(target, metadata, material, force=args.force)
    except (app_keys.AppKeyError, kc.KeySourceError) as exc:
        return _fail(exc)

    print(f"{target.describe()}: {len(target.appkey_ids())} application key(s)")
    print("\n".join(f"  {line}" for line in _key_map(target)))
    return 0


def _key_map(target) -> list:
    """Every application key the keyset now holds."""
    entries = [
        kc.appkey_entry(target.read_appkey(key_id), key_id)
        for key_id in target.appkey_ids()
    ]
    return app_keys.describe(app_keys.parse_entries(entries, target.describe()))


def _material(args) -> bytes:
    if args.key:
        body = args.key[2:] if args.key[:2].lower() == "0x" else args.key
        try:
            return bytes.fromhex(body)
        except ValueError:
            raise MaterialError(f"--key is not hex: {args.key!r}") from None
    return _from_file(args.key_from_file, args.type, args.key_bits)


def _from_file(path: Path, key_type: str, key_bits: int) -> bytes:
    try:
        data = path.read_bytes()
    except OSError as exc:
        raise MaterialError(f"cannot read {path} ({exc})") from None

    if caps.KEY_TYPES[key_type].material == "raw":
        return data
    return _from_pem(data, path, key_type, key_bits)


def _from_pem(data: bytes, path: Path, key_type: str, key_bits: int) -> bytes:
    """The key material a PEM holds, encoded as psa_import_key takes it.

    The curve and its size are held against the arguments rather than trusted:
    a PEM that does not match the declared key would otherwise be provisioned as
    whatever @key_bits claimed, and the mismatch would surface as a failed
    signature long after the install.
    """
    from cryptography.hazmat.primitives import serialization
    from cryptography.hazmat.primitives.asymmetric import ec

    key = None
    for load in (
        serialization.load_pem_public_key,
        lambda blob: serialization.load_pem_private_key(blob, password=None),
    ):
        try:
            key = load(data)
            break
        except (ValueError, TypeError):
            continue
    if key is None:
        raise MaterialError(
            f"{path} is not a PEM public or private key this tool can read. An "
            f"encrypted private key is not supported."
        )
    if not isinstance(key, ec.EllipticCurvePublicKey | ec.EllipticCurvePrivateKey):
        raise MaterialError(
            f"{path} holds no elliptic-curve key. This tool reads secp256r1 and "
            f"secp384r1 keys from a PEM file; pass any other key as hex with --key."
        )

    sizes = {"secp256r1": 256, "secp384r1": 384}
    if key.curve.name not in sizes:
        raise MaterialError(
            f"{path} holds a {key.curve.name} key, and {key_type} keys are secp256r1 "
            f"or secp384r1."
        )
    if sizes[key.curve.name] != key_bits:
        raise MaterialError(
            f"{path} holds a {sizes[key.curve.name]}-bit key and --key-bits says "
            f"{key_bits}."
        )

    wants_point = caps.KEY_TYPES[key_type].material == "ecc_point_secp_r1"
    if isinstance(key, ec.EllipticCurvePrivateKey):
        if not wants_point:
            return key.private_numbers().private_value.to_bytes(key_bits // 8, "big")
        print(f"{TOOL}: took the public point from the private key in {path}", file=sys.stderr)
        key = key.public_key()
    elif not wants_point:
        raise MaterialError(
            f"{path} holds a public key, and {key_type} is a key pair. Pass the private "
            f"key, or name a public key type with --type."
        )
    return key.public_bytes(
        serialization.Encoding.X962, serialization.PublicFormat.UncompressedPoint
    )


def _fail(message, *, usage: bool = False) -> int:
    print(f"{TOOL}: {message}", file=sys.stderr)
    return 1 if usage else 2


if __name__ == "__main__":
    sys.exit(main())
