# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""IronSide image configuration blob (struct ironside_conf_img).

This is *deployment* configuration, not signing - it describes the DFU-entry
triggers, the optional non-secure watchdog, the power-fail comparator and the
board's crystal and regulator values UROT applies during its boot sequence. It is
generated independently (by the image-config generator or ironside_bundle.py
update-config), then embedded verbatim by the signer as the protected
IMAGE_TLV_IRONSIDE_CONF (0x17E1) TLV of the UROT image - so it is covered by the
UROT signature and changes with a UROT firmware update.

The layout mirrors struct ironside_conf_img in the firmware's conf_img.h - fourteen
little-endian u32 words (56 B):
  formatversion, board.lfxo, board.hfxo, board.vreg, dfu.trigger.gpio,
  dfu.trigger.resetreas, dfu.trigger.fault, dfu.trigger.reserved, wdtstart.crv,
  pof.pofcon, authop.revoke, authop.reserved[3]
UROT reads no field of a config whose formatversion it does not support, so only
the supported version is ever emitted here. The board.* words are mandatory: UROT
refuses to boot unless all three are set.
"""

import struct
import sys

CONF_IMG_SIZE = 56
CONF_IMG_FORMATVERSION = 0x00010000  # 1.0
_WORDS = CONF_IMG_SIZE // 4

# DFU.TRIGGER.GPIO word (bit 31 enable, 9..8 pull, 7..5 port, 4..0 pin). PULL
# values are the PIN_CNF enum (no value 2).
GPIO_ENABLE = 1 << 31
GPIO_PULL = {"disabled": 0, "pulldown": 1, "pullup": 3}

# DFU.TRIGGER.RESETREAS bits (== RESET.RESETREAS positions); only these are honoured.
RESETREAS = {
    "resetpin": 1 << 0,
    "dog0": 1 << 1,
    "dog1": 1 << 2,
    "lockup": 1 << 7,
}


# DFU.TRIGGER.FAULT word (bit 31 enable, every other bit reserved and zero). UROT
# honours only this exact word.
FAULT_ENABLE = 1 << 31


# WDTSTART.CRV - the reload value UROT starts the non-secure watchdog with. 0 turns
# the feature off; anything else must be at least the datasheet minimum, below which
# there is no usable timeout (the RoT drops such a value, leaving no watchdog).
# timeout[s] = (CRV + 1) / 32768, the watchdog counting the 32.768 kHz LFCLK.
WDT_CRV_DISABLED = 0
WDT_CRV_MIN = 0xF
WDT_CRV_MAX = 0xFFFFFFFF
WDT_LFCLK_HZ = 32768


# POF.POFCON word (bit 0 POF, 4..1 THRESHOLD, bit 7 EVENTDISABLE) - the REGULATORS
# power-fail comparator configuration UROT programs. 0 leaves the comparator alone.
# THRESHOLD is the raw code, not a voltage: the same code is a different threshold on
# each part, and a blob does not know which part will run it.
POF_DISABLED = 0
POF_ENABLE = 1 << 0
POF_THRESHOLD_POS = 1
POF_THRESHOLD_MAX = 0xF
POF_EVENTDISABLE = 1 << 7


# BOARD words. Bit 31 SET marks a word as configured; UROT refuses to boot on a
# word without it.
#   BOARD.LFXO: 8..1 load cap in pF, bit 0 bypass (external clock, no crystal)
#   BOARD.HFXO: 7..0 load cap in pF
#   BOARD.VREG: bit 0 DC/DC (0 = LDO)
BOARD_SET = 1 << 31
BOARD_LFXO_LOAD_CAP_PF_POS = 1
BOARD_LFXO_LOAD_CAP_PF_MAX = 0xFF
BOARD_LFXO_BYPASS = 1 << 0
BOARD_HFXO_LOAD_CAP_PF_MAX = 0xFF
BOARD_VREG_DCDC = 1 << 0
BOARD_LFXO_SUPPORTED = (
    BOARD_SET | (BOARD_LFXO_LOAD_CAP_PF_MAX << BOARD_LFXO_LOAD_CAP_PF_POS) | BOARD_LFXO_BYPASS
)
BOARD_HFXO_SUPPORTED = BOARD_SET | BOARD_HFXO_LOAD_CAP_PF_MAX
BOARD_VREG_SUPPORTED = BOARD_SET | BOARD_VREG_DCDC

# AUTHOP.REVOKE: one more than the newest AUTHOP generation UROT revokes, with
# every older one, so revoking generation N stores N + 1. 0 revokes nothing.
AUTHOP_GENERATIONS = 3

# The nRF54L15, nRF54LM20 and nRF54LV10 DK values, applied to any board word a
# configuration leaves unset.
DK_LFXO_LOAD_CAP_PF = 17
DK_HFXO_LOAD_CAP_PF = 15
DK_VREG_DCDC = True


class ConfImgError(Exception):
    pass


def encode_gpio(port, pin, pull):
    """Pack a DFU.TRIGGER.GPIO word (ENABLE set). Raises on bad input."""
    if not 0 <= port <= 0x7:
        raise ConfImgError(f"gpio port {port} out of range (0..7)")
    if not 0 <= pin <= 0x1F:
        raise ConfImgError(f"gpio pin {pin} out of range (0..31)")
    if pull not in GPIO_PULL:
        raise ConfImgError(f"gpio pull {pull!r} not one of {sorted(GPIO_PULL)}")
    return GPIO_ENABLE | (GPIO_PULL[pull] << 8) | (port << 5) | pin


def encode_resetreas(names):
    """OR the named reset reasons into a DFU.TRIGGER.RESETREAS mask."""
    mask = 0
    for name in names:
        if name not in RESETREAS:
            raise ConfImgError(f"resetreas {name!r} not one of {sorted(RESETREAS)}")
        mask |= RESETREAS[name]
    return mask


def encode_wdt_crv(crv):
    """Validate a WDTSTART.CRV value (0 = no watchdog). Raises on bad input."""
    if crv == WDT_CRV_DISABLED:
        return WDT_CRV_DISABLED
    if not WDT_CRV_MIN <= crv <= WDT_CRV_MAX:
        raise ConfImgError(
            f"wdt crv {crv} out of range (0 to disable, otherwise "
            f"0x{WDT_CRV_MIN:x}..0x{WDT_CRV_MAX:x})"
        )
    return crv


def encode_pofcon(threshold, *, event_disable=False):
    """Pack a POF.POFCON word with the comparator enabled. Raises on bad input.

    `threshold` is the raw 4-bit THRESHOLD code, not a voltage - see POF_THRESHOLD_MAX.
    `event_disable` suppresses the POFWARN event the comparator would otherwise
    generate.

    The word is built only from POF, THRESHOLD and EVENTDISABLE, so no configuration
    this function produces can reach POFCON's internal fields.
    """
    if not 0 <= threshold <= POF_THRESHOLD_MAX:
        raise ConfImgError(
            f"pof threshold code {threshold} out of range (0..0x{POF_THRESHOLD_MAX:x})"
        )
    word = POF_ENABLE | (threshold << POF_THRESHOLD_POS)
    return word | POF_EVENTDISABLE if event_disable else word


def encode_board_lfxo(load_cap_pf: int, *, bypass: bool = False) -> int:
    """Pack a BOARD.LFXO word. Raises on bad input."""
    if not 0 <= load_cap_pf <= BOARD_LFXO_LOAD_CAP_PF_MAX:
        raise ConfImgError(
            f"lfxo load cap {load_cap_pf} out of range (0..{BOARD_LFXO_LOAD_CAP_PF_MAX})"
        )
    word = BOARD_SET | (load_cap_pf << BOARD_LFXO_LOAD_CAP_PF_POS)
    return word | BOARD_LFXO_BYPASS if bypass else word


def encode_board_hfxo(load_cap_pf: int) -> int:
    """Pack a BOARD.HFXO word. Raises on bad input."""
    if not 0 <= load_cap_pf <= BOARD_HFXO_LOAD_CAP_PF_MAX:
        raise ConfImgError(
            f"hfxo load cap {load_cap_pf} out of range (0..{BOARD_HFXO_LOAD_CAP_PF_MAX})"
        )
    return BOARD_SET | load_cap_pf


def encode_board_vreg(*, dcdc: bool) -> int:
    """Pack a BOARD.VREG word."""
    return BOARD_SET | BOARD_VREG_DCDC if dcdc else BOARD_SET


def is_board_word_set(word: int, supported: int) -> bool:
    """Whether UROT accepts @word: SET, and no bit outside @supported."""
    return bool(word & BOARD_SET) and not word & ~supported


def with_board_defaults(blob: bytes | None) -> bytes:
    """Fill every unset board word of @blob with the DK value, and warn if any was.

    None stands for an empty configuration. A blob of an unsupported
    formatversion is returned unchanged, since UROT reads none of its words.
    """
    if blob is None:
        blob = pack()
    words = list(_words(blob))
    if words[0] != CONF_IMG_FORMATVERSION:
        return blob

    defaults = (
        ("lfxo", 1, BOARD_LFXO_SUPPORTED, encode_board_lfxo(DK_LFXO_LOAD_CAP_PF)),
        ("hfxo", 2, BOARD_HFXO_SUPPORTED, encode_board_hfxo(DK_HFXO_LOAD_CAP_PF)),
        ("vreg", 3, BOARD_VREG_SUPPORTED, encode_board_vreg(dcdc=DK_VREG_DCDC)),
    )
    filled = []
    for name, index, supported, dk_word in defaults:
        if not is_board_word_set(words[index], supported):
            words[index] = dk_word
            filled.append(name)
    if not filled:
        return blob

    print(
        f"warning: image config: board {', '.join(filled)} not set, applied the nRF54L DK "
        f"values (LFXO {DK_LFXO_LOAD_CAP_PF} pF, HFXO {DK_HFXO_LOAD_CAP_PF} pF, VREGMAIN "
        f"{'DC/DC' if DK_VREG_DCDC else 'LDO'}). They must match this board's crystals and "
        f"regulator circuit. Set them with ironside_bundle.py update-config --board-lfxo, "
        f"--board-hfxo and --board-vreg.",
        file=sys.stderr,
    )
    return struct.pack(f"<{len(words)}I", *words)


def _words(blob: bytes) -> tuple:
    """The u32 words of a blob of the format's size."""
    if len(blob) != CONF_IMG_SIZE:
        raise ConfImgError(f"config blob is {len(blob)} bytes, expected {CONF_IMG_SIZE}")
    return struct.unpack(f"<{_WORDS}I", blob)


