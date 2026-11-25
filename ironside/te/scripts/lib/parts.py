# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""The supported parts, and the one place their several spellings are reconciled.

IronSide TE runs on more than one nRF54L part, and they are not interchangeable -
they differ in NVM size, RAM size, and therefore in the whole partition layout and
in where the provisioning firmware can be staged. A tool that picks the wrong one
does not fail to build -- it programs the wrong bytes, and the symptoms (a silent
hang, or IronSide DFU booting instead of the App) read like firmware bugs.

Three spellings of "the part" exist across the tooling and none is derivable from
another by string surgery:

  * the SoC / package key  ``nrf54lv10a`` -- component.toml, lib/uicr.py's
    RRAM_SIZE_KB, ironside_download.py, and the ``-D*_SOC`` bare-metal builds
  * the MDK device macro   ``NRF54LV10A_XXAA`` -- selects the memory map in
    ironside_client/layout.h and the provisioning images' layout header
  * the J-Link device name ``nRF54LV10A_M33`` -- pylink's database, which holds
    only core-suffixed names (the bare device name is not in it)

and a fourth, ``deviceName`` as ``nrfutil device device-info`` reports it, which
is what lets any of them be derived from an attached board rather than passed in.

Deliberately no west board targets here. This module ships inside the IronSide Integration Kit,
which never builds firmware -- west and Zephyr are outside its dependency
boundary. A caller that needs board targets adds that column itself.
"""

from __future__ import annotations

import os
import re
import subprocess
from dataclasses import dataclass

from . import jlink_util, serial_port

NRFUTIL = os.environ.get("IRONSIDE_INTEGRATION_KIT_NRFUTIL", "nrfutil")


class PartError(RuntimeError):
    """The part could not be identified, or is not one IronSide supports."""


@dataclass(frozen=True)
class Part:
    soc: str  # the package / SoC key
    mdk_macro: str  # MDK device macro (the memory-map selector)
    jlink_device: str  # pylink / J-Link Commander device name
    device_name: str  # what `nrfutil device device-info` prints
    nvm_kb: int  # usable NVM, for cross-checking lib/uicr.py
    ram_kb: int  # usable RAM
    # The part's RAM. Above ram_kb it holds VPRCTX-RAM and the CRACEN ProtectedRAM.
    ram_physical_kb: int
    # The step the secure/non-secure RAM split moves in, in bytes: the coarsest of
    # the MPC override's granularity and the SAU and secure MPU's 32 bytes. Equal
    # to the SDK layout header's IRONSIDE_RAM_SPLIT_GRANULE for the part.
    ram_split_granule: int


# The two nRF54LM20 dice are separate entries because the release tooling versions
# them as separate streams (each component.toml declares one per die), even though
# they share a memory map -- hence the same MDK macro and J-Link device.
PARTS: dict[str, Part] = {
    "nrf54lm20a": Part(
        "nrf54lm20a", "NRF54LM20A_XXAA", "nRF54LM20A_M33", "nRF54LM20A", 2036, 511, 512, 32
    ),
    "nrf54lm20b": Part(
        "nrf54lm20b", "NRF54LM20A_XXAA", "nRF54LM20A_M33", "nRF54LM20B", 2036, 511, 512, 32
    ),
    "nrf54lv10a": Part(
        "nrf54lv10a", "NRF54LV10A_XXAA", "nRF54LV10A_M33", "nRF54LV10A", 1012, 191, 192, 32
    ),
    "nrf54l15": Part("nrf54l15", "NRF54L15_XXAA", "nRF54L15_M33", "nRF54L15", 1524, 256, 256, 4096),
}

_BY_DEVICE_NAME = {p.device_name.lower(): p for p in PARTS.values()}


def by_soc(soc: str) -> Part:
    try:
        return PARTS[soc.lower()]
    except KeyError:
        raise PartError(f"unsupported SoC {soc!r} -- known: {', '.join(sorted(PARTS))}") from None


def device_name(serial_number: str | int | None = None) -> str:
    """The `deviceName` nrfutil reports for a probe, e.g. "nRF54LV10A".

    With no serial the flag is omitted and nrfutil answers for the sole attached
    probe -- the same autodetect every other board selector here falls back to.
    Ambiguity is refused first, and by name - nrfutil's own answer to several
    attached probes is a picker, and picking one at random is how an image gets
    built for the part that is not under test.
    """
    if serial_number is None:
        try:
            attached = jlink_util.connected_jlink_serials()
        except serial_port.SerialPortError as exc:
            raise PartError(str(exc)) from None
        if len(attached) > 1:
            raise PartError(
                f"{len(attached)} J-Link probes are attached "
                f"({', '.join(sorted(attached))}); say which one with a serial "
                f"number -- the part cannot be read off 'whichever answers first'"
            )
        # A single known probe is named explicitly rather than left to autodetect.
        serial_number = next(iter(attached), None)
    cmd = [NRFUTIL, "device", "device-info"]
    if serial_number is not None:
        cmd += ["--serial-number", str(serial_number)]
    try:
        out = subprocess.run(cmd, capture_output=True, text=True, check=True).stdout
    except FileNotFoundError:
        raise PartError(f"{NRFUTIL} not found on PATH") from None
    except subprocess.CalledProcessError as e:
        raise PartError(f"{' '.join(cmd)} failed:\n{(e.stderr or e.stdout).strip()}") from None
    m = re.search(r"^\s*deviceName:\s*(\S+)", out, re.MULTILINE)
    if not m:
        raise PartError(f"no deviceName reported for {_which(serial_number)}:\n{out}")
    return m.group(1)


def for_serial(serial_number: str | int | None = None) -> Part:
    """The part physically attached at this J-Link serial, or at the sole probe.

    Refuses rather than guessing - a part nrfutil can name but this table cannot is
    a part IronSide has no memory map for, and assuming another part would program
    the wrong layout onto it. Passing None asks the sole attached
    probe and is refused the same way when there is more than one.
    """
    name = device_name(serial_number)
    part = _BY_DEVICE_NAME.get(name.lower())
    if part is None:
        raise PartError(
            f"DK {_which(serial_number)} reports deviceName {name!r}, which IronSide "
            f"does not support -- known: "
            f"{', '.join(sorted(p.device_name for p in PARTS.values()))}"
        )
    return part


def _which(serial_number) -> str:
    """How to name the probe a message is about, when it may not have a serial."""
    return str(serial_number) if serial_number is not None else "the attached board"
