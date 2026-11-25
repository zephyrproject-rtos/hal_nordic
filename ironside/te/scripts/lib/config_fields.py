# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Build configuration parsers and profile validators from the field resource.

The resource names every field the base, image and provisioning configuration
profiles hold: the flag it is spelled as, the values it accepts, the help it
prints, and whether the bundle tool, the generator or both offer it. A field
name is both the flag without its leading dashes and the key it takes in a
profile file, so one declaration covers the command line and the JSON profile.

Nothing here encodes a value. A field needing more than a type conversion names
a parser hook, resolved on use to a function in the module that owns the
encoding, so importing this module pulls in no encoder.
"""

from __future__ import annotations

import argparse
import importlib
import json
from dataclasses import dataclass
from pathlib import Path

RESOURCE = Path(__file__).resolve().parent.parent / "resources" / "config_fields.json"

SCHEMA = 1

BUNDLE = "bundle"
GENERATOR = "generator"
SCOPES = (BUNDLE, GENERATOR)

VALUE_KINDS = ("string", "int", "int0", "float", "flag", "string-list")

# Parser hooks as (module, attribute), imported on use. Reading the resource
# must not require the encoders: a caller with no CBOR support still gets the
# values a field accepts.
_PARSER_HOOKS = {
    "ap-closure": (".prov_conf_args", "closure"),
    "provision-user-seed": (".prov_conf_args", "user_seed"),
    "ikg-seed-select": (".base_conf_args", "ikg_seed_select"),
}


class ConfigFieldError(Exception):
    """The field resource is malformed, or a caller named something absent."""


@dataclass(frozen=True)
class Enum:
    """A named set of values a field accepts, with whatever each one carries.

    Declaration order is the order help text lists them in.
    """

    name: str
    values: tuple[dict, ...]

    @property
    def choices(self) -> tuple[str, ...]:
        return tuple(v["name"] for v in self.values)

    @property
    def disable_value(self) -> str | None:
        return next((v["name"] for v in self.values if v.get("disables")), None)

    def attributes(self, key: str) -> dict:
        """{value name: attribute} over the values carrying @key."""
        return {v["name"]: v[key] for v in self.values if key in v}


@dataclass(frozen=True)
class Field:
    name: str
    dest: str
    value: str
    help: str
    profile: bool
    scope: tuple[str, ...]
    editing_only: bool
    hidden: bool
    metavar: str | None
    unit: str
    default: object
    default_value: str | None
    values_enum: str | None
    off_label: str | None
    value_format: str | None
    join: str
    enums: dict
    constraints: tuple[str, ...]
    reference_kb: int | None
    parser: str | None
    label: str | None
    region: str | None
    partition: str | None
    exclusive_with: tuple[str, ...]
    disables: str | None
    disabled_by: str | None

    @property
    def flag(self) -> str:
        return f"--{self.name}"

    @property
    def is_switch(self) -> bool:
        return self.value == "flag"

    @property
    def display_label(self) -> str:
        """What a listing calls this field: its own label, else its flag name."""
        return self.label or self.name


@dataclass(frozen=True)
class Profile:
    name: str
    title: str
    short_title: str
    generator_title: str
    profile_file: str
    summary: str
    fields: tuple[Field, ...]
    scopes: dict
    all_or_nothing: dict | None
    formatversions: tuple[str, ...]

    def field(self, name: str) -> Field:
        for entry in self.fields:
            if entry.name == name:
                return entry
        raise ConfigFieldError(f"{self.name} configuration has no {name!r} field")


def profile(name: str) -> Profile:
    try:
        return PROFILES[name]
    except KeyError:
        raise ConfigFieldError(
            f"unknown configuration profile {name!r} (known: {', '.join(PROFILES)})"
        ) from None


def enum(name: str) -> Enum:
    try:
        return ENUMS[name]
    except KeyError:
        raise ConfigFieldError(f"unknown value set {name!r} (known: {', '.join(ENUMS)})") from None


def choices(name: str) -> tuple[str, ...]:
    return enum(name).choices


def fields(profile_name: str, *, scope: str = BUNDLE, editing: bool = True) -> tuple[Field, ...]:
    """The fields @scope offers, in the order they are declared."""
    return tuple(
        f
        for f in profile(profile_name).fields
        if scope in f.scope and (editing or not f.editing_only)
    )


def profile_flags(profile_name: str) -> dict:
    """{field name: argparse destination} for the fields a profile file holds."""
    return {f.name: f.dest for f in profile(profile_name).fields if f.profile}


def add_arguments(
    ap,
    profile_name: str,
    group_title: str | None = None,
    *,
    editing: bool = True,
    scope: str = BUNDLE,
):
    """Add a profile's flags to @ap and return what they were added to."""
    prof = profile(profile_name)
    target = ap if group_title is None else ap.add_argument_group(group_title)
    for entry in fields(profile_name, scope=scope, editing=editing):
        target.add_argument(entry.flag, **_argument(entry, prof, scope))
    return target


