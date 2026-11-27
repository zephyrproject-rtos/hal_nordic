# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Import and export JSON profiles for IronSide bundle configuration."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from types import SimpleNamespace

from . import base_conf_args
from . import bundle_edit as be
from . import conf_img, fw_bundle, img_conf_args, parts, prov_conf_args, uicr
from . import config_fields as cf
from . import generate_production_hex as ph
from . import prov_batches as pb
from . import psa_key_ram as pkr

STDIO = Path("-")


def _region_flags(region: str) -> tuple:
    """The base-config profile fields the @region image reads, in field order."""
    return tuple(f.name for f in cf.profile("base").fields if f.profile and f.region == region)


IROT_BASE_PROFILE_FLAGS = _region_flags("irot")
UROT_BASE_PROFILE_FLAGS = _region_flags("urot")
IROT_KEY_ROLES = ("urot",)
UROT_KEY_ROLES = ("dfu", "app", "authopkey")

# The partition a base-config profile field carries, by field name.
_PARTITION_OF = {
    f.name: f.partition for f in cf.profile("base").fields if f.profile and f.partition
}
KEY_SLOT_STORAGE_SIZE = cf.profile("base").field("key-slot-storage-size-bytes")
IKG_SEED_SELECT = cf.profile("base").field("ikg-seed-select")


class ProfileError(Exception):
    """A profile file is malformed or inconsistent."""


def profile_label(path: Path, *, writing=False) -> str:
    """A name for @path in errors and status lines."""
    if path == STDIO:
        return "<stdout>" if writing else "<stdin>"
    return str(path)


def _validate(data, profile: str, label: str) -> dict:
    try:
        return cf.validate_profile(data, profile, label)
    except cf.ConfigFieldError as exc:
        raise ProfileError(str(exc)) from exc


def _to_namespace(data: dict, profile: str) -> SimpleNamespace:
    return SimpleNamespace(
        **{dest: data.get(flag) for flag, dest in cf.profile_flags(profile).items()}
    )


def _gpio_to_cli(gpio: dict) -> str:
    port, pin = gpio["port"], gpio["pin"]
    level = gpio["active_level"]
    pull = gpio["pull"]
    implied = img_conf_args.GPIO_LEVEL_PULL[level]
    if pull == implied:
        return f"P{port}.{pin},{level}"
    return f"P{port}.{pin},{level},{pull}"


def _pof_to_cli(pof: dict) -> str:
    code = pof["threshold"]
    return f"{code},{img_conf_args.POF_NOEVENT}" if pof["event_disable"] else str(code)


def _board_lfxo_to_cli(lfxo: dict) -> str:
    cap = lfxo["load_cap_pf"]
    return f"{cap},{img_conf_args.BOARD_LFXO_BYPASS}" if lfxo["bypass"] else str(cap)


def export_base(bundle: be.Editable) -> dict:
    """All base-config profile fields, including defaults, as CLI flag spellings.

    The IKG SEED selection has an entry only once set: unset is auto.
    """
    parts = be.effective_partitions_kb(bundle)
    out = {flag: parts[partition] for flag, partition in _PARTITION_OF.items()}
    out[KEY_SLOT_STORAGE_SIZE.name] = be.key_slot_storage_size(bundle)
    selected = be.ikg_seed_select(bundle)
    if selected is not None:
        out[IKG_SEED_SELECT.name] = selected
    return out


def export_image(conf: bytes | None) -> dict:
    """All image-config profile fields, including defaults, as CLI flag spellings.

    A board field that is not set has no entry, so importing the profile leaves
    it unset.
    """
    if conf is None:
        return {
            "no-dfu-trigger-gpio": True,
            "dfu-trigger-resetreas": [img_conf_args.RESETREAS_NONE],
            "no-dfu-trigger-fault": True,
            "wdtstart-crv-ms": 0,
            "no-pof": True,
            "authop-revoke-gen": "none",
        }
    decoded = conf_img.unpack(conf)
    out = {}
    gpio = decoded["dfu_trigger"]["gpio"]
    if gpio:
        out["dfu-trigger-gpio"] = _gpio_to_cli(gpio)
    else:
        out["no-dfu-trigger-gpio"] = True
    resetreas = decoded["dfu_trigger"]["resetreas"]
    out["dfu-trigger-resetreas"] = resetreas if resetreas else [img_conf_args.RESETREAS_NONE]
    if decoded["dfu_trigger"]["fault"]:
        out["dfu-trigger-fault"] = True
    else:
        out["no-dfu-trigger-fault"] = True
    crv = decoded["wdtstart"]["crv"]
    out["wdtstart-crv-ms"] = round(decoded["wdtstart"]["timeout_s"] * 1000) if crv else 0
    pof = decoded["pof"]
    if pof["enabled"]:
        out["pof"] = _pof_to_cli(pof)
    else:
        out["no-pof"] = True
    board = decoded["board"]
    if board["lfxo"]:
        out["board-lfxo"] = _board_lfxo_to_cli(board["lfxo"])
    if board["hfxo"]:
        out["board-hfxo"] = board["hfxo"]["load_cap_pf"]
    if board["vreg"]:
        out["board-vreg"] = board["vreg"]["mode"]
    gen = decoded["authop"]["revoke_gen"]
    out["authop-revoke-gen"] = "none" if gen is None else str(gen)
    return out


