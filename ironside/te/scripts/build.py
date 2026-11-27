#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Build an IronSide Integration Kit sample with CMake and Ninja.

Build output is stored under samples/build/<sample> and reused for incremental
builds. Pass --clean to remove the sample build directory before configuring.

The App is built for the configuration of one IronSide bundle: --bundle exports
it (ironside_bundle.py export-config --app-header), and --app-config names a header
exported before. Either way the header lands in the build directory, under
bundle-config/.

Dependencies: see samples/PREREQUISITES.md. Override paths with the env vars
below.

Env overrides, each with its default:
  IRONSIDE_INTEGRATION_KIT_GCC            path/name of the Arm GCC (arm-none-eabi-gcc)
  IRONSIDE_INTEGRATION_KIT_RISCV_GCC      path/name of a RISC-V GCC (searched on PATH; only
                                          samples that run code on a VPR core need one)
  IRONSIDE_INTEGRATION_KIT_NRFX           path to the nrfx checkout (<sdk>/samples/deps/nrfx)
  IRONSIDE_INTEGRATION_KIT_CMSIS          path to the CMSIS-Core include directory
                                          (<sdk>/samples/deps/CMSIS_6/CMSIS/Core/Include)
  IRONSIDE_INTEGRATION_KIT_CMAKE          cmake executable (cmake)
  IRONSIDE_INTEGRATION_KIT_NINJA          ninja executable (ninja)
  IRONSIDE_INTEGRATION_KIT_SAMPLES_BUILD  sample build root (<sdk>/samples/build)
