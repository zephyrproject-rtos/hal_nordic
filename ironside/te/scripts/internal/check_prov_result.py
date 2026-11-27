#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Validate provisioning results saved by J-Link Commander.

Every operation must succeed or report an existing write-once value. The
exported IAK public key is the device identity artifact retained by production.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from lib import prov_abi as abi
from lib import prov_batches as pb

# ALREADY_EXISTS indicates an idempotent write to a populated target.
ACCEPT = (abi.PSA_SUCCESS, abi.PSA_ERROR_ALREADY_EXISTS)


def batch_from_bundle(bundle: Path) -> list[dict]:
    """Return the ordered operations stored at CMD_ADDR in a bundle."""
    from intelhex import IntelHex

    ih = IntelHex(str(bundle))
    starts = [s for s, _ in ih.segments()]
    if abi.CMD_ADDR not in starts:
        raise SystemExit(
            f"check_prov_result.py: {bundle} has no provisioning batch at "
            f"{abi.CMD_ADDR:#010x}. Use a HEX from ironside_bundle.py build-production-hex."
        )
    end = next(e for s, e in ih.segments() if s == abi.CMD_ADDR)
    raw = bytes(ih.tobinarray(start=abi.CMD_ADDR, size=end - abi.CMD_ADDR))
    try:
        return abi.parse_input_batch(raw)["operations"]
    except ValueError as exc:
        raise SystemExit(f"check_prov_result.py: invalid batch in {bundle}: {exc}")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="Check provisioning results saved by J-Link Commander."
    )
    parser.add_argument(
        "--bundle", type=Path, required=True, help="bundle HEX file used for provisioning"
    )
    parser.add_argument(
        "--result", type=Path, required=True, help="saved provisioning result buffer"
    )
    parser.add_argument(
        "--soc",
        required=True,
        choices=sorted(abi.LAYOUTS),
        help="SoC the bundle was built for, which sets the provisioning batch address",
    )
    parser.add_argument(
        "--iak-out", type=Path, help="write the exported IAK public key as raw binary"
    )
    args = parser.parse_args(argv)
    abi.select_soc(args.soc)

    ops = batch_from_bundle(args.bundle)
    raw = args.result.read_bytes()
    if not any(raw):
        raise SystemExit(
            f"check_prov_result.py: {args.result} contains no result. "
            "Provisioning did not run or timed out."
        )
    try:
        out = abi.parse_result(raw)
    except RuntimeError as exc:
        raise SystemExit(f"check_prov_result.py: {exc}")
    results = out["results"]

    print(f"bundle: {args.bundle}")
    print(f"result: {args.result} ({len(results)}/{len(ops)} operations)")

    failed, aborted, pubkey = [], [], None
    export_ran = False
    for i, op in enumerate(ops):
        name = abi.op_name(op)
        if i >= len(results):
            # The firmware writes one result for each operation it reaches.
            print(f"  {i:2d}   {name:<20} did not run")
            failed.append((i, name, "no result"))
            continue
        status = results[i].get("status")
        if "pubkey" in results[i]:
            pubkey = results[i]["pubkey"]
        if status in ACCEPT:
            pass
        elif status == abi.ITE_ERROR_ABORTED:
            aborted.append((i, name))
        else:
            failed.append((i, name, abi.status_name(status)))
        if name == "export-iak-pubkey" and status in ACCEPT:
            export_ran = True
        print(f"  {i}: {name}: {abi.status_name(status)}")

    if len(results) > len(ops):
        # Surplus entries indicate stale data or mismatched result and bundle files.
        print(f"error: result has {len(results) - len(ops)} extra entries")
        failed.append((len(ops), "-", "surplus results"))

    if pubkey is not None:
        print(f"\nIAK public key: {pubkey.hex()}")
        if args.iak_out:
            args.iak_out.write_bytes(pubkey)
            print(f"wrote {args.iak_out}")
    elif export_ran:
        # Report a missing key only when the export itself succeeded.
        i = next(i for i, op in enumerate(ops) if abi.op_name(op) == "export-iak-pubkey")
        print("\nerror: export-iak-pubkey returned no key")
        failed.append((i, "export-iak-pubkey", "no pubkey returned"))

    if failed:
        seed_problem = pb.seed_failure(ops, results)
        if seed_problem is not None:
            print(f"\nerror: {seed_problem}")
        print(f"\nfailed: {len(failed)} operations")
        for i, name, why in failed:
            print(f"  {i}: {name}: {why}")
        if aborted:
            print(
                f"  {len(aborted)} operations aborted: "
                + ", ".join(f"op {i} {n}" for i, n in aborted)
            )
        print("Device is not provisioned. Return it to Empty before retrying.")
        return 1

    print(f"\npassed: {len(results)} operations")
    return 0


if __name__ == "__main__":
    sys.exit(main())
