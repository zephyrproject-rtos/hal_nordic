# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Build provisioning batches, and read what a bundle states about them.

A batch is built where it is used rather than carried, so the only copy of any
batch's bytes is the route hex it was spliced into. What a bundle holds instead
comes in two parts: the configuration, which is the deployment's choice of AP
closure method and whether to enroll User SEED, and takes no key material, and the record,
which is what preparation produced (the key source, its digest, the counter
floors, and whether the OTA KEKs were withheld).

A batch covering "install" imports the OTA KEKs, so it can only be built from
the keyset the bundle was prepared with; every other batch takes no key
material at all. Such a batch, and the route hex carrying it, hold raw OTA KEKs
and must be stored with restricted access.
"""

from __future__ import annotations

from .python_version import require_python

require_python()

import hashlib  # noqa: E402
import json  # noqa: E402

from . import app_key_caps as caps  # noqa: E402
from . import app_keys as ak  # noqa: E402
from . import fw_bundle  # noqa: E402
from . import prov_abi as abi  # noqa: E402

# Where the provisioning record rides, owned by the format module: a key in the
# prov-secure package's user manifest, beside the route hexes it describes.
RECORD = fw_bundle.PROVISIONING_RECORD
CONFIG = fw_bundle.PROVISIONING_CONFIG

RECORD_VERSION = 2

# The operation groups, in the order a batch must apply them. The seed precedes
# every IKG consumer, and AP closure precedes erase and secure-AP protection.
KINDS = ("seed", "install", "approtect", "protect")

# What each group does, for the tools that describe a batch to a user.
DESCRIPTIONS = {
    "seed": "check or seed the IKG SEED and export the IAK public key",
    "install": "provision rollback counters and OTA KEKs",
    "approtect": "close the non-secure AP (IronSide_APPROTECT)",
    "protect": "set the protection registers",
}

# The operation groups a lifecycle plan can ask for, over every (current,
# target, mode) combination the transition planner accepts. Held as data because
# the planner lives with the provisioning tool rather than here. The two are kept
# in agreement by a check that enumerates every plan the tool accepts and fails
# on a group set that is missing here or unreachable there, so a new lifecycle
# edge shows up as a failed check rather than as a bundle that cannot serve it.
REACHABLE_KINDS = (
    ("seed",),
    ("install",),
    ("protect",),
    ("approtect",),
    ("seed", "install"),
    ("install", "protect"),
    ("approtect", "protect"),
    ("seed", "install", "protect"),
    ("install", "approtect", "protect"),
    ("seed", "install", "approtect", "protect"),
)

# What a batch does with the IKG SEED. "check" writes nothing and stops the
# batch unless the device is seeded already, so a bundle nobody configured
# never chooses a device's identity root. "user" enrolls User SEED, which is
# irreversible.
SEED_RESOLUTIONS = abi.SEED_MODES
SEED_LABELS = {
    abi.SEED_CHECK: "no, use the IKG SEED the device already has",
    abi.SEED_USER: "yes, enroll User SEED",
}
DEFAULT_IKG_SEED = abi.SEED_CHECK

# The provision-user-seed values a bundle stores, and the SEED mode each names.
USER_SEED_CHOICES = {"no": abi.SEED_CHECK, "yes": abi.SEED_USER}

# The IKG SEED selections each SEED mode permits. check leaves the slots as the
# device has them, and the device holds the selection against those.
SEED_SELECTIONS = {
    abi.SEED_CHECK: ("nordic", "user"),
    abi.SEED_USER: ("user",),
}

# A missing image has no deployment floor. A carried image uses its own version.
DEFAULT_COUNTERS = {"urot": 0, "app": 0, "dfu": 0}
DEFAULT_AP_CLOSURE = "auth-lock"

COUNTER_ROLES = ("urot", "app", "dfu")

# Map deployment-facing closure names to their ABI values.
CLOSURE_METHODS = {"hardware-fixed": "hw-lock", "authenticated": "auth-lock"}
CLOSURE_LABELS = {"hw-lock": "hardware fixed", "auth-lock": "authenticated"}
DEFAULT_CLOSURE_METHOD = "authenticated"

# The configuration fields, spelled as the update-config flags are.
CONFIG_AP_CLOSURE = "ap-closure"
CONFIG_PROVISION_USER_SEED = "provision-user-seed"
# The field provision-user-seed replaced. Its values do not map onto the new
# field, so a configuration that names it is refused.
RETIRED_CONFIG_IKG_SEED = "ikg-seed"

# The inventory of application keys the record keeps: the shape of each key and
# its key ID. Never the material -- the route hexes carry that already,
# and a second copy would double what a leaked bundle leaks.
APP_KEYS = "application-keys"

# Separates the IKG SEED mode from the group it qualifies in a batch name,
# the way "@" separates the AP-closure mode.
SEED_SEP = ":"


class BatchError(Exception):
    """A batch cannot be built, or the provisioning record cannot be read."""


def name(kinds, ap_closure=None, ikg_seed=None) -> str:
    """The canonical name for a set of operation groups.

    @ap_closure is required exactly when the groups close the non-secure AP, and
    @ikg_seed exactly when they cover the seed group. Each is rejected otherwise, so one
    set of groups has one name. An omitted value falls back to the deployment
    default rather than being refused, because every value is buildable either way.
    """
    unknown = [k for k in kinds if k not in KINDS]
    if unknown:
        raise BatchError(f"unknown operation group(s): {', '.join(unknown)}")
    ordered = [k for k in KINDS if k in kinds]
    if not ordered:
        raise BatchError("a batch covers at least one operation group")
    if "seed" in ordered:
        seed = DEFAULT_IKG_SEED if ikg_seed is None else ikg_seed
        if seed not in SEED_RESOLUTIONS:
            raise BatchError(
                f"IKG SEED mode must be one of {'/'.join(sorted(SEED_RESOLUTIONS))}, "
                f"got {seed!r}"
            )
        ordered[ordered.index("seed")] = f"seed{SEED_SEP}{seed}"
    elif ikg_seed is not None:
        raise BatchError(
            f"{'+'.join(ordered)} covers no seed group, so it takes no IKG SEED mode"
        )
    base = "+".join(ordered)
    if "approtect" not in ordered:
        if ap_closure is not None:
            raise BatchError(f"{base} closes no access port, so it takes no AP-closure mode")
        return base
    closure = DEFAULT_AP_CLOSURE if ap_closure is None else ap_closure
    if closure not in abi.APPROTECT_MODE:
        raise BatchError(
            f"AP-closure mode must be one of {'/'.join(sorted(abi.APPROTECT_MODE))}, "
            f"got {closure!r}"
        )
    return f"{base}@{closure}"


def parse(batch_name: str) -> tuple:
    """(groups, AP-closure mode or None, IKG SEED mode or None) for a name."""
    base, sep, closure = batch_name.partition("@")
    seed = None
    tokens = []
    for token in base.split("+"):
        group, seed_sep, entry = token.partition(SEED_SEP)
        if seed_sep:
            if group != "seed":
                raise BatchError(
                    f"{batch_name}: only the seed group takes an IKG SEED mode, "
                    f"not {group!r}"
                )
            if entry not in SEED_RESOLUTIONS:
                raise BatchError(f"{batch_name}: unknown IKG SEED mode {entry!r}")
            seed = entry
        tokens.append(group)
    unknown = [k for k in tokens if k not in KINDS]
    if unknown:
        raise BatchError(f"{batch_name}: unknown operation group(s): {', '.join(unknown)}")
    ordered = tuple(k for k in KINDS if k in tokens)
    if ("seed" in ordered) != (seed is not None):
        raise BatchError(
            f"{batch_name}: a batch names an IKG SEED mode exactly when it covers the seed group"
        )
    if sep and closure not in abi.APPROTECT_MODE:
        raise BatchError(f"{batch_name}: unknown AP-closure mode {closure!r}")
    if ("approtect" in ordered) != bool(sep):
        raise BatchError(
            f"{batch_name}: a batch names an AP-closure mode exactly when it closes the AP"
        )
    return ordered, (closure if sep else None), seed


def needs_ota_kek(kinds) -> bool:
    """Whether this group set can only be built with the OTA KEKs.

    Enrolling the seed and enabling protection in one batch is refused without
    the KEK imports, because those imports are what stops a batch whose seed did
    not land from reaching the irreversible protection operations.
    """
    return "seed" in kinds and "protect" in kinds


def names(*, ota_kek: bool = True) -> tuple:
    """Every batch name a bundle carries.

    Without the OTA KEKs the seed-and-protect combinations cannot be built at
    all, so they are absent rather than wrong.
    """
    out = []
    for kinds in REACHABLE_KINDS:
        if not ota_kek and needs_ota_kek(kinds):
            continue
        closures = sorted(abi.APPROTECT_MODE) if "approtect" in kinds else (None,)
        seeds = sorted(SEED_RESOLUTIONS) if "seed" in kinds else (None,)
        out += [name(kinds, closure, seed) for seed in seeds for closure in closures]
    return tuple(out)


def describe(batch_name: str) -> str:
    """One line saying what a batch does."""
    kinds, closure, seed = parse(batch_name)
    what = ", then ".join(DESCRIPTIONS[k] for k in kinds)
    detail = [d for d in (SEED_LABELS.get(seed), closure) if d]
    return f"{what} ({'; '.join(detail)})" if detail else what


def seed_failure(operations, results) -> str | None:
    """Explain why a batch's IKG SEED check stopped it, or None if none did.

    @operations is the batch's input operations and @results the device's
    results for them, in order. The explanation names the bundle setting, since
    that is the input the operator changes.
    """
    for op, res in zip(operations, results):
        params = op.get("check-ikg-seed")
        status = res.get("status")
        if params is None or status in (abi.PSA_SUCCESS, abi.ITE_ERROR_ABORTED):
            continue
        seed = abi.seed_mode_checked(params.get("seeding"))
        return _seed_check_message(seed, bool(params.get("unseeded-ok")), status)
    return None


def _seed_check_message(seed: str | None, before_write: bool, status: int) -> str:
    guide = 'See the section "User SEED provisioning" in the user guide.'
    if status == abi.PSA_ERROR_CORRUPTION_DETECTED:
        return "The device's IKG SEED slots are invalid and cannot be repaired."
    if status == abi.PSA_ERROR_NOT_SUPPORTED:
        return (
            "The provisioning firmware does not support check-ikg-seed. Use the "
            "firmware from this bundle's release."
        )
    if seed == abi.SEED_CHECK and status == abi.PSA_ERROR_BAD_STATE:
        return (
            "The device is unseeded, and this bundle (provision-user-seed: no) does "
            "not seed it. Run ironside_bundle.py update-config --provision-user-seed "
            f"yes, then retry with the updated bundle. {guide}"
        )
    if seed == abi.SEED_USER and before_write and status == abi.PSA_ERROR_NOT_PERMITTED:
        return (
            "The device's User SEED entry is revoked, so this bundle "
            "(provision-user-seed: yes) cannot enroll it. Nothing was written. Use a "
            f"bundle with provision-user-seed: no. {guide}"
        )
    if seed == abi.SEED_USER and not before_write:
        return (
            f"Seeding the device failed ({abi.status_name(status)}). Provisioning "
            f"stopped before using the IKG SEED."
        )
    return f"check-ikg-seed failed: {abi.status_name(status)}."


def counters_arg(arg: str) -> dict:
    """Parse "urot=N,app=N,dfu=N" into the three initial counter values.

    With no argument, counters come from the versions of the
    images the bundle carries. Any omitted role keeps 0. set_counters() instead
    keeps the recorded value because it edits selected roles.

    The values are what the batch writes into the counter stores, which only
    provision an empty store, so a provisioned device cannot have them lowered. The bundle's
    record can be re-derived any number of times before that. One above an
    image's own counter is refused either way.
    """
    out = dict(DEFAULT_COUNTERS)
    for item in arg.split(","):
        if not item.strip():
            continue
        role, sep, value = item.partition("=")
        role = role.strip()
        if not sep or role not in out:
            raise BatchError(
                f"expected urot=N,app=N,dfu=N with roles {', '.join(COUNTER_ROLES)}, got {item!r}"
            )
        try:
            out[role] = int(value, 0)
        except ValueError:
            raise BatchError(
                f"counter value for {role} must be an integer, got {value!r}"
            ) from None
        if not 0 <= out[role] <= 0xFFFFFFFF:
            raise BatchError(f"counter value for {role} must fit in 32 bits, got {out[role]}")
    return out


def closure_arg(spec: str) -> str:
    """Parse an access port closure method into the ABI spelling.

    Both vocabularies are accepted: the deployment's ("authenticated"), which is
    what the bundle tool documents, and prov_abi's ("auth-lock"), which the batch
    names and the bench lifecycle script use. One value comes out either way, so
    nothing downstream has to know which was typed.
    """
    method = spec.strip().lower()
    if method in CLOSURE_METHODS:
        return CLOSURE_METHODS[method]
    if method in abi.APPROTECT_MODE:
        return method
    raise BatchError(
        f"access port closure method must be one of "
        f"{', '.join(sorted(CLOSURE_METHODS))}, got {spec!r}"
    )


def closure_label(closure: str) -> str:
    """How the access port closure method @closure is shown to a reader."""
    return CLOSURE_LABELS.get(closure, closure)


def build(batch_name: str, *, counters=None, keks=(), app_keys=()) -> bytes:
    """Encode the batch @batch_name names.

    @keks is the three OTA KEK generations, or empty to leave them unprovisioned.
    A batch covering "install" without them provisions counters only, which
    leaves a device unable to accept an encrypted update.

    @app_keys is the application's own persistent keys as (attributes, material)
    pairs.
    They ride inside the "install" group rather than earning a group of their
    own: the batch name space is a closed set held against the lifecycle
    planner, and whether a bundle carries application keys is a property of that
    bundle, not of the transitions a device can make.
    """
    kinds, closure, seed = parse(batch_name)
    if kinds == ("seed",):
        # The seed batch exports the IAK on its own, with no KEK import to gate.
        return abi.build_provision_ikg_seed(seed)
    counters = counters or DEFAULT_COUNTERS
    install = "install" in kinds
    try:
        return abi.build_merged(
            seed=seed,
            counters=tuple(counters[role] for role in COUNTER_ROLES) if install else None,
            keks=tuple(keks) if install else (),
            app_keys=tuple(app_keys) if install else (),
            # The KEK imports fail closed before the export when no seed landed.
            # With no KEKs there is nothing to gate it, and the export hangs.
            export_iak="seed" in kinds and bool(keks),
            approtect_mode=closure,
            protect_fields=tuple(abi.PROTECT_ORDER) if "protect" in kinds else (),
        )
    except ValueError as exc:
        if app_keys and install:
            _refuse_excess(batch_name, counters, keks, app_keys, exc)
        raise BatchError(f"{batch_name}: {exc}") from exc


def _refuse_excess(batch_name, counters, keks, app_keys, exc) -> None:
    """Restate a too-long batch as the number of application keys that fit.

    The decoder's cap is on operations, and a caller who supplied 52 keys cannot
    act on "65 operations": the transition is not theirs to split. Rebuilding
    the same batch without the keys gives the room they actually have.
    """
    if "operations, maximum" not in str(exc):
        return
    room = abi.MAX_BATCH_OPS - len(
        abi.parse_input_batch(build(batch_name, counters=counters, keks=keks))["operations"]
    )
    raise BatchError(
        f"{len(app_keys)} application keys do not fit the {batch_name!r} batch, "
        f"which has room for {room}. Provision fewer keys."
    ) from exc


def build_all(*, counters=None, keks=(), app_keys=()) -> dict:
    """Every batch a bundle carries, keyed by name.

    Built in the order the names come, so the batch with the least room for
    application keys is not necessarily the first to refuse them -- every name
    is built, and any one of them refusing refuses the set.
    """
    return {
        n: build(n, counters=counters, keks=keks, app_keys=app_keys)
        for n in names(ota_kek=bool(keks))
    }


def keys_digest(provisioned: dict) -> str:
    """A digest over the verification keys a bundle provisions.

    Recorded with the batches so a bundle whose keyset was replaced without
    rebuilding them is detectable. It covers the verification half only - the OTA
    KEKs a batch imports appear nowhere in the bundle's UICR, so their agreement
    rests on both halves being written by the same command.
    """
    parts = []
    for role in sorted(provisioned):
        for gen in sorted(provisioned[role]):
            state = provisioned[role][gen]
            if state.get("provisioned") and state.get("key"):
                parts.append(f"{role}:{gen}:{state['key']}")
    return hashlib.sha256("\n".join(parts).encode("ascii")).hexdigest()


def record(
    *, keyset: str, counters=None, ota_kek: bool = True, digest: str = "", app_keys=()
) -> dict:
    """What `update-keys` recorded about a prepared bundle.

    @digest describes the verification keys the bundle provisions, so a bundle
    re-keyed without rebuilding its route hexes is detectable; @keyset records
    the source's spelling for a reader, which proves nothing on its own.
    @app_keys is the inventory of the application's own keys -- their shape and
    key IDs, never their material -- which is what lets a reader with no keyset
    say which key IDs the route hexes import into.

    The AP closure method and IKG SEED entry are not here. They are the
    deployment's configuration, settable with no key material and before a
    bundle is prepared, so they live in their own member (see config()).

    The batches are absent too: each one is built where it is used, so the only
    copy of a batch's bytes is the route hex carrying it.

    Plain JSON types throughout: it rides in the prov-secure package's user
    manifest, which is JSON.
    """
    return {
        "version": RECORD_VERSION,
        "keyset": keyset,
        "keys-digest": digest,
        "counters": dict(counters or DEFAULT_COUNTERS),
        "ota-kek": bool(ota_kek),
        APP_KEYS: [dict(entry) for entry in app_keys],
    }


def validate(rec: dict) -> dict:
    """Return @rec, or raise BatchError when it cannot be used."""
    if not isinstance(rec, dict):
        raise BatchError(f"the {RECORD} record is not a map")
    version = rec.get("version")
    if version != RECORD_VERSION:
        raise BatchError(
            f"the {RECORD} record is version {version!r}, and this tool reads "
            f"version {RECORD_VERSION}. Run ironside_bundle.py update-keys again."
        )
    values = rec.get("counters")
    if not isinstance(values, dict):
        raise BatchError(f"the {RECORD} record carries no rollback counter map")
    _check_counter_values(values)
    recorded = rec.get(APP_KEYS)
    if not isinstance(recorded, list):
        raise BatchError(f"the {RECORD} record carries no application-key inventory")
    for entry in recorded:
        if not isinstance(entry, dict) or not isinstance(entry.get("id"), int):
            raise BatchError(f"the {RECORD} record: {entry!r} is not an application-key entry")
        try:
            caps.check_key_id(entry["id"])
        except caps.CapsError as exc:
            raise BatchError(f"the {RECORD} record names an application key whose {exc}") from None
    return rec


def closure_method(closure: str) -> str:
    """The deployment spelling of an AP-closure mode, for a carried value.

    The inverse of closure_arg(). Values a person reads or edits are stored in
    the vocabulary the flags and the guide use ("authenticated"), while the
    batch names use prov_abi's ("auth-lock").
    """
    for method, mode in CLOSURE_METHODS.items():
        if mode == closure:
            return method
    return closure


def config(*, ap_closure: str = DEFAULT_AP_CLOSURE, ikg_seed: str = DEFAULT_IKG_SEED) -> dict:
    """The provisioning configuration a bundle carries.

    Two deployment decisions, both permanent once a device applies them, and
    neither derivable from the bundle. Field names are the `update-config` flags
    without their leading dashes and values use the same vocabulary the flags
    take, so a carried configuration and an exported profile are one format.
    @ikg_seed is a SEED mode, stored as its provision-user-seed value.
    """
    return {
        CONFIG_AP_CLOSURE: closure_method(ap_closure),
        CONFIG_PROVISION_USER_SEED: user_seed_choice(ikg_seed),
    }


def validate_config(cfg: dict) -> dict:
    """Return @cfg filled in from the defaults, or raise on a value it cannot use.

    An absent field takes its default rather than being refused: a release ships
    every field, so a missing one means a hand-written profile naming only what
    it wanted to change.
    """
    if not isinstance(cfg, dict):
        raise BatchError(f"{CONFIG} is not a map")
    if RETIRED_CONFIG_IKG_SEED in cfg:
        raise BatchError(
            f"{CONFIG} names {RETIRED_CONFIG_IKG_SEED}, which this release replaces with "
            f"{CONFIG_PROVISION_USER_SEED}. Create the bundle again with this release's "
            f"ironside_bundle.py, then set --{CONFIG_PROVISION_USER_SEED} yes or no"
        )
    unknown = sorted(set(cfg) - set(config()))
    if unknown:
        raise BatchError(
            f"{CONFIG} names unknown field(s): {', '.join(unknown)}. "
            f"Expected: {', '.join(sorted(config()))}"
        )
    out = config()
    out.update(cfg)
    try:
        # The closure takes either spelling and stores the deployment one, so a
        # hand-edited file and an exported profile round-trip the same.
        out[CONFIG_AP_CLOSURE] = closure_method(closure_arg(out[CONFIG_AP_CLOSURE]))
        out[CONFIG_PROVISION_USER_SEED] = user_seed_choice(
            user_seed_arg(out[CONFIG_PROVISION_USER_SEED])
        )
    except BatchError as exc:
        raise BatchError(f"{CONFIG}: {exc}") from None
    return out


def _check_counter_values(values: dict) -> None:
    """Raise unless every counter in @values is a 32-bit unsigned integer."""
    unknown = sorted(role for role in values if role not in COUNTER_ROLES)
    if unknown:
        raise BatchError(
            f"unknown counter role(s): {', '.join(unknown)}. "
            f"Expected: {', '.join(COUNTER_ROLES)}"
        )
    for role, value in values.items():
        if isinstance(value, bool) or not isinstance(value, int):
            raise BatchError(f"counter value for {role} must be an integer, got {value!r}")
        if not 0 <= value <= 0xFFFFFFFF:
            raise BatchError(f"counter value for {role} must fit in 32 bits, got {value}")


def ap_closure(cfg: dict) -> str:
    """The access port closure method @cfg selects, as prov_abi spells it."""
    return closure_arg(cfg.get(CONFIG_AP_CLOSURE) or DEFAULT_AP_CLOSURE)


def set_ap_closure(cfg: dict, closure: str) -> bool:
    """Select @closure in a configuration. True when it changed.

    Takes either spelling, for the reason closure_arg() does.
    """
    method = closure_method(closure_arg(closure))
    if cfg.get(CONFIG_AP_CLOSURE) == method:
        return False
    cfg[CONFIG_AP_CLOSURE] = method
    return True


def selection_problem(selected: str | None, seed: str) -> str | None:
    """Why the IKG SEED selection @selected cannot boot a device provisioned with
    the SEED mode @seed, or None when it can.

    None is auto: the provisioning firmware selects from the slot states.
    """
    if selected is None:
        return None
    permitted = SEED_SELECTIONS[seed]
    if selected not in permitted:
        return (
            f"the base configuration selects {selected} SEED, but "
            f"{CONFIG_PROVISION_USER_SEED} {user_seed_choice(seed)} permits only "
            f"{' or '.join(permitted)}"
        )
    return None


def user_seed_arg(spec: str) -> str:
    """Parse a provision-user-seed value into the SEED mode it names."""
    choice = spec.strip().lower()
    if choice not in USER_SEED_CHOICES:
        raise BatchError(
            f"{CONFIG_PROVISION_USER_SEED} must be one of {', '.join(USER_SEED_CHOICES)}, "
            f"got {spec!r}"
        )
    return USER_SEED_CHOICES[choice]


def user_seed_choice(seed: str) -> str:
    """The provision-user-seed value that names the SEED mode @seed."""
    for choice, mode in USER_SEED_CHOICES.items():
        if mode == seed:
            return choice
    raise BatchError(f"unknown IKG SEED mode {seed!r}")


def ikg_seed(cfg: dict) -> str:
    """The SEED mode @cfg configures."""
    choice = cfg.get(CONFIG_PROVISION_USER_SEED)
    return user_seed_arg(choice) if choice else DEFAULT_IKG_SEED


def set_ikg_seed(cfg: dict, seed: str) -> bool:
    """Configure the SEED mode @seed. True when it changed."""
    choice = user_seed_choice(seed)
    if cfg.get(CONFIG_PROVISION_USER_SEED) == choice:
        return False
    cfg[CONFIG_PROVISION_USER_SEED] = choice
    return True


def has_ota_kek(rec: dict) -> bool:
    """Whether the bundle this record describes provisions the OTA KEKs."""
    return bool(rec.get("ota-kek"))


def seed_label(seed: str) -> str:
    """How the SEED mode @seed is shown to a reader."""
    return SEED_LABELS.get(seed, seed)


def counters(rec: dict) -> dict:
    """The initial rollback counters a record holds."""
    return dict(rec.get("counters") or DEFAULT_COUNTERS)


def counters_in(blob: bytes):
    """The counters a batch provisions, as the device reads them, or None.

    The counters are recorded twice over -- here, in the operations a route hex
    applies, and in the record's own map -- so `check` can hold the two against
    each other. A reader consulting only the map could not tell a bundle whose
    route hexes were re-materialized from one whose floors have since moved.
    """
    try:
        return abi.batch_counters(blob)
    except ValueError as exc:
        raise BatchError(str(exc)) from exc


def app_keys(rec: dict) -> list:
    """The application keys a record says the bundle provisions."""
    return [dict(entry) for entry in rec.get(APP_KEYS) or ()]


def app_key_ids_in(blob: bytes) -> tuple:
    """The key IDs a batch imports application keys under, in order.

    Read out of the operations the device applies, so they can be held against
    the record's inventory the way counters_in() is held against its counter
    map. The two are written by one command, so a disagreement means a route hex
    was patched.
    """
    try:
        return abi.batch_app_key_ids(blob)
    except ValueError as exc:
        raise BatchError(str(exc)) from exc


def operations_in(blob: bytes) -> tuple:
    """The operation names a batch applies, in order. For naming a difference."""
    return tuple(abi.op_name(op) for op in _operations(blob))


def comparable(blob: bytes) -> tuple:
    """Everything in a batch that does not depend on the keyset.

    The operations, in order, with their arguments -- the AP-closure mode, the
    SEED entry, the counter roles and values, the protection fields, the
    continue-on-error flags -- and each imported KEK reduced to its length.

    This is what lets a prepared bundle be reviewed with no keyset: the KEK
    values are the only bytes a reader cannot reproduce from the record, so
    masking them and comparing the rest catches everything else, including a
    route hex that would close the access port by a method the bundle does not
    record. A caller holding the keyset compares the bytes outright instead.

    The application keys are left out altogether rather than masked. Neither
    their attributes nor their material can be reproduced from the record, which
    keeps only the inventory, so the key IDs they import under are what a
    keyless reader holds them to -- app_key_ids_in() against that inventory.
    """
    out = []
    for op in _operations(blob):
        name = abi.op_name(op)
        if name == "import-key":
            continue
        args = op.get(name)
        if name == "import-fw-kek" and isinstance(args, dict):
            args = {k: (len(v) if k == "kek" else v) for k, v in args.items()}
        out.append((name, repr(args), bool(op.get("continue-on-error"))))
    return tuple(out)


def _operations(blob: bytes) -> list:
    """The operations a batch applies, or a BatchError naming why it cannot say."""
    try:
        return abi.parse_input_batch(blob)["operations"]
    except ValueError as exc:
        raise BatchError(str(exc)) from exc


# Stand-ins for the OTA KEKs where a check has no key source: the same length as
# the real thing, so every operation lands where it would, and obviously not a
# key. comparable() masks them out of the comparison.
PLACEHOLDER_KEKS = tuple(b"\x00" * 32 for _ in range(abi.KEK_GENERATIONS))


def set_counters(rec: dict, wanted: dict) -> bool:
    """Set the initial rollback counters in a record. True when changed.

    A role omitted from @wanted keeps its recorded value. counters_arg() instead
    uses 0 because it parses a complete set of deployment floors.

    Only the record moves here. The floors a device actually applies live in the
    batches inside the route hexes, so the caller re-materializes those from the
    new values -- which is what carries the change into what a station programs.
    """
    values = counters(rec)
    values.update(wanted)
    _check_counter_values(values)
    changed = values != dict(rec.get("counters") or {})
    rec["counters"] = values
    return changed


def available(rec: dict, batch_name: str) -> bool:
    """Whether the bundle this record describes can supply @batch_name.

    A bundle prepared without the OTA KEKs cannot combine seed enrollment and
    protection: the KEK imports are what stop a batch whose seed did not land
    from reaching the irreversible protection operations.
    """
    return has_ota_kek(rec) or not needs_ota_kek(parse(batch_name)[0])


def build_from(rec: dict, batch_name: str, *, keks=(), app_keys=()) -> bytes:
    """Build @batch_name at the decisions @rec records.

    The counters come from the record; @keks must be the key source's OTA KEKs
    for a batch covering "install", and is ignored for one that does not -- which
    is what keeps `protect`, `approtect` and their combinations buildable with no
    key source at all. @app_keys rides in the same group and comes from the same
    source, so a caller passing one passes both.
    """
    if not available(rec, batch_name):
        raise BatchError(
            f"this bundle provisions no OTA KEK, so it cannot supply the "
            f"{batch_name!r} batch: enrolling the IKG seed and enabling protection "
            f"in one batch needs the KEK imports that stop an unseeded part from "
            f"being locked. Prepare the bundle with the OTA KEKs to use this flow."
        )
    if "install" in parse(batch_name)[0] and not keks and has_ota_kek(rec):
        raise BatchError(
            f"the {batch_name!r} batch provisions the OTA KEKs, so it can only be "
            f"built from the keyset the bundle was prepared with. Name it with "
            f"--keyset."
        )
    return build(batch_name, counters=counters(rec), keks=keks, app_keys=app_keys)


def check_digest(rec: dict, provisioned: dict) -> None:
    """Raise unless @rec describes the keyset @provisioned holds."""
    recorded = rec.get("keys-digest") or ""
    if not recorded:
        return
    if recorded != keys_digest(provisioned):
        raise BatchError(
            "this bundle was prepared for provisioning from a different keyset "
            "than the one it now provisions, so its provisioning firmware would "
            "import one keyset's OTA KEKs beside another's verification keys. "
            "Run ironside_bundle.py update-keys to settle both from one source."
        )


def _self_check() -> int:
    """Assert the invariants this module is relied on for."""
    failures = []

    def check(what, got, want):
        if got != want:
            failures.append(f"{what}: got {got!r}, want {want!r}")

    def raises(what, fn):
        try:
            fn()
        except BatchError:
            return
        failures.append(f"{what}: expected a refusal")

    keks = [bytes([g]) * 32 for g in range(abi.KEK_GENERATIONS)]
    check("names with KEKs", len(names()), 19)
    check("names without KEKs", len(names(ota_kek=False)), 13)
    check(
        "the KEK-gated names",
        sorted(set(names()) - set(names(ota_kek=False))),
        [
            "seed:check+install+approtect+protect@auth-lock",
            "seed:check+install+approtect+protect@hw-lock",
            "seed:check+install+protect",
            "seed:user+install+approtect+protect@auth-lock",
            "seed:user+install+approtect+protect@hw-lock",
            "seed:user+install+protect",
        ],
    )
    # Every seed-bearing set is carried at every resolution, and no other set
    # names one.
    check(
        "every seed-bearing name is carried at every resolution",
        all(
            n.replace(f"seed{SEED_SEP}{parse(n)[2]}", f"seed{SEED_SEP}{other}") in names()
            for n in names()
            if "seed" in parse(n)[0]
            for other in SEED_RESOLUTIONS
        ),
        True,
    )
    check(
        "only seed-bearing names carry a resolution",
        sorted({("seed" in parse(n)[0]) == (parse(n)[2] is not None) for n in names()}),
        [True],
    )

    full = build_all(keks=keks)
    check("every name built", sorted(full), sorted(names()))
    bare = build_all()
    check("every KEK-free name built", sorted(bare), sorted(names(ota_kek=False)))

    # The single-phase batch runs every group in one fixed order, with the SEED
    # checked or seeded first and every consumer of it after. The User SEED
    # enrollment is checked before it writes and after; check only checks.
    for entry, seed_ops in (
        ("check", ["check-ikg-seed"]),
        ("user", ["check-ikg-seed", "enroll-ikg-seed", "check-ikg-seed"]),
    ):
        single = abi.parse_input_batch(
            full[f"seed{SEED_SEP}{entry}+install+approtect+protect@auth-lock"]
        )
        ops = [abi.op_name(op) for op in single["operations"]]
        check(
            f"the single-phase order at {entry}",
            ops,
            seed_ops
            + ["select-ikg-seed", "init-urot-storage"]
            + ["provision-counter"] * 3
            + ["import-fw-kek"] * abi.KEK_GENERATIONS
            + ["export-iak-pubkey", "set-approtect"]
            + [f"set-{f}" for f in abi.PROTECT_ORDER],
        )
        # Only the SEED write tolerates its own failure, since it returns
        # ALREADY_EXISTS on an entry already seeded. The checks around it, and
        # every later operation, abort the batch instead.
        check(
            f"only the SEED write continues on error at {entry}",
            [abi.op_name(op) for op in single["operations"] if op.get("continue-on-error")],
            [n for n in seed_ops if n != "check-ikg-seed"],
        )
    # The two closures differ in the batch, which is why they are separate names.
    if full["approtect@auth-lock"] == full["approtect@hw-lock"]:
        failures.append("the two AP closures encode identically")
    # And so do the SEED modes, which is why they are separate names too.
    if full[f"seed{SEED_SEP}check+install"] == full[f"seed{SEED_SEP}user+install"]:
        failures.append("the two IKG SEED modes encode identically")

    # The record holds what preparation produced, not batches and not the
    # deployment's configuration, and refuses what it cannot use.
    back = validate(record(keyset="keyring:x", digest="ab"))
    check("the record names no batches", "batches" in back, False)
    check(
        "the record names no configuration",
        [f for f in (CONFIG_AP_CLOSURE, CONFIG_PROVISION_USER_SEED) if f in back],
        [],
    )
    check("the digest is kept", back["keys-digest"], "ab")
    check("the record is JSON", json.loads(json.dumps(back)), back)
    raises("a foreign record version", lambda: validate({"version": 99}))
    raises("a record with no counters", lambda: validate({"version": RECORD_VERSION}))
    raises("a record that is not a map", lambda: validate("nope"))

    # The configuration is its own map, settable with no record anywhere near it.
    # This is what an assembly stage holding no keyset edits.
    cfg = validate_config(config())
    check("the default closure", ap_closure(cfg), DEFAULT_AP_CLOSURE)
    check("the default SEED mode", ikg_seed(cfg), DEFAULT_IKG_SEED)
    check("the configuration is JSON", json.loads(json.dumps(cfg)), cfg)
    check("the SEED mode changes", set_ikg_seed(cfg, "user"), True)
    check("the same SEED mode is no change", set_ikg_seed(cfg, "user"), False)
    check("the SEED mode is stored as yes", cfg[CONFIG_PROVISION_USER_SEED], "yes")
    check("the SEED mode edit is kept", ikg_seed(validate_config(cfg)), "user")
    raises("an unknown SEED mode", lambda: set_ikg_seed(cfg, "bogus"))
    check("every SEED mode has a label", sorted(SEED_LABELS), sorted(SEED_RESOLUTIONS))
    check("every SEED mode has a value", sorted(USER_SEED_CHOICES.values()), sorted(SEED_RESOLUTIONS))
    check("the default SEED mode exists", DEFAULT_IKG_SEED in SEED_RESOLUTIONS, True)
    set_ikg_seed(cfg, DEFAULT_IKG_SEED)
    check("the closure changes", set_ap_closure(cfg, "hw-lock"), True)
    check("the same closure is no change", set_ap_closure(cfg, "hw-lock"), False)
    check("the closure edit is kept", ap_closure(validate_config(cfg)), "hw-lock")
    raises("an unknown closure", lambda: set_ap_closure(cfg, "bogus"))

    # A partial profile fills in from the defaults rather than being refused, so
    # a hand-written file naming one field is usable.
    check(
        "a partial configuration fills in",
        validate_config({CONFIG_PROVISION_USER_SEED: "yes"}),
        {CONFIG_AP_CLOSURE: closure_method(DEFAULT_AP_CLOSURE), CONFIG_PROVISION_USER_SEED: "yes"},
    )
    check("an empty configuration is the defaults", validate_config({}), config())

    # A carried value uses the vocabulary the flags and the guide use, while a
    # batch name uses prov_abi's. Either spelling is accepted on the way in.
    check(
        "the stored closure is the deployment spelling",
        config()[CONFIG_AP_CLOSURE],
        DEFAULT_CLOSURE_METHOD,
    )
    check(
        "reading one back gives the ABI spelling",
        ap_closure(config()),
        DEFAULT_AP_CLOSURE,
    )
    for spelling in ("hardware-fixed", "hw-lock"):
        stored = validate_config({CONFIG_AP_CLOSURE: spelling})
        check(f"{spelling} stores as hardware-fixed", stored[CONFIG_AP_CLOSURE], "hardware-fixed")
        check(f"{spelling} reads back as hw-lock", ap_closure(stored), "hw-lock")
    check("every ABI mode has a deployment spelling", sorted(
        closure_method(m) for m in abi.APPROTECT_MODE), sorted(CLOSURE_METHODS))
    raises("a configuration field that does not exist", lambda: validate_config({"bogus": 1}))
    raises("a configuration naming an unknown closure", lambda: validate_config(
        {CONFIG_AP_CLOSURE: "bogus"}))
    raises("a configuration naming an unknown provision-user-seed value", lambda: validate_config(
        {CONFIG_PROVISION_USER_SEED: "user"}))
    raises("a configuration naming the replaced ikg-seed field", lambda: validate_config(
        {RETIRED_CONFIG_IKG_SEED: "check"}))
    raises("a configuration that is not a map", lambda: validate_config("nope"))

    # A batch built from a record is the batch built from those values directly:
    # the record is the decisions, build() is the only builder.
    moved = {"urot": 0x09000000, "app": 0x05000000, "dfu": 7}
    rec = record(keyset="keyring:x", counters={"urot": 1, "app": 2, "dfu": 3})
    check("setting the counters changes the record", set_counters(rec, moved), True)
    check("the recorded counters follow", rec["counters"], moved)
    check("the same counters are no change", set_counters(rec, moved), False)
    from_source = build_all(counters=moved, keks=keks)
    # Compare by name because a 24-batch byte dump is not a failure a reader can
    # act on, and the name is what says which one drifted.
    check(
        "every batch built from the record is the rebuild",
        sorted(n for n in from_source if build_from(rec, n, keks=keks) != from_source[n]),
        [],
    )
    check(
        "a partial set keeps the roles left out",
        (set_counters(rec, {"app": 4}), rec["counters"]),
        (True, {"urot": 0x09000000, "app": 4, "dfu": 7}),
    )
    raises(
        "a counter role that does not exist in a record",
        lambda: set_counters(rec, {"bogus": 1}),
    )
    raises("a counter value above 32 bits", lambda: set_counters(rec, {"app": 1 << 32}))
    raises("a counter value that is not an integer", lambda: set_counters(rec, {"app": "1"}))

    # The counters a batch applies are readable back out of it, which is what
    # `check` holds against the record's own map. The ABI role indices are pinned
    # here because lib/prov_abi.py's own self-check is not one CI runs: a swap
    # would provision the app floor for UROT.
    check(
        "the ABI role indices",
        counters_in(abi.build_provision_counters(1, 2, 3)),
        {"urot": 1, "app": 2, "dfu": 3},
    )
    check(
        "the counters in an install batch are the record's",
        counters_in(build_from(rec, f"seed{SEED_SEP}check+install", keks=keks)),
        rec["counters"],
    )
    check("a batch provisioning no counter reports none", counters_in(full["protect"]), None)

    # comparable() masks the KEK values and NOTHING else. This is what a keyless
    # review of a prepared bundle rests on, so each thing it must still catch is
    # pinned: the same batch built with placeholder KEKs compares equal, and a
    # different AP closure, SEED entry or floor does not.
    real = build("install", counters=moved, keks=keks)
    check(
        "the KEK values are outside the comparison",
        comparable(real),
        comparable(build("install", counters=moved, keks=PLACEHOLDER_KEKS)),
    )
    if real == build("install", counters=moved, keks=PLACEHOLDER_KEKS):
        failures.append("the placeholder KEKs encode identically to real ones")
    for what, a, b in (
        ("the AP closure mode", "approtect@auth-lock", "approtect@hw-lock"),
        (
            "the IKG SEED entry",
            f"seed{SEED_SEP}check+install",
            f"seed{SEED_SEP}user+install",
        ),
    ):
        if comparable(build(a, keks=keks)) == comparable(build(b, keks=keks)):
            failures.append(f"{what} is not covered by comparable()")
    if comparable(build("install", counters=moved, keks=keks)) == comparable(
        build("install", counters={"urot": 1, "app": 2, "dfu": 3}, keks=keks)
    ):
        failures.append("the rollback floors are not covered by comparable()")

    # Only "install" needs the key source. This is what keeps ironside_protect.py
    # and the two-phase second phase buildable with no key material at all.
    for keyless in ("protect", "approtect@auth-lock", "approtect+protect@hw-lock"):
        check(
            f"{keyless} needs no key material",
            build_from(rec, keyless),
            build(keyless, counters=counters(rec)),
        )
    raises(
        "an install batch built with no key source",
        lambda: build_from(rec, f"seed{SEED_SEP}check+install"),
    )
    # A bundle prepared without the KEKs says so, and then the KEK-gated batches
    # are absent rather than wrong.
    bare_rec = record(keyset="keyring:x", ota_kek=False)
    check("a KEK-free record admits it", has_ota_kek(bare_rec), False)
    check(
        "and refuses only the KEK-gated names",
        sorted(n for n in names() if not available(bare_rec, n)),
        sorted(set(names()) - set(names(ota_kek=False))),
    )
    raises(
        "a KEK-gated batch from a KEK-free record",
        lambda: build_from(bare_rec, f"seed{SEED_SEP}check+install+protect"),
    )
    check(
        "an install batch from a KEK-free record needs no keys",
        build_from(bare_rec, f"seed{SEED_SEP}check+install"),
        build(f"seed{SEED_SEP}check+install", counters=counters(bare_rec)),
    )

    # The digest is what makes a re-keyed bundle detectable.
    keys = {"urot": {0: {"provisioned": True, "key": "aa"}}}
    check("a digest over no keys is stable", keys_digest({}), keys_digest({}))
    check("the digest passes its own keyset", check_digest(record(
        keyset="keyring:x", digest=keys_digest(keys)), keys), None)
    raises(
        "a bundle re-keyed since it was prepared",
        lambda: check_digest(record(keyset="keyring:x", digest=keys_digest(keys)), {}),
    )
    check(
        "a record with no digest recorded is not held to one",
        check_digest(record(keyset="keyring:x"), keys),
        None,
    )

    # Both vocabularies name the same two modes, and nothing else does.
    check("the friendly spellings", closure_arg("hardware-fixed"), "hw-lock")
    check("the ABI spellings still parse", closure_arg("auth-lock"), "auth-lock")
    check("the default method", closure_arg(DEFAULT_CLOSURE_METHOD), DEFAULT_AP_CLOSURE)
    check("every ABI mode has a label", sorted(CLOSURE_LABELS), sorted(abi.APPROTECT_MODE))
    check(
        "every method maps to an ABI mode",
        sorted(set(CLOSURE_METHODS.values())),
        sorted(abi.APPROTECT_MODE),
    )
    raises("an unknown closure method", lambda: closure_arg("open"))

    # Naming refuses what it cannot spell unambiguously.
    raises("an AP closure on a batch that closes nothing", lambda: name(("install",), "hw-lock"))
    raises(
        "a SEED mode on a batch that enrolls none",
        lambda: name(("install",), None, "user"),
    )
    raises("an unknown SEED mode", lambda: name(("seed",), None, "bogus"))
    raises(
        "a name missing the SEED mode it must have",
        lambda: parse("seed+install"),
    )
    raises(
        "a SEED mode on a group that cannot take one",
        lambda: parse(f"install{SEED_SEP}user"),
    )
    raises("an unknown operation group", lambda: name(("bogus",)))
    raises("an unknown AP-closure mode", lambda: name(("approtect",), "bogus"))
    raises("a name with a closure it must not have", lambda: parse("install@hw-lock"))
    raises("a name missing the closure it must have", lambda: parse("approtect"))
    raises("an unbuildable batch name", lambda: build_from(rec, "protect@hw-lock"))
    raises("a counter role that does not exist", lambda: counters_arg("bogus=1"))

    # --- application keys ride inside the install group ---
    import struct as _struct

    def _attrs(key_id, key_bits=256):
        return _struct.pack(
            ak.ATTRS_PACK, 0x2400, key_bits, ak.PERSISTENCE_ROTATING,
            0x00030300, 0, 0, key_id, 0,
        )

    app = tuple((_attrs(key_id), bytes(32)) for key_id in (164, 168, 172))
    app_inventory = [{"id": key_id, "type": 0x2400, "bits": 256} for key_id in (164, 168, 172)]
    keyed = build_all(keks=keks, app_keys=app)

    # The no-keys path has to stay byte-identical: every other check in the
    # repository that builds a bundle takes it.
    check("no application keys is the batch set it was before",
          build_all(keks=keks, app_keys=()), full)
    check("application keys do not widen the batch name space",
          sorted(keyed), sorted(full))
    check(
        "the install group carries one import-key per application key",
        [abi.op_name(o) for o in abi.parse_input_batch(keyed["install"])["operations"]],
        ["select-ikg-seed", "init-urot-storage"] + ["provision-counter"] * 3
        + ["import-fw-kek"] * 3 + ["import-key"] * 3,
    )
    check(
        "no batch outside the install group imports an application key",
        sorted(n for n in keyed if "import-key" in
               [abi.op_name(o) for o in abi.parse_input_batch(keyed[n])["operations"]]),
        sorted(n for n in keyed if "install" in parse(n)[0]),
    )
    app_rec = validate(record(keyset="keyring:x", app_keys=app_inventory))
    check(
        "the key IDs a batch imports under are the key IDs the record names",
        app_key_ids_in(build_from(app_rec, "install", keks=keks, app_keys=app)),
        (164, 168, 172),
    )
    check(
        "the recorded inventory round-trips through the record",
        app_keys(validate(json.loads(json.dumps(app_rec)))),
        app_inventory,
    )
    check("a record with no application keys records an empty inventory",
          app_keys(record(keyset="keyring:x")), [])
    raises(
        "a record naming a key ID outside the user range",
        lambda: validate(record(keyset="keyring:x", app_keys=[{"id": ak.KEY_ID_MAX + 1}])),
    )
    raises(
        "a record naming a key ID reserved for the OTA KEKs",
        lambda: validate(record(keyset="keyring:x", app_keys=[{"id": ak.RESERVED_KEY_ID_MIN}])),
    )

    # A keyless reader can reproduce neither an application key's attributes nor
    # its material, so comparable() leaves the imports out and the key IDs are
    # what the record is held to instead.
    check(
        "the application keys are outside the comparison",
        comparable(build("install", counters=moved, keks=keks, app_keys=app)),
        comparable(build("install", counters=moved, keks=PLACEHOLDER_KEKS)),
    )

    # Moving the counter floors is a configuration edit, and a batch rebuilt
    # from the moved record still carries the application keys.
    moved = {"urot": 9, "app": 5, "dfu": 7}
    check("the counters still move on a record naming application keys",
          set_counters(app_rec, moved), True)
    check(
        "a batch built from the moved record is the rebuild, keys included",
        build_from(app_rec, "install", keks=keks, app_keys=app),
        build("install", counters=moved, keks=keks, app_keys=app),
    )
    check(
        "the counters are still readable out of an application-key batch",
        counters_in(build("install", counters=moved, keks=keks, app_keys=app)),
        moved,
    )

    # The operation cap is the device decoder's, and the message has to name a
    # number the caller can act on: the keys they may provision, not the
    # operations the batch ends up with.
    widest = max(
        len(abi.parse_input_batch(blob)["operations"]) for blob in full.values()
    )
    check("the widest batch without application keys", widest, 16)
    room = abi.MAX_BATCH_OPS - widest
    check("the application keys that fit beside it", room, 48)
    fits = tuple((_attrs(key_id), bytes(32)) for key_id in range(1, 1 + room))
    check(
        "a full complement of application keys fits every batch",
        max(
            len(abi.parse_input_batch(blob)["operations"])
            for blob in build_all(keks=keks, app_keys=fits).values()
        ),
        abi.MAX_BATCH_OPS,
    )
    try:
        build_all(keks=keks, app_keys=fits + ((_attrs(1 + room), bytes(32)),))
    except BatchError as exc:
        # Stated as keys, not as operations: the caller cannot split the
        # transition, only provision fewer keys.
        check("one key more than fits is refused as a key count",
              (f"{room + 1} application keys" in str(exc),
               f"room for {room}" in str(exc)),
              (True, True))
    else:
        failures.append("one application key more than fits: expected a refusal")

    # A failed SEED check is explained in terms of the bundle setting, and a
    # batch whose check passed has nothing to explain.
    def seed_result(blob, failing_check, status):
        ops = abi.parse_input_batch(blob)["operations"]
        checks = [i for i, op in enumerate(ops) if abi.op_name(op) == "check-ikg-seed"]
        results = [{"status": 0} for _ in ops]
        results[checks[failing_check]] = {"status": status}
        for i in range(checks[failing_check] + 1, len(ops)):
            results[i] = {"status": abi.ITE_ERROR_ABORTED}
        return seed_failure(ops, results)

    unseeded = seed_result(full[f"seed{SEED_SEP}check+install"], 0, abi.PSA_ERROR_BAD_STATE)
    check(
        "an unseeded device under check names the setting to configure",
        ("unseeded" in unseeded, "update-config --provision-user-seed yes" in unseeded),
        (True, True),
    )
    revoked = seed_result(full[f"seed{SEED_SEP}user+install"], 0, abi.PSA_ERROR_NOT_PERMITTED)
    check(
        "a revoked User SEED is refused before any SEED write",
        ("revoked" in revoked, "Nothing was written" in revoked,
         "provision-user-seed: no" in revoked),
        (True, True, True),
    )
    torn = seed_result(full[f"seed{SEED_SEP}user+install"], 1, abi.PSA_ERROR_CORRUPTION_DETECTED)
    check("torn SEED slots are reported as unrepairable", "cannot be repaired" in torn, True)
    failed = seed_result(full[f"seed{SEED_SEP}user+install"], 1, abi.PSA_ERROR_BAD_STATE)
    check("a seeding that did not land says so", "Seeding the device failed" in failed, True)
    clean = abi.parse_input_batch(full[f"seed{SEED_SEP}check+install"])["operations"]
    check("a passing check explains nothing", seed_failure(clean, [{"status": 0}] * len(clean)), None)

    for line in failures:
        print(f"FAIL  {line}")
    print(f"prov_batches: {'FAILED' if failures else 'ok'} ({len(failures)} failure(s))")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(_self_check())
