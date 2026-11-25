# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Shared host-side firmware update flow.

The flow signs an image, encrypts it, starts IronSide DFU, uploads
over MCUmgr SMP, and resets to apply. Device operations remain in their dedicated
tools. This module defines their order and reset timing.
"""

import argparse
import logging as log
import subprocess
import sys
import textwrap
import time
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRIPTS))
from lib import adac_cli, mcuboot_encrypt, serial_port  # noqa: E402

DFU_ENCRYPT = SCRIPTS / "internal" / "dfu_encrypt.py"
DFU_UPLOAD = SCRIPTS / "internal" / "dfu_upload.py"

# Must match the IronSide DFU devicetree UART rate.
SMP_BAUD = 1000000

# Candidate updates copy and verify the image during reset. Starting another
# reset too early can leave the destination slot partially written.
APPLY_SETTLE = 5.0

# In-place updates need no copy delay. Interrupted validation retries on the next
# boot without changing the uploaded slot. This is keyed by target because an App
# uploaded as a candidate still requires the delay.
IN_PLACE_TARGET = "app"

# An update cycle may wait through an apply boot.
ENTRY_TIMEOUT = 45.0

# Manual DFU entry allows time for an operator-triggered reset.
MANUAL_ENTRY_TIMEOUT = 180.0


def prog() -> str:
    """Return the invoked command name for messages."""
    return Path(sys.argv[0]).stem or "update"


def fail(msg: str) -> int:
    """Log a failure and return the command exit status."""
    log.error("%s: %s", prog(), msg)
    return 2


def run(cmd, step: str, capture: bool = True, execute: bool = True):
    """Run one update step in a separate process.

    Captured output is shown on failure, and captured warnings on success. When
    execution is disabled, print the command without running it.
    """
    argv = [str(c) for c in cmd]
    if not execute:
        log.info("  $ %s", " ".join(argv))
        return
    log.debug("$ %s", " ".join(argv))
    proc = subprocess.run(argv, capture_output=capture, text=True)
    if proc.returncode == 0:
        # A successful step writes only warnings to stderr, and the user must see them.
        if capture and proc.stderr:
            log.warning("%s", proc.stderr.rstrip())
        return
    if capture:
        out = ((proc.stdout or "") + (proc.stderr or "")).rstrip()
        if out:
            log.error("%s", textwrap.indent(out, "  "))
    raise SystemExit(
        f"{prog()}: {step} failed with exit code {proc.returncode}\ncommand: {' '.join(argv)}"
    )


def attempt(cmd, capture: bool = True, execute: bool = True):
    """Run an optional step and return whether it succeeded.

    Preview mode prints the command and reports success.
    """
    argv = [str(c) for c in cmd]
    if not execute:
        log.info("  $ %s", " ".join(argv))
        return True
    log.debug("$ %s", " ".join(argv))
    proc = subprocess.run(argv, capture_output=capture, text=True)
    if proc.returncode == 0:
        if capture:
            log.debug(
                "%s", textwrap.indent(((proc.stdout or "") + (proc.stderr or "")).rstrip(), "  ")
            )
        return True
    log.debug("refused (exit %d): %s", proc.returncode, adac_cli.last_line(proc) or "no output")
    return False


def not_installed(args) -> str:
    """Return an error when no installed IronSide TE answers ADAC.

    Skip the check when ADAC is disabled because the probe cannot distinguish an
    empty device from a missing debugger.
    """
    if args.dry_run or args.skip_adac_bootmode_set:
        return ""
    resident = adac_cli.lifecycle(args.serial_number)
    if resident is None:
        return (
            "No IronSide TE responded over ADAC. Install IronSide TE first "
            "or use --skip-adac-bootmode-set without a debugger."
        )
    log.debug("ADAC answered: lifecycle '%s'", resident)
    return ""


def signed_bin(role, signed_hex: Path, out_dir: Path, *, dry_run: bool = False) -> Path:
    """Convert a signed Intel HEX image to binary for encryption."""
    out = out_dir / f"{role}.signed.bin"
    if dry_run:
        return out
    from lib import mcuboot_image as mci  # noqa: PLC0415

    data, _ = mci.read_image(signed_hex)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(data)
    return out


def encrypt_image(
    role, signed: Path, ota: Path, *, keyset, kek_gen, verbose=False, dry_run=False
) -> Path:
    """Write the OTA package @ota for the signed binary image @signed.

    Every OTA package is written here, so the update tools and the bundle tool
    produce the same encrypted image. Encryption validates the signed input and
    holds all OTA KEK access.
    """
    encrypt = [sys.executable, DFU_ENCRYPT, signed, "-o", ota]
    if keyset is not None:
        encrypt += ["--keyset", keyset]
    if kek_gen is not None:
        encrypt += ["--kek-gen", str(kek_gen)]
    log.info("encrypting %s", role.upper())
    run(encrypt, f"encrypting {role.upper()}", capture=not verbose, execute=not dry_run)
    return ota


def enter_dfu_mode(args) -> bool:
    """Request one-shot DFU mode and reset into it.

    BOOTMODE_SET persists through reset in GPREGRET. Older UROT versions may
    reject the command. In that case the command still resets normally and the
    flow waits for another DFU trigger.
    """
    log.info("requesting DFU boot and reset")
    armed = attempt(
        adac_cli.argv(args.serial_number, "boot-mode", "dfu"),
        capture=not args.verbose,
        execute=not (args.dry_run or args.plan_only),
    )
    if not armed:
        log.warning(
            "warning: UROT refused the DFU boot request. Waiting for IronSide DFU "
            "to start through another trigger."
        )
    return armed


def await_agent(args, armed: bool):
    """Wait for the IronSide DFU SMP port.

    Resolve the port for each cycle because resets re-enumerate the probe's virtual serial ports.
    Manual entry receives the longer operator timeout.
    """
    if args.dry_run or args.plan_only:
        return args.port or "<the IronSide DFU SMP port>"
    if armed:
        log.info("waiting for IronSide DFU's SMP port")
        timeout = ENTRY_TIMEOUT
    else:
        timeout = MANUAL_ENTRY_TIMEOUT
        log.info("waiting up to %.0f seconds for IronSide DFU", timeout)
    return serial_port.wait(
        serial_port.SMP,
        port=args.port,
        serial=args.serial_number,
        timeout=timeout,
        announce=lambda p: log.debug("  %s appeared, settling", p),
        warn=log.warning,
    )


def upload_and_apply(role, ota, args, target):
    """Enter DFU, upload an image, and reset to apply it.

    Candidate targets require a copy delay. In-place targets do not.
    """
    armed = not args.skip_adac_bootmode_set and enter_dfu_mode(args)
    port = await_agent(args, armed)

    # dfu_upload.py wraps nrfutil mcu-manager serial image-upload.
    upload = [
        sys.executable,
        DFU_UPLOAD,
        ota,
        "--port",
        port,
        "--baud",
        str(SMP_BAUD),
        "--target",
        target,
        "--reset",
    ]
    size = ota.stat().st_size if ota.is_file() else 0
    log.info("uploading %s%s", role.upper(), f" ({size / 1000:.0f} kB)" if size else "")
    run(
        upload,
        f"uploading {role.upper()}",
        capture=not args.verbose,
        execute=not (args.dry_run or args.plan_only),
    )

    if args.dry_run or args.plan_only:
        return
    if target == IN_PLACE_TARGET:
        log.debug("%s was written in place: no candidate copy to wait out", role.upper())
        return
    log.info("waiting for %s update to apply", role.upper())
    time.sleep(APPLY_SETTLE)


def validate_args(args, ap, *, bundle: bool = False):
    """Validate option combinations before starting work."""
    ota_dir = getattr(args, "ota_dir", False)
    if args.ota_image or ota_dir:
        if args.keyset is not None or args.kek_gen is not None:
            ap.error(
                "--ota-image and --ota-dir upload prebuilt OTA images; they "
                "take no --keyset or --kek-gen"
            )
    if bundle:
        path = args.bundle
        if path is None:
            ap.error("give ZIP|DIR (or --ite-bundle)")
        if args.ota_image or ota_dir:
            if args.ota_image:
                if not path.is_file():
                    ap.error(
                        f"{path} is not a file (--ota-image expects one encrypted {mcuboot_encrypt.OTA_ARTIFACT_SUFFIX} file)"
                    )
            if ota_dir:
                if not path.is_dir():
                    ap.error(
                        f"{path} is not a directory (--ota-dir expects a directory of {mcuboot_encrypt.OTA_ARTIFACT_SUFFIX} files)"
                    )
            if args.ota_image:
                if not args.component or len(args.component) != 1:
                    ap.error(
                        "--ota-image uploads one update artifact; give exactly one --component"
                    )
    else:
        path = args.image
        if args.ota_image and not path.is_file():
            ap.error(f"no such OTA image: {path}")
        if ota_dir:
            ap.error(f"{prog()} does not take --ota-dir")


def add_arguments(
    ap: argparse.ArgumentParser,
    out_dir_help: str,
    dry_run_help: str,
    signing: bool = True,
    *,
    ota_dir: bool = False,
):
    """Add shared update-flow options.

    Signing options are omitted when input is already signed. Keyset remains
    available because OTA encryption always needs it.
    """
    input_format = ap.add_argument_group("input format")
    ota_mode = input_format.add_mutually_exclusive_group()
    ota_mode.add_argument(
        "--ota-image",
        action="store_true",
        help=f"treat the input as one encrypted {mcuboot_encrypt.OTA_ARTIFACT_SUFFIX} file and skip encryption",
    )
    if ota_dir:
        ota_mode.add_argument(
            "--ota-dir",
            action="store_true",
            help=f"treat the input as a directory of encrypted {mcuboot_encrypt.OTA_ARTIFACT_SUFFIX} files and skip encryption",
        )

    encryption = ap.add_argument_group("OTA encryption")
    encryption.add_argument(
        "--keyset",
        dest="keyset",
        metavar="SPEC",
        help=(
            "key source for signing and encryption as "
            if signing
            else "key source for encryption as "
        )
        + "keyring:NAME or dir:PATH (default: only keyset in the OS keyring)",
    )
    if signing:
        signing_options = ap.add_argument_group("firmware signing")
        signing_options.add_argument(
            "--gen",
            type=int,
            choices=(0, 1, 2),
            help="signing key generation (default: 0). Installing the update revokes "
            "older generations.",
        )
    encryption.add_argument(
        "--kek-gen",
        type=int,
        choices=(0, 1, 2),
        help="OTA KEK generation (default: 0). A successful upload revokes older generations.",
    )
    if signing:
        signing_options.add_argument(
            "--image-version",
            metavar="V",
            help="version to sign. This also sets the security counter unless "
            "--security-counter is used.",
        )
        signing_options.add_argument(
            "--security-counter",
            type=int,
            metavar="N",
            help="security counter to sign. Accepted increases cannot be reversed.",
        )
    encryption.add_argument("-o", "--out-dir", type=Path, help=out_dir_help)

    device = ap.add_argument_group("device connection")
    device.add_argument(
        "--port",
        metavar="DEV",
        help="DFU SMP serial port (default: virtual serial port 0 of the selected probe; "
        "--serial-number lets the tool wait for the probe after a reset)",
    )
    device.add_argument(
        "--serial-number",
        type=int,
        default=None,
        help="J-Link serial number used to select the board and SMP port",
    )

    dfu_entry = ap.add_argument_group("DFU entry over ADAC")
    dfu_entry.add_argument(
        "--skip-adac-bootmode-set",
        action="store_true",
        help="do not request DFU entry over ADAC (wait for another entry trigger)",
    )

    execution = ap.add_argument_group("execution")
    execution.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="print commands and live output",
    )
    run_mode = execution.add_mutually_exclusive_group()
    run_mode.add_argument(
        "--plan-only",
        action="store_true",
        help="build artifacts and print device actions without running them",
    )
    run_mode.add_argument("--dry-run", action="store_true", help=dry_run_help)
    return ap