"""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from lib import app_config, parts  # noqa: E402

SDK = Path(__file__).resolve().parent.parent
SAMPLES = SDK / "samples"
GCC = os.environ.get("IRONSIDE_INTEGRATION_KIT_GCC", "arm-none-eabi-gcc")
# Optional second toolchain: passed through only when set, so the samples' own
# PATH search stays in charge otherwise.
RISCV_GCC = os.environ.get("IRONSIDE_INTEGRATION_KIT_RISCV_GCC", "")
NRFX = Path(os.environ.get("IRONSIDE_INTEGRATION_KIT_NRFX", SAMPLES / "deps" / "nrfx")).resolve()
CMSIS = Path(
    os.environ.get("IRONSIDE_INTEGRATION_KIT_CMSIS",
                   SAMPLES / "deps" / "CMSIS_6" / "CMSIS" / "Core" / "Include")
).resolve()
CMAKE = os.environ.get("IRONSIDE_INTEGRATION_KIT_CMAKE", "cmake")
NINJA = os.environ.get("IRONSIDE_INTEGRATION_KIT_NINJA", "ninja")
TOOLCHAIN = SAMPLES / "cmake" / "arm-none-eabi.cmake"
SUPPORTED_SOCS = tuple(parts.PARTS)


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Build an IronSide Integration Kit sample or test with CMake and Ninja. "
        "The sample is selected by directory name. By default, output "
        "is written to samples/build/<sample>/."
    )
    ap.add_argument(
        "sample",
        nargs="?",
        default="hello_world",
        help="directory name under samples/ (default: hello_world)",
    )
    ap.add_argument(
        "--soc",
        required=True,
        choices=SUPPORTED_SOCS,
        help="target SoC",
    )
    config = ap.add_mutually_exclusive_group(required=True)
    config.add_argument(
        "--bundle",
        type=Path,
        metavar="ZIP|DIR",
        help="IronSide bundle the App runs under; its configuration places the App "
        "in NVM and RAM",
    )
    config.add_argument(
        "--app-config",
        type=Path,
        metavar="FILE",
        help=f"{app_config.HEADER_NAME} exported from that bundle with "
        "ironside_bundle.py export-config --app-header",
    )
    ap.add_argument(
        "--clean", action="store_true", help="delete the sample build directory before configuring"
    )
    args = ap.parse_args()
    if args.app_config is not None and args.app_config.name != app_config.HEADER_NAME:
        ap.error(f"--app-config must name a file called {app_config.HEADER_NAME}")
    sample = args.sample
    # Tests are available in development workspaces but not release bundles.
    candidates = (SAMPLES / sample, SDK.parent / "test" / "tests" / sample)
    sample_dir = next((d for d in candidates if d.is_dir()), None)
    if sample_dir is None:
        sys.exit(f"Sample or test not found: {sample}")

    build_root = Path(os.environ.get("IRONSIDE_INTEGRATION_KIT_SAMPLES_BUILD",
                                     SAMPLES / "build")).resolve()
    build_dir = build_root / sample
    print(f"source: {sample_dir}")
    print(f"build directory: {build_dir}")

    if args.clean and build_dir.exists():
        print(f"removing {build_dir}")
        shutil.rmtree(build_dir)

    # Build trees are shared across SoCs. Recreate the tree when the target changes
    # because object files and the generated linker script survive reconfiguration.
    stamp = build_dir / ".soc"
    if build_dir.exists():
        previous = stamp.read_text().strip() if stamp.is_file() else None
        if previous != args.soc:
            print(f"removing {build_dir}: SoC changed from {previous or 'unknown'} to {args.soc}")
            shutil.rmtree(build_dir)

    # CMake refuses a cache generated from another source directory, as happens when one
    # build root holds samples built from the repository and from a bundle copy.
    cache = build_dir / "CMakeCache.txt"
    if cache.is_file():
        cached_source = next(
            (
                line.partition("=")[2].strip()
                for line in cache.read_text(errors="replace").splitlines()
                if line.startswith("CMAKE_HOME_DIRECTORY:")
            ),
            None,
        )
        if cached_source != str(SAMPLES):
            print(f"removing {build_dir}: cache source is {cached_source or 'unknown'}")
            shutil.rmtree(build_dir)

    configure = [
        CMAKE,
        "-G",
        "Ninja",
        "-S",
        str(SAMPLES),
        "-B",
        str(build_dir),
        f"-DCMAKE_TOOLCHAIN_FILE={TOOLCHAIN}",
        f"-DSAMPLE={sample}",
        f"-DIRONSIDE_INTEGRATION_KIT_GCC={GCC}",
        f"-DIRONSIDE_INTEGRATION_KIT_NRFX={NRFX}",
        f"-DIRONSIDE_INTEGRATION_KIT_CMSIS={CMSIS}",
        f"-DIRONSIDE_INTEGRATION_KIT_SOC={args.soc}",
    ]
    # The header the App is compiled with sits in the build tree, where sign.py
    # finds it beside the image. It is rewritten only when it changes, so an
    # unchanged configuration rebuilds nothing.
    header = build_dir / app_config.BUILD_SUBDIR / app_config.HEADER_NAME
    if args.bundle is not None:
        subprocess.run(
            [sys.executable, str(Path(__file__).resolve().parent / "ironside_bundle.py"),
             "export-config", str(args.bundle), "--app-header", str(header)],
            check=True,
        )
    else:
        if not args.app_config.is_file():
            sys.exit(f"build.py: {args.app_config} not found")
        text = args.app_config.read_text()
        if not header.is_file() or header.read_text() != text:
            header.parent.mkdir(parents=True, exist_ok=True)
            header.write_text(text)
    configure.append(f"-DIRONSIDE_BUNDLE_CONFIG_DIR={header.parent}")
    if RISCV_GCC:
        configure.append(f"-DIRONSIDE_INTEGRATION_KIT_RISCV_GCC={RISCV_GCC}")
    print("configure:", " ".join(configure))
    subprocess.run(configure, check=True)
    stamp.write_text(f"{args.soc}\n")

    build = [NINJA, "-C", str(build_dir)]
    print("build:", " ".join(build))
    subprocess.run(build, check=True)

    print(f"wrote {build_dir / f'{sample}.hex'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
