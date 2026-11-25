# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""J-Link helpers: headless pylink open and ambiguous-probe guards for nrfutil."""

from __future__ import annotations

import os
import sys

from . import serial_port


def connected_jlink_serials() -> set[str]:
    """Return the serials of the attached J-Link probes, as nrfutil lists them.

    Raises SerialPortError when nrfutil cannot list them.
    """
    return {str(serial) for serial in serial_port.list_ports()}


def nrfutil_select(serial_number: str | int | None) -> list[str]:
    """Return nrfutil ``--serial-number`` args, or exit if autodetect is ambiguous."""
    if serial_number is not None:
        return ["--serial-number", str(serial_number)]
    _refuse_several_probes(_attached_serials())
    return []


def resolve_jlink_serial(serial_number: str | int | None) -> str | None:
    """Resolve a pylink serial; exit when more than one probe and none was given."""
    if serial_number is not None:
        return str(serial_number)
    serials = _attached_serials()
    _refuse_several_probes(serials)
    return next(iter(serials), None)


def _attached_serials() -> set[str]:
    try:
        return connected_jlink_serials()
    except serial_port.SerialPortError as exc:
        sys.exit(f"error: {exc}")


def _refuse_several_probes(serials: set[str]) -> None:
    if len(serials) > 1:
        sys.exit(
            f"error: {len(serials)} J-Link probes connected ({', '.join(sorted(serials))}); "
            "pass --serial-number - autodetect opens the SEGGER probe-picker GUI."
        )


def create_jlink():
    """Construct a pylink ``JLink`` with GUI dialogs suppressed."""
    import pylink

    dll = os.environ.get("JLINK_DLL")
    lib = pylink.Library(dllpath=dll) if dll else None
    jlink = pylink.JLink(lib=lib)
    # Must run before open() - see SEGGER "SuppressGUI" command string.
    jlink.exec_command("SuppressGUI 1")
    return jlink


def open_jlink(
    serial_number: str | int | None,
    *,
    speed: int = 4000,
    configure: bool = True,
):
    """Open a headless SWD session on the selected probe."""
    import pylink.enums as enums

    serial = resolve_jlink_serial(serial_number)
    jlink = create_jlink()
    jlink.open(serial_no=serial)
    jlink.set_tif(enums.JLinkInterfaces.SWD)
    jlink.set_speed(speed)
    if configure:
        jlink.coresight_configure()
    return jlink
