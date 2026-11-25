#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Build a programmable HEX file from an IronSide bundle."""

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib.python_version import require_python  # noqa: E402

require_python()
from lib import fw_bundle, mcuboot_image, uicr  # noqa: E402
from lib import parts as soc_parts  # noqa: E402
from lib import prov_abi as abi  # noqa: E402

INTERNAL = Path(__file__).resolve().parent
MERGEHEX = INTERNAL / "mergehex.py"

UNSIGNED_ROLES = ("irot",)
SIGNED_ROLES = ("urot", "dfu", "app")
ALL_ROLES = UNSIGNED_ROLES + SIGNED_ROLES

LCS_BUNDLES = {
    "assembly-and-test": {
        "required": ("irot", "urot"),
        "allowed": ALL_ROLES,
        "base_conf": True,
        "otp": True,
        "prov": fw_bundle.PROV_SECURE,
    },
    "prot-provisioning": {
        "required": (),
        "allowed": ("app", "dfu"),
        "base_conf": False,
        "otp": False,
        "prov": fw_bundle.PROV_NONSECURE,
    },
}

DEFAULT_LCS = "assembly-and-test"


def image_arg(arg):
    """Parse an --image ROLE=PATH value."""
    name, sep, path = arg.partition("=")
    if not sep or not path:
        raise argparse.ArgumentTypeError(f"expected ROLE=PATH, got {arg!r}")
    if name not in ALL_ROLES:
        raise argparse.ArgumentTypeError(
            f"role must be one of {', '.join(ALL_ROLES)}, got {name!r}"
        )
    return (name, Path(path))


def run(cmd, dry_run=False):
    """Run a command or print it during a dry run."""
    if dry_run:
        print("  $ " + " ".join(str(c) for c in cmd))
        return
    subprocess.run([str(c) for c in cmd], check=True)


def _is_signed(path: Path) -> bool:
    """Return whether a file carries an IronSide image signature, alone or hybrid."""
    try:
        data, _ = mcuboot_image.read_image(path)
        _, sig = mcuboot_image.split_signed(data)
    except mcuboot_image.SignError:
        return False
    return sig is not None


