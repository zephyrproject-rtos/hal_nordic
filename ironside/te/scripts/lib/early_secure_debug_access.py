# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""UICR.USERCNF entries that open secure debug at reset.

The entries are for development only. Hardware applies them before IROT runs, so
a debugger can halt or trace secure code from its reset vector. They never lock a
signal, so UROT's debug policy still decides the access it hands over. An engaged
APPROTECT or SECUREAPPROTECT locks the matching signal first, and the entry for
it has no effect.
"""

from __future__ import annotations

from . import uicr

MODE_OFF = "off"
MODE_DEBUG = "debug"
MODES = (MODE_OFF, MODE_DEBUG)
# A USERCNF list this module did not write, or wrote in part.
MODE_UNKNOWN = "unknown"

# TAMPC PROTECT CTRL writes need the key, and a VALUE write is ignored until
# WRITEPROTECTION has been cleared.
TAMPC_CTRL_KEY = 0x50FA << 16
TAMPC_CTRL_CLEAR_WRITEPROTECTION = TAMPC_CTRL_KEY | (0xF << 4)
TAMPC_CTRL_VALUE_HIGH = TAMPC_CTRL_KEY | 0x1
TAMPC_DOMAIN0_DBGEN_CTRL_OFF = 0x500
TAMPC_DOMAIN0_SPIDEN_CTRL_OFF = 0x510
TAMPC_SIZE = 0x1000

TAMPC_BASES = {
    "nrf54lm20a": 0x500EF000,
    "nrf54lm20b": 0x500EF000,
    "nrf54lv10a": 0x500EF000,
    "nrf54l15": 0x500DC000,
}


def tampc_base(soc: str | None) -> int:
    """Return the TAMPC base of the longest supported part name that prefixes @soc."""
    for family in sorted(TAMPC_BASES, key=len, reverse=True):
        if soc and soc.startswith(family):
            return TAMPC_BASES[family]
    raise uicr.UicrError(
        f"early secure debug access: unknown SoC {soc!r} (known: {', '.join(sorted(TAMPC_BASES))})"
    )


def entries(soc: str | None, mode: str) -> list[tuple[int, int]]:
    """Return the ordered USERCNF (ADDR, DATA) entries for @mode on @soc.

    Mode off returns no entries.
    """
    if mode not in MODES:
        raise uicr.UicrError(
            f"early secure debug access: unknown mode {mode!r} (one of {', '.join(MODES)})"
        )
    if mode == MODE_OFF:
        return []
    base = tampc_base(soc)
    out = []
    for off in (TAMPC_DOMAIN0_DBGEN_CTRL_OFF, TAMPC_DOMAIN0_SPIDEN_CTRL_OFF):
        out += [(base + off, TAMPC_CTRL_CLEAR_WRITEPROTECTION), (base + off, TAMPC_CTRL_VALUE_HIGH)]
    return out


def classify(slots: list[tuple[int, int]], soc: str | None) -> str:
    """Return the mode of the USERCNF entries hardware applies.

    Entries are matched by address, so an entry that writes TAMPC but is not the
    exact set from entries() yields MODE_UNKNOWN.
    """
    base = tampc_base(soc)
    applied = slots[: uicr.usercnf_end(slots)]
    found = [entry for entry in applied if base <= entry[0] < base + TAMPC_SIZE]
    if not found:
        return MODE_OFF
    if found == entries(soc, MODE_DEBUG):
        return MODE_DEBUG
    return MODE_UNKNOWN
