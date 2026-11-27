# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Resolve a DK's virtual serial ports from the J-Link probe's own numbering.

The on-board J-Link exposes two ports. Port 0 carries IronSide DFU's SMP transport
and port 1 the application console. The probe reports which host port is which
through `nrfutil device list`, so the result does not depend on how the host names
or orders its ports (Windows COM numbers do not follow interface order).

The probe's virtual serial ports exist whenever the board is powered, even while no
firmware runs. A reset makes the probe re-enumerate them, so callers that reset the
board resolve again afterward with `wait()`.

Runnable standalone as a bench check:

    python serial_port.py --serial-number <serial>            # print the console port
    python serial_port.py --serial-number <serial> --smp      # print the SMP port
    python serial_port.py --list                              # every probe and its ports
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import time

SMP = 0
CONSOLE = 1
PORT_NAMES = {SMP: "SMP transport (port 0)", CONSOLE: "console (port 1)"}

NRFUTIL = os.environ.get("IRONSIDE_INTEGRATION_KIT_NRFUTIL", "nrfutil")
LIST_ARGS = ["--json", "--skip-overhead", "device", "list", "--traits", "jlink,serialPorts"]

# One nrfutil call.
LIST_TIMEOUT = 15.0

# Bounds probe re-enumeration after a reset, not target boot time.
DISCOVER_TIMEOUT = 20.0

# A port must stay listed under one name this long before it is handed out, so an
# upload does not start while the probe is still re-enumerating.
STABLE = 1.0

POLL = 0.5

BY_ID_DIR = "/dev/serial/by-id"


class SerialPortError(RuntimeError):
    """No usable port; the message gives the diagnosis."""


class _PermanentError(SerialPortError):
    """A failure that waiting for the probe cannot fix."""


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Print a DK's virtual serial port, as reported by the probe."
    )
    ap.add_argument(
        "--serial-number", help="J-Link serial number (default: the only probe attached)"
    )
    which = ap.add_mutually_exclusive_group()
    which.add_argument(
        "--smp", action="store_true", help="print the SMP port, virtual serial port 0"
    )
    which.add_argument(
        "--console",
        action="store_true",
        help="print the console port, virtual serial port 1 (default)",
    )
    ap.add_argument(
        "--list", action="store_true", help="print every attached probe and its ports, then exit"
    )
    ap.add_argument(
        "--timeout",
        type=float,
        default=DISCOVER_TIMEOUT,
        help="seconds to wait for the port (default: %(default)s)",
    )
    args = ap.parse_args()

    if args.list:
        try:
            listing = list_ports()
        except SerialPortError as exc:
            sys.exit(f"error: {exc}")
        for serial in sorted(listing):
            ports_by_index = listing[serial]
            ports = "  ".join(f"port{i}={ports_by_index[i]}" for i in sorted(ports_by_index))
            print(f"{serial}  {ports or '(no serial ports)'}")
        return 0

    index = SMP if args.smp else CONSOLE
    print(resolve(index, serial=args.serial_number, timeout=args.timeout, flag=None))
    return 0


def list_ports(timeout: float = LIST_TIMEOUT) -> dict[int, dict[int, str]]:
    """Return {probe serial: {port index: host port name}} for every attached J-Link.

    A probe that reports no usable serial ports maps to an empty dict. Raises
    SerialPortError when nrfutil is missing, fails, times out, or prints no device list.
    """
    return _parse(_run_nrfutil(timeout))


def _run_nrfutil(timeout: float) -> str:
    cmd = [NRFUTIL, *LIST_ARGS]
    try:
        # nrfutil reads stdin; leaving it attached swallows the caller's input.
        proc = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout,
            stdin=subprocess.DEVNULL,
        )
    except FileNotFoundError:
        raise _PermanentError(
            f"nrfutil not found (looked for {NRFUTIL}); install it or set "
            "IRONSIDE_INTEGRATION_KIT_NRFUTIL"
        ) from None
    except subprocess.TimeoutExpired:
        raise SerialPortError(f"{' '.join(cmd)} timed out after {timeout:.0f} s") from None
    if proc.returncode != 0:
        tail = "\n".join((proc.stderr or proc.stdout).strip().splitlines()[-5:])
        raise SerialPortError(f"{' '.join(cmd)} exited {proc.returncode}:\n{tail}")
    return proc.stdout


def _parse(text: str) -> dict[int, dict[int, str]]:
    """Extract the device list from nrfutil's JSON output.

    The output is one JSON document, or JSON Lines with the list in an info event
    and again in the task_end event. Windows binaries end lines with CRLF.
    """
    for line in text.splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            devices = _find_devices(json.loads(line))
        except ValueError:
            continue
        if devices is not None:
            return _group_by_serial(devices)
    raise SerialPortError(f"{' '.join([NRFUTIL, *LIST_ARGS])} printed no device list")


def _find_devices(obj) -> list | None:
    """Depth-first search for the "devices" list, which nests at different depths."""
    if isinstance(obj, dict):
        devices = obj.get("devices")
        if isinstance(devices, list):
            return devices
        for value in obj.values():
            found = _find_devices(value)
            if found is not None:
                return found
    return None


def _group_by_serial(devices: list) -> dict[int, dict[int, str]]:
    """Map each J-Link serial to its virtual serial ports.

    A device without a numeric serial (CMSIS-DAP) is skipped. A serial port without
    a port index (a native USB CDC function) is skipped.
    """
    grouped: dict[int, dict[int, str]] = {}
    for device in devices:
        try:
            serial = int(device["serialNumber"])
        except (KeyError, TypeError, ValueError):
            continue
        ports_by_index = grouped.setdefault(serial, {})
        for entry in device.get("serialPorts") or []:
            index, name = entry.get("vcom"), entry.get("comName")
            if index is not None and name:
                ports_by_index[int(index)] = name
    return grouped


