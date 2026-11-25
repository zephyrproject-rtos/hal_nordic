#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Create, inspect, and customize IronSide bundles."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from lib import app_keys as ak  # noqa: E402
from lib import bundle_config as bc  # noqa: E402
from lib import bundle_edit as be  # noqa: E402
from lib import bundle_profile as bp  # noqa: E402
from lib import (  # noqa: E402
    conf_img,
    fw_bundle,
    hybrid,
    img_conf_args,
    mcuboot_image,
    parts,
    prov_conf_args,
    uicr,
    update_flow,
)
from lib import config_fields as cf  # noqa: E402
from lib import early_secure_debug_access as esda  # noqa: E402
from lib import generate_production_hex as ph  # noqa: E402
from lib import prov_batches as pb  # noqa: E402
from lib.bundle_edit import Usage  # noqa: E402
from lib.mcuboot_encrypt import OTA_ARTIFACT_SUFFIX  # noqa: E402

SCRIPTS = Path(__file__).resolve().parent
DOWNLOAD = SCRIPTS / "ironside_download.py"
import ironside_download as dl  # noqa: E402

# Development-only command. It is left out of the help and of the command list
# in parser errors.
ESDA_COMMAND = "early-secure-debug-access"


# ---------------------------------------------------------------------------
# Reporting helpers
# ---------------------------------------------------------------------------
def _provenance(manifest) -> str:
    """The channel, build date and ref a package's manifest records.

    Off the release channel this is what tells two packages of the same version
    apart, so it belongs in every listing beside the version.
    """
    channel, ref = manifest.get("channel"), manifest.get("ref")
    stamp = (manifest.get("timestamp") or "")[:10]
    return "_".join(part for part in (channel, stamp, ref) if part)


def _signature_verifies(path, recorded_hex) -> bool:
    """Whether the image at @path verifies against the key value the bundle recorded.

    The recorded value is the digest of the key (for a hybrid image, the pair of keys)
    the image carries, so it must match that digest as well as the signatures
    verifying.
    """
    if hybrid.is_hybrid_signed(mcuboot_image.read_image(path)[0]):
        digest = hybrid.verify_signed(path)
    else:
        digest = mcuboot_image.verify_signed(path)
    return digest is not None and digest.hex() == recorded_hex


def _matching_generation(keys, role, pubkey_hex):
    """Find a provisioned key by role and public key."""
    for gen, state in sorted((keys.get(role) or {}).items()):
        if state.get("key") == pubkey_hex:
            return gen, state
    return None


_COMPONENT_FLAG_WIDTH = 4
_COMPONENT_FLAG_LEGEND = (
    "flags: C/d/-=config (-=unconfigurable); g0..g2=signed; u-=unsigned; "
    f"--=not signable ({_COMPONENT_FLAG_WIDTH} chars)"
)


def _component_config(pkg) -> str:
    """Whether the component's image still matches what the release shipped."""
    if pkg.role not in fw_bundle.CONFIGURABLE_ROLES:
        return "n/a"
    return "default" if pkg.unsigned == pkg.released_image else "customized"


def _component_signature(pkg) -> str:
    """Signature state for signable roles; not signable otherwise."""
    if pkg.role not in be.SIGNABLE_ROLES:
        return "n/a"
    return "signed" if pkg.signed_image is not None else "unsigned"


def _component_flags(pkg, keys=None) -> str:
    config_state = _component_config(pkg)
    if config_state == "n/a":
        config = "-"
    elif config_state == "default":
        config = "d"
    else:
        config = "C"
    if pkg.role not in be.SIGNABLE_ROLES:
        sig = "--"
    elif pkg.signed_image is None:
        sig = "u-"
    else:
        sig = "g-"
        if pkg.signing_key and keys is not None:
            match = _matching_generation(keys, pkg.role, pkg.signing_key)
            if match is not None:
                sig = f"g{match[0]}"
    return f"{config} {sig}"


def _signing_generation(pkg, keys=None):
    """Generation index of @pkg's recorded signing key, or None."""
    if (
        pkg.role not in be.SIGNABLE_ROLES
        or pkg.signed_image is None
        or not pkg.signing_key
        or keys is None
    ):
        return None
    match = _matching_generation(keys, pkg.role, pkg.signing_key)
    return match[0] if match is not None else None


def _signing_state(pkg, keys=None):
    """provisioned / revoked / unset for @pkg's recorded signing key, or None.

    Beside the generation rather than derived from it by a consumer: a bundle
    can provision a generation and revoke it, and the JSON is the form nothing
    should have to re-derive that from.
    """
    if (
        pkg.role not in be.SIGNABLE_ROLES
        or pkg.signed_image is None
        or not pkg.signing_key
        or keys is None
    ):
        return None
    match = _matching_generation(keys, pkg.role, pkg.signing_key)
    return be.generation_state_label(match[1]) if match is not None else None


def _component_row(pkg, keys=None) -> dict:
    row = {
        "flags": _component_flags(pkg, keys),
        "config": _component_config(pkg),
        "signature": _component_signature(pkg),
        "signing_gen": _signing_generation(pkg, keys),
        "signing_state": _signing_state(pkg, keys),
        "ident": pkg.ident,
        "role": pkg.role,
        "feature_level": pkg.feature_level,
        "version": pkg.version,
        "release_date": pkg.release_date,
        "provenance": _provenance(pkg.manifest),
        "signing_key": pkg.signing_key,
        "counter": be.package_counter(pkg) if pkg.role in be.SIGNABLE_ROLES else None,
    }
    # The key appears only when the mode is not off, so released bundles do not
    # advertise the development-only command.
    if pkg.role == "irot":
        access = be.early_secure_debug_access_mode(pkg)
        if access != esda.MODE_OFF:
            row["early_secure_debug_access"] = access
    if pkg.role == "app":
        try:
            otp = be.otp_words(pkg)
        except (mcuboot_image.SignError, ValueError):
            otp = {}  # `check` reports the unreadable image.
        if otp:
            row["uicr_otp_words"] = len(otp)
    return row


def _provisions_counters(role, closure_method) -> bool:
    """Whether the batches @role's carried routes run write the counter floors.

    The floors ride in the `install` group run by secure provisioning firmware.
    The non-secure phase only closes the access port. This is derived
    from the carried routes rather than asserted, so it cannot outlive a change
    to which phase installs.
    """
    # Only group membership is read here, so the configured SEED mode does not matter.
    # The default is passed because carried_hexes() requires a valid one.
    for prov_role, _member, batch_name in ph.carried_hexes(
        pb.closure_arg(closure_method), pb.DEFAULT_IKG_SEED
    ):
        if prov_role == role and "install" in pb.parse(batch_name)[0]:
            return True
    return False


def _component_detail(row, prov) -> list:
    """(label, value) detail pairs for one component, for verbose listings.

    Per component rather than in a section of its own, because every fact here
    is about that one package: the key its signature was made with, the
    initial rollback counter its own image justifies, and -- for the provisioning
    firmware -- the counters the batches it runs provision. Formats the row
    _component_row() already built, so nothing here re-reads an image.
    """
    detail = []
    role = row["role"]
    if row["signature"] != "n/a":
        if row["signature"] == "unsigned":
            detail.append(("signed with", "not signed (run sign-component)"))
        elif not row["signing_key"]:
            detail.append(("signed with", "an unrecorded key, so nothing can check the signature"))
        else:
            if row["signing_gen"] is None:
                detail.append(
                    ("signed with", f"a key this bundle does not provision for the {role} role")
                )
            else:
                detail.append(
                    ("signed with", f"{role} g{row['signing_gen']} ({row['signing_state']})")
                )
            detail.append(("signing key", row["signing_key"]))
        # Tested for None, not truthiness - 0 is a legitimate counter.
        if row["counter"] is not None:
            line = bc.counter_value(row["counter"], role)
            floor = (prov.get("counters") or {}).get(role) if prov else None
            # The floor and the image it ships beside are provisioned together,
            # so a disagreement belongs where the reader is already looking.
            # `check` owns the verdict and repair. This function only reports.
            if floor is not None and floor != row["counter"]:
                line += f", initial rollback counter {bc.counter_value(floor, role)}"
            detail.append(("security counter", line))
    if (
        role in fw_bundle.PROV_ROLES
        and prov
        and prov.get("carried")
        and _provisions_counters(role, prov["ap-closure"])
    ):
        detail.append(("provisions counters", bc.counter_line(prov["counters"]) or "not recorded"))
    return detail


