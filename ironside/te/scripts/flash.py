#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Program an Intel HEX image with nrfutil.

Only flash ranges used by the image are erased. The device is pin-reset after
programming so the new image boots; pass --no-reset to leave it as programmed.
Additional arguments are passed to nrfutil device program.
"""

import argparse
import subprocess
import sys

NRFUTIL = "nrfutil"


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        usage="%(prog)s [-h] [--no-reset] IMAGE.hex [NRFUTIL_ARG ...]",
        allow_abbrev=False,
    )
    parser.add_argument("image", metavar="IMAGE.hex", help="Intel HEX image to program")
    parser.add_argument(
        "--no-reset",
        action="store_true",
        help="do not reset the device after programming",
    )
    # Every other argument, such as --serial-number, goes to nrfutil device program.
    args, nrfutil_args = parser.parse_known_args()

    # nrfutil defaults to RESET_NONE, which leaves the new image unbooted. A pin
    # reset needs no debug access, so it also works once the access ports are
    # closed, and unlike RESET_HARD it is not an ADAC-entry request.
    options = ["chip_erase_mode=ERASE_RANGES_TOUCHED_BY_FIRMWARE"]
    if not args.no_reset:
        options.append("reset=RESET_PIN")
    cmd = [
        NRFUTIL,
        "device",
        "program",
        "--firmware",
        args.image,
        "--options",
        ",".join(options),
        *nrfutil_args,
    ]
    print(" ".join(cmd))
    return subprocess.run(cmd).returncode


if __name__ == "__main__":
    sys.exit(main())
