#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Read the IronSide boot report from IronSide DFU over SMP."""

import argparse
import json
import sys

from lib import boot_report, serial_port
from lib.smp_serial import DFU_ID_BOOT_REPORT, GROUP_DFU, SmpError, SmpSerial

SMP_BAUD = 1000000


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--port", help="IronSide DFU SMP serial port (default: the probe's virtual serial port 0)"
    )
    ap.add_argument("--serial-number", help="J-Link serial number of the board to read")
    ap.add_argument(
        "--baud", type=int, default=SMP_BAUD, help="UART baud rate (default: %(default)s)"
    )
    ap.add_argument("--json", action="store_true", help="print the boot report as JSON")
    args = ap.parse_args()

    port = serial_port.resolve(serial_port.SMP, port=args.port, serial=args.serial_number)
    try:
        rsp = fetch_boot_report(port, args.baud)
    except (SmpError, ValueError) as exc:
        sys.exit(f"error: {exc}")

    if args.json:
        print(json.dumps(rsp, sort_keys=True))
        return

    print(boot_report.format_human(rsp))


def fetch_boot_report(port, baud=SMP_BAUD):
    """Return the decoded boot-report map.

    Raises SmpError when the transaction fails and ValueError when the report
    is malformed.
    """
    with SmpSerial.open(port, baud) as smp:
        rsp = smp.read_map(GROUP_DFU, DFU_ID_BOOT_REPORT)
    validate_boot_report(rsp)
    return rsp


def validate_boot_report(rsp):
    """Raise ValueError unless `rsp` is a successful, complete boot report."""
    rc = rsp.get("rc", -1)
    if rc != 0:
        raise ValueError(f"IronSide DFU returned rc={rc}")

    missing = [k for k in boot_report.BOOT_REPORT_KEYS if k not in rsp]
    if missing:
        raise ValueError(f"response missing fields: {', '.join(missing)}")

    for key in boot_report.VERSION_KEYS:
        ver = rsp[key]
        if not isinstance(ver, list) or len(ver) != 4:
            raise ValueError(f"{key} must be a 4-element array, got {ver!r}")
        if any(not isinstance(x, int) or x < 0 or x > 255 for x in ver):
            raise ValueError(f"{key} bytes out of range: {ver!r}")


if __name__ == "__main__":
    main()
