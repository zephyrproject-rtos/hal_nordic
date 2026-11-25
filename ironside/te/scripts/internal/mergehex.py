#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Merge Intel HEX files."""

import argparse
import sys
from pathlib import Path

from intelhex import IntelHex


def _load(path: Path) -> IntelHex:
    part = IntelHex(str(path))
    # A provisioning bundle has no single entry point. Inputs may contain
    # conflicting CS:IP or EIP records, which intelhex rejects. nrfutil ignores
    # these records, and prov_run.py uses the fixed PROV_ENTRY_PC ABI address.
    part.start_addr = None
    return part


def _displaced(merged: IntelHex, part: IntelHex) -> list:
    """Return ranges in merged that part will replace."""
    resident = set(merged.addresses())
    hits = sorted(a for a in part.addresses() if a in resident)
    ranges, start, prev = [], None, None
    for addr in hits:
        if start is None:
            start = addr
        elif addr != prev + 1:
            ranges.append((start, prev))
            start = addr
        prev = addr
    if start is not None:
        ranges.append((start, prev))
    return ranges


def merge(inputs, out: Path, overrides=()) -> None:
    merged = IntelHex()
    for f in inputs:
        part = _load(f)
        merged.merge(part, overlap="error")

    # Overrides may replace ordinary inputs, but not earlier overrides.
    applied = IntelHex()
    for f in overrides:
        part = _load(f)
        for start, end in _displaced(merged, part):
            size = end - start + 1
            print(f"override: {f} replaces {size} byte(s) at 0x{start:08X}..0x{end:08X}")
        applied.merge(part, overlap="error")
        merged.merge(part, overlap="replace")
    out.parent.mkdir(parents=True, exist_ok=True)
    merged.write_hex_file(str(out))


def main() -> int:
    ap = argparse.ArgumentParser(description="Merge Intel HEX files.")
    ap.add_argument("inputs", nargs="+", type=Path, help="input HEX files")
    ap.add_argument(
        "--override",
        action="append",
        default=[],
        type=Path,
        metavar="HEX",
        help="HEX file that may replace input bytes (may be repeated)",
    )
    ap.add_argument("-o", "--out", type=Path, required=True, help="output HEX file")
    args = ap.parse_args()

    for f in (*args.inputs, *args.override):
        if not f.is_file():
            sys.exit(f"HEX file not found: {f}")
    try:
        merge(args.inputs, args.out, args.override)
    except Exception as e:
        sys.exit(f"Could not merge HEX files: {e}")

    print(f"wrote {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