def _show(bundle, sections, as_json, verbose=False) -> int:
    urot = bundle.by_role("urot")
    conf = be.image_config(urot.effective) if urot else None
    keys = be.provisioned_keys(bundle)
    # Read for the components section too. The counters a verbose listing shows
    # under the provisioning firmware come from the carried member, so
    # `show --components -v` must not depend on the other section being asked for.
    prov = (
        bp.show_provisioning(bundle)
        if "provisioning_config" in sections or (verbose and "components" in sections)
        else None
    )

    data = {
        "bundle": str(bundle.path),
        "soc": bundle.soc,
        "components": [
            _component_row(p, keys) for p in sorted(bundle.packages, key=lambda p: p.ident)
        ],
        "base_config": bp.show_base(bundle) if "base_config" in sections else None,
        "urot_image_config": (bp.show_image(conf) if "image_config" in sections else None),
        "ram_layout": (bp.ram_layout(bundle) if "base_config" in sections else None),
        "provisioning_config": (prov if "provisioning_config" in sections else None),
        "unrecognized_members": bundle.unrecognized,
    }
    if as_json:
        wanted = set(sections) | {"bundle", "soc"}
        if "image_config" in wanted:
            wanted = (wanted - {"image_config"}) | {"urot_image_config"}
        if "base_config" in wanted:
            wanted |= {"ram_layout"}
        print(json.dumps({k: v for k, v in data.items() if k in wanted}, indent=2, default=str))
        return 0

    print(
        f"{bundle.path}"
        + (f"  ({bundle.soc})" if bundle.soc else "")
        + ("  [directory]" if bundle.is_directory else "")
    )
    if "components" in sections:
        print("\ncomponents:")
        print(f"  {_COMPONENT_FLAG_LEGEND}")
        print()
        for c in data["components"]:
            access = c.get("early_secure_debug_access")
            marker = f"  early-secure-debug-access: {access}" if access else ""
            if c.get("uicr_otp_words"):
                marker += f"  UICR.OTP: {c['uicr_otp_words']} of {uicr.OTP_WORDS} words"
            feature_level = "-" if c["feature_level"] is None else f"fl{c['feature_level']}"
            release_date = c["release_date"] or "-"
            print(
                f"  {c['flags']}  {c['ident']:<28} {feature_level:<5} {c['version']:<20} "
                f"{release_date:<10} {c['provenance']:<28}{marker}"
            )
            if not verbose:
                continue
            for label, value in _component_detail(c, prov):
                print(f"        {label + ':':<22} {value}")
        for name in bundle.unrecognized:
            print(f"  - --  {name:<28} (not a component archive, kept unchanged)")
    if "base_config" in sections:
        bc.print_base(data["base_config"], verbose)
        bc.print_ram_layout(data["ram_layout"])
    if "image_config" in sections:
        bc.print_image(data["urot_image_config"])
    if "provisioning_config" in sections:
        bc.print_provisioning(data["provisioning_config"], verbose)
    if not verbose:
        print("\nUse -v to show key values and per-component details.")
    return 0


# ---------------------------------------------------------------------------
# check
# ---------------------------------------------------------------------------
def _readable(pkg):
    """Return image parts and an error string without raising."""
    try:
        return be.image_parts(pkg.effective), None
    except (mcuboot_image.SignError, ValueError) as exc:
        return None, f"{pkg.ident}: {exc}"


def _check(bundle, args=None) -> int:
    problems, warnings, signing = [], [], []

    # Skip further checks for unreadable images to avoid duplicate failures.
    unreadable = set()
    for pkg in be.uicr_carriers(bundle):
        _, error = _readable(pkg)
        if error:
            problems.append(error)
            unreadable.add(pkg.ident)

    socs = {}
    for pkg in bundle.packages:
        if pkg.soc:
            socs.setdefault(pkg.soc, []).append(pkg.ident)
    if len(socs) > 1:
        problems.append(
            "components target different SoCs: "
            + "; ".join(f"{soc} ({', '.join(who)})" for soc, who in sorted(socs.items()))
        )

    unsatisfied, absent = be.check_deps(bundle)
    problems += unsatisfied

    readable = [pkg for pkg in bundle.packages if pkg.ident not in unreadable]
    region = uicr.decode(
        be.aux_lookup(
            be.Editable(path=bundle.path, is_directory=bundle.is_directory, packages=readable)
        )
    )["region"]
    keys = region.get("keys", {})
    authop_problem = be.authop_generations_problem(keys)
    if authop_problem:
        problems.append(f"the key set {authop_problem}")

    # Each UICR address must have one owning component.
    claimed = {}
    for pkg in be.region_carriers(bundle):
        if pkg.ident in unreadable:
            continue
        _, _, aux = be.image_parts(pkg.effective)
        for addr in aux.addresses():
            if addr in claimed and claimed[addr] != pkg.ident:
                problems.append(
                    f"{claimed[addr]} and {pkg.ident} both write UICR "
                    f"0x{addr:08X}; each component carries the words it reads"
                )
                break
            claimed[addr] = pkg.ident
    problems += be.otp_problems(bundle, skip=unreadable)

    # And the words those components carry must sit where this checkout's layout
    # says. A component from either side of a UICR layout change merges into a
    # region that decodes, with every field one slot out.
    problems += be.region_layout_problems(
        region,
        soc=bundle.soc,
        carriers=tuple(r for r in be.SLICE_CARRIERS if bundle.by_role(r) is not None),
    )

    irot = bundle.by_role("irot")
    if irot is not None and irot.ident not in unreadable:
        if be.early_secure_debug_access_mode(irot) == esda.MODE_UNKNOWN:
            problems.append(
                f"{irot.ident} carries USERCNF entries for the debug signal "
                "registers that are not an early secure debug access mode. Run "
                "`early-secure-debug-access --mode off` to restore the released entries."
            )
        else:
            warnings += _early_secure_debug_access_warnings(bundle)

    for pkg in sorted(bundle.packages, key=lambda p: p.ident):
        if pkg.role not in be.SIGNABLE_ROLES or pkg.ident in unreadable:
            continue
        problem = be.counter_problem(pkg)
        if problem is not None:
            problems.append(problem)
            continue
        signed = pkg.signed_image
        if signed is None:
            warnings.append(
                f"{pkg.ident} is not signed; it must be before it can "
                f"be programmed (`sign-component`)"
            )
            continue
        recorded = pkg.signing_key
        if recorded is None:
            problems.append(
                f"{pkg.ident} carries a signature but no "
                f"{fw_bundle.USER_MANIFEST_MEMBER} naming the key it "
                f"was made with, so nothing can check it"
            )
        else:
            sig_ok = False
            retired = False
            with be.as_file(signed) as path:
                try:
                    retired = mcuboot_image.has_retired_signature(
                        mcuboot_image.read_image(path)[0]
                    )
                    sig_ok = not retired and _signature_verifies(path, recorded)
                except (mcuboot_image.SignError, ValueError) as exc:
                    problems.append(f"{pkg.ident}: {exc}")
                    recorded = None
            if retired:
                problems.append(
                    f"{pkg.ident} is signed with Ed25519, which IronSide TE no longer "
                    "verifies. Sign it again with `sign-component`."
                )
            elif recorded is not None and not sig_ok:
                problems.append(
                    f"{pkg.ident}: the signature does not verify "
                    f"against its recorded key {recorded[:16]}.... "
                    "The image or key record has changed."
                )
            match = _matching_generation(keys, pkg.role, recorded) if recorded is not None else None
            if match is None and recorded is not None:
                problems.append(
                    f"{pkg.ident} is signed with a key this bundle does not "
                    f"provision for the {pkg.role} role. Run `update-keys` with "
                    "the key source that signed it."
                )
            elif match is not None and match[1]["revoked"]:
                problems.append(
                    f"{pkg.ident} is signed with {pkg.role} generation "
                    f"{match[0]}, which this bundle revokes"
                )
            elif match is not None and sig_ok:
                gen, state = match
                signing.append(
                    (
                        pkg.ident,
                        f"{pkg.role} g{gen} ({be.generation_state_label(state)})",
                    )
                )

        # Detect image configuration changes made after signing.
        with be.as_file(signed) as path:
            try:
                region_bytes, _ = mcuboot_image.split_signed(mcuboot_image.read_image(path)[0])
            except mcuboot_image.SignError as exc:
                problems.append(f"{pkg.ident}: {exc}")
                continue
        unsigned_data, _, unsigned_aux = be.image_parts(pkg.unsigned)
        if region_bytes != unsigned_data:
            problems.append(
                f"{pkg.ident}: {fw_bundle.USER_SIGNED_MEMBER} does not cover the "
                "current image. Re-sign it."
            )
        _, _, signed_aux = be.image_parts(signed)
        if dict(signed_aux.todict()) != dict(unsigned_aux.todict()):
            problems.append(
                f"{pkg.ident}: the UICR words differ between "
                f"{fw_bundle.USER_SIGNED_MEMBER} and "
                f"{fw_bundle.USER_IMAGE_MEMBER}. They must match."
            )

        if pkg.role == "dfu":
            slot = be.dfu_slot_size(bundle)
            with be.as_file(signed) as path:
                size = len(mcuboot_image.read_image(path)[0])
            if slot is not None and size > slot:
                problems.append(
                    f"{pkg.ident}: the signed image is 0x{size:x} bytes but DFUIMAGE is 0x{slot:x}"
                )

    dfu = bundle.by_role("dfu")
    if dfu is not None and dfu.ident not in unreadable:
        problems += be.dfu_placement_problems(bundle)

    sizes = region.get("partitions_kb") or {}
    if sizes.get("dfuimage") == 0 and bundle.by_role("dfu") is not None:
        warnings.append(
            "DFUIMAGE is 0, which disables the IronSide DFU image, but "
            "the bundle still carries "
            f"{bundle.by_role('dfu').ident}; it would go unused"
        )
    if not keys:
        warnings.append(
            "no verification keys are provisioned. Run `update-keys "
            "--keyset <spec>` before installation."
        )
    else:
        # Replacing IROT or UROT can remove the keys carried by that component.
        empty = [
            role
            for role in uicr.ROLES
            if not any(
                state["provisioned"] and not state["revoked"]
                for state in (keys.get(role) or {}).values()
            )
        ]
        if empty:
            problems.append(
                f"the keyset has no usable {', '.join(empty)} key. Run "
                "`update-keys` again with the same key source."
            )
    problems += _check_prov_hexes(bundle, warnings, getattr(args, "keyset", None))
    problems += _check_counters(bundle, warnings)
    problems += _check_app_keys(bundle)
    problems += _check_application_key_space(bundle)
    warnings += absent
    for name in bundle.unrecognized:
        warnings.append(f"{name} is not a component archive. It is kept unchanged.")

    if signing:
        width = max(len(ident) for ident, _ in signing)
        for ident, detail in signing:
            print(f"sign  {ident:<{width}}  {detail}")
    for line in problems:
        print(f"FAIL  {line}")
    for line in warnings:
        print(f"warn  {line}")
    summary = f"{len(bundle.packages)} component(s)"
    if signing:
        summary += f", {len(signing)} signed"
    if not problems and not warnings:
        print(f"ok    {bundle.path}: {summary}, nothing to report")
    elif not problems:
        print(f"ok    {bundle.path}: {summary}, {len(warnings)} warning(s), no failures")
    return 2 if problems else 0


