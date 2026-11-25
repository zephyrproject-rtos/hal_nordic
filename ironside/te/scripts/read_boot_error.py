#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Read the CTRL-AP MAILBOX RX register and decode it as a BOOTFAIL word.

IROT and UROT write one 32-bit BOOTFAIL word to the CTRL-AP mailbox only when
a stage halts: which stage halted and why. The halted device keeps the word
readable. A CTRL-AP soft reset does not clear the mailbox, so after one the word
can come from an earlier boot. This reads the word over SWD without resetting or
halting the target.
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path
from typing import TYPE_CHECKING

sys.path.insert(0, str(Path(__file__).resolve().parent))
from lib import bootfail, jlink_util  # noqa: E402

if TYPE_CHECKING:
    import pylink

# CTRL-AP index and the host-side mailbox register offsets. The device side
# writes through its own MAILBOX registers; RXDATA/RXSTATUS are the
# debugger's view of the same device-to-host channel.
CTRL_AP_IDX = 2
CTRL_AP_RXDATA = 0x28
CTRL_AP_RXSTATUS = 0x2C  # 1 = word pending (device wrote, host not yet read)

# DP register bank (DPBANKSEL == 0) and the power-up handshake needed before
# any AP register access.
DP_CTRL_STAT = 0x1
DP_SELECT = 0x2
CSYSPWRUPREQ = 1 << 30
CSYSPWRUPACK = 1 << 31
CDBGPWRUPREQ = 1 << 28
CDBGPWRUPACK = 1 << 29
PWRUP_REQ = CSYSPWRUPREQ | CDBGPWRUPREQ
PWRUP_ACK = CSYSPWRUPACK | CDBGPWRUPACK


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--serial-number",
        dest="serial",
        help="J-Link serial number (default: sole attached board)",
    )
    ap.add_argument("--serial", dest="serial", help=argparse.SUPPRESS)  # older spelling
    ap.add_argument(
        "--speed", type=int, default=4000, help="SWD speed in kHz (default: %(default)s)"
    )
    ap.add_argument("--json", action="store_true", help="print the result as JSON")
    args = ap.parse_args()

    jlink = jlink_util.open_jlink(args.serial, speed=args.speed)
    try:
        power_up(jlink)
        word, pending = read_mailbox_rx(jlink)
    finally:
        jlink.close()

    status = bootfail.decode(word)
    status["pending"] = pending

    if args.json:
        print(json.dumps(status))
        return

    print(f"BOOTFAIL word: 0x{word:08x} (pending={'yes' if pending else 'no'})")
    for line in describe(status):
        print(line)


def describe(status: dict) -> list[str]:
    """Return the human-readable lines for a decoded BOOTFAIL word."""
    if status["status_word"] == 0:
        return ["no BOOTFAIL word: no stage has halted since the mailbox was cleared"]
    if not status["valid"]:
        return ["not a BOOTFAIL word (an ADAC session may have used the mailbox)"]

    lines = [f"stage: {status['stage']}"]
    if status["error"] == 0:
        lines.append("error: none")
    else:
        lines.append(
            f"error: 0x{status['error']:02X} {status['error_name']} ({status['error_text']})"
        )
    return lines


def power_up(jlink: pylink.JLink, timeout_s: float = 1.0) -> None:
    """Request and wait for debug + system power via DP CTRL/STAT."""
    dp_select(jlink, apsel=0, apbanksel=0)
    jlink.coresight_write(DP_CTRL_STAT, PWRUP_REQ, ap=False)

    deadline = time.monotonic() + timeout_s
    while True:
        status = jlink.coresight_read(DP_CTRL_STAT, ap=False)
        if (status & PWRUP_ACK) == PWRUP_ACK:
            return
        if time.monotonic() > deadline:
            raise TimeoutError(
                f"timed out waiting for debug/system power-up (CTRL/STAT = 0x{status:08X})"
            )
        time.sleep(0.001)


def dp_select(jlink: pylink.JLink, apsel: int, apbanksel: int = 0) -> None:
    """Program the DP SELECT register: which AP and which AP register bank."""
    jlink.coresight_write(DP_SELECT, ((apsel & 0xFF) << 24) | ((apbanksel & 0xF) << 4), ap=False)


def read_mailbox_rx(jlink: pylink.JLink) -> tuple[int, bool]:
    """Return (word, pending) from the CTRL-AP MAILBOX RX register."""
    pending = ap_read(jlink, CTRL_AP_IDX, CTRL_AP_RXSTATUS) != 0
    word = ap_read(jlink, CTRL_AP_IDX, CTRL_AP_RXDATA)
    return word, pending


def ap_read(jlink: pylink.JLink, apsel: int, offset: int) -> int:
    """Read an AP register addressed by its full byte offset."""
    dp_select(jlink, apsel, (offset >> 4) & 0xF)
    return jlink.coresight_read((offset >> 2) & 0x3, ap=True)


if __name__ == "__main__":
    main()