def given(args, profile_name: str) -> bool:
    """Whether the caller named any of a profile's fields.

    A switch counts when it is set, and every other field when it carries a
    value, so a field whose disabling value is 0 or an empty list still counts.
    """
    return any(_is_set(args, entry) for entry in profile(profile_name).fields)


def _is_set(args, entry: Field) -> bool:
    value = getattr(args, entry.dest, None)
    return bool(value) if entry.is_switch else value is not None


def display(profile_name: str, data: dict, *, region: str | None = None) -> list[tuple[str, str]]:
    """(label, value) pairs for the profile fields @data carries, for a listing.

    A field @data does not name has no line. A field turned off renders as the
    off label the resource gives it, and the switch that turns it off has no
    line of its own - it is read as its partner's value.
    """
    out = []
    for entry in profile(profile_name).fields:
        if entry.disables or (region is not None and entry.region != region):
            continue
        if _is_off(entry, data):
            out.append((entry.display_label, entry.off_label))
        elif entry.name in data:
            out.append((entry.display_label, _rendered(entry, data[entry.name])))
    return out


def _is_off(entry: Field, data: dict) -> bool:
    """Whether @data turns @entry off, by its switch or by its own value."""
    if entry.off_label is None:
        return False
    if entry.disabled_by and data.get(entry.disabled_by):
        return True
    if entry.name not in data:
        return False
    value = data[entry.name]
    if not value:
        return True
    if isinstance(value, list) and entry.values_enum:
        return value == [enum(entry.values_enum).disable_value]
    return False


def _rendered(entry: Field, value) -> str:
    if entry.value_format == "hex":
        return f"0x{value:x}"
    if entry.values_enum:
        labels = enum(entry.values_enum).attributes("label")
        if isinstance(value, list):
            return entry.join.join(labels.get(name, name) for name in value)
        return labels.get(value, value)
    return f"{value} {entry.unit}" if entry.unit else str(value)


def exclusive_conflict(args, profile_name: str) -> tuple | None:
    """The first pair of flags a caller set that the resource declares exclusive.

    Returns the two flags in declaration order, or None. The caller raises,
    because the error type belongs to whichever encoder owns the field.
    """
    prof = profile(profile_name)
    for entry in prof.fields:
        if not _is_set(args, entry):
            continue
        partners = (*entry.exclusive_with, *((entry.disables,) if entry.disables else ()))
        for name in partners:
            other = prof.field(name)
            if not _is_set(args, other):
                continue
            first, second = sorted((entry, other), key=prof.fields.index)
            return first.flag, second.flag
    return None


def validate_profile(data, profile_name: str, label: str) -> dict:
    """Check a loaded profile file against the resource and return it.

    Raises ConfigFieldError naming @label, which is the file the data came from.
    """
    if not isinstance(data, dict):
        raise ConfigFieldError(f"{label}: expected a JSON object, got {type(data).__name__}")
    allowed = profile_flags(profile_name)
    unknown = sorted(set(data) - set(allowed))
    if unknown:
        raise ConfigFieldError(f"{label}: unknown field(s): {', '.join(unknown)}")
    group = profile(profile_name).all_or_nothing
    if group:
        missing = [name for name in group["fields"] if name not in data]
        if missing and len(missing) != len(group["fields"]):
            raise ConfigFieldError(
                f"{label}: {group['what']} are all-or-nothing; also give " + ", ".join(missing)
            )
    return data