# ---------------------------------------------------------------------------
# init
# ---------------------------------------------------------------------------
def _released_carries_batch(pkg, cmd_addr) -> bool:
    """Whether the released image already holds data where the batch goes.

    A release that did would make the carried member ambiguous - there would be no
    telling the firmware's own bytes from a spliced batch.
    """
    try:
        return be.batch_at(pkg.released_image, cmd_addr) is not None
    except be.BundleEditError:
        return False


def _readable_record(bundle):
    """The provisioning record, or None when the bundle has none it can read.

    Used where an unreadable record is not this command's business to report -
    `update-keys` rewrites it, and everything else says so already.
    """
    try:
        return be.provisioning_record(bundle)
    except be.BundleEditError:
        return None


def _check_counters(bundle, warnings) -> list:
    """Hold the provisioned counter floors against the images they ship with.

    Worth checking separately from the writes that set them: `add-component` and
    `update-component` can put a different image in after a floor was recorded,
    and a floor above that image's own counter is a device that refuses the
    firmware it was provisioned with, permanently.
    """
    rec = _readable_record(bundle)
    if rec is None:
        return []
    over = be.check_counters(bundle, rec.get("counters") or {})
    problems = [
        f"{ident} presents counter 0x{have:08X} "
        f"({mcuboot_image.counter_display(have, role)}) "
        f"but this bundle provisions the {role} initial rollback counter at "
        f"0x{wanted:08X}, which would refuse it. Re-sign that component, which sets "
        f"the initial rollback counter from the image, "
        f"or run `update-keys`."
        for role, wanted, have, ident in over
    ]
    # A floor below the image still boots -- the boot gate ratchets it -- so it
    # is a warning rather than a failure. It is still a gap - until that first
    # boot the device accepts an image older than the one it shipped with.
    refused = {role for role, _, _, _ in over}
    for role, have, want, ident in be.stale_counters(bundle):
        if role in refused:
            continue
        if ident is None:
            warnings.append(
                f"this bundle provisions the {role} initial rollback counter at "
                f"{bc.counter_value(have, role)} but carries no {role} image to justify it. "
                f"Run `update-keys` to set the initial rollback counters from the images "
                f"it does carry."
            )
            continue
        warnings.append(
            f"this bundle provisions the {role} initial rollback counter at "
            f"{bc.counter_value(have, role)} "
            f"but {ident} presents {bc.counter_value(want, role)}, so an older {role} image is "
            f"accepted until the first boot ratchets it. Re-sign that component, or run "
            f"`update-keys`, to set the initial rollback counter from the image."
        )
    return problems


def _check_app_keys(bundle) -> list:
    """Hold the recorded application-key IDs against each other.

    The inventory is written by the command that builds the imports, so the two
    agree on a bundle this tool produced. Two recorded keys with one ID mean the
    record was edited by something else -- and the device would report the
    resulting collision as ALREADY_EXISTS on the second key, which reads as
    success. The key IDs the route hexes actually import under are held against
    this same inventory by _check_batch_shape().
    """
    rec = _readable_record(bundle)
    if rec is None:
        return []
    seen = set()
    problems = []
    for key in pb.app_keys(rec):
        if key["id"] in seen:
            problems.append(
                f"this bundle provisions two application keys with key ID "
                f"{key['id']}. The second import would fail on a key ID the first "
                f"already holds."
            )
        seen.add(key["id"])
    return problems


def _check_application_key_space(bundle) -> list:
    """Refuse a bundle whose application keys the device has no space for.

    The install batch stops at the first record the device refuses, partway
    through a write-once install.
    """
    rec = _readable_record(bundle)
    if rec is None:
        return []
    if not be.application_key_space(bundle, rec).is_exceeded:
        return []
    return [
        "the application keys exceed the application key space. Raise "
        "--urotstorage-size-kb with `update-config`, or provision fewer application keys."
    ]


def _check_prov_hexes(bundle, warnings, keyset=None) -> list:
    """Hold each carried provisioning hex against the batch it should be.

    This is the property a programming artifact rests on, checked offline on the
    bundle rather than on the output of the tool that merges it - the member is
    the released provisioning firmware plus exactly the configured route's batch,
    at the command address of the part the bundle declares.

    Without @keyset the batch is checked for its SHAPE -- the operations its
    recorded name calls for, in order, provisioning the recorded floors -- which
    is everything about it that does not depend on the OTA KEKs. That keeps the
    review offline and key-free, which is what a reviewer holding a prepared
    bundle and no keyset can do. With @keyset the bytes are compared outright.
    """
    try:
        rec = be.provisioning_record(bundle)
    except be.BundleEditError as exc:
        return [str(exc)]
    if rec is None:
        # Said out loud rather than skipped. The record lives in the prov-secure
        # package, so removing or replacing that component takes it away -- and a
        # bundle that provisions keys but can run no batch would otherwise look
        # complete right up to the point a station has nothing to program.
        if be.provisioned_keys(bundle):
            missing = fw_bundle.identifiers([fw_bundle.PROV_SECURE])
            warnings.append(
                "this bundle provisions verification keys but is not prepared "
                "for provisioning"
                + (
                    f": it carries no {missing} component, which holds the "
                    f"provisioning configuration and runs the batches. Add it "
                    f"with `add-component`, then run `update-keys`."
                    if bundle.by_role(fw_bundle.PROV_SECURE) is None
                    else ". Run `update-keys --keyset <spec>`."
                )
            )
        return []
    try:
        cmd_addr = pb.abi.layout_for(ph.resolve_soc(bundle)).cmd_addr
    except (ph.ProdhexError, parts.PartError) as exc:
        return [f"the provisioning hexes cannot be checked: {exc}"]

    problems = []
    keks = pb.PLACEHOLDER_KEKS
    app_keys = ()
    if keyset is not None and pb.has_ota_kek(rec):
        try:
            keks = be.ota_kek_material(keyset)
            app_keys = ak.ops(be.app_key_material(keyset))
        except be.BundleEditError as exc:
            return [str(exc)]
    exact = keks is not pb.PLACEHOLDER_KEKS or not pb.has_ota_kek(rec)

    cfg = be.provisioning_config(bundle)
    for role, member, batch_name in ph.carried_hexes(pb.ap_closure(cfg), pb.ikg_seed(cfg)):
        pkg = bundle.by_role(role)
        if pkg is None or ph.unavailable(rec, batch_name):
            continue
        blob = pkg.get(member)
        if blob is None:
            warnings.append(
                f"{pkg.ident} carries no {member}, so this bundle cannot write "
                f"the {batch_name.split('@')[0]} programming artifact. Run "
                f"`update-keys --keyset` to write it."
            )
            continue
        try:
            want = pb.build_from(rec, batch_name, keks=keks, app_keys=app_keys)
            got = be.batch_at(blob, cmd_addr)
        except pb.BatchError as exc:
            problems.append(f"{pkg.ident}: {exc}")
            continue
        except be.BundleEditError as exc:
            problems.append(f"{pkg.ident}: {member} is {exc}")
            continue
        if got is None:
            problems.append(
                f"{pkg.ident}: {member} carries no provisioning batch at "
                f"0x{cmd_addr:08X}, where the firmware reads one"
            )
        elif exact and got != want:
            problems.append(
                f"{pkg.ident}: {member} does not carry this bundle's "
                f"{batch_name!r} batch. Run `update-keys --keyset` to rewrite it."
            )
        elif not exact:
            problems += _check_batch_shape(pkg, member, batch_name, got, want, rec)
        if _released_carries_batch(pkg, cmd_addr):
            problems.append(
                f"{pkg.ident}: the released {fw_bundle.IMAGE_MEMBER} already "
                f"carries data at 0x{cmd_addr:08X}, where the batch goes"
            )
    return problems