def _merge_path(role: str, path: Path, signed: Path | None) -> Path:
    """Return the HEX file to merge for a role."""
    if role in UNSIGNED_ROLES:
        return path
    if signed is not None:
        return signed
    if not _is_signed(path):
        raise ValueError(
            f"{role} image is not signed: {path}. Sign it with sign.py or "
            "ironside_bundle.py sign-component."
        )
    return path


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        description="Create a programmable HEX file from an IronSide bundle."
    )
    ap.add_argument(
        "firmware_bundle",
        nargs="?",
        type=Path,
        metavar="ZIP|DIR",
        help="prepared IronSide bundle as a ZIP file or directory",
    )
    ap.add_argument(
        "--lcs",
        default=DEFAULT_LCS,
        choices=sorted(LCS_BUNDLES),
        help="lifecycle state to build for (default: %(default)s)",
    )
    ap.add_argument(
        "-o",
        "--out-dir",
        type=Path,
        help="directory for extracted files and intermediate output (default: temporary directory)",
    )
    ap.add_argument(
        "--bundle", type=Path, help="output HEX path (default: <out-dir>/<lcs>_bundle.hex)"
    )
    ap.add_argument(
        "--image",
        action="append",
        default=[],
        type=image_arg,
        metavar="ROLE=PATH",
        help=f"override an image with ROLE=PATH (supported roles: "
        f"{', '.join(ALL_ROLES)}); may be repeated",
    )
    ap.add_argument(
        "--prov-firmware", type=Path, metavar="HEX", help="override the provisioning firmware"
    )
    ap.add_argument(
        "--data",
        action="append",
        default=[],
        type=Path,
        metavar="HEX",
        help="additional HEX file to merge (may be repeated)",
    )
    ap.add_argument(
        "--prov-only",
        action="store_true",
        help="merge only the provisioning firmware and the batch, leaving the "
        "bundle's firmware images out",
    )
    ap.add_argument(
        "--batch",
        type=Path,
        metavar="FILE",
        help="binary provisioning batch to place at the command address",
    )
    ap.add_argument(
        "--prov-route",
        choices=(fw_bundle.ROUTE_SINGLE_PHASE, fw_bundle.ROUTE_TWO_PHASE),
        help="use the provisioning firmware and batch carried for the selected "
        "production route",
    )
    ap.add_argument(
        "--soc",
        choices=sorted(abi.LAYOUTS),
        help="SoC the command address belongs to (default: the one the bundle declares)",
    )
    ap.add_argument("--dry-run", action="store_true", help="print commands without running them")
    args = ap.parse_args(argv)

    if args.out_dir is None:
        args.out_dir = Path(tempfile.mkdtemp(prefix="ironside-prov-artifacts-"))

    spec = LCS_BUNDLES[args.lcs]
    images = dict(args.image)
    if len(images) != len(args.image):
        return _fail("the same --image role was given twice")

    signed_paths: dict[str, Path | None] = {}
    bundle_read = None
    if args.firmware_bundle is not None:
        explicit = set(images)
        wanted = [] if args.prov_only else [r for r in spec["allowed"] if r not in images]
        if args.prov_firmware is None:
            wanted.append(spec["prov"])
        try:
            bundle_read = fw_bundle.read(
                args.firmware_bundle.resolve(), args.out_dir / "components", wanted
            )
        except fw_bundle.BundleError as exc:
            return _fail(str(exc))
        # The recipe says which images an artifact holds. fw_bundle.IMAGE_ROLES
        # leaves out the App.
        carried = {
            role: bundle_read.components[role].image
            for role in spec["allowed"]
            if role in bundle_read.components
        }
        images = {**carried, **images}
        for role in spec["allowed"]:
            if role in explicit:
                signed_paths[role] = None
            elif role in bundle_read.components:
                signed_paths[role] = bundle_read.signed(role)
        if args.prov_firmware is None and args.prov_route is not None:
            # The carried member holds the firmware and its batch together, so
            # this artifact is a merge of bundle members and nothing else.
            args.prov_firmware = bundle_read.prov_hex(spec["prov"], args.prov_route)
            if args.prov_firmware is None:
                return _fail(
                    f"{args.firmware_bundle} carries no provisioning firmware "
                    f"for the {args.prov_route} route. Run ironside_bundle.py "
                    f"update-config --ap-closure to write it."
                )
        if args.prov_firmware is None:
            args.prov_firmware = bundle_read.image(spec["prov"])
            if args.prov_firmware is None:
                return _fail(
                    f"{args.firmware_bundle} carries no "
                    f"{fw_bundle.identifiers([spec['prov']])} component "
                    f"for --lcs {args.lcs}. Pass --prov-firmware HEX."
                )

    if args.batch is not None and args.prov_route is not None:
        return _fail(
            "--batch and --prov-route both supply provisioning commands. Pass only one."
        )
    if args.prov_only and images:
        return _fail(
            f"--prov-only cannot be combined with --image "
            f"({', '.join(sorted(images))} given). Remove --image or omit --prov-only."
        )
    unknown = [r for r in images if r not in spec["allowed"]]
    if unknown:
        return _fail(
            f"--lcs {args.lcs} does not accept "
            f"{', '.join(sorted(unknown))} "
            f"(allowed: {', '.join(spec['allowed']) or 'none'})"
        )
    # --prov-only merges no firmware image, so the recipe's image requirements do
    # not apply - what it writes is the provisioning firmware and its batch. That
    # is the whole of the two-phase second phase and of the developer protect
    # flows, all of which run on a part whose images are already programmed.
    absent = [] if args.prov_only else [r for r in spec["required"] if r not in images]
    if absent:
        missing = ", ".join(f"{r}=<hex>" for r in absent)
        if args.firmware_bundle is not None:
            return _fail(
                f"--lcs {args.lcs} requires --image {missing}. "
                f"{args.firmware_bundle} has no "
                f"{fw_bundle.identifiers(absent)} component."
            )
        return _fail(f"--lcs {args.lcs} requires --image {missing}")
    for role, path in images.items():
        if not path.is_file():
            return _fail(f"no such {role} image: {path}")
    if args.prov_firmware is None:
        return _fail("--prov-firmware is required when no IronSide bundle is given")
    if not args.prov_firmware.is_file():
        return _fail(f"no such provisioning firmware: {args.prov_firmware}")
    for extra in args.data:
        if not extra.is_file():
            return _fail(f"no such --data hex: {extra}")

    if spec["base_conf"] and images and not args.dry_run:
        try:
            carried = uicr.decode(uicr.files_reader(images.values()))["region"]
        except uicr.UicrError as exc:
            return _fail(str(exc))
        if not carried.get("keys"):
            return _fail(
                "Bundle has no provisioned keys. Run ironside_bundle.py update-keys first."
            )

    parts = []
    try:
        for role in ALL_ROLES:
            if role not in images:
                continue
            src = _merge_path(role, images[role], signed_paths.get(role))
            if not spec["otp"]:
                otp = _otp_addresses(src)
                if otp:
                    return _fail(
                        f"{role} image {src} writes UICR.OTP at 0x{otp[0]:08X}, which "
                        f"--lcs {args.lcs} does not program. UICR.OTP data goes in the "
                        f"--lcs assembly-and-test artifact; write later data with "
                        f"ironside_platform_mem_write()."
                    )
            parts.append(src)
    except ValueError as exc:
        return _fail(str(exc))

    if not args.dry_run:
        args.out_dir.mkdir(parents=True, exist_ok=True)
    out_bundle = args.bundle or (args.out_dir / f"{args.lcs}_bundle.hex")

    parts.append(args.prov_firmware)
    parts += args.data

    secret = False
    if args.batch is not None or args.prov_route is not None:
        # The command address is part-specific. Refuse a part that conflicts
        # with the bundle because the firmware would not read the batch.
        what = "--batch" if args.batch is not None else "--prov-route"
        declared = bundle_read.soc if bundle_read is not None else None
        soc = args.soc or declared
        if soc is None:
            return _fail(
                f"{what} needs the SoC to place the command address. The bundle "
                f"declares none, so pass --soc."
            )
        if args.soc is not None and declared is not None and args.soc != declared:
            return _fail(f"--soc {args.soc} disagrees with the bundle, which declares {declared}")
        try:
            abi.select_soc(soc)
        except soc_parts.PartError as exc:
            return _fail(str(exc))

    if args.batch is not None:
        if not args.batch.is_file():
            return _fail(f"no such batch file: {args.batch}")
        if args.dry_run:
            print(f"  # batch {args.batch} -> {abi.CMD_ADDR:#010x} ({soc})")
        else:
            staged = args.out_dir / f"{args.batch.stem}.hex"
            secret = _batch_hex(args.batch.read_bytes(), staged)
            parts.append(staged)

    if args.prov_route is not None and not args.dry_run:
        # A carried member brings its batch with it, so secrecy is a property of
        # the bytes rather than of a --batch flag that used to imply them.
        carried_batch = _staged_batch(args.prov_firmware)
        if carried_batch is None:
            return _fail(
                f"{args.prov_firmware} carries no provisioning batch at "
                f"{abi.CMD_ADDR:#010x} ({soc}). Run ironside_bundle.py "
                f"update-config --ap-closure to rewrite it."
            )
        secret = _imports_kek(carried_batch)

    run([sys.executable, MERGEHEX, *parts, "-o", out_bundle], args.dry_run)
    if secret and not args.dry_run:
        # The merged HEX carries the raw OTA KEKs the batch imports.
        Path(out_bundle).chmod(0o600)
    return 0