def parser_hook(name: str):
    """The argparse type a field's parser hook names."""
    try:
        module_name, attribute = _PARSER_HOOKS[name]
    except KeyError:
        raise ConfigFieldError(f"unknown parser hook {name!r}") from None
    return getattr(importlib.import_module(module_name, __package__), attribute)


def _argument(entry: Field, prof: Profile, scope: str) -> dict:
    """The add_argument() keywords a field declares."""
    kwargs = {
        "dest": entry.dest,
        "help": argparse.SUPPRESS if entry.hidden else _help(entry, prof, scope),
    }
    if entry.is_switch:
        kwargs["action"] = "store_true"
        return kwargs
    kwargs["default"] = entry.default
    if entry.metavar is not None:
        kwargs["metavar"] = entry.metavar
    if entry.parser is not None:
        kwargs["type"] = parser_hook(entry.parser)
    elif entry.value == "int":
        kwargs["type"] = int
    elif entry.value == "int0":
        kwargs["type"] = _int_any_base
    elif entry.value == "float":
        kwargs["type"] = float
    elif entry.value == "string-list":
        kwargs["nargs"] = "+"
    return kwargs


def _int_any_base(text: str) -> int:
    return int(text, 0)


def _help(entry: Field, prof: Profile, scope: str) -> str:
    """A field's help, with its value sets, default and constraints filled in."""
    text = _fill(entry.help, _help_tokens(entry))
    if entry.constraints:
        note = _fill(prof.scopes[scope]["omitted-note"], {"reference-kb": str(entry.reference_kb)})
        text += " (" + ", ".join([*entry.constraints, note]) + ")"
    return text


def _help_tokens(entry: Field) -> dict:
    tokens = {
        token: spec["join"].join(choices(spec["enum"])) for token, spec in entry.enums.items()
    }
    if entry.default_value is not None:
        tokens["default"] = entry.default_value
    return tokens


def _fill(template: str, tokens: dict) -> str:
    for token, value in tokens.items():
        template = template.replace("{" + token + "}", value)
    return template


def _load() -> dict:
    try:
        document = json.loads(RESOURCE.read_text())
    except (OSError, json.JSONDecodeError) as exc:
        raise ConfigFieldError(f"{RESOURCE}: {exc}") from exc
    if document.get("schema") != SCHEMA:
        raise ConfigFieldError(
            f"{RESOURCE}: schema {document.get('schema')!r} is not the supported {SCHEMA}"
        )
    return document


def _read_enums(document: dict) -> dict:
    out = {}
    for name, spec in document["enums"].items():
        values = tuple(spec["values"])
        if not values:
            raise ConfigFieldError(f"{RESOURCE}: value set {name!r} is empty")
        out[name] = Enum(name=name, values=values)
    return out


def _read_field(spec: dict, profile_name: str) -> Field:
    name = spec["name"]
    where = f"{RESOURCE}: {profile_name} field {name!r}"
    kind = spec.get("value", "string")
    if kind not in VALUE_KINDS:
        raise ConfigFieldError(f"{where}: unknown value kind {kind!r}")
    scope = tuple(spec.get("scope", SCOPES))
    unknown_scope = [s for s in scope if s not in SCOPES]
    if unknown_scope:
        raise ConfigFieldError(f"{where}: unknown scope(s) {', '.join(unknown_scope)}")
    enums = {}
    for token, reference in spec.get("enums", {}).items():
        if isinstance(reference, str):
            reference = {"enum": reference}
        enums[token] = {"enum": reference["enum"], "join": reference.get("join", "|")}
    shown = spec.get("display", {})
    return Field(
        name=name,
        dest=spec.get("dest", name.replace("-", "_")),
        value=kind,
        help=spec.get("help", ""),
        profile=bool(spec.get("profile")),
        scope=scope,
        editing_only=bool(spec.get("editing-only")),
        hidden=bool(spec.get("hidden")),
        metavar=spec.get("metavar"),
        unit=spec.get("unit", ""),
        default=spec.get("default"),
        default_value=spec.get("default-value", spec.get("default")),
        values_enum=spec.get("values"),
        off_label=shown.get("off"),
        value_format=shown.get("format"),
        join=shown.get("join", " "),
        enums=enums,
        constraints=tuple(spec.get("constraints", ())),
        reference_kb=spec.get("reference-kb"),
        parser=spec.get("parser"),
        label=spec.get("label"),
        region=spec.get("region"),
        partition=spec.get("partition"),
        exclusive_with=tuple(spec.get("exclusive-with", ())),
        disables=spec.get("disables"),
        disabled_by=spec.get("disabled-by"),
    )