def _check_batch_shape(pkg, member, batch_name, got, want, rec) -> list:
    """Everything about a spliced batch that does not depend on the OTA KEKs.

    Every operation and argument except the KEK values themselves, plus the
    initial rollback counters and the application-key IDs held against the record's own
    map. The floors matter most: they are recorded twice over -- in the record
    and in the operations a device applies -- and this is what holds the two
    together now that a batch's bytes live only in the hex.
    """
    try:
        same = pb.comparable(got) == pb.comparable(want)
        floors, recorded = pb.counters_in(got), pb.counters(rec)
        key_ids = pb.app_key_ids_in(got)
    except pb.BatchError as exc:
        return [f"{pkg.ident}: {member}: {exc}"]
    if not same:
        ran, wanted_ops = pb.operations_in(got), pb.operations_in(want)
        detail = (
            f"it applies {', '.join(ran)} where that batch applies "
            f"{', '.join(wanted_ops)}"
            if ran != wanted_ops
            else "the operations match by name but not by what they write"
        )
        return [
            f"{pkg.ident}: {member} does not run this bundle's {batch_name!r} "
            f"batch: {detail}. Run `update-keys --keyset` to rewrite it."
        ]
    if floors is not None and floors != recorded:
        return [
            f"{pkg.ident}: {member} provisions initial rollback counters "
            f"{bc.counter_map(floors)} but this bundle records {bc.counter_map(recorded)}. "
            f"Run `update-keys --keyset` to rewrite it."
        ]
    named = tuple(key["id"] for key in pb.app_keys(rec))
    if "install" in pb.parse(batch_name)[0] and key_ids != named:
        return [
            f"{pkg.ident}: {member} imports application keys with key IDs "
            f"{', '.join(str(i) for i in key_ids) or 'none'} but this bundle records "
            f"{', '.join(str(i) for i in named) or 'none'}. "
            f"Run `update-keys --keyset` to rewrite it."
        ]
    return []


def _resolve_init_out(args, source: be.Editable | None) -> Path:
    """The bundle path `init` writes: the named ZIP, else the ironside_download.py name.

    For @source the SoC and channel come from its components, never from this
    IronSide Integration Kit, since nothing is downloaded.
    """
    if args.out is not None:
        return Path(args.out)
    if source is None:
        return dl.default_output_path(channel=getattr(args, "channel", None), soc=args.soc)
    soc = _sole_source_value(args.source, "SoC", {p.soc for p in source.packages if p.soc})
    # The channel becomes part of the file name, so only a known one is used.
    recorded = [p.manifest.get("channel") or dl.RELEASE_CHANNEL for p in source.packages]
    unknown = [str(channel) for channel in recorded if channel not in dl.CHANNELS]
    if unknown:
        raise be.BundleEditError(
            f"{args.source} holds components of an unknown channel ({', '.join(unknown)}). "
            "Specify the output ZIP."
        )
    channel = _sole_source_value(args.source, "channel", set(recorded))
    return Path(dl.bundle_name(soc, channel))


def _sole_source_value(source_path: Path, what: str, values: set) -> str:
    if len(values) > 1:
        raise be.BundleEditError(
            f"{source_path} holds components of different {what}s "
            f"({', '.join(sorted(values))}). Specify the output ZIP."
        )
    if not values:
        raise be.BundleEditError(
            f"{source_path} holds components that name no {what}. Specify the output ZIP."
        )
    return values.pop()


def _resolve_keyset(keyset):
    """Return the named keyset or the sole keyset in the OS keyring."""
    if keyset is not None:
        return keyset
    sys.path.insert(0, str(SCRIPTS))
    import iron_keychain as kc

    return kc.sole_keyset_spec()


def _init(args) -> int:
    """Create a bundle from local packages or a download."""
    # Validate the key source before downloading.
    if args.keyset is not None:
        be.check_key_source(args.keyset)
    source = None
    if args.source is not None:
        # --force replaces the output bundle, which --from writes as well.
        named = [flag for flag in dl.download_options_named(args) if flag != "--force"]
        if named:
            raise Usage(f"--from cannot be combined with download options ({' '.join(named)})")
        source = be.packages_from(args.source)
        if not source.packages:
            raise be.BundleEditError(f"{args.source} holds no component archives")
    elif getattr(args, "soc", None) is None:
        raise Usage("init: name the SoC to download with --soc, such as --soc nrf54lm20b")

    bundle_out = _resolve_init_out(args, source)

    if not getattr(args, "dry_run", False):
        dl.refuse_existing(bundle_out, force=getattr(args, "force", False))

    if source is not None:
        bundle = be._checked(
            be.Editable(path=bundle_out, is_directory=False, packages=source.packages)
        )
        names = be.write_as(bundle, bundle_out)
        print(f"wrote {bundle_out}")
        for name in names:
            print(f"  {name}")
        for name in source.unrecognized:
            print(f"  skipped {name}: no known component identifier")
    else:
        download = subprocess.run(
            [sys.executable, str(DOWNLOAD), *dl.download_argv(args, out=bundle_out)]
        )
        if download.returncode:
            return download.returncode

    # The components were just chosen, so a layout disagreement between them and
    # this checkout is reported here -- not at the programming step, which would
    # burn it into a part's write-once words.
    if bundle_out.exists():
        chosen = be.read(bundle_out)
        problems = be.base_conf_problems(chosen) + be.dfu_placement_problems(chosen)
        if problems:
            for line in problems:
                print(f"FAIL  {line}", file=sys.stderr)
            print(
                f"ironside_bundle: {bundle_out} was written but must not be "
                f"installed. Keys were not provisioned.",
                file=sys.stderr,
            )
            return 2

    return _init_keyset(args, bundle_out) if args.keyset is not None else 0


def _init_keyset(args, bundle_out: Path) -> int:
    """Provision keys and sign every component in a new bundle."""
    out = bundle_out
    if not out.exists():
        raise be.BundleEditError(
            f"--update-keys-and-sign-all-with-keyset: {out} was not created. "
            "--dry-run and --forget-login do not create a bundle."
        )
    bundle = be.read(out)
    _update_keys(bundle, args)
    status = _sign(bundle, args)
    rewritten = be.save(bundle)
    print(f"updated {out}" + (f" ({', '.join(rewritten)})" if rewritten else ""))
    return status


# ---------------------------------------------------------------------------
# Edits
# ---------------------------------------------------------------------------
def _resolve_app_keys(keyset):
    """The application keys @keyset holds, or none when it holds none.

    They come from the keyset the verification keys and the OTA KEKs come from,
    so a device cannot be provisioned with application keys belonging to one
    deployment and verification keys belonging to another. Application keys are
    optional: a keyset with none yields a bundle carrying none.

    Read before anything is written, so a keyset this tool refuses produces one
    diagnostic and no half-finished report.
    """
    try:
        return be.app_key_material(keyset)
    except be.BundleEditError as exc:
        raise Usage(f"--keyset: {exc}") from None