def _otp_addresses(path: Path) -> list[int]:
    """Return the UICR.OTP byte addresses a HEX image writes, in order."""
    if path.suffix.lower() != ".hex":
        return []
    get = uicr.file_reader(path)
    return [a for a in range(uicr.OTP_BASE, uicr.OTP_BASE + uicr.OTP_SIZE) if get(a) is not None]


def _batch_hex(blob: bytes, out: Path) -> bool:
    """Write @blob as a HEX at the firmware ABI command address.

    Returns whether the batch imports OTA KEKs, which is what makes the merged
    artifact secret.
    """
    from intelhex import IntelHex

    staged = IntelHex()
    for i, byte in enumerate(blob):
        staged[abi.CMD_ADDR + i] = byte
    staged.write_hex_file(str(out))
    return _imports_kek(blob)


def _imports_kek(blob) -> bool:
    """Whether a batch imports OTA KEKs, which is what makes a HEX secret."""
    if not blob:
        return False
    ops = abi.parse_input_batch(bytes(blob))["operations"]
    return any(abi.op_name(op) == "import-fw-kek" for op in ops)


def _staged_batch(hexfile: Path):
    """The batch a HEX carries at the command address, or None."""
    from intelhex import IntelHex

    loaded = IntelHex()
    loaded.loadhex(str(hexfile))
    for start, end in loaded.segments():
        if start == abi.CMD_ADDR:
            return loaded.tobinstr(start=start, end=end - 1)
    return None


def _fail(msg: str) -> int:
    print(f"build_prov_artifacts: {msg}", file=sys.stderr)
    return 2


if __name__ == "__main__":
    try:
        sys.exit(main())
    except uicr.UicrError as exc:
        sys.exit(f"build_prov_artifacts: {exc}")
    except subprocess.CalledProcessError as exc:
        sys.exit(f"build_prov_artifacts: sub-step failed ({exc})")
