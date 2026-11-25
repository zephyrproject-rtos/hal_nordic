#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Protect an Open device by moving it to Secured or to PRoT Provisioning.

Secured closes all access ports. PRoT Provisioning closes the secure and AUX
access ports and keeps the non-secure access port open. Both transitions
disable unauthenticated ERASEALL.
"""

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parent
PROV_RUN = SCRIPTS / "internal" / "prov_run.py"

sys.path.insert(0, str(SCRIPTS))
from lib import adac_cli, generate_production_hex as prodhex  # noqa: E402
from lib import bundle_edit as be  # noqa: E402

# Lifecycle names used by ADAC.
FROM_STATE = "open"
FROM_ADAC = "assembly"
TARGET_KEEP_NS_OPEN = "prot"
TARGET_PROT_ADAC = "prot-provisioning"
TARGET_FULL = "secured"

# The flow each target runs, and the state ADAC must then report.
FLOWS = {
    TARGET_FULL: (prodhex.FLOW_PROTECT, TARGET_FULL),
    TARGET_KEEP_NS_OPEN: (prodhex.FLOW_PROT_OPEN, TARGET_PROT_ADAC),
}

# This development tool always uses authenticated locking so authenticated
# debug remains available. It does not use the bundle's configured closure
# because hardware-fixed closure is permanent and unsuitable for a bench board.
AP_CLOSURE = "auth-lock"

EXIT_USAGE = 2
EXIT_STATE = 3
EXIT_READ = 4
EXIT_VERIFY = 5


def run(cmd, capture=False):
    print("  $ " + " ".join(str(c) for c in cmd))
    argv = [str(c) for c in cmd]
    if capture:
        return subprocess.run(argv, check=True, capture_output=True, text=True).stdout
    subprocess.run(argv, check=True)
    return ""


def _select(serial) -> list:
    """The nrfutil arguments selecting a J-Link serial, or none."""
    return ["--serial-number", str(serial)] if serial is not None else []


def build_artifact(bundle: Path, flow: str, out_dir: Path) -> Path:
    """Build the HEX containing the provisioning firmware and its batch.

    The device is already installed, so the artifact contains no firmware
    images.
    """
    try:
        editable = be.read(bundle)
    except be.BundleEditError as exc:
        print(f"ironside_protect: {exc}", file=sys.stderr)
        raise SystemExit(EXIT_USAGE) from None
    try:
        written = prodhex.write(
            editable,
            flow=flow,
            out_dir=out_dir,
            ap_closure=AP_CLOSURE,
            report=lambda line: print(f"  {line}"),
        )
    except (prodhex.ProdhexError, be.BundleEditError) as exc:
        print(f"ironside_protect: {exc}", file=sys.stderr)
        raise SystemExit(EXIT_USAGE) from None
    return written[0]


def program(artifact: Path, serial) -> None:
    """Program the artifact whole and start what is now in RAM.

    No erase and no recover: the artifact writes RAM only, and the device's
    installation is what this transition protects rather than replaces. The part
    must not boot between the program and the run, which `nrfutil device
    program` gives by defaulting to RESET_NONE. A boot would reach IronSide DFU
    at the address the provisioning firmware occupies. The pin reset commits the
    transition. CTRLAPHARD would request ADAC instead.
    """
    run(
        [
            "nrfutil",
            "device",
            "program",
            "--firmware",
            artifact,
            "--options",
            "chip_erase_mode=ERASE_NONE",
            *_select(serial),
        ]
    )
    run(
        [
            sys.executable,
            PROV_RUN,
            "run-bundle",
            "--bundle",
            artifact,
            "--mode",
            "secure",
            "--device",
            "Cortex-M33",
            *(["--serial", str(serial)] if serial is not None else []),
        ]
    )
    run(["nrfutil", "device", "reset", "--reset-kind", "RESET_PIN", *_select(serial)])


def verify_adac(want, serial):
    print(f"verifying {want} over ADAC (resets the device)")
    name = adac_cli.require_lifecycle(serial, want)
    if name is None:
        print(
            f"ironside_protect: expected ADAC lifecycle {want!r}; device did not report it",
            file=sys.stderr,
        )
        return EXIT_VERIFY
    print(f"verified: {name}")
    return 0


def main(argv=None):
    p = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    p.add_argument(
        "bundle",
        nargs="?",
        metavar="ZIP|DIR",
        help="prepared IronSide bundle containing secure provisioning firmware",
    )
    p.add_argument("--ite-bundle", dest="ite_bundle", metavar="ZIP|DIR", help=argparse.SUPPRESS)
    p.add_argument(
        "--keep-nonsecure-ap-open",
        action="store_true",
        help="transition to PRoT Provisioning and leave the "
        "non-secure access port open (default: Secured)",
    )
    p.add_argument("--serial-number", dest="serial", help="J-Link serial number (default: auto)")
    p.add_argument("--serial", dest="serial", help=argparse.SUPPRESS)  # older spelling
    p.add_argument("--speed", help=argparse.SUPPRESS)
    args = p.parse_args(argv)

    if args.ite_bundle is not None:
        if args.bundle is not None:
            p.error("the IronSide bundle was given twice, positionally and as --ite-bundle")
        args.bundle = args.ite_bundle
    if args.bundle is None:
        p.error("the IronSide bundle is required")

    target = TARGET_KEEP_NS_OPEN if args.keep_nonsecure_ap_open else TARGET_FULL
    flow, expected = FLOWS[target]
    print(f"protecting device: {FROM_STATE} -> {target}")

    tmpdir = Path(tempfile.mkdtemp(prefix="ironside-protect-"))
    try:
        name = adac_cli.lifecycle(args.serial)
        if name is None:
            print(
                "ironside_protect: no ADAC response from IronSide TE. Nothing was written.",
                file=sys.stderr,
            )
            return EXIT_READ
        if name != FROM_ADAC:
            print(
                f"ironside_protect: device must be in {FROM_STATE}. "
                f"ADAC reports {name!r}. Nothing was written.",
                file=sys.stderr,
            )
            return EXIT_STATE
        print(f"  current state : {FROM_STATE} (ADAC: {name})")

        artifact = build_artifact(Path(args.bundle), flow, tmpdir)
        program(artifact, args.serial)
    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)

    return verify_adac(expected, args.serial)


if __name__ == "__main__":
    try:
        code = main()
    except subprocess.CalledProcessError as exc:
        print(f"ironside_protect: command failed: {exc}", file=sys.stderr)
        code = 1
    raise SystemExit(code)