def _update_keys(bundle, args) -> int:
    keyset = _resolve_keyset(args.keyset)
    app_keys = _resolve_app_keys(keyset)
    manifest = be.set_keys(bundle, keyset)
    print(f"keyset from {manifest.get('source', keyset)}")
    print(f"  provisioned: {', '.join(manifest.get('provision') or []) or '(none)'}")
    print(f"  revoked:     {', '.join(manifest.get('revoke') or []) or '(none)'}")
    print("  UROT keys written to IROT. APP, DFU, and AUTHOP keys written to UROT.")

    # A closure or seed named here is written as configuration first, so the
    # firmware spliced below is built at the values the bundle then states.
    if args.ap_closure is not None:
        be.set_ap_closure(bundle, args.ap_closure)
    if args.provision_user_seed is not None:
        be.set_ikg_seed(bundle, args.provision_user_seed)
    closure = _carried_closure(bundle, args.ap_closure)
    seed = _carried_ikg_seed(bundle, args.provision_user_seed)

    # The provisioning firmware is spliced from the same source in the same
    # command, so the OTA KEKs a device imports cannot belong to a different key
    # set than the verification keys it is provisioned with.
    # After set_keys, so the recorded digest describes the keys just written.
    # Nothing reaches the disk unless every half succeeds - the bundle is saved
    # once, after the handler returns.
    record = be.set_provisioning_record(
        bundle,
        keyset,
        counters=args.counters,
        without_ota_kek=args.without_ota_kek,
        app_keys=app_keys,
    )
    print("provisioning configuration:")
    print(f"  counters:    {bc.counter_line(record['counters'])}")
    print(f"  AP closure:  {pb.closure_label(closure)}")
    print(f"  User SEED:   {pb.seed_label(seed)}")
    if record["ota_kek"]:
        print("  OTA KEKs:    3 generations, imported by the batches built from this.")
    else:
        print("  OTA KEKs:    none. This device cannot accept an encrypted update.")
    _report_app_keys(record["app_keys"])

    # The programmable provisioning firmware, one member per production route:
    # every byte a station programs is then a bundle member, and each route's
    # batch exists only there.
    print("provisioning firmware:")
    _report_prov_hexes(*ph.set_prov_hexes(bundle, keyset))
    if record["ota_kek"]:
        print("  these carry the OTA KEKs in the clear. Keep this bundle private.")

    # Report signatures that do not match the newly provisioned keys.
    region_keys = be.provisioned_keys(bundle)
    for pkg in bundle.packages:
        if pkg.signing_key and _matching_generation(region_keys, pkg.role, pkg.signing_key) is None:
            print(f"  warning: {pkg.ident} is signed with a key this keyset does not provision")
    return 0


def _report_app_keys(keys) -> None:
    """The application keys the batches import, and the key ID of each one.

    Printed in full rather than behind a verbosity flag: the key IDs are what a
    reader holds against the keyset, and there are at most a few dozen.

    The privacy line is its own, not folded into the OTA KEK line above, because
    a bundle built without the KEKs but with application keys is still secret
    while that line says the opposite.
    """
    if not keys:
        print("  application keys: none")
        return
    key_ids = ", ".join(str(k.key_id) for k in keys)
    print(
        f"  application keys: {len(keys)}, key ID{'s' if len(keys) > 1 else ''} "
        f"{key_ids}, in the clear. Keep this bundle private."
    )
    for line in ak.describe(keys):
        print(f"    {line}")
    permanent = ak.permanent(keys)
    if permanent:
        named = ", ".join(str(k.key_id) for k in permanent)
        print(
            f"    note: key ID{'s' if len(permanent) > 1 else ''} {named} "
            f"{'are' if len(permanent) > 1 else 'is'} not rotating, so no destroy "
            f"frees {'them' if len(permanent) > 1 else 'it'} for a new key"
        )
    print(
        "    a key ID already holding a key keeps it: the import reports "
        "ALREADY_EXISTS, which reads as success"
    )


def _report_stale_counters(bundle) -> None:
    """Say so when the recorded counter floors no longer match the images.

    Not fixed here: moving a component in or out is not a statement about
    versions, and this is the wrong command to make one. `sign-component` sets
    the floors from the images it signs and `update-keys` sets them from the
    images the bundle carries -- either is deliberate, where a silent re-derive
    here would move a write-once floor as a side effect of an inventory change.

    A floor left below an image's own counter still boots -- the boot gate
    ratchets it -- but until then the device accepts an older image than the one
    it shipped with, which is the whole point of setting the floor from the
    version.
    """
    stale = be.stale_counters(bundle)
    if not stale:
        return
    print("  warning: the initial rollback counters no longer match the images:")
    for role, have, want, ident in stale:
        if ident is None:
            print(
                f"    {role}: provisions {bc.counter_value(have, role)}, but this bundle "
                f"now carries no {role} image"
            )
        else:
            print(
                f"    {role}: provisions {bc.counter_value(have, role)}, "
                f"{ident} presents {bc.counter_value(want, role)}"
            )
    print(
        "  run `sign-component --all --keyset <spec>`, or `update-keys "
        "--keyset <spec>`, to set the initial rollback counters from the images this "
        "bundle now carries"
    )


def _report_prov_hexes(written, skipped) -> None:
    """What the route members now are, and which routes this bundle cannot serve."""
    for ident, member, batch_name in written:
        print(f"  {ident}: {member} ({batch_name})")
    for ident, member, batch_name in skipped:
        print(
            f"  {ident}: no {member}. The {batch_name!r} batch requires the OTA "
            f"KEKs, so this bundle cannot program that route."
        )


def _carried_closure(bundle, requested):
    """The AP closure method to record: the one asked for, else the one carried.

    Re-running update-keys must not silently reset a configured closure back to
    the default -- it is provisioning CONFIGURATION, and nothing else about a key
    rotation says anything about it.
    """
    if requested is not None:
        return requested
    try:
        return pb.ap_closure(be.provisioning_config(bundle))
    except be.BundleEditError:
        # An unreadable configuration is what this command can replace, so it
        # must not be what stops it. The default is taken and reported.
        return pb.DEFAULT_AP_CLOSURE


def _carried_ikg_seed(bundle, requested):
    """The SEED mode to record: the one asked for, else the one carried.

    Re-running update-keys must not silently reset a configured mode back to
    the default, for the reason _carried_closure() gives - it is provisioning
    configuration, and a key rotation says nothing about it.
    """
    if requested is not None:
        return requested
    try:
        return pb.ikg_seed(be.provisioning_config(bundle))
    except be.BundleEditError:
        return pb.DEFAULT_IKG_SEED


def _sign(bundle, args) -> int:
    if args.all:
        targets = [p for p in bundle.packages if p.role in be.SIGNABLE_ROLES]
    else:
        targets = []
        for name in args.component:
            pkg = bundle.by_name(name)
            if pkg is None:
                raise be.BundleEditError(
                    f"{bundle.path} carries no {name} component; it holds "
                    + ", ".join(sorted(p.ident for p in bundle.packages))
                )
            targets.append(pkg)
    if not targets:
        raise Usage(
            f"sign-component: specify a component, or "
            f"--all for every signable one ({', '.join(be.SIGNABLE_ROLES)})"
        )

    for pkg in targets:
        result = be.sign(
            bundle,
            pkg,
            keyset=args.keyset,
            gen=args.gen,
            version=args.version,
            security_counter=args.security_counter,
            keep_security_counter=args.keep_security_counter,
        )
        slot = result["slot_size"]
        partition = be.SLOT_PARTITION.get(pkg.role, "").upper()
        print(f"{pkg.ident}: {result['report']}")
        for line in result["warnings"]:
            print(f"  warning: {line}")
        print(
            f"  -> {fw_bundle.USER_SIGNED_MEMBER}"
            + (f", slot 0x{slot:x} from the bundle's {partition}" if slot else "")
            + f", key {result['signing_key'][:16]}... recorded in "
            f"{fw_bundle.USER_MANIFEST_MEMBER}"
        )
        match = _matching_generation(be.provisioned_keys(bundle), pkg.role, result["signing_key"])
        if match is None:
            print(
                "  warning: this bundle does not provision that key for this "
                "role. Run update-keys with the same source."
            )
    _set_signed_counters(bundle, args.keyset)
    return 0