def resolve(index: int, **kwargs) -> str:
    """Call wait() for a CLI: warnings go to stderr and a failure exits with the diagnosis."""
    kwargs.setdefault("warn", lambda msg: print(f"warning: {msg}", file=sys.stderr))
    try:
        return wait(index, **kwargs)
    except SerialPortError as exc:
        sys.exit(f"error: {exc}")


def wait(
    index: int,
    *,
    port: str | None = None,
    serial=None,
    timeout: float = DISCOVER_TIMEOUT,
    stable: float = STABLE,
    announce=None,
    warn=None,
    flag: str | None = "--port",
) -> str:
    """Return the host port for port `index`, waiting for the probe to enumerate.

    `port` is returned verbatim. Without `serial` it is returned at once, since a
    non-J-Link transport has nothing to look up. With `serial` the call first waits
    until nrfutil lists that probe, then calls `warn(message)` if `port` is not the
    one the probe reports for `index`.

    Otherwise the port is the one nrfutil reports for the probe with `serial`, or
    for the only probe attached. A name must stay listed for `stable` seconds. A
    listing error does not end the wait unless retrying cannot help; the last error
    is raised at `timeout`. A port that is present but not yet stable at `timeout`
    is returned. `announce(port)` is called once, when the port first appears. `flag`
    names the option that lets the user pick a port when several probes are attached
    (None when the caller has no such option).
    """
    if port is None:
        return _settle(
            lambda: _stable_name(select(list_ports(), index, serial, flag=flag)),
            timeout,
            stable,
            announce,
        )
    if serial is None:
        return port
    probe_serial = _as_serial(serial)
    ports_by_index = _settle(
        lambda: _probe_ports(list_ports(), probe_serial), timeout, 0.0, None
    )
    expected = ports_by_index.get(index)
    if warn is not None and expected is not None and not _is_same_port(port, expected):
        warn(
            f"{port} is not the {PORT_NAMES[index]} port of probe {probe_serial}; "
            f"nrfutil reports {expected}"
        )
    return port


def _settle(poll, timeout: float, stable: float, announce):
    """Call `poll()` until it returns the same value for `stable` seconds.

    `poll` raises SerialPortError while the answer is not available.
    """
    deadline = time.monotonic() + timeout
    held = None
    announced = False
    error = None
    while True:
        try:
            value = poll()
        except _PermanentError:
            raise
        except SerialPortError as exc:
            value, error = None, exc
        now = time.monotonic()
        if value is None:
            held = None
        else:
            if held is None or held[0] != value:
                held = (value, now)
                if announce is not None and not announced:
                    announce(value)
                    announced = True
            if now - held[1] >= stable:
                return value
        if now >= deadline:
            if held is not None:
                return held[0]
            raise error
        time.sleep(POLL)


def select(
    ports: dict[int, dict[int, str]], index: int, serial=None, *, flag: str | None = "--port"
) -> str:
    """Pick the host port for port `index` out of one list_ports() result.

    Without `serial`, the only attached probe is used. Several probes are refused
    rather than guessed.
    """
    if serial is None:
        if len(ports) > 1:
            raise _PermanentError(
                f"{len(ports)} J-Link probes attached ({_serials(ports)}). "
                f"Pass --serial-number{f' or {flag}' if flag else ''}."
            )
        if not ports:
            raise SerialPortError("no J-Link probe is attached")
        serial = next(iter(ports))
    ports_by_index = _probe_ports(ports, _as_serial(serial))
    if index not in ports_by_index:
        listed = "  ".join(f"port{i}={ports_by_index[i]}" for i in sorted(ports_by_index))
        raise SerialPortError(f"probe {serial} reports no {PORT_NAMES[index]}; ports: {listed}")
    return ports_by_index[index]


def _probe_ports(ports: dict[int, dict[int, str]], serial: int) -> dict[int, str]:
    if serial not in ports:
        raise SerialPortError(
            f"no J-Link probe with serial {serial} is attached; attached: {_serials(ports)}"
        )
    if not ports[serial]:
        raise SerialPortError(f"probe {serial} is listed but reports no serial ports")
    return ports[serial]


def _serials(ports: dict[int, dict[int, str]]) -> str:
    return ", ".join(str(s) for s in sorted(ports)) or "none"


def _as_serial(serial) -> int:
    """Return a probe serial as a number; by-id names zero-pad it to 12 digits."""
    try:
        return int(serial)
    except (TypeError, ValueError):
        raise _PermanentError(f"not a valid probe serial number: {serial!r}") from None


def _stable_name(com_name: str) -> str:
    """Return the by-id alias of a host port when one exists, else the port itself.

    The alias names the same node. It survives re-enumeration, so a port resolved
    once stays bound to its board.
    """
    if os.path.isdir(BY_ID_DIR):
        target = os.path.realpath(com_name)
        for entry in sorted(os.listdir(BY_ID_DIR)):
            alias = os.path.join(BY_ID_DIR, entry)
            if os.path.realpath(alias) == target:
                return alias
    return com_name


def _is_same_port(a: str, b: str) -> bool:
    return a == b or os.path.realpath(a) == os.path.realpath(b)


if __name__ == "__main__":
    sys.exit(main())
