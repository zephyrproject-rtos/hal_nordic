#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Generate a HEX file for hardware UICR registers."""

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import uicr

SDK = Path(__file__).resolve().parent.parent.parent

# Host HEX writes are for development and bench use. Production writes these
# fields on-device through the provisioning image.
PROTECT_FIELDS = ("secapprotect", "auxapprotect", "eraseprotect", "approtect")


def parse_usercnf(spec):
    """Parse an ADDR=DATA USERCNF entry, both integers in any Python base."""
    addr_s, sep, data_s = spec.partition("=")
    if not sep:
        raise uicr.UicrError(f"--usercnf {spec!r}: expected ADDR=DATA")
    try:
        addr, data = int(addr_s, 0), int(data_s, 0)
    except ValueError:
        raise uicr.UicrError(f"--usercnf {spec!r}: ADDR and DATA must be integers") from None
    for name, value in (("ADDR", addr), ("DATA", data)):
        if not 0 <= value <= uicr.ERASED:
            raise uicr.UicrError(f"--usercnf {spec!r}: {name} 0x{value:X} is not a u32")
    return addr, data


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate a HEX file for hardware UICR registers.")
    ap.add_argument(
        "--dump", type=Path, metavar="FILE", help="decode a HEX file created by this tool"
    )
    ap.add_argument(
        "-o",
        "--output",
        "--out",
        type=Path,
        dest="output",
        help="output HEX file (default: build/uicr_hw.hex)",
    )

    ap.add_argument(
        "--soc",
        choices=sorted(uicr.IROT_SIZE_KB),
        help="target SoC, which sets the IROT size and the BOOTCONF limit (required with "
        "--bootconf unless --bootconf-value is given)",
    )

    bc = ap.add_argument_group("BOOTCONF (makes IROT immutable)")
    bc.add_argument("--bootconf", action="store_true", help="include the BOOTCONF register")
    bc.add_argument(
        "--bootconf-size-kb",
        type=int,
        metavar="KB",
        help="immutable boot region size (default: the IROT size of --soc)",
    )
    bc.add_argument(
        "--bootconf-value",
        type=lambda s: int(s, 0),
        metavar="UINT",
        help="raw BOOTCONF value (overrides --bootconf-size-kb)",
    )

    uc = ap.add_argument_group("USERCNF")
    uc.add_argument(
        "--secure-wdt", action="store_true", help="start the secure boot watchdog before IROT"
    )
    uc.add_argument(
        "--secure-wdt-timeout",
        type=float,
        metavar="SECONDS",
        default=uicr.SECURE_WDT_DEFAULT_TIMEOUT_S,
        help="watchdog timeout in seconds (default: %(default)s)",
    )
    uc.add_argument(
        "--usercnf",
        action="append",
        default=[],
        metavar="ADDR=DATA",
        help="USERCNF entry in ADDR=DATA format, after the watchdog entries (may be repeated)",
    )

    pr = ap.add_argument_group("Protection registers (development only)")
    pr.add_argument("--secapprotect", action="store_true", help="close/lock the secure AP")
    pr.add_argument("--auxapprotect", action="store_true", help="close/lock the auxiliary AP")
    pr.add_argument("--eraseprotect", action="store_true", help="block unauthenticated ERASEALL")
    pr.add_argument("--approtect", action="store_true", help="close the non-secure AP (HW_LOCK)")
    args = ap.parse_args()

    if args.dump is not None:
        print(json.dumps(uicr.format_report(uicr.decode_file(args.dump)), indent=2))
        return 0

    writes = []
    if args.bootconf:
        if args.bootconf_value is not None:
            value = args.bootconf_value
        elif args.soc is None:
            sys.exit("gen_uicr_hw: --bootconf needs --soc to size BOOTCONF")
        else:
            size_kb = args.bootconf_size_kb
            if size_kb is None:
                size_kb = uicr.irot_size_kb(args.soc)
            value = uicr.bootconf_value(size_kb, args.soc)
        writes += uicr.bootconf_writes(value)
    elif args.bootconf_value is not None or args.bootconf_size_kb is not None:
        sys.exit("gen_uicr_hw: --bootconf-value and --bootconf-size-kb require --bootconf")

    if args.secure_wdt or args.usercnf:
        entries = []
        if args.secure_wdt:
            entries += uicr.secure_wdt_entries(args.secure_wdt_timeout)
        entries += [parse_usercnf(spec) for spec in args.usercnf]
        writes += uicr.usercnf_full_writes(entries)

    protect = [f for f in PROTECT_FIELDS if getattr(args, f)]
    for field in protect:
        writes += uicr.hw_protect_writes(field)

    if not writes:
        sys.exit("gen_uicr_hw: no UICR registers selected")

    out = args.output or (SDK / "build" / "uicr_hw.hex")
    uicr.emit_hex(writes, out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except uicr.UicrError as e:
        sys.exit(f"gen_uicr_hw: {e}")