def _set_signed_counters(bundle, keyset) -> None:
    """Move the initial rollback counters onto the images now in the bundle.

    Signing is where an image's counter is decided -- from `--version`, from
    `--security-counter`, or from what the release already carried -- so it is
    where the floor a device will be provisioned with has to follow. Left behind,
    a floor below the image lets a device accept something older than what
    shipped until the first boot ratchets it, and a floor above it is a device
    that refuses the firmware in this very bundle, permanently.

    Every counter role is re-derived from the images the bundle carries, which is
    what `update-keys` derives with no `--counters` - the floors are a statement
    about the images, and this is the command that changes them. A floor pinned
    by `update-keys --counters` therefore does not survive the next signing.

    Uses the @keyset this command already signs with - the floors live inside the
    production route hexes, and re-splicing those runs an install batch through
    the builder, which imports the OTA KEKs.
    """
    try:
        rec = be.provisioning_record(bundle)
    except be.BundleEditError:
        # `update-keys` rewrites an unreadable record and `check` reports it, but
        # staying silent here would leave a floor un-moved without saying so.
        print(
            "  warning: this bundle's provisioning record cannot be read, so the "
            "initial rollback counters were left as they are. Run `update-keys` to rebuild it."
        )
        return
    if rec is None:
        return
    if keyset is None:
        print(
            "  warning: the initial rollback counters were left as they are. Moving them "
            "rewrites the production provisioning firmware, which imports the "
            "OTA KEKs, so it needs --keyset."
        )
        return
    # Both halves are put back if the re-splice cannot finish - a record carrying
    # new floors beside route hexes carrying the old ones fails `check`.
    state = be.prov_state(bundle)
    moved = be.set_counters(bundle)
    if not moved:
        print("provisioning config: the initial rollback counters already match the images")
        return
    try:
        # The floors are inside the batches, and the batches live in what a
        # station programs -- so the route members are re-written here, exactly
        # as update-config does after a closure edit.
        written, skipped = ph.set_prov_hexes(bundle, keyset)
    except (ph.ProdhexError, be.BundleEditError) as exc:
        # Reported, not raised - the signatures are already in this bundle and
        # main() writes nothing if a handler raises.
        be.restore_prov_state(bundle, state)
        print(
            f"  warning: the initial rollback counters were left as they are: the "
            f"provisioning route members could not be rewritten ({exc})"
        )
        return
    print("provisioning config: initial rollback counters follow the images just signed:")
    for role, (before, after) in moved.items():
        print(f"  {role}: {bc.counter_value(before, role)} -> {bc.counter_value(after, role)}")
        if after < before:
            # The bundle record is editable, but an earlier device value is not.
            print(
                f"    warning: a device already provisioned from this bundle has "
                f"the {bc.counter_value(before, role)} initial rollback counter in write-once "
                f"storage. It cannot be lowered, so the device will refuse this {role} image."
            )
    _report_prov_hexes(written, skipped)


# ---------------------------------------------------------------------------
# Production programming artifacts
# ---------------------------------------------------------------------------
def _build_production_hex(bundle, args) -> int:
    """Write the flow's HEX files through the shared artifact generator."""
    # Neither production flow takes a key source: both program a carried route
    # hex, whose batch was spliced in when the bundle was prepared. That is what
    # lets a party holding a prepared bundle and no keyset write the artifacts.
    flow = ph.FLOW_TWO_PHASE if args.two_phase else ph.FLOW_SINGLE
    for line in _early_secure_debug_access_warnings(bundle):
        print(f"warning: {line}")
    ph.write(
        bundle,
        flow=flow,
        out_dir=args.out_dir,
        soc=args.soc,
    )
    return 0


def _select_ota_packages(bundle, names) -> list:
    """The signed packages to write OTA packages for, in boot-chain order."""
    if names:
        selected = []
        for name in names:
            pkg = bundle.by_name(name)
            if pkg is None or pkg.role not in be.SIGNABLE_ROLES:
                raise Usage(
                    f"build-ota-package: {bundle.path} carries no updatable {name} "
                    f"component ({fw_bundle.identifiers(be.SIGNABLE_ROLES)})"
                )
            if pkg.signed_image is None:
                raise be.BundleEditError(
                    f"{pkg.ident} is not signed. Sign it with sign-component first."
                )
            if pkg not in selected:
                selected.append(pkg)
        return sorted(selected, key=lambda p: be.SIGNABLE_ROLES.index(p.role))

    selected = [
        pkg
        for role in be.SIGNABLE_ROLES
        if (pkg := bundle.by_role(role)) is not None and pkg.signed_image is not None
    ]
    unsigned = [
        pkg.ident
        for pkg in bundle.packages
        if pkg.role in be.SIGNABLE_ROLES and pkg.signed_image is None
    ]
    for ident in unsigned:
        print(f"skipped {ident}: not signed")
    if not selected:
        raise be.BundleEditError(
            f"{bundle.path} has no signed component to package. Sign with sign-component first."
        )
    return selected


def _build_ota_package(bundle, args) -> int:
    """Write one encrypted OTA package per signed component."""
    packages = _select_ota_packages(bundle, args.component)

    args.out_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="ironside-ota-") as scratch:
        work = Path(scratch)
        for pkg in packages:
            signed_hex = work / f"{pkg.role}.signed.hex"
            signed_hex.write_bytes(pkg.signed_image)
            ota = args.out_dir / f"{fw_bundle.package_basename(pkg.name)}{OTA_ARTIFACT_SUFFIX}"
            update_flow.encrypt_image(
                pkg.role,
                update_flow.signed_bin(pkg.role, signed_hex, work),
                ota,
                keyset=args.keyset,
                kek_gen=args.kek_gen,
            )
            print(f"wrote {ota}")
    return 0


def _add_component(bundle, args) -> int:
    packages = be.packages_from(args.source).packages
    if args.component:
        packages = [
            p
            for p in packages
            if args.component in (p.role, p.ident, p.ident.removeprefix("ironside-te-"))
        ]
        if not packages:
            raise be.BundleEditError(f"{args.source} holds no {args.component} component")
    for pkg in packages:
        what = be.add_package(bundle, pkg, replace=args.force)
        print(f"{pkg.ident} {pkg.version}: {what}")
    _report_stale_counters(bundle)
    return 0


def _report_replaced_prov(bundle, keyset) -> None:
    """Re-splice the route hexes onto a replaced provisioning package, or say why not.

    Replacing prov-secure replaces its user manifest too, so the provisioning
    record goes with it -- correctly: a record describes route hexes, and those
    are gone as well. Both come back from `update-keys`, which is the one command
    that settles the record and the firmware from a single key source.
    """
    if be.provisioning_record(bundle) is None:
        print(
            "  this bundle is no longer prepared for provisioning: the "
            "replacement carries neither the provisioning configuration nor the "
            "route firmware. Run `update-keys --keyset <spec>` to prepare it again."
        )
        return
    if keyset is None:
        print(
            "  warning: the provisioning route firmware was not rewritten, so "
            "this bundle cannot program a production route. Run `update-keys "
            "--keyset <spec>`."
        )
        return
    _report_prov_hexes(*ph.set_prov_hexes(bundle, keyset))


def _resolve_update_idents(bundle, args) -> list[str]:
    """Component identifiers to fetch when update-component downloads."""
    if args.all:
        return [p.ident for p in bundle.packages]
    if not args.component:
        raise Usage("update-component: name --component or --all when downloading")
    idents = []
    for name in args.component:
        matched = [
            p
            for p in bundle.packages
            if name in (p.role, p.ident, p.ident.removeprefix("ironside-te-"))
        ]
        if not matched:
            raise be.BundleEditError(
                f"{bundle.path} carries no {name} component; it holds "
                + ", ".join(sorted(p.ident for p in bundle.packages))
            )
        idents.extend(p.ident for p in matched)
    return list(dict.fromkeys(idents))


def _update_component(bundle, args) -> int:
    """Replace components and retain image configuration by default."""
    if args.source is None:
        if not args.all and not args.component:
            raise Usage("update-component: name --component or --all when downloading")
        dl.apply_defaults(args)
        if bundle.soc:
            args.soc = bundle.soc
        idents = _resolve_update_idents(bundle, args)
        with tempfile.TemporaryDirectory(prefix="ironside-update-") as scratch:
            dl.fetch_packages_to_dir(args, Path(scratch), idents=idents)
            if args.dry_run:
                return 0
            source = be.packages_from(Path(scratch)).packages
            if not source:
                raise be.BundleEditError("the selected builds could not be read back")
            return _apply_component_updates(bundle, args, source)

    if dl.download_using_remote(args):
        raise Usage(
            "update-component: download options cannot be combined with "
            f"SOURCE ({' '.join(dl.download_options_named(args))})"
        )
    source = be.packages_from(args.source).packages
    return _apply_component_updates(bundle, args, source)


