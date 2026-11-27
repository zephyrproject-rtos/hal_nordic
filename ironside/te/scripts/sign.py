#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Sign firmware for IronSide secure boot.

IronSide verifies the MCUboot header, body, and protected TLVs. App and DFU images
are signed with ECDSA P-384 over SHA-384 and carry the signature and the public
key. The UROT image is signed with ECDSA P-384 and ML-DSA-65 together, using the
hybrid key pair of --gen, and carries both signatures and both public keys. Raw
firmware receives MCUboot metadata. Existing metadata is preserved unless an
option overrides it.

The image role selects the signing key and protected image ID. Released images
already contain an image ID, which must match --role. The device must be
provisioned from the same key source.

--pad-header applies only to raw firmware. IronSide TE expects the 32-byte header at
the slot base and boots the vector table 0x800 bytes into the slot; the gap between
them is body. Bare-metal applications normally need the gap prepended. Zephyr images
reserve it through CONFIG_ROM_START_OFFSET.

The security counter defaults to the packed image version. A UROT or DFU image
with metadata keeps its counter instead, because that counter also carries the
security epoch of the release. Increasing the counter permanently prevents images
with lower counters from booting. UROT image
configuration is stored in a protected TLV and covered by the signature.
"""

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from lib import conf_img as ci
from lib import hybrid, parts, uicr
from lib import component_counter
from lib import mcuboot_image as mci

# Share role metadata with the image generator to keep update routing consistent.
ROLES = mci.ROLES
IMAGE_ID = mci.IMAGE_ID
PAD_HEADER_DEFAULT = mci.PAD_HEADER_DEFAULT

ROLE_DEFAULT = "app"


def slot_size_for(role, soc):
    """Return the primary slot size under the reference partition layout."""
    if role == "urot":
        return uicr.urot_slot_size(soc)
    placed = uicr.partition_addresses(uicr.REFERENCE_PARTITIONS_KB, soc=soc)
    return placed["slot0" if role == "app" else "dfuimage"][1]


def _uint(s):
    """Parse an integer, including literals with a base prefix."""
    return int(s, 0)


def _iron_keychain():
    """Import keychain support only when --keyset is used."""
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import iron_keychain as kc

    return kc


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Sign firmware for IronSide secure boot. "
        "Raw firmware receives MCUboot metadata. Existing metadata "
        "is preserved unless an option overrides it."
    )
    ap.add_argument("image", type=Path, help="input HEX or binary image")
    ap.add_argument(
        "--role", choices=ROLES, default=ROLE_DEFAULT, help=f"image role (default: {ROLE_DEFAULT})"
    )
    ap.add_argument(
        "--soc",
        choices=sorted(parts.PARTS),
        help="target SoC, which sets the slot size (required unless --slot-size is given)",
    )
    ap.add_argument(
        "-o",
        "--out",
        type=Path,
        required=True,
        help="output path. Use .hex for Intel HEX or .bin for raw binary.",
    )

    keys = ap.add_argument_group("signing key")
    keys.add_argument(
        "-k",
        "--key",
        type=Path,
        help="ECDSA P-384 PEM signing key instead of --keyset (app, dfu)",
    )
    keys.add_argument(
        "--keyset",
        dest="keyset",
        metavar="SPEC",
        help="key source as keyring:NAME or dir:PATH (default: only keyset in the OS keyring)",
    )
    keys.add_argument(
        "--gen", type=int, default=0, choices=(0, 1, 2), help="signing key generation (default: 0)"
    )

    metadata = ap.add_argument_group("image metadata")
    metadata.add_argument(
        "-v",
        "--version",
        default=None,
        metavar="MAJOR.MINOR.PATCH[.TWEAK]",
        help=f"image version (default: existing version or {mci.VERSION_DEFAULT})",
    )
    metadata.add_argument(
        "-s",
        "--security-counter",
        type=_uint,
        metavar="N",
        help="security counter (default: derived from image version; a UROT or DFU "
        "image with metadata keeps its counter)",
    )
    metadata.add_argument("--keep-security-counter", action="store_true", help=argparse.SUPPRESS)

    advanced = ap.add_argument_group("advanced options")
    advanced.add_argument(
        "-S", "--slot-size", type=_uint, help="slot size in bytes (default: per-role primary slot)"
    )
    advanced.add_argument("--align", default="16", help=argparse.SUPPRESS)
    advanced.add_argument(
        "--pad-header",
        dest="pad_header",
        action="store_true",
        default=None,
        help="prepend the 0x800-byte gap that holds the header ahead of the vector table",
    )
    advanced.add_argument(
        "--no-pad-header",
        dest="pad_header",
        action="store_false",
        help="use the gap the input reserves ahead of its vector table",
    )
    advanced.add_argument("--pad", action="store_true", help="pad output to the full slot size")
    advanced.add_argument(
        "--no-uicr", action="store_true", help="omit UICR data when reflashing an installed device"
    )

    # UROT loads this protected image-configuration TLV at boot.
    advanced.add_argument("--conf-img", type=Path, help="image configuration to embed (urot only)")
    # Tests that assert UROT refuses a config without board fields need it signed as is.
    advanced.add_argument("--conf-img-verbatim", action="store_true", help=argparse.SUPPRESS)
    args = ap.parse_args()

    if not args.image.is_file():
        sys.exit(f"Image not found: {args.image}")
    if args.key is not None and args.keyset is not None:
        sys.exit("sign.py: use only one of --key and --keyset")
    if args.soc is None and args.slot_size is None:
        sys.exit(f"sign.py: name the target SoC with --soc ({', '.join(sorted(parts.PARTS))})")
    is_hybrid = args.role == hybrid.ROLE
    if is_hybrid and args.key is not None:
        sys.exit(
            f"sign.py: --role {hybrid.ROLE} signs with a hybrid key pair, which --key cannot "
            "name. Use --keyset."
        )

    try:
        prebuilt = mci.has_metadata(args.image)
    except mci.SignError as exc:
        sys.exit(f"sign.py: {exc}")

    # Validate the version before accessing signing keys.
    version = counter = None
    try:
        if args.version is not None:
            version = mci.parse_version(args.version)
        elif not prebuilt:
            version = mci.parse_version(mci.VERSION_DEFAULT)
    except mci.SignError as exc:
        sys.exit(f"sign.py: {exc}")
    # The counter of a UROT or DFU release carries its security epoch, which a
    # version cannot express, so a new version keeps it.
    is_component = args.role in component_counter.ROLES
    if is_component and args.version is not None and args.security_counter is None:
        if not prebuilt:
            sys.exit(
                f"sign.py: a raw {args.role} image takes no security counter from --version. "
                "Pass --security-counter."
            )
        keep_counter = True
    else:
        keep_counter = args.keep_security_counter

    if args.security_counter is not None:
        counter = args.security_counter
    elif version is not None and not keep_counter:
        counter = mci.security_counter_from_version(version)

    if keep_counter:
        # Preserve the existing security counter while changing the image version.
        if args.security_counter is not None:
            sys.exit("sign.py: use only one of --security-counter and --keep-security-counter")
        if not prebuilt:
            sys.exit("sign.py: --keep-security-counter requires an image with existing metadata")
        try:
            counter = mci.security_counter_of(args.image)
        except mci.SignError as exc:
            sys.exit(f"sign.py: {args.image}: {exc}")

    if prebuilt:
        # Header construction options do not apply to images with metadata.
        stale = [
            flag
            for flag, given in (
                ("--pad-header/--no-pad-header", args.pad_header is not None),
            )
            if given
        ]
        if stale:
            sys.exit(
                f"sign.py: {args.image} already has image metadata. Remove {' and '.join(stale)}."
            )
        # The embedded role must match the key selected by --role.
        try:
            embedded = mci.read_unsigned(mci.read_image(args.image)[0])[3].get(
                mci.TLV_IROT_IMAGE_ID
            )
        except mci.SignError as exc:
            sys.exit(f"sign.py: {args.image}: {exc}")
        if embedded is not None and embedded[0] != IMAGE_ID[args.role]:
            was = next((r for r, v in IMAGE_ID.items() if v == embedded[0]), f"id {embedded[0]}")
            hint = "" if args.role != ROLE_DEFAULT else f" Use --role {was}."
            sys.exit(f"sign.py: {args.image} was built for {was}, not {args.role}.{hint}")

    conf_img = None
    if args.conf_img is not None:
        if args.role != "urot":
            sys.exit("sign.py: --conf-img is only supported for --role urot")
        if not args.conf_img.is_file():
            sys.exit(f"Image configuration not found: {args.conf_img}")
        conf_img = args.conf_img.read_bytes()
    if args.role == "urot":
        # UROT does not boot without the board fields, so every signed UROT
        # carries them.
        try:
            if conf_img is None and prebuilt:
                conf_img = mci.find_protected_tlv(args.image, mci.TLV_IRONSIDE_CONF)
            if not args.conf_img_verbatim:
                conf_img = ci.with_board_defaults(conf_img)
        except (mci.SignError, ci.ConfImgError) as exc:
            sys.exit(f"sign.py: {exc}")

    # Without an explicit source, require exactly one keyset in the keyring.
    pems = []
    if args.key is not None:
        if not args.key.is_file():
            sys.exit(f"Signing key not found: {args.key}")
        key_label = args.key.name
    else:
        kc = _iron_keychain()
        try:
            source = kc.KeySource.parse(args.keyset or kc.sole_keyset_spec())
            key_ids = kc.signing_key_ids(args.role, args.gen)
            pems = [source.get_bytes(key_id) for key_id in key_ids]
        except kc.KeySourceError as exc:
            sys.exit(f"sign.py: {exc}")
        absent = [key_id for key_id, pem in zip(key_ids, pems, strict=True) if pem is None]
        if absent:
            sys.exit(
                f"sign.py: {', '.join(absent)} not found in {source.describe()}. "
                "Run iron_keychain.py new or choose another --keyset."
            )
        key_label = ", ".join(source.label(key_id) for key_id in key_ids)

    slot_size = args.slot_size if args.slot_size is not None else slot_size_for(args.role, args.soc)
    pad_header = args.pad_header if args.pad_header is not None else PAD_HEADER_DEFAULT[args.role]

    unsigned_args = {
        "slot_size": slot_size,
        "version": version,
        "security_counter": counter,
        "pad_header": pad_header,
        "image_id": IMAGE_ID[args.role],
        "conf_img": conf_img,
    }
    try:
        if is_hybrid:
            ecdsa_key = hybrid.load_ecdsa_key(pems[0], label=key_label)
            mldsa_key = hybrid.load_mldsa_key(pems[1], label=key_label)
            signed, base = hybrid.sign_image(
                args.image, ecdsa_key, mldsa_key, pad=args.pad, **unsigned_args
            )
        else:
            key = (
                mci.load_signing_key(args.key)
                if not pems
                else mci.load_signing_key_from_pem(pems[0], label=key_label)
            )
            signed, base = mci.sign_image(args.image, key, pad=args.pad, **unsigned_args)
        # Binary output cannot represent discontiguous UICR data. --no-uicr also
        # omits it when programming firmware onto a device with write-once UICR.
        aux = mci.read_aux(args.image)
        mci.write_image(args.out, signed, base, aux=None if args.no_uicr else aux)
        if len(aux) and not args.no_uicr and not str(args.out).lower().endswith(".hex"):
            print(f"warning: binary output omits {len(aux)} bytes of UICR data")
    except mci.SignError as e:
        sys.exit(f"sign.py: {e}")

    # Report metadata from the signed bytes because prebuilt input may supply it.
    signed_version = mci.parse_version_header(signed)
    signed_counter = int.from_bytes(mci.find_protected_tlv(args.out, mci.TLV_SEC_CNT), "little")
    counter_note = (
        "kept"
        if keep_counter
        else "derived"
        if args.security_counter is None
        else "set"
    )
    print(
        f"signed {args.role}: {args.out} "
        f"(version {mci.version_display(signed_version)}, security counter "
        f"0x{signed_counter:08x} {counter_note}, key {key_label})"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
