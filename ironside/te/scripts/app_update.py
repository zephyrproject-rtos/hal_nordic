#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Install a signed application on a device running IronSide TE.

The input is produced by sign.py. This command encrypts it with the OTA KEK,
boots IronSide DFU, uploads the artifact, and resets the device. It does not sign
or change the image metadata.

HEX input is converted to the firmware byte stream. Any UICR data is omitted
because UICR is write-once and is provisioned during installation.

The default target writes directly to the application slot. The candidate target
stages the image for signature and rollback-counter verification before UROT
replaces the installed application. A candidate must fit the candidate partition.

A successful upload does not confirm that the new application booted because its
version is not readable from the host.
"""

import argparse
import logging as log
import sys
import tempfile
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPTS))
from lib import mcuboot_image as mci  # noqa: E402
from lib import mcuboot_encrypt  # noqa: E402
from lib import serial_port, update_flow  # noqa: E402
from lib.update_flow import fail  # noqa: E402

# The input must already be signed for the application role.
ROLE = "app"

TARGETS = ("app", "candidate")
TARGET_DEFAULT = "app"


def wrong_role(image: Path) -> str:
    """Return why image cannot be installed as an application, or an empty string."""
    try:
        tlv = mci.find_protected_tlv(image, mci.TLV_IROT_IMAGE_ID)
    except (mci.SignError, OSError) as exc:
        log.debug("could not read the image id from %s: %s", image, exc)
        return ""
    if tlv is None or not tlv:
        return ""
    was = next((r for r, v in mci.IMAGE_ID.items() if v == tlv[0]), None)
    if was == ROLE:
        return ""
    named = f"the {was.upper()} role" if was else f"image id {tlv[0]}"
    return (
        f"{image} is signed as {named}, not as an App. Use sign.py "
        "--role app or install the component with ironside_update.py."
    )


def to_binary(image: Path, out_dir: Path) -> Path:
    """Return the upload byte stream, converting HEX input and omitting UICR."""
    if image.suffix.lower() != ".hex":
        return image
    data, base = mci.read_image(image)
    out = out_dir / f"{ROLE}.signed.bin"
    mci.write_image(out, data, base)
    log.info("converted %s to a %d-byte binary image", image.name, len(data))
    return out


def resolve_output_paths(out: Path | None, image: Path) -> tuple[Path, Path]:
    """Return the work directory and encrypted artifact path."""
    if out is None:
        work_dir = Path(tempfile.mkdtemp(prefix="app-update-"))
        return work_dir, work_dir / f"{image.stem}{mcuboot_encrypt.OTA_ARTIFACT_SUFFIX}"
    if mcuboot_encrypt.is_ota_artifact(out):
        out.parent.mkdir(parents=True, exist_ok=True)
        return out.parent, out
    out.mkdir(parents=True, exist_ok=True)
    return out, out / f"{image.stem}{mcuboot_encrypt.OTA_ARTIFACT_SUFFIX}"


def describe(image: Path) -> str:
    """Return the signed image version and security counter when readable."""
    bits = []
    try:
        data, _ = mci.read_image(image)
        bits.append("v" + mci.version_display(mci.parse_version_header(data)))
    except Exception as exc:
        log.debug("could not read the version from %s: %s", image, exc)
    try:
        # security_counter_of() only accepts images that have not been signed.
        tlv = mci.find_protected_tlv(image, mci.TLV_SEC_CNT)
        if tlv is not None and len(tlv) == 4:
            bits.append(f"counter 0x{int.from_bytes(tlv, 'little'):08x}")
    except Exception as exc:
        log.debug("could not read the security counter from %s: %s", image, exc)
    return ", ".join(bits)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        description="Install a signed App through IronSide DFU. Pass a signed "
        "HEX or binary image to encrypt and upload it. Use "
        "--ota-image to upload an existing update artifact. The app target "
        "writes in place. The candidate target verifies the update "
        "before replacing the installed App."
    )
    ap.add_argument(
        "image",
        type=Path,
        help=f"signed App as HEX or binary, or an encrypted {mcuboot_encrypt.OTA_ARTIFACT_SUFFIX} file with --ota-image",
    )
    update = ap.add_argument_group("App update")
    update.add_argument(
        "--target",
        choices=TARGETS,
        default=TARGET_DEFAULT,
        help=f"upload destination (default: {TARGET_DEFAULT}). app writes in "
        "place. candidate stages the update for verification.",
    )
    update_flow.add_arguments(
        ap,
        out_dir_help="OTA output path or directory (default: temporary directory)",
        dry_run_help="print commands without encrypting or accessing the device",
        ota_dir=False,
        signing=False,
    )
    args = ap.parse_args(argv)

    update_flow.validate_args(args, ap)

    # Keep the update transcript free of log metadata.
    log.basicConfig(format="%(message)s", level=log.DEBUG if args.verbose else log.INFO)

    if args.ota_image:
        ota = args.image
        log.info("OTA image: %s", ota)
        why = update_flow.not_installed(args)
        if why:
            return fail(why)
        update_flow.upload_and_apply(ROLE, ota, args, args.target)
        log.info(
            "%s",
            "dry run complete"
            if args.dry_run
            else "plan complete, no device changes"
            if args.plan_only
            else f"uploaded App to {args.target}",
        )
        if not (args.dry_run or args.plan_only):
            log.debug("the running version is not host-readable; nothing was verified")
        return 0

    if not args.image.is_file():
        return fail(f"Image not found: {args.image}")
    why = wrong_role(args.image)
    if why:
        return fail(why)

    what = describe(args.image)
    log.info("App: %s%s", args.image, f" ({what})" if what else "")

    out_dir, ota = resolve_output_paths(args.out_dir, args.image)
    ota = update_flow.encrypt_image(
        ROLE,
        to_binary(args.image, out_dir),
        ota,
        keyset=args.keyset,
        kek_gen=args.kek_gen,
        verbose=args.verbose,
        dry_run=args.dry_run,
    )

    why = update_flow.not_installed(args)
    if why:
        return fail(why)

    update_flow.upload_and_apply(ROLE, ota, args, args.target)

    log.info(
        "%s",
        "dry run complete"
        if args.dry_run
        else "plan complete, no device changes"
        if args.plan_only
        else f"uploaded App to {args.target}",
    )
    if not (args.dry_run or args.plan_only):
        log.debug("the running version is not host-readable; nothing was verified")
    log.debug("artifacts: %s", out_dir)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except serial_port.SerialPortError as exc:
        sys.exit(fail(str(exc)))