def read_json(path: Path) -> dict:
    """Load a profile from @path, or from stdin when @path is STDIO."""
    label = profile_label(path)
    try:
        text = sys.stdin.read() if path == STDIO else path.read_text()
        data = json.loads(text)
    except json.JSONDecodeError as exc:
        raise ProfileError(f"{label}: {exc}") from exc
    return data


def load_base(path: Path) -> dict:
    return _validate(read_json(path), "base", profile_label(path))


def load_image(path: Path) -> dict:
    return _validate(read_json(path), "image", profile_label(path))


def write_json(path: Path, data: dict) -> None:
    text = json.dumps(data, indent=2) + "\n"
    if path == STDIO:
        sys.stdout.write(text)
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)


def apply_base(bundle: be.Editable, data: dict) -> None:
    if KEY_SLOT_STORAGE_SIZE.name in data:
        be.set_key_slot_storage_size(bundle, data[KEY_SLOT_STORAGE_SIZE.name])
    if IKG_SEED_SELECT.name in data:
        try:
            entry = base_conf_args.ikg_seed_select(str(data[IKG_SEED_SELECT.name]))
        except argparse.ArgumentTypeError as exc:
            raise ProfileError(str(exc)) from exc
        be.set_ikg_seed_select(bundle, entry)
    if not any(flag in data for flag in _PARTITION_OF):
        return
    sizes = {partition: data[flag] for flag, partition in _PARTITION_OF.items()}
    uicr.partition_addresses(
        [sizes[name] for name in uicr.PARTITION_NAMES], soc=bundle.require_soc()
    )
    be.set_partitions(bundle, {name: sizes[name] for name in uicr.PARTITION_NAMES})


def apply_image(bundle: be.Editable, data: dict) -> None:
    ns = _to_namespace(data, "image")
    if not img_conf_args.given(ns):
        return
    be.edit_image_config(bundle.require("urot"), ns)


def show_base(bundle: be.Editable) -> dict:
    """base_config for `show`, split by what IROT and UROT each read."""
    exported = export_base(bundle)
    region = be.region_report(bundle)["region"]
    keys = region.get("keys", {})
    irot = {flag: exported[flag] for flag in IROT_BASE_PROFILE_FLAGS}
    if keys.get("urot"):
        irot["keys"] = {"urot": keys["urot"]}
    urot = {flag: exported.get(flag, uicr.IKG_SEED_SELECT_AUTO) for flag in UROT_BASE_PROFILE_FLAGS}
    urot_keys = {role: keys[role] for role in UROT_KEY_ROLES if keys.get(role)}
    if urot_keys:
        urot["keys"] = urot_keys
    lock = region.get("authlock")
    if lock:
        urot["authlock"] = lock
    return {"irot": irot, "urot": urot}


def export_provisioning(bundle: be.Editable) -> dict:
    """The provisioning-configuration profile of @bundle.

    The carried configuration verbatim: it is already in the profile's field
    names and vocabulary, so an export and a re-import round-trip unchanged.
    """
    return dict(be.provisioning_config(bundle))


def show_error(prov: dict) -> str:
    """Why a bundle has no readable provisioning configuration."""
    return prov.get("error") or (
        "not prepared: run update-keys to provision a keyset and its provisioning firmware"
    )


def load_provisioning(path: Path) -> dict:
    return _validate(read_json(path), "provisioning", profile_label(path))


