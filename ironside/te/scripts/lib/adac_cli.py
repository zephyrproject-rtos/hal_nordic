# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Shared helpers for running adac.py as a subprocess."""

import json
import logging as log
import subprocess
import sys
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parent.parent
ADAC = SCRIPTS / "adac.py"

# UROT readiness was measured below 100 ms on an nRF54LM20B DK at Open.
# A timeout is treated as no installed UROT, so probing need not wait for
# adac.py's five-second mailbox timeout.
READY_TIMEOUT_MS = 300


def argv(serial, *args) -> list:
    """Build an adac.py command for a probe serial number."""
    cmd = [sys.executable, str(ADAC)]
    if serial is not None:
        cmd += ["--serial-number", str(serial)]
    return cmd + [str(a) for a in args]


def lifecycle(serial, ready_timeout_ms: int = READY_TIMEOUT_MS):
    """Return the lifecycle reported by UROT, or None if ADAC does not answer.

    The probe resets the device into ADAC mode and back into a normal boot.
    """
    cmd = argv(serial, "--json", "--ready-timeout-ms", ready_timeout_ms, "discovery")
    log.debug("$ %s", " ".join(cmd))
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        log.debug(
            "nothing answered ADAC (exit %d): %s", proc.returncode, last_line(proc) or "no output"
        )
        return None
    try:
        state = json.loads(proc.stdout)["psa_lifecycle"]
    except (json.JSONDecodeError, KeyError, TypeError) as exc:
        log.warning(
            "unexpected adac.py discovery response (%s). Assuming IronSide TE is installed",
            exc,
        )
        return "unreported"
    # Delay the pylink dependency until the device probe runs.
    sys.path.insert(0, str(SCRIPTS))
    import adac

    return adac.PSA_LIFECYCLE_NAME.get(state, f"0x{state:04X}")


def require_lifecycle(
    serial, *allowed_names: str, ready_timeout_ms: int = READY_TIMEOUT_MS
) -> str | None:
    """Return UROT's lifecycle if it is allowed, otherwise None."""
    name = lifecycle(serial, ready_timeout_ms=ready_timeout_ms)
    if name is None:
        return None
    return name if name in allowed_names else None


def last_line(proc) -> str:
    """Return the last line of subprocess output."""
    said = ((proc.stderr or "") + (proc.stdout or "")).strip().splitlines()
    return said[-1].strip() if said else ""
