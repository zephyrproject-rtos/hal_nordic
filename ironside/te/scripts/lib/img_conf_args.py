# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Parse image-configuration arguments and encode them into a config blob.

The flags are declared in the configuration-field resource and built from it by
lib/config_fields.py. What stays here is the encoding the resource cannot
express: the GPIO trigger and power-fail spellings, and how a named field
replaces one word of an existing blob while the others are kept. Clearing a
field takes its explicit disable value or option.

The structure layout and the encoders themselves remain in lib/conf_img.py.
"""

from . import conf_img as ci
from . import config_fields as cf

PROFILE = "image"

# UROT ignores unknown versions and applies its defaults instead.
SUPPORTED_FORMATVERSIONS = cf.profile(PROFILE).formatversions

GPIO_LEVELS = cf.choices("gpio-level")
# UROT derives the active level from the stored pull.
GPIO_LEVEL_PULL = cf.enum("gpio-level").attributes("implied-pull")

# An omitted flag preserves the mask, while this value clears it.
RESETREAS_NONE = cf.enum("resetreas").disable_value

# The optional token on --pof that suppresses the POFWARN event.
POF_NOEVENT = "noevent"


def add_arguments(ap, group_title=None, *, editing=False):
    """Add the image-configuration parameters.

    Editing mode includes the explicit clearing options. A new configuration
    starts with every optional field disabled and the board fields unset.
    """
    return cf.add_arguments(ap, PROFILE, group_title, editing=editing)


def given(args) -> bool:
    """Whether the caller specified an image-configuration field."""
    return cf.given(args, PROFILE)


def parse_gpio(spec):
    """Parse P<port>.<pin>[,<low|high>][,<pull>] into (port, pin, pull).

    Level and pull names are disjoint, so either may be omitted and their order
    does not matter.
    """
    body, *fields = (f.strip().lower() for f in spec.split(","))
    given = {}
    for field in fields:
        if not field:
            continue
        kind = "level" if field in GPIO_LEVELS else "pull" if field in ci.GPIO_PULL else None
        if kind is None:
            raise ci.ConfImgError(f"--dfu-trigger-gpio {spec!r}: unknown value {field!r}")
        if kind in given:
            raise ci.ConfImgError(f"--dfu-trigger-gpio {spec!r}: {kind} given twice")
        given[kind] = field

    body = body.lstrip("p")
    try:
        port_s, pin_s = body.split(".")
        port, pin = int(port_s, 0), int(pin_s, 0)
    except ValueError:
        raise ci.ConfImgError(
            f"--dfu-trigger-gpio {spec!r}: expected P<port>.<pin>[,LEVEL][,PULL]"
        ) from None
    pull = given.get("pull") or GPIO_LEVEL_PULL[given.get("level", "low")]
    return port, pin, pull


def parse_pof(spec):
    """Parse CODE[,noevent] into (threshold, event_disable).

    CODE is the raw per-part THRESHOLD code, in any integer base.
    """
    code, *fields = (f.strip().lower() for f in spec.split(","))
    event_disable = False
    for field in fields:
        if field != POF_NOEVENT:
            raise ci.ConfImgError(f"--pof {spec!r}: unknown value {field!r}")
        if event_disable:
            raise ci.ConfImgError(f"--pof {spec!r}: {POF_NOEVENT} given twice")
        event_disable = True

    try:
        threshold = int(code, 0)
    except ValueError:
        raise ci.ConfImgError(
            f"--pof {spec!r}: expected a THRESHOLD code, optionally followed by {POF_NOEVENT}"
        ) from None
    return threshold, event_disable


# The optional token on --board-lfxo that drives it from an external clock.
BOARD_LFXO_BYPASS = "bypass"


def parse_board_lfxo(spec):
    """Parse CAP_PF[,bypass] into (load_cap_pf, bypass)."""
    cap, *fields = (f.strip().lower() for f in spec.split(","))
    bypass = False
    for field in fields:
        if field != BOARD_LFXO_BYPASS:
            raise ci.ConfImgError(f"--board-lfxo {spec!r}: unknown value {field!r}")
        if bypass:
            raise ci.ConfImgError(f"--board-lfxo {spec!r}: {BOARD_LFXO_BYPASS} given twice")
        bypass = True

    try:
        load_cap_pf = int(cap, 0)
    except ValueError:
        raise ci.ConfImgError(
            f"--board-lfxo {spec!r}: expected CAP_PF, optionally followed by {BOARD_LFXO_BYPASS}"
        ) from None
    return load_cap_pf, bypass


VREG_MODES = cf.choices("vreg-mode")


def parse_board_vreg(spec: str) -> bool:
    """Parse a --board-vreg MODE into whether the main regulator runs DC/DC."""
    mode = spec.strip().lower()
    if mode not in VREG_MODES:
        raise ci.ConfImgError(f"--board-vreg {spec!r}: expected {' or '.join(VREG_MODES)}")
    return mode == "dcdc"


def split_names(values):
    """Split a list of space-separated or comma-separated names."""
    out = []
    for value in values:
        out += [n.strip().lower() for n in value.split(",") if n.strip()]
    return out


def apply(args, current=None) -> bytes:
    """Replace the specified fields in a packed configuration blob.

    A current value of None starts with all optional fields disabled. Existing
    raw words are preserved because decoding a GPIO trigger loses its original
    pull. Board fields the arguments do not name stay as they were, unset
    included; signing fills unset ones.
    """
    conflict = cf.exclusive_conflict(args, PROFILE)
    if conflict is not None:
        raise ci.ConfImgError(f"{conflict[0]} and {conflict[1]} cannot be combined")

    if current is None:
        gpio = resetreas = crv = pofcon = board_lfxo = board_hfxo = board_vreg = fault = 0
        authop_revoke = 0
    else:
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
        ) = ci.unpack_raw(current)
        if not ci.is_supported(current):
            raise ci.ConfImgError(
                f"configuration format "
                f"{formatversion >> 16}.{formatversion & 0xFFFF} is not supported "
                "and cannot be edited"
            )

    if args.authop_revoke_gen is not None:
        authop_revoke = ci.encode_authop_revoke(parse_authop_gen(args.authop_revoke_gen))

    if getattr(args, "no_dfu_trigger_gpio", False):
        gpio = 0
    elif args.dfu_trigger_gpio is not None:
        gpio = ci.encode_gpio(*parse_gpio(args.dfu_trigger_gpio))

    if args.dfu_trigger_resetreas is not None:
        names = split_names(args.dfu_trigger_resetreas)
        if RESETREAS_NONE in names:
            if len(names) > 1:
                raise ci.ConfImgError(
                    f"--dfu-trigger-resetreas {RESETREAS_NONE!r} cannot be "
                    "combined with other reset reasons"
                )
            resetreas = 0
        else:
            resetreas = ci.encode_resetreas(names)

    if getattr(args, "no_dfu_trigger_fault", False):
        fault = 0
    elif args.dfu_trigger_fault:
        fault = ci.FAULT_ENABLE

    if args.wdtstart_crv_ms is not None:
        # A disabled watchdog has no timeout for crv_from_ms() to convert.
        crv = (
            ci.WDT_CRV_DISABLED
            if args.wdtstart_crv_ms == 0
            else ci.crv_from_ms(args.wdtstart_crv_ms)
        )
    elif args.wdt_crv is not None:
        crv = args.wdt_crv

    if getattr(args, "no_pof", False):
        pofcon = ci.POF_DISABLED
    elif args.pof is not None:
        threshold, event_disable = parse_pof(args.pof)
        pofcon = ci.encode_pofcon(threshold, event_disable=event_disable)

    if args.board_lfxo is not None:
        load_cap_pf, bypass = parse_board_lfxo(args.board_lfxo)
        board_lfxo = ci.encode_board_lfxo(load_cap_pf, bypass=bypass)

    if args.board_hfxo is not None:
        board_hfxo = ci.encode_board_hfxo(args.board_hfxo)

    if args.board_vreg is not None:
        board_vreg = ci.encode_board_vreg(dcdc=parse_board_vreg(args.board_vreg))

    return ci.pack(
        gpio=gpio,
        resetreas=resetreas,
        fault=fault,
        wdtstart_crv=crv,
        pofcon=pofcon,
        board_lfxo=board_lfxo,
        board_hfxo=board_hfxo,
        board_vreg=board_vreg,
        authop_revoke=authop_revoke,
    )


def parse_authop_gen(spec: str) -> int | None:
    """Parse an AUTHOP generation, or none, into the generation or None."""
    text = spec.strip().lower()
    if text == "none":
        return None
    try:
        return int(text)
    except ValueError:
        raise ci.ConfImgError(
            f"--authop-revoke-gen {spec!r}: expected 0..{ci.AUTHOP_GENERATIONS - 1} or none"
        ) from None


def described(blob) -> str:
    """Describe the enabled fields in a configuration blob."""
    report = ci.unpack(blob)
    gpio, resetreas = report["dfu_trigger"]["gpio"], report["dfu_trigger"]["resetreas"]
    pof = report["pof"]
    parts = [
        f"gpio P{gpio['port']}.{gpio['pin']} active-{gpio['active_level']} ({gpio['pull']})"
        if gpio
        else "gpio off",
        f"resetreas {'+'.join(resetreas)}" if resetreas else "resetreas none",
        "fault on" if report["dfu_trigger"]["fault"] else "fault off",
        f"ns watchdog {report['wdtstart']['timeout_s']:.3f}s"
        if report["wdtstart"]["crv"]
        else "ns watchdog off",
        _pof_described(pof) if pof["pofcon"] else "pof off",
        _board_described(report["board"]),
        _authop_described(report["authop"]),
    ]
    return ", ".join(parts)


def _authop_described(authop) -> str:
    gen = authop["revoke_gen"]
    return f"authop revoke gen{gen} and older" if gen is not None else "authop revoke none"


def _pof_described(pof) -> str:
    """Describe a decoded power-fail comparator field."""
    state = f"pof threshold code {pof['threshold']}"
    if not pof["enabled"]:
        state += " (comparator off)"
    if pof["event_disable"]:
        state += " (POFWARN off)"
    return state


def _board_described(board) -> str:
    """Describe the decoded board fields."""
    lfxo, hfxo, vreg = board["lfxo"], board["hfxo"], board["vreg"]
    parts = [
        f"lfxo {lfxo['load_cap_pf']}pF" + (" bypass" if lfxo["bypass"] else "")
        if lfxo
        else "lfxo unset",
        f"hfxo {hfxo['load_cap_pf']}pF" if hfxo else "hfxo unset",
        f"vreg {vreg['mode']}" if vreg else "vreg unset",
    ]
    return "board " + "/".join(parts)