def apply_provisioning(bundle: be.Editable, data: dict) -> bool:
    """Apply a provisioning-configuration profile. True when @bundle changed.

    Through the same edit path `update-config --ap-closure` uses, so a profile
    and a flag cannot diverge. Writes the configuration and nothing else, so it
    needs no key source: `update-keys` is what carries a change into the route
    firmware.
    """
    ns = _to_namespace(data, "provisioning")
    if not prov_conf_args.given(ns):
        return False
    changed = False
    if ns.ap_closure is not None:
        changed |= be.set_ap_closure(bundle, prov_conf_args.closure(ns.ap_closure))
    if ns.provision_user_seed is not None:
        changed |= be.set_ikg_seed(bundle, prov_conf_args.user_seed(ns.provision_user_seed))
    return changed


def show_provisioning(bundle: be.Editable) -> dict:
    """provisioning_config for `show`: the configuration, and what it applies to.

    The rest of the carried member is reported beside it because nothing else
    shows it, and a bundle whose batches are stale, KEK-free or missing their
    route hexes is otherwise indistinguishable from a good one.
    """
    rec = None
    error = ""
    try:
        rec = be.provisioning_record(bundle)
        cfg = be.provisioning_config(bundle)
    except be.BundleEditError as exc:
        # Reported rather than raised - `show` describes a bundle, and one whose
        # provisioning state is unreadable is exactly a bundle worth describing.
        return {"carried": False, "prepared": False, "error": str(exc)}
    closure = pb.ap_closure(cfg)
    seed = pb.ikg_seed(cfg)
    stale = None
    if rec is not None:
        try:
            pb.check_digest(rec, be.provisioned_keys(bundle))
        except pb.BatchError as exc:
            stale = str(exc)
    return {
        # The configuration is always readable: it is set before a bundle is
        # prepared. @prepared is the record's own presence.
        "carried": True,
        "prepared": rec is not None,
        "error": error,
        "ap-closure": pb.closure_method(closure),
        "ap-closure-label": pb.closure_label(closure),
        pb.CONFIG_PROVISION_USER_SEED: pb.user_seed_choice(seed),
        "provision-user-seed-label": pb.seed_label(seed),
        "counters": dict((rec or {}).get("counters") or {}),
        "ota-kek": pb.has_ota_kek(rec or {}),
        "application-keys": pb.app_keys(rec or {}),
        "keyset": (rec or {}).get("keyset") or "",
        "provisioning-hexes": [
            f"{ident}/{member}" for ident, member in _carried_prov_hexes(bundle, closure, seed)
        ],
        "missing-hexes": [
            f"{fw_bundle.identifiers([role])}/{member}"
            for role, member in _absent_prov_hexes(bundle, closure, seed)
        ],
        "unavailable-routes": [
            batch_name
            for _, _, batch_name in ph.carried_hexes(closure, seed)
            if rec is not None and ph.unavailable(rec, batch_name)
        ],
        "stale": stale,
        "application-key-space": (
            _show_key_space(be.application_key_space(bundle, rec)) if rec is not None else None
        ),
    }


def _show_key_space(use) -> dict:
    return {
        "used": use.used,
        "capacity": use.capacity,
        "free": use.free,
        "keys": use.application_keys,
    }


def _carried_prov_hexes(bundle: be.Editable, closure: str, seed: str) -> list:
    out = []
    for role, member, _ in ph.carried_hexes(closure, seed):
        pkg = bundle.by_role(role)
        if pkg is not None and pkg.get(member) is not None:
            out.append((pkg.ident, member))
    return out


def _absent_prov_hexes(bundle: be.Editable, closure: str, seed: str) -> list:
    """Route members that are missing and should not be.

    A route a KEK-free bundle cannot express is reported separately - it is a
    property of how the bundle was prepared, not something to repair.
    """
    rec = be.provisioning_record(bundle)
    out = []
    for role, member, batch_name in ph.carried_hexes(closure, seed):
        if rec is not None and ph.unavailable(rec, batch_name):
            continue
        pkg = bundle.by_role(role)
        if pkg is None or pkg.get(member) is None:
            out.append((role, member))
    return out


def show_image(conf: bytes | None) -> dict:
    """image_config for `show`: all profile fields, including UROT defaults."""
    profile = export_image(conf)
    if conf is None:
        return profile
    decoded = conf_img.unpack(conf)
    if not decoded["supported"]:
        return {**profile, "formatversion": decoded["formatversion"], "supported": False}
    return profile


def ram_layout(bundle: be.Editable) -> dict | None:
    """The RAM areas the base configuration of @bundle gives, for `show`.

    None when the bundle names no SoC, since the non-secure RAM's size is the
    part's.
    """
    if bundle.soc is None:
        return None
    part = parts.by_soc(bundle.soc)
    return pkr.ram_layout(be.key_slot_storage_size(bundle), part.ram_kb, part.ram_physical_kb)
