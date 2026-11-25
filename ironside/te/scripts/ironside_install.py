#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Erase the device to Empty, install a prepared IronSide bundle, and leave it Open.

Reinstalling requires --force because UICR is write-once.
"""

import argparse
import logging as log
import subprocess
import sys
import tempfile
import textwrap
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPTS))
from lib.python_version import require_python  # noqa: E402

require_python()
from lib import adac_cli, fw_bundle, generate_production_hex as prodhex, uicr  # noqa: E402
from lib import bundle_edit as be  # noqa: E402

# Avoid conflict with local partition-layout variables.
from lib import parts as soc_parts  # noqa: E402

INTERNAL = SCRIPTS / "internal"
PROV_RUN = INTERNAL / "prov_run.py"


BUILD_SUBDIR = "build"

# The install artifact programs the App, and its UICR.OTP words, with the
# IronSide images.
PROGRAMMED_ROLES = (*fw_bundle.IMAGE_ROLES, "app")
INSTALL_ROLES = (*PROGRAMMED_ROLES, fw_bundle.PROV_SECURE)

# The DFU image is optional.
REQUIRED_ROLES = ("irot", "urot", fw_bundle.PROV_SECURE)


def unusable(release) -> str:
    """Return why release cannot be used, or an empty string."""
    required = REQUIRED_ROLES
    absent = [role for role in required if release.image(role) is None]
    if not absent:
        return ""
    why = f"carries no {fw_bundle.identifiers(absent)} component"
    if fw_bundle.PROV_SECURE in absent:
        return f"{why}. It is required for installation."
    return why


def unsigned(release, roles) -> str:
    """Return why release lacks signed roles, or an empty string."""
    missing = [
        role
        for role in roles
        if role != "irot" and role in release.components and release.signed(role) is None
    ]
    if missing:
        return (
            f"has no signed {fw_bundle.identifiers(missing)} image. Sign it "
            "with ironside_bundle.py sign-component first."
        )
    return ""


def provisions(bundle: Path, irot: Path = None) -> list:
    """Describe the write-once UICR data carried by bundle."""
    try:
        get = uicr.file_reader(bundle)
        region = uicr.decode(get)["region"]
    except Exception:  # noqa: BLE001
        # Summary generation must not prevent installation.
        return []

    lines = []
    gens = region.get("keys", {})
    roles = [role for role in uicr.ROLES if role in gens]
    held = {
        role: [g for g, state in sorted(gens[role].items()) if state["provisioned"]]
        for role in roles
    }
    gone = {
        role: [g for g, state in sorted(gens[role].items()) if state["revoked"]] for role in roles
    }
    provisioned = ", ".join(_spell(role, held[role]) for role in roles if held[role])
    revoked = ", ".join(_spell(role, gone[role]) for role in roles if gone[role])
    if provisioned:
        lines.append(f"keys: {provisioned}")
    if revoked:
        # Revocation in the write-once region is permanent.
        lines.append(f"revoked keys: {revoked} (permanent)")

    parts = region.get("partitions_kb")
    if parts:
        layout = ", ".join(f"{name}={size} kB" for name, size in parts.items())
        lines.append(f"partitions: {layout}{_layout_origin(parts, irot)}")

    also = []
    version = region.get("irot_formatversion")
    if version is not None and version != uicr.ERASED:
        also.append(f"format version {version >> 16}.{version & 0xFFFF}")
    authlock = region.get("authlock", {})
    if authlock.get("provisioned"):
        # The marker and lock state are stored in separate words.
        state = "locked" if authlock.get("locked") else "unlocked"
        also.append(f"APPROTECTAUTHLOCK marker (provisioned, {state})")
    if also:
        lines.append("also: " + ", ".join(also))

    otp = uicr.otp_words(get)
    if otp:
        lines.append(f"UICR.OTP: {len(otp)} of {uicr.OTP_WORDS} words")

    if region.get("decommissioned"):
        lines.append("decommissioned: yes (terminal state)")
    return lines


def _spell(role, generations) -> str:
    """Format a role and its provisioned key generations."""
    if len(generations) == uicr.GENERATIONS:
        return f"{role}.gen0-{uicr.GENERATIONS - 1}"
    return f"{role}.gen" + "+".join(str(gen) for gen in generations)


def _layout_origin(parts: dict, irot: Path) -> str:
    """Describe whether parts matches the layout supplied by irot."""
    if irot is None:
        return ""
    try:
        shipped = uicr.decode_file(Path(irot))["region"].get("partitions_kb")
    except Exception:  # noqa: BLE001
        # Layout labeling must not prevent installation.
        return ""
    if shipped is None:
        return ""
    if shipped == parts:
        return " (release default)"
    return (
        " (custom, release default: "
        + ", ".join(f"{name}={size} kB" for name, size in shipped.items())
        + ")"
    )


def _select(serial) -> list:
    """The nrfutil arguments selecting a J-Link serial, or none."""
    return ["--serial-number", str(serial)] if serial is not None else []


def run(cmd, step: str, may_fail: bool = False):
    """Run an install step and return whether it succeeded."""
    argv = [str(c) for c in cmd]
    proc = subprocess.run(argv, capture_output=True, text=True)
    if proc.returncode == 0:
        return True
    out = ((proc.stdout or "") + (proc.stderr or "")).rstrip()
    if may_fail:
        return False
    if out:
        log.error("%s", textwrap.indent(out, "  "))
    raise SystemExit(
        f"ironside_install: {step} failed with exit code {proc.returncode}\n"
        f"command: {' '.join(argv)}"
    )


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        description="Erase the device to Empty, install IronSide TE from a prepared "
        "IronSide bundle, and leave the device Open. It builds and programs a "
        "bring-up artifact using the production artifact generator. Use "
        "--force to erase and reinstall an existing installation."
    )
    ap.add_argument(
        "bundle",
        type=Path,
        metavar="ZIP|DIR",
        help="prepared IronSide bundle as a ZIP file or directory",
    )
    install_options = ap.add_argument_group("installation")
    install_options.add_argument(
        "--keyset",
        dest="keyset",
        metavar="SPEC",
        help="key source the bundle was prepared with, as keyring:NAME or "
        "dir:PATH (default: only keyset in the OS keyring). Holds the OTA KEKs "
        "the install batch imports, and the AUTHOP key for the authenticated "
        "erase of an existing installation. Verification keys come from the bundle",
    )
    install_options.add_argument(
        "--force",
        action="store_true",
        help="erase and reinstall an existing IronSide TE installation",
    )
    artifacts = ap.add_argument_group("artifacts")
    artifacts.add_argument(
        "-o",
        "--out-dir",
        type=Path,
        help="directory for the install HEX and intermediate files "
        "(default: a temporary directory). The generated HEX carries raw OTA KEKs",
    )

    device = ap.add_argument_group("device connection")
    device.add_argument(
        "--serial-number",
        type=int,
        default=None,
        help="J-Link serial number",
    )

    args = ap.parse_args(argv)

    # Keep the install transcript free of log metadata.
    log.basicConfig(format="%(message)s", level=log.INFO)

    # Retain temporary artifacts so failed runs can be inspected.
    out_dir = args.out_dir or Path(tempfile.mkdtemp(prefix="ironside-install-"))
    build_dir = out_dir / BUILD_SUBDIR
    build_dir.mkdir(parents=True, exist_ok=True)
    # Resolve the SoC once because slot sizes and provisioning addresses differ.
    soc, whence = None, None
    try:
        soc = soc_parts.for_serial(args.serial_number).soc
    except soc_parts.PartError as exc:
        return _fail(str(exc))
    whence = "the attached board"

    bundle = args.bundle
    # The second phase uses the non-secure provisioning firmware.
    roles = INSTALL_ROLES
    try:
        release = fw_bundle.read(bundle.resolve(), build_dir / "components", roles)
    except fw_bundle.BundleError as exc:
        return _fail(str(exc))
    why = unusable(release)
    if why:
        return _fail(f"{bundle} {why}")
    programmed = [role for role in PROGRAMMED_ROLES if role in release.components]
    why = unsigned(release, programmed)
    if why:
        return _fail(f"{bundle} {why}")
    # Reject a bundle for another SoC before erasing the device.
    if release.soc is not None:
        if soc is None:
            soc, whence = release.soc, "the IronSide bundle"
        elif release.soc != soc:
            return _fail(
                f"IronSide bundle targets {release.soc}, but {whence} reports "
                f"{soc}. Installation was stopped before programming."
            )
    log.info("bundle: %s", bundle)
    if soc is not None:
        log.info("part: %s, from %s", soc, whence)

    resident = adac_cli.lifecycle(args.serial_number)
    if resident is not None and not args.force:
        return _fail(
            "IronSide TE is already installed. Use --force to erase and reinstall it."
        )
    # Authenticated erase requires a resident UROT.
    erasing = args.force and resident is not None
    if args.force and not erasing:
        log.info("no existing installation to erase")

    # The artifact is written through the same code a production line's is, so a
    # developer install exercises the production mechanism rather than a second
    # one beside it. It carries the batch at the address the provisioning
    # firmware reads it from, so nothing is staged over the debug link below.
    #
    # The bring-up flow ends at Open and is library-only, absent from the bundle
    # tool's command line - it leaves the access ports open, which is not
    # something a production line should be able to ask for by accident.
    install_hex = build_dir / prodhex.FLOWS[prodhex.FLOW_OPEN][0].out
    log.info("building the install artifact")
    try:
        prodhex.write(
            be.read(bundle),
            flow=prodhex.FLOW_OPEN,
            out_dir=build_dir,
            keyset=args.keyset,
            soc=soc,
            report=lambda *_: None,
        )
    except (prodhex.ProdhexError, be.BundleEditError) as exc:
        return _fail(str(exc))
    merged = install_hex.is_file()

    if erasing:
        # ADAC erase authenticates with the AUTHOP key on protected devices.
        erase = adac_cli.argv(args.serial_number, "erase-all")
        if args.keyset is not None:
            erase += ["--keyset", args.keyset]
        log.info("erasing the device")
        erased = run(erase, "the ADAC erase-all", may_fail=True)
        if not erased:
            log.warning("warning: authenticated erase failed. Trying unauthenticated erase.")
            unauth = ["nrfutil", "device", "erase", "--all"]
            if args.serial_number is not None:
                unauth += ["--serial-number", str(args.serial_number)]
            run(unauth, "the unauthenticated erase")

    log.info("installing: %s", ", ".join(role.upper() for role in programmed))
    # Report write-once configuration from the bundle before programming it.
    released_irot = (
        release.components["irot"].released_image if "irot" in release.components else None
    )
    conf = provisions(install_hex, released_irot) if merged else []
    if conf:
        log.info("base configuration (replaces existing values):")
        for line in conf:
            log.info("  %s", line)

    # Run the production programming sequence on the artifact above. The order
    # is required:
    #
    # * Recover with adac.py's unauthenticated CTRL-AP ERASEALL, which reliably
    #   leaves the application access port programmable after previous content.
    # * Program the complete HEX, including RAM. RAM contains the provisioning
    #   firmware and its batch.
    # * Do not boot between programming and execution. nrfutil device program
    #   defaults to RESET_NONE. A boot would enter IronSide DFU at the
    #   provisioning firmware address.
    # * Commit with a pin reset. CTRLAPHARD would request ADAC instead of booting.
    steps = (
        (
            "recovering the device",
            "the recover to Empty",
            adac_cli.argv(args.serial_number, "recover"),
        ),
        (
            "programming the install artifact",
            "programming the install artifact",
            [
                "nrfutil",
                "device",
                "program",
                "--firmware",
                str(install_hex),
                "--options",
                "chip_erase_mode=ERASE_NONE",
                *_select(args.serial_number),
            ],
        ),
        (
            "running the provisioning firmware",
            "the provisioning run",
            [
                sys.executable,
                PROV_RUN,
                "run-bundle",
                "--bundle",
                str(install_hex),
                "--mode",
                "secure",
                "--device",
                "Cortex-M33",
                *(["--serial", str(args.serial_number)] if args.serial_number else []),
            ],
        ),
        (
            "resetting to commit",
            "the commit reset",
            [
                "nrfutil",
                "device",
                "reset",
                "--reset-kind",
                "RESET_PIN",
                *_select(args.serial_number),
            ],
        ),
    )
    for announce, what, cmd in steps:
        log.info("%s", announce)
        run(cmd, what)

    log.info("installed IronSide TE (Open)")
    return 0


def _fail(msg: str) -> int:
    log.error("ironside_install: %s", msg)
    return 2


if __name__ == "__main__":
    sys.exit(main())
