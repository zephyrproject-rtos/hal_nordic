#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Add MCUboot metadata to unsigned firmware."""

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import hybrid
from lib import mcuboot_image as mci

SDK = Path(__file__).resolve().parent.parent.parent


def _uint(s):
    return int(s, 0)


def report(image: Path) -> dict:
    """Decode metadata from an unsigned image."""
    data, base = mci.read_image(image)
    header_size, version, body, tlvs = mci.read_unsigned(data)
    conf = tlvs.get(mci.TLV_IRONSIDE_CONF)
    image_id = tlvs.get(mci.TLV_IROT_IMAGE_ID)
    out = {
        "base": None if base is None else f"0x{base:08X}",
        "header_size": f"0x{header_size:X}",
        "version": mci.version_display(version),
        "image_size": len(body),
        "signed_len": len(data),
        "security_counter": f"0x{int.from_bytes(tlvs[mci.TLV_SEC_CNT], 'little'):08x}",
    }
    if image_id is not None:
        ident = image_id[0]
        role = next((r for r, v in mci.IMAGE_ID.items() if v == ident), None)
        out["image_id"] = ident if role is None else f"{ident} ({role})"
    if conf is not None:
        out["conf_img_bytes"] = len(conf)
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description="Add MCUboot metadata to unsigned firmware.")
    ap.add_argument("role", nargs="?", choices=mci.ROLES, help="image role")
    ap.add_argument("image", nargs="?", type=Path, help="unsigned firmware without image metadata")
    ap.add_argument("-o", "--out", type=Path, help="output image")
    ap.add_argument(
        "-v",
        "--version",
        default=mci.VERSION_DEFAULT,
        metavar="MAJOR.MINOR.PATCH[.TWEAK]",
        help=f"image version (default: {mci.VERSION_DEFAULT})",
    )
    ap.add_argument(
        "-s",
        "--security-counter",
        type=_uint,
        metavar="N",
        help="rollback counter (default: derived from --version)",
    )
    ap.add_argument(
        "-S",
        "--slot-size",
        type=_uint,
        default=0,
        help="slot size in bytes (default: 0, no size check)",
    )
    ap.add_argument(
        "--pad-header",
        dest="pad_header",
        action="store_true",
        default=None,
        help="prepend the 0x800-byte gap that holds the header ahead of the vector table",
    )
    ap.add_argument(
        "--no-pad-header",
        dest="pad_header",
        action="store_false",
        help="use the gap the input reserves ahead of its vector table",
    )
    ap.add_argument(
        "--conf-img", type=Path, metavar="BLOB", help="image configuration to embed (urot only)"
    )
    ap.add_argument(
        "--dump", type=Path, metavar="FILE", help="decode metadata from an unsigned image"
    )
    args = ap.parse_args()

    if args.dump is not None:
        print(json.dumps(report(args.dump), indent=2))
        return 0
    missing = [
        name
        for name, value in (("role", args.role), ("image", args.image), ("-o/--out", args.out))
        if value is None
    ]
    if missing:
        ap.error(f"missing {', '.join(missing)}. Use --dump to decode an image.")

    if not args.image.is_file():
        sys.exit(f"Image not found: {args.image}")

    conf_img = None
    if args.conf_img is not None:
        if args.role != "urot":
            sys.exit("gen_img_meta: --conf-img is only supported for urot")
        if not args.conf_img.is_file():
            sys.exit(f"Image configuration not found: {args.conf_img}")
        conf_img = args.conf_img.read_bytes()

    # Reject prior output so stale metadata cannot survive a version change.
    if mci.has_metadata(args.image):
        sys.exit(
            f"gen_img_meta: {args.image} already has image metadata. "
            "Use raw firmware as input or use sign.py to change metadata."
        )

    pad_header = mci.PAD_HEADER_DEFAULT[args.role] if args.pad_header is None else args.pad_header
    try:
        unsigned, base = mci.build_unsigned(
            args.image,
            slot_size=args.slot_size,
            version=args.version,
            security_counter=args.security_counter,
            pad_header=pad_header,
            image_id=mci.IMAGE_ID[args.role],
            conf_img=conf_img,
            signature_area_size=(
                hybrid.SIGNATURE_AREA_SIZE if args.role == hybrid.ROLE else mci.SIGNATURE_AREA_SIZE
            ),
        )
        mci.write_image(args.out, unsigned, base)
    except mci.SignError as exc:
        sys.exit(f"gen_img_meta: {exc}")

    print(f"wrote {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
