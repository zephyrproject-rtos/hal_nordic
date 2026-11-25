#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Package an Intel HEX image and manifest as an IronSide TE component archive."""

from __future__ import annotations

import argparse
import json
import sys
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from lib import component_package, fw_bundle, parts  # noqa: E402


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )

    component = ap.add_argument_group("component")
    component.add_argument(
        "--hex",
        type=Path,
        required=True,
        metavar="HEX",
        help="Intel HEX image to package",
    )
    # Validated by the library rather than by choices, so the component identifier
    # (ironside-te-app) is accepted beside the short form (app), as it is
    # everywhere else a component is named.
    component.add_argument(
        "--role",
        required=True,
        metavar="ROLE",
        help="component role: " + ", ".join(sorted(component_package.ROLE_MAP)),
    )
    component.add_argument(
        "--version",
        required=True,
        metavar="MAJOR.MINOR.PATCH[.PATCH2]",
        help="component version recorded in the manifest and file name",
    )
    component.add_argument(
        "--soc",
        required=True,
        choices=sorted(parts.PARTS),
        metavar="SOC",
        help="SoC the image was built for: " + ", ".join(sorted(parts.PARTS)),
    )
    component.add_argument(
        "--name",
        metavar="NAME",
        help="manifest name (default: the component identifier)",
    )

    execution = ap.add_argument_group("execution")
    # No -v: --version is a required option here, and the two would read as one.
    execution.add_argument(
        "--verbose",
        action="store_true",
        help="list the archive members",
    )

    args = ap.parse_args()

    try:
        package = component_package.create_package(
            args.hex,
            args.role,
            args.version,
            args.soc,
            name=args.name,
        )
    except component_package.ComponentPackageError as exc:
        print(f"create_component_archive: {exc}", file=sys.stderr)
        return 2

    print(f"wrote {package}")
    # Read back what was written rather than restating the inputs, so the report
    # is the package's own manifest.
    with zipfile.ZipFile(package) as zf:
        manifest = json.loads(zf.read(fw_bundle.MANIFEST_MEMBER))
        for field in ("name", "soc", "version"):
            print(f"  {field}: {manifest[field]}")
        if args.verbose:
            for info in zf.infolist():
                print(f"  {info.filename} ({info.file_size} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
