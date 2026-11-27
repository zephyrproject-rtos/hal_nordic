#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Update UROT or IronSide DFU on an installed device.

Prepare signatures and image configuration with ironside_bundle.py. This command
encrypts each selected image with the OTA KEK, boots IronSide DFU, uploads
the candidate, and resets the device to apply it.

The candidate partition holds one image, so multiple components are updated in
separate cycles. UROT is updated before DFU. The boot-mode request is rearmed and
the SMP port is resolved for each cycle because apply resets can re-enumerate the
serial port.

IROT is immutable and cannot be updated. BLE DFU bundles are unsupported because
the upload transport is serial SMP.

A successful upload does not confirm the installed version because image versions
are not readable from the host.
"""

import argparse
import logging as log
import sys
import tempfile
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPTS))
from lib import fw_bundle, serial_port, update_flow  # noqa: E402
from lib.update_flow import fail, signed_bin  # noqa: E402

# Preserve boot-chain order. IROT is immutable after UICR.BOOTCONF is written.
UPDATE_ROLES = tuple(role for role in fw_bundle.IMAGE_ROLES if role != "irot")

# dfu_upload.py wraps nrfutil mcu-manager serial image-upload.
UPLOADABLE_DFU = "ironside-te-dfu-serial"

# The signed image ID determines which RoT consumes the candidate.
UPLOAD_TARGET = "candidate"


def unusable(release, roles) -> str:
    """Return why release cannot supply roles, or an empty string."""
    absent = [role for role in roles if release.image(role) is None]
    if absent:
        return f"has no {fw_bundle.identifiers(absent)} component"
    dfu = release.components.get("dfu")
    if "dfu" in roles and dfu is not None and dfu.ident != UPLOADABLE_DFU:
        return (
            f"uses the unsupported {dfu.ident} transport. This command "
            f"supports {UPLOADABLE_DFU}. Use --component urot to update "
            "UROT only."
        )
    return ""


def unsigned(release, roles) -> str:
    """Return why release cannot supply signed roles, or an empty string."""
    missing = [
        role for role in roles if role in release.components and release.signed(role) is None
    ]
    if missing:
        return (
            f"has no signed {fw_bundle.identifiers(missing)} image. Sign it "
            "with ironside_bundle.py sign-component first."
        )
    return ""


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        description="Update UROT or IronSide DFU through IronSide DFU. Pass a "
        "prepared IronSide bundle to encrypt and upload its signed "
        "components. Use --ota-image or --ota-dir to upload existing "
        "update artifacts."
    )
    ap.add_argument(
        "bundle",
        type=Path,
        nargs="?",
        default=None,
        metavar="ZIP|DIR",
        help="prepared IronSide bundle, an update artifact with --ota-image, or an "
        "update artifact directory with --ota-dir",
    )
    # Retain the released spelling for compatibility.
    ap.add_argument(
        "--ite-bundle",
        dest="ite_bundle",
        type=Path,
        metavar="ZIP|DIR",
        help=argparse.SUPPRESS,
    )
    update = ap.add_argument_group("firmware update")
    update.add_argument(
        "--component",
        action="append",
        choices=UPDATE_ROLES,
        metavar="{{{}}}".format(",".join(UPDATE_ROLES)),
        help="component to update (may be repeated, default: all available). "
        "Components update in UROT then DFU order.",
    )
    update_flow.add_arguments(
        ap,
        out_dir_help="directory for update artifacts (default: temporary directory)",
        dry_run_help="print commands without encrypting or accessing the device",
        ota_dir=True,
        signing=False,
    )
    args = ap.parse_args(argv)

    if args.ite_bundle is not None:
        if args.bundle is not None:
            ap.error("IronSide bundle was given more than once")
        args.bundle = args.ite_bundle

    update_flow.validate_args(args, ap, bundle=True)

    # Keep the update transcript free of log metadata.
    log.basicConfig(format="%(message)s", level=log.DEBUG if args.verbose else log.INFO)

    out_dir = args.out_dir or Path(tempfile.mkdtemp(prefix="ironside-update-"))
    out_dir.mkdir(parents=True, exist_ok=True)

    if args.ota_image or args.ota_dir:
        selected = (
            [role for role in UPDATE_ROLES if role in set(args.component)]
            if args.component
            else list(UPDATE_ROLES)
        )
        if args.ota_image:
            role = selected[0]
            ota = args.bundle
            log.info("OTA image: %s (%s)", ota, role.upper())
            why = update_flow.not_installed(args)
            if why:
                return fail(why)
            update_flow.upload_and_apply(role, ota, args, UPLOAD_TARGET)
            log.info(
                "%s",
                "dry run complete"
                if args.dry_run
                else "plan complete, no device changes"
                if args.plan_only
                else f"uploaded {role.upper()}",
            )
        else:
            log.info("update artifact directory: %s", args.bundle)
            log.info("components: %s", ", ".join(r.upper() for r in selected))
            why = update_flow.not_installed(args)
            if why:
                return fail(why)
            for role in selected:
                try:
                    ota = fw_bundle.ota_in_dir(args.bundle, role)
                except fw_bundle.BundleError as exc:
                    return fail(str(exc))
                update_flow.upload_and_apply(role, ota, args, UPLOAD_TARGET)
            log.info(
                "%s",
                "dry run complete"
                if args.dry_run
                else "plan complete, no device changes"
                if args.plan_only
                else "uploaded " + ", ".join(r.upper() for r in selected),
            )
        if not (args.dry_run or args.plan_only):
            log.debug("the applied versions are not host-readable; nothing was verified")
        return 0

    log.debug("unpacking %s", args.bundle)
    try:
        release = fw_bundle.read(args.bundle.resolve(), out_dir / "components", UPDATE_ROLES)
    except fw_bundle.BundleError as exc:
        return fail(str(exc))

    # Apply components in boot-chain order regardless of argument order.
    present = [role for role in UPDATE_ROLES if release.image(role) is not None]
    selected = (
        [role for role in UPDATE_ROLES if role in set(args.component)]
        if args.component
        else present
    )
    if not selected:
        return fail(
            f"{args.bundle} has no updatable component ({fw_bundle.identifiers(UPDATE_ROLES)})"
        )
    why = unusable(release, selected)
    if why:
        return fail(f"{args.bundle} {why}")
    why = unsigned(release, selected)
    if why:
        return fail(f"{args.bundle} {why}")
    log.info("bundle: %s", args.bundle)
    log.debug("%s: %s", args.bundle.name, release.summary)
    log.info("components: %s", ", ".join(r.upper() for r in selected))

    why = update_flow.not_installed(args)
    if why:
        return fail(why)

    staged = {
        role: update_flow.encrypt_image(
            role,
            signed_bin(role, release.signed(role), out_dir, dry_run=args.dry_run),
            out_dir / release.components[role].ota_name,
            keyset=args.keyset,
            kek_gen=args.kek_gen,
            verbose=args.verbose,
            dry_run=args.dry_run,
        )
        for role in selected
    }

    for role in selected:
        update_flow.upload_and_apply(role, staged[role], args, UPLOAD_TARGET)

    log.info(
        "%s",
        "dry run complete"
        if args.dry_run
        else "plan complete, no device changes"
        if args.plan_only
        else "uploaded " + ", ".join(r.upper() for r in selected),
    )
    if not (args.dry_run or args.plan_only):
        log.debug("the applied versions are not host-readable; nothing was verified")
    log.debug("artifacts: %s", out_dir)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except serial_port.SerialPortError as exc:
        sys.exit(fail(str(exc)))