def _apply_component_updates(bundle, args, source) -> int:
    wanted = set(args.component or [])
    updated = False
    replaced_prov = False
    for new in source:
        old = bundle.by_role(new.role)
        if wanted and not (wanted & {new.role, new.ident, new.ident.removeprefix("ironside-te-")}):
            continue
        if old is None and not args.add:
            print(f"{new.ident} {new.version}: not in this bundle, skipped (--add to put it in)")
            continue
        # Only UROT carries an image configuration. IROT has no MCUboot header to read one from.
        carried = (
            be.image_config(old.unsigned)
            if (old and old.role == "urot" and args.reapply_config)
            else None
        )
        drops_urot_config = old and old.role == "urot" and not args.reapply_config
        drops_authop_setting = (
            drops_urot_config and be.authop_revoke_gen(old.unsigned) is not None
        )
        access = be.early_secure_debug_access_mode(old) if old and old.role == "irot" else None
        # Replacing a component also replaces the keys it carries.
        dropped_keys = sorted(be.provisioned_keys(bundle)) if old else []
        was = f"{old.ident} {old.version}" if old else None
        be.add_package(bundle, new, replace=True)
        print(f"{new.ident} {new.version}: " + (f"replaces {was}" if was else "added"))
        if drops_authop_setting:
            print(
                f"  warning: {old.ident} carries an AUTHOP revocation setting, which "
                "--drop-config leaves off the new UROT. A device that has not applied it "
                "yet no longer revokes those generations. Leave out --drop-config to "
                "carry it onto the new UROT."
            )
        if old is not None and old.customized:
            print(f"  dropped {was}'s customization")
            if access and access != esda.MODE_OFF:
                print(
                    f"  dropped early secure debug access ({access}). "
                    "Run early-secure-debug-access again to restore it."
                )
            remaining = sorted(be.provisioned_keys(bundle))
            gone = [role for role in dropped_keys if role not in remaining]
            if gone:
                print(f"  removed {', '.join(gone)} keys. Run update-keys again.")
        if carried is not None:
            be.set_image_config(new, carried)
            print(
                f"  re-applied image configuration "
                f"({img_conf_args.described(carried)}), now unsigned"
            )
        if new.role in fw_bundle.PROV_ROLES:
            replaced_prov = True
        updated = True
    if not updated:
        label = args.source if args.source is not None else "the selected builds"
        raise be.BundleEditError(f"{label} holds nothing that updates a component of {bundle.path}")
    # A new provisioning package arrives with released members only, so the route
    # hexes are re-spliced onto it. Replacing prov-secure also takes the record
    # with it, which is why this reports rather than repairs when it is gone: the
    # repair is `update-keys`, and it needs the keyset.
    if replaced_prov:
        _report_replaced_prov(bundle, getattr(args, "keyset", None))
    _report_stale_counters(bundle)
    return 0


def _delete_component(bundle, args) -> int:
    pkg = bundle.by_name(args.component)
    if pkg is None:
        raise be.BundleEditError(
            f"{bundle.path} carries no {args.component} component; it holds "
            + ", ".join(sorted(p.ident for p in bundle.packages))
        )
    be.remove_package(bundle, pkg)
    print(f"{pkg.ident} {pkg.version}: removed")
    unsatisfied, absent = be.check_deps(bundle)
    for line in unsatisfied + absent:
        print(f"  warning: {line}")
    _report_stale_counters(bundle)
    return 0


# ---------------------------------------------------------------------------
# early-secure-debug-access
# ---------------------------------------------------------------------------
def _set_early_secure_debug_access(bundle, args) -> int:
    pkg = be.set_early_secure_debug_access(bundle, args.mode)
    print(f"{pkg.ident}: early secure debug access {be.early_secure_debug_access_mode(pkg)}")
    for line in _early_secure_debug_access_warnings(bundle):
        print(f"  warning: {line}")
    return 0


def _early_secure_debug_access_warnings(bundle) -> list:
    """The warning for an IROT that opens debug access at reset, or none."""
    irot = bundle.by_role("irot")
    if irot is None:
        return []
    mode = be.early_secure_debug_access_mode(irot)
    if mode in (esda.MODE_OFF, esda.MODE_UNKNOWN):
        return []
    return [
        f"{irot.ident} carries early secure debug access ({mode}). "
        "It opens debug access, secure included, from reset on a device whose "
        "access-port protection is not engaged. Use it for development only, and "
        "run `early-secure-debug-access --mode off` before building production artifacts."
    ]


# ---------------------------------------------------------------------------
# Command line
# ---------------------------------------------------------------------------
def _add_key_source(ap, *, resign=False):
    """Add key source options."""
    what = "re-sign with" if resign else "sign with"
    # dest stays "keyset": the flag is what a user types, and every handler and
    # the `init -S` alias already read args.keyset.
    ap.add_argument(
        "--keyset",
        dest="keyset",
        metavar="SPEC",
        help=f"IronSide key source to {what} as keyring:NAME or dir:PATH "
        "(default: only keyset in the OS keyring)",
    )
    ap.add_argument(
        "--gen", type=int, default=0, choices=(0, 1, 2), help="signing key generation (default: 0)"
    )


def _counters_arg(value):
    try:
        return pb.counters_arg(value)
    except pb.BatchError as exc:
        raise argparse.ArgumentTypeError(str(exc)) from None