def _read_profiles(document: dict, enums: dict) -> dict:
    out = {}
    for name, spec in document["profiles"].items():
        entries = tuple(_read_field(f, name) for f in spec["fields"])
        title = spec["title"]
        out[name] = Profile(
            name=name,
            title=title,
            short_title=spec.get("short-title", title),
            generator_title=spec.get("generator-title", title),
            profile_file=spec.get("profile-file", ""),
            summary=spec.get("summary", ""),
            fields=entries,
            scopes=spec.get("scopes", {}),
            all_or_nothing=spec.get("all-or-nothing"),
            formatversions=tuple(spec.get("formatversions", ())),
        )
        _check_profile(out[name], enums)
    return out


def _check_profile(prof: Profile, enums: dict) -> None:
    """Refuse a profile naming a field, value set or help token that is absent."""
    where = f"{RESOURCE}: {prof.name} configuration"
    names = {f.name for f in prof.fields}
    dests = [f.dest for f in prof.fields]
    if len(set(dests)) != len(dests):
        raise ConfigFieldError(f"{where}: two fields share one destination")
    for entry in prof.fields:
        tokens = set(entry.enums)
        for spec in entry.enums.values():
            if spec["enum"] not in enums:
                raise ConfigFieldError(f"{where}: {entry.name!r} names value set {spec['enum']!r}")
        if entry.default_value is not None:
            tokens.add("default")
        _check_tokens(entry.help, tokens, f"{where}: {entry.name!r} help")
        for other in (*entry.exclusive_with, entry.disables, entry.disabled_by):
            if other is not None and other not in names:
                raise ConfigFieldError(f"{where}: {entry.name!r} names absent field {other!r}")
        if entry.constraints and entry.reference_kb is None:
            raise ConfigFieldError(f"{where}: {entry.name!r} has constraints but no reference size")
    for scope, spec in prof.scopes.items():
        if scope not in SCOPES:
            raise ConfigFieldError(f"{where}: unknown scope {scope!r}")
        _check_tokens(spec["omitted-note"], {"reference-kb"}, f"{where}: {scope} note")
    if prof.all_or_nothing:
        absent = [n for n in prof.all_or_nothing["fields"] if n not in names]
        if absent:
            raise ConfigFieldError(f"{where}: all-or-nothing names {', '.join(absent)}")


def _check_tokens(template: str, allowed: set, where: str) -> None:
    rest = _fill(template, dict.fromkeys(allowed, ""))
    if "{" in rest:
        raise ConfigFieldError(f"{where}: unresolvable token in {template!r}")


_DOCUMENT = _load()
ENUMS = _read_enums(_DOCUMENT)
PROFILES = _read_profiles(_DOCUMENT, ENUMS)


