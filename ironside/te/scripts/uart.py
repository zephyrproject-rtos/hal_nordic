#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Print output from a DK's application console (the probe's virtual serial port 1).

The port comes from the J-Link probe's own numbering. Pass --serial-number when
several probes are attached.
"""

import argparse
import sys
import time

from lib import serial_port


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--port", default=None, help="serial port (default: the probe's virtual serial port 1)"
    )
    ap.add_argument(
        "--serial-number", help="J-Link serial number (default: the only probe attached)"
    )
    ap.add_argument("--baud", type=int, default=115200, help="baud rate (default: %(default)s)")
    args = ap.parse_args()

    try:
        import serial  # pyserial
    except ImportError:
        sys.exit("pyserial not installed: pip install pyserial")

    port = serial_port.resolve(serial_port.CONSOLE, port=args.port, serial=args.serial_number)

    print(f"# listening on {port} @ {args.baud} (Ctrl-C to quit)")
    # Reopen after target resets cause the J-Link serial port to re-enumerate.
    try:
        while True:
            try:
                # Do not share the input queue with another serial client.
                with serial.Serial(port, args.baud, timeout=1, exclusive=True) as ser:
                    while True:
                        data = ser.read(256)
                        if data:
                            sys.stdout.write(data.decode(errors="replace"))
                            sys.stdout.flush()
            except serial.SerialException:
                time.sleep(0.5)  # port went away (e.g. target reset) - reopen
    except KeyboardInterrupt:
        return 0


if __name__ == "__main__":
    sys.exit(main())