def _parser(with_hidden_commands: bool = False):
    """Build the command-line parser.

    Hidden commands are registered only when @with_hidden_commands is set,
    because argparse lists every registered command in its invalid-choice error.
    """
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="command", metavar="COMMAND", title="commands")

    def command(name, help_text):
        p = sub.add_parser(name, help=help_text, description=help_text)
        return p

    def hidden_command(name, help_text):
        return sub.add_parser(name, description=help_text)

    def with_bundle(p):
        p.add_argument(
            "bundle",
            type=Path,
            metavar="ZIP|DIR",
            help="IronSide bundle as a ZIP file or directory",
        )
        p.add_argument(
            "-o",
            "--out",
            type=Path,
            metavar="ZIP",
            help="write a new bundle instead of editing in place",
        )
        return p

    p = command("init", "create a bundle from published or local components")
    p.add_argument(
        "out",
        nargs="?",
        type=Path,
        default=None,
        metavar="ZIP",
        help="output bundle (default: ironside_download.py naming)",
    )
    p.add_argument(
        "--from",
        dest="source",
        type=Path,
        metavar="PATH",
        help="local component archive or directory (default: download published components)",
    )
    dl.add_arguments(p)
    # dest=keyset: what the two handlers this then runs already read their key source
    # from, so they run here unchanged rather than through a translation.
    p.add_argument(
        "-S",
        "--update-keys-and-sign-all-with-keyset",
        dest="keyset",
        metavar="SPEC",
        help="provision keys and sign all components using keyring:NAME or dir:PATH",
    )
    # The signing knobs that run under --update-keys-and-sign-all-with-keyset, as
    # defaults rather than as flags of their own: `init` stays one decision --
    # which keyset -- and `sign-component` stays the only place a version or a
    # counter is chosen.
    # The provisioning-configuration fields come from the same table the
    # `update-keys` flags are built from, so a field added there is carried here
    # too. `init` runs _update_keys(), which reads every one of them, and a
    # missing default is an AttributeError on the documented Quick Start line.
    p.set_defaults(
        all=True,
        component=[],
        gen=0,
        version=None,
        security_counter=None,
        keep_security_counter=False,
        counters=None,
        without_ota_kek=False,
        **dict.fromkeys(cf.profile_flags(prov_conf_args.PROFILE).values()),
    )

    p = with_bundle(command("show", "show bundle contents and configuration"))
    scope = p.add_argument_group("scope (default: all)")
    scope.add_argument("--components", action="store_true", help="show components")
    scope.add_argument("--base-config", action="store_true", help="show base configuration")
    scope.add_argument("--image-config", action="store_true", help="show UROT image configuration")
    scope.add_argument(
        "--provisioning-config",
        action="store_true",
        help="show provisioning configuration",
    )
    p.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="add the public key values, per-component signing and counter "
        "detail, and the rest of the provisioning configuration",
    )
    # --json is the machine-readable form and is unaffected by -v - a consumer
    # parsing it wants every field regardless of how a reader wants it printed.
    p.add_argument("--json", action="store_true", help="print JSON")

    p = with_bundle(command("check", "check bundle contents and signatures"))
    p.add_argument(
        "--keyset",
        dest="keyset",
        metavar="SPEC",
        help="key source as keyring:NAME or dir:PATH. Without one the "
        "provisioning firmware is checked for the operations and rollback "
        "floors its recorded batch calls for, which is everything about it "
        "that does not depend on the OTA KEKs; with one, byte for byte",
    )

    p = with_bundle(command("export-config", "write configuration profiles"))
    bc.add_export_arguments(p)

    p = with_bundle(command("import-config", "apply configuration profiles"))
    bc.add_import_arguments(p)
    _add_key_source(p, resign=True)

    p = with_bundle(command("update-config", "set configuration fields the bundle carries"))
    bc.add_update_arguments(p)
    _add_key_source(p, resign=True)

    p = with_bundle(
        command("update-keys", "provision a keyset: verification keys and provisioning batches")
    )
    p.add_argument(
        "--keyset",
        dest="keyset",
        metavar="SPEC",
        help="IronSide key source as keyring:NAME or dir:PATH. Missing roles "
        "and generations are revoked. Any application keys the set holds are "
        "provisioned into the URoT storage under their own key IDs (default: only "
        "keyset in the OS keyring)",
    )
    batch = p.add_argument_group("provisioning batches (built from the same key source)")
    batch.add_argument(
        "--counters",
        type=_counters_arg,
        default=None,
        metavar="urot=N,app=N,dfu=N",
        help="initial rollback counters. urot, app, and dfu are 32-bit integers "
        "(default: each image's counter). Provisioned counters cannot be lowered",
    )
    batch.add_argument("--without-ota-kek", action="store_true", help=argparse.SUPPRESS)
    prov_conf_args.add_arguments(p, "provisioning configuration (kept as configured when omitted)")

    p = with_bundle(command("sign-component", "sign a bundle's images"))
    p.add_argument(
        "--component",
        action="append",
        default=[],
        metavar="NAME",
        help="the component to sign, by role or identifier (repeatable)",
    )
    p.add_argument(
        "--all",
        action="store_true",
        help=f"sign every signable component ({', '.join(be.SIGNABLE_ROLES)})",
    )
    _add_key_source(p)
    p.add_argument(
        "-v",
        "--version",
        metavar="MAJOR.MINOR.PATCH[.TWEAK]",
        help="image version (default: release version)",
    )
    p.add_argument(
        "-s",
        "--security-counter",
        type=lambda s: int(s, 0),
        metavar="N",
        help="security counter (default: release value, or derived from --version)",
    )
    p.add_argument(
        "--keep-security-counter",
        action="store_true",
        help="bump the version without retiring the resident image",
    )

    p = command("build-production-hex", "write the HEX files a production line programs")
    p.add_argument(
        "bundle",
        type=Path,
        metavar="ZIP|DIR",
        help="prepared IronSide bundle as a ZIP file or directory",
    )
    p.add_argument(
        "--two-phase",
        action="store_true",
        help="write one HEX per phase, for a line where a trusted entity "
        "installs and seals and a non-trusted one then closes the non-secure "
        "access port (default: one HEX, one entity)",
    )
    p.add_argument(
        "-o",
        "--out-dir",
        type=Path,
        default=Path("."),
        metavar="DIR",
        help="directory for the HEX files (default: the working directory)",
    )
    p.add_argument(
        "--soc",
        metavar="NAME",
        help="expected SoC name. It must match the bundle (default: use the bundle's SoC)",
    )

    p = command("build-ota-package", "write the encrypted OTA packages for the signed components")
    p.add_argument(
        "bundle",
        type=Path,
        metavar="ZIP|DIR",
        help="signed bundle as a ZIP file or directory",
    )
    p.add_argument(
        "--keyset",
        dest="keyset",
        required=True,
        metavar="SPEC",
        help="key source of the OTA KEK as keyring:NAME or dir:PATH",
    )
    p.add_argument(
        "--kek-gen",
        type=int,
        required=True,
        choices=(0, 1, 2),
        help="OTA KEK generation to wrap each package under. A device that installs "
        "a package revokes the generations below it",
    )
    p.add_argument(
        "--component",
        action="append",
        default=[],
        metavar="NAME",
        help="component to package: urot, dfu, app or an identifier (repeatable, "
        "default: every signed component)",
    )
    p.add_argument(
        "-o",
        "--out-dir",
        type=Path,
        default=Path("."),
        metavar="DIR",
        help="directory for the .ota.bin files (default: the working directory)",
    )

    p = with_bundle(command("add-component", "put a component into a bundle"))
    p.add_argument("source", type=Path, metavar="PATH", help="component archive or source bundle")
    p.add_argument(
        "--component", metavar="NAME", help="which component to take, when PATH holds several"
    )
    p.add_argument(
        "--force", action="store_true", help="replace a component already filling that role"
    )
    p = with_bundle(command("update-component", "replace components from a newer source"))
    p.add_argument(
        "source",
        nargs="?",
        type=Path,
        default=None,
        metavar="PATH",
        help="component archive or source bundle (default: download)",
    )
    dl.add_arguments(p, by_version=True)
    p.add_argument(
        "--component",
        action="append",
        default=[],
        metavar="NAME",
        help="component to update (repeatable; required when downloading)",
    )
    p.add_argument(
        "--all", action="store_true", help="download updates for every component in the bundle"
    )
    p.add_argument(
        "--add", action="store_true", help="also put in components the bundle does not have yet"
    )
    p.set_defaults(reapply_config=True)
    p.add_argument(
        "--drop-config",
        action="store_false",
        dest="reapply_config",
        help="use the new component's default image configuration",
    )
    p.add_argument(
        "--reapply-config",
        action="store_true",
        dest="reapply_config",
        default=argparse.SUPPRESS,
        help=argparse.SUPPRESS,
    )

    p = with_bundle(command("delete-component", "remove a component from a bundle"))
    p.add_argument(
        "component", metavar="NAME", help="the component to remove, by role or identifier"
    )

    if not with_hidden_commands:
        return ap

    p = with_bundle(
        hidden_command(
            ESDA_COMMAND,
            "open secure debug from reset through IROT's UICR.USERCNF entries (development only)",
        )
    )
    p.add_argument(
        "--mode",
        required=True,
        choices=esda.MODES,
        help="off: the released entries; debug: open DBGEN and SPIDEN for "
        "invasive debug and trace",
    )
    return ap


# Which commands only read, so nothing is written back and --out means nothing.
READ_ONLY = {"show", "check", "export-config", "build-production-hex", "build-ota-package"}

HANDLERS = {
    "show": lambda b, a: _show(b, _sections(a), a.json, a.verbose),
    "check": _check,
    "export-config": bc.export,
    "import-config": bc.apply_profiles,
    "update-config": bc.update,
    "update-keys": _update_keys,
    "sign-component": _sign,
    "build-production-hex": _build_production_hex,
    "build-ota-package": _build_ota_package,
    "add-component": _add_component,
    "update-component": _update_component,
    "delete-component": _delete_component,
    ESDA_COMMAND: _set_early_secure_debug_access,
}


def _sections(args):
    named = {
        name
        for name, on in (
            ("components", args.components),
            ("base_config", args.base_config),
            ("image_config", args.image_config),
            ("provisioning_config", args.provisioning_config),
        )
        if on
    }
    return named or {"components", "base_config", "image_config", "provisioning_config"}


def main(argv=None) -> int:
    if argv is None:
        argv = sys.argv[1:]
    ap = _parser(with_hidden_commands=argv[:1] == [ESDA_COMMAND])
    # init forwards the download options verbatim, so its own parse must tolerate
    # flags it has never heard of. Every other command owns all of its arguments.
    if argv[:1] == ["init"]:
        args = ap.parse_args(argv)
        try:
            return _init(args)
        except (Usage, be.BundleEditError, dl.DownloadError) as exc:
            return _fail(exc)
    args = ap.parse_args(argv)
    if args.command is None:
        ap.print_help()
        return 1

    try:
        bundle = be.read(args.bundle)
        status = HANDLERS[args.command](bundle, args)
        if args.command not in READ_ONLY:
            if args.out is not None:
                be.write_as(bundle, args.out)
                print(f"wrote {args.out}")
            else:
                rewritten = be.save(bundle)
                if rewritten:
                    print(f"updated {args.bundle} ({', '.join(rewritten)})")
                elif bundle.structure_changed:
                    print(f"updated {args.bundle}")
                else:
                    print(f"{args.bundle} is unchanged")
        return status
    except Usage as exc:
        print(f"ironside_bundle: {exc}", file=sys.stderr)
        return 1
    except (
        be.BundleEditError,
        bp.ProfileError,
        fw_bundle.BundleError,
        ph.ProdhexError,
        pb.BatchError,
        uicr.UicrError,
        conf_img.ConfImgError,
        mcuboot_image.SignError,
        dl.DownloadError,
    ) as exc:
        return _fail(exc)


def _fail(exc) -> int:
    print(f"ironside_bundle: {exc}", file=sys.stderr)
    return 1 if isinstance(exc, Usage) else 2


if __name__ == "__main__":
    sys.exit(main())