def _self_check() -> int:
    """Hold the resource against the modules that encode what it declares.

    A value the resource accepts but an encoder refuses is a flag that parses
    and then fails at the point a bundle is written, so the two tables are
    compared here rather than trusted to stay in step. Imports the encoders,
    which nothing else in this module does.
    """
    from . import conf_img as ci
    from . import prov_batches as pb

    failures = []

    def check(what, got, want):
        if got != want:
            failures.append(f"{what}: got {got!r}, want {want!r}")

    closure = enum("ap-closure")
    check("the closure methods", closure.attributes("abi"), pb.CLOSURE_METHODS)
    check(
        "the closure labels",
        {abi: closure.attributes("label")[name] for name, abi in closure.attributes("abi").items()},
        pb.CLOSURE_LABELS,
    )
    check(
        "the default closure method",
        PROFILES["provisioning"].field("ap-closure").default_value,
        pb.DEFAULT_CLOSURE_METHOD,
    )

    seed = enum("provision-user-seed")
    check("the User SEED choices", seed.attributes("abi"), pb.USER_SEED_CHOICES)
    check(
        "the User SEED labels",
        {abi: seed.attributes("label")[name] for name, abi in seed.attributes("abi").items()},
        pb.SEED_LABELS,
    )
    check(
        "the default User SEED choice",
        pb.USER_SEED_CHOICES[PROFILES["provisioning"].field("provision-user-seed").default_value],
        pb.DEFAULT_IKG_SEED,
    )

    resetreas = enum("resetreas")
    check(
        "the reset reasons",
        sorted(n for n in resetreas.choices if n != resetreas.disable_value),
        sorted(ci.RESETREAS),
    )
    check("the GPIO pulls", sorted(choices("gpio-pull")), sorted(ci.GPIO_PULL))
    check(
        "the implied pull of every level",
        sorted(enum("gpio-level").attributes("implied-pull").values()),
        sorted({"pullup", "pulldown"}),
    )
    check(
        "the image configuration format version",
        PROFILES["image"].formatversions,
        (f"{ci.CONF_IMG_FORMATVERSION >> 16}.{ci.CONF_IMG_FORMATVERSION & 0xFFFF}",),
    )
    check(
        "the power-fail comparator's disable partner",
        PROFILES["image"].field("pof").disabled_by,
        PROFILES["image"].field("no-pof").name,
    )
    check(
        "the bits the power-fail comparator configuration may reach",
        ci.POF_ENABLE | (ci.POF_THRESHOLD_MAX << ci.POF_THRESHOLD_POS) | ci.POF_EVENTDISABLE,
        ci.encode_pofcon(ci.POF_THRESHOLD_MAX, event_disable=True),
    )
    check("the regulator modes", sorted(choices("vreg-mode")), ["dcdc", "ldo"])
    check(
        "the bits the LFXO board field may reach",
        ci.BOARD_LFXO_SUPPORTED,
        ci.encode_board_lfxo(ci.BOARD_LFXO_LOAD_CAP_PF_MAX, bypass=True),
    )
    check(
        "the bits the HFXO board field may reach",
        ci.BOARD_HFXO_SUPPORTED,
        ci.encode_board_hfxo(ci.BOARD_HFXO_LOAD_CAP_PF_MAX),
    )
    check(
        "the bits the regulator board field may reach",
        ci.BOARD_VREG_SUPPORTED,
        ci.encode_board_vreg(dcdc=True),
    )

    from . import uicr

    partitions = [f for f in PROFILES["base"].fields if f.partition]
    check("the partitions", [f.partition for f in partitions], list(uicr.PARTITION_NAMES))
    check(
        "the reference layout",
        [f.reference_kb for f in partitions],
        list(uicr.REFERENCE_PARTITIONS_KB),
    )
    check(
        "the partitions a zero size disables",
        [f.partition for f in partitions if "0 disables" in f.constraints],
        list(uicr.PARTITION_OPTIONAL),
    )
    for entry in partitions:
        check(
            f"{entry.name} states the block size",
            f"multiple of {uicr.PARTITION_BLOCK_KB}" in entry.constraints,
            True,
        )
    check(
        "the IKG SEED selections",
        sorted(choices("ikg-seed-select")),
        sorted([*uicr.IKG_SEED_SELECTIONS, uicr.IKG_SEED_SELECT_AUTO]),
    )
    check(
        "the base configuration format version",
        PROFILES["base"].field("format-version").default,
        uicr.SUPPORTED_FORMATVERSIONS[0],
    )

    # Every profile builds a parser, which resolves the parser hooks and proves
    # each declared destination is one argparse accepts.
    for name, prof in PROFILES.items():
        for scope in SCOPES:
            if not fields(name, scope=scope):
                continue
            ap = argparse.ArgumentParser(add_help=False)
            add_arguments(ap, name, None, scope=scope)
            parsed = vars(ap.parse_args([]))
            check(
                f"{name}/{scope} parses into the declared destinations",
                sorted(parsed),
                sorted(f.dest for f in fields(name, scope=scope)),
            )
        check(
            f"{name} profile keys are the flag spellings",
            sorted(profile_flags(name)),
            sorted(f.name for f in prof.fields if f.profile),
        )

    for line in failures:
        print(f"FAIL  {line}")
    print(f"config_fields: {'FAILED' if failures else 'ok'} ({len(failures)} failure(s))")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(_self_check())