def is_supported(blob: bytes) -> bool:
    """Whether UROT reads @blob: the supported formatversion."""
    return _words(blob)[0] == CONF_IMG_FORMATVERSION


def encode_authop_revoke(gen: int | None) -> int:
    """Pack an AUTHOP.REVOKE word for revoking generation @gen and every older one,
    or nothing when @gen is None. Raises on bad input."""
    if gen is None:
        return 0
    if not 0 <= gen < AUTHOP_GENERATIONS:
        raise ConfImgError(
            f"AUTHOP generation {gen} out of range (0..{AUTHOP_GENERATIONS - 1})"
        )
    return gen + 1


def authop_revoke_gen(word: int) -> int | None:
    """The newest AUTHOP generation an AUTHOP.REVOKE word revokes, or None. UROT
    reads a value above the generation count as no revocation."""
    return word - 1 if 0 < word <= AUTHOP_GENERATIONS else None


def wdt_timeout_s(crv):
    """The watchdog timeout a CRV encodes, in seconds (0.0 when disabled)."""
    return 0.0 if crv == WDT_CRV_DISABLED else (crv + 1) / WDT_LFCLK_HZ


def crv_from_ms(ms):
    """CRV for a timeout in milliseconds: CRV = round(ms/1000 * 32768) - 1.

    The LFCLK quantizes the achievable timeout, so wdt_timeout_s() on the result is
    what the watchdog will actually do - callers should report that rather than the
    requested value.
    """
    if ms <= 0:
        raise ConfImgError(
            f"watchdog timeout {ms} ms must be positive (use 0/omit the flag for no watchdog)"
        )
    crv = round(ms * WDT_LFCLK_HZ / 1000) - 1
    if crv < WDT_CRV_MIN:
        raise ConfImgError(
            f"watchdog timeout {ms} ms -> CRV {crv}, below the "
            f"datasheet minimum 0x{WDT_CRV_MIN:x} (the RoT would "
            f"drop it, leaving no watchdog)"
        )
    if crv > WDT_CRV_MAX:
        raise ConfImgError(
            f"watchdog timeout {ms} ms -> CRV {crv}, too large (max 0x{WDT_CRV_MAX:x})"
        )
    return crv


