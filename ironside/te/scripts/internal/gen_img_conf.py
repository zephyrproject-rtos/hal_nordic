#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Generate an IronSide image configuration."""

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import conf_img as ci
from lib import img_conf_args

SUPPORTED_FORMATVERSIONS = img_conf_args.SUPPORTED_FORMATVERSIONS


def dump(path):
    """Decode a raw configuration or a signed image."""
    if path.stat().st_size == ci.CONF_IMG_SIZE:
        return ci.unpack(path.read_bytes())
    from lib import mcuboot_image as mb

    blob = mb.find_protected_tlv(path, mb.TLV_IRONSIDE_CONF)
    if blob is None:
        raise ci.ConfImgError(f"{path}: no image configuration in the protected TLV area")
    return ci.unpack(blob)


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate an IronSide image configuration.")
    ap.add_argument(
        "-o", "--output", "--out", type=Path, dest="output", help="output configuration file"
    )
    ap.add_argument(
        "--dump", type=Path, metavar="FILE", help="decode a configuration or signed image"
    )
    ap.add_argument(
        "--format-version",
        default=SUPPORTED_FORMATVERSIONS[0],
        metavar="VER",
        help="configuration format version (default: %(default)s)",
    )
    img_conf_args.add_arguments(ap)
    args = ap.parse_args()

    if args.dump is not None:
        print(json.dumps(dump(args.dump), indent=2))
        return 0
    if args.output is None:
        ap.error("-o/--output is required unless --dump is used")
    if args.format_version not in SUPPORTED_FORMATVERSIONS:
        sys.exit(
            f"gen_img_conf: unsupported format version "
            f"{args.format_version} (supported: "
            f"{', '.join(SUPPORTED_FORMATVERSIONS)})"
        )

    # With no current configuration, apply() disables unspecified fields.
    blob = img_conf_args.apply(args)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(blob)
    print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except ci.ConfImgError as e:
        sys.exit(f"gen_img_conf: {e}")