def pack(
    *,
    gpio=0,
    resetreas=0,
    fault=0,
    wdtstart_crv=0,
    pofcon=0,
    board_lfxo=0,
    board_hfxo=0,
    board_vreg=0,
    authop_revoke=0,
):
    """Pack struct ironside_conf_img (56 B)."""
    wdtstart_crv = encode_wdt_crv(wdtstart_crv)
    return struct.pack(
        f"<{_WORDS}I",
        CONF_IMG_FORMATVERSION,
        board_lfxo,
        board_hfxo,
        board_vreg,
        gpio,
        resetreas,
        fault,
        0,  # dfu.trigger.reserved
        wdtstart_crv,
        pofcon,
        authop_revoke,
        0,
        0,
        0,  # authop.reserved[3]
    )


def unpack_raw(blob):
    """The stored words of a config blob:
    (formatversion, gpio, resetreas, crv, pofcon, board_lfxo, board_hfxo, board_vreg,
    authop_revoke, fault).

    unpack() renders the fields for a reader; an editor needs them back exactly as
    they were stored, so it can replace one and keep the others. Going through the
    decoded form would not do: the GPIO word's active level is derived from its
    pull, so re-packing a decoded trigger has to re-invent the pull it came from.
    """
    words = _words(blob)
    (
        formatversion,
        board_lfxo,
        board_hfxo,
        board_vreg,
        gpio,
        resetreas,
        fault,
        _reserved,
        crv,
        pofcon,
        authop_revoke,
        _ar0,
        _ar1,
        _ar2,
    ) = words
    return (
        formatversion,
        gpio,
        resetreas,
        crv,
        pofcon,
        board_lfxo,
        board_hfxo,
        board_vreg,
        authop_revoke,
        fault,
    )


def unpack(blob):
    """Decode a blob into named fields - the inverse of pack().

    Reports what the RoT would make of it, so `supported` is False for a
    formatversion UROT does not know, and an unset board word decodes to None.
    """
    (
        formatversion,
        gpio,
        resetreas,
        crv,
        pofcon,
        board_lfxo,
        board_hfxo,
        board_vreg,
        authop_revoke,
        fault,
    ) = unpack_raw(blob)
    out = {
        "formatversion": f"{formatversion >> 16}.{formatversion & 0xFFFF}",
        "supported": is_supported(blob),
        "authop": {"revoke_gen": authop_revoke_gen(authop_revoke)},
        "dfu_trigger": {"gpio": None, "resetreas": [], "fault": fault == FAULT_ENABLE},
        "wdtstart": {"crv": crv, "timeout_s": wdt_timeout_s(crv)},
        "pof": {
            "pofcon": pofcon,
            "enabled": bool(pofcon & POF_ENABLE),
            "threshold": (pofcon >> POF_THRESHOLD_POS) & POF_THRESHOLD_MAX,
            "event_disable": bool(pofcon & POF_EVENTDISABLE),
        },
        "board": {"lfxo": None, "hfxo": None, "vreg": None},
    }
    if is_board_word_set(board_lfxo, BOARD_LFXO_SUPPORTED):
        out["board"]["lfxo"] = {
            "load_cap_pf": (board_lfxo >> BOARD_LFXO_LOAD_CAP_PF_POS) & BOARD_LFXO_LOAD_CAP_PF_MAX,
            "bypass": bool(board_lfxo & BOARD_LFXO_BYPASS),
        }
    if is_board_word_set(board_hfxo, BOARD_HFXO_SUPPORTED):
        out["board"]["hfxo"] = {"load_cap_pf": board_hfxo & BOARD_HFXO_LOAD_CAP_PF_MAX}
    if is_board_word_set(board_vreg, BOARD_VREG_SUPPORTED):
        out["board"]["vreg"] = {"mode": "dcdc" if board_vreg & BOARD_VREG_DCDC else "ldo"}
    if gpio & GPIO_ENABLE:
        pull = (gpio >> 8) & 0x3
        pull_name = next((n for n, v in GPIO_PULL.items() if v == pull), pull)
        out["dfu_trigger"]["gpio"] = {
            "port": (gpio >> 5) & 0x7,
            "pin": gpio & 0x1F,
            "pull": pull_name,
            # The RoT derives the active level from the pull - pullup means the
            # trigger is active-low, as UROT reads it.
            "active_level": "low" if pull == GPIO_PULL["pullup"] else "high",
        }
    out["dfu_trigger"]["resetreas"] = sorted(
        name for name, bit in RESETREAS.items() if resetreas & bit
    )
    return out
