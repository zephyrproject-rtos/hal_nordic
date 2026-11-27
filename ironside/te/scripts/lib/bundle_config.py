# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""The bundle tool's configuration commands, and how a configuration is printed.

update-config sets the fields a bundle carries, export-config and import-config
move them through JSON profile files, and the listing functions render what a
bundle holds. Every flag and profile key comes from the configuration-field
resource through lib/config_fields.py, so adding a field there reaches all
three commands and the listings without a change here.

What stays here is what a command does with a field once it has one: which
package the value lands in, what invalidates a signature, and what a reader is
told afterwards.
"""

from __future__ import annotations

from pathlib import Path

from . import app_config
from . import bundle_edit as be
from . import bundle_profile as bp
from . import config_fields as cf
from . import fw_bundle, img_conf_args, prov_conf_args, uicr
from . import mcuboot_image as mci
from . import prov_batches as pb
from . import psa_key_ram as pkr

BASE = cf.profile("base")
IMAGE = cf.profile("image")
PROVISIONING = cf.profile("provisioning")

AP_CLOSURE = PROVISIONING.field("ap-closure")
KEY_SLOT_STORAGE_SIZE = BASE.field("key-slot-storage-size-bytes")
IKG_SEED_SELECT = BASE.field("ikg-seed-select")
PROVISION_USER_SEED = PROVISIONING.field("provision-user-seed")

# The profiles export-config writes and import-config applies, in the order
# their options are offered.
PROFILES = (BASE, IMAGE, PROVISIONING)

# The partition each base-config field sizes, by argparse destination.
_PARTITION_BY_DEST = {f.dest: f.partition for f in BASE.fields if f.partition}
_FLAG_BY_PARTITION = {f.partition: f.flag for f in BASE.fields if f.partition}


# ---------------------------------------------------------------------------
# Command line
# ---------------------------------------------------------------------------
def add_update_arguments(p) -> None:
    """Add update-config's configuration groups and the re-sign option."""
    cf.add_arguments(p, BASE.name, f"{BASE.title} (per product)", scope=cf.BUNDLE)
    cf.add_arguments(p, IMAGE.name, f"{IMAGE.title} (in the UROT image)")
    cf.add_arguments(p, PROVISIONING.name, f"{PROVISIONING.title} (per deployment)")
    _add_resign(p)


def add_export_arguments(p) -> None:
    _add_profile_options(p, "write", "stdout")
    p.add_argument(
        "--app-header",
        type=Path,
        default=None,
        metavar="FILE",
        help=f"write {app_config.HEADER_NAME}, the configuration an application is built "
        f"against; put its directory on the application's include path "
        f"(name it {app_config.HEADER_NAME})",
    )


def add_import_arguments(p) -> None:
    _add_profile_options(p, "apply", "stdin")
    _add_resign(p)


def _add_profile_options(p, verb: str, stream: str) -> None:
    for prof in PROFILES:
        p.add_argument(
            f"--{prof.name}",
            nargs="?",
            const=bp.STDIO,
            default=None,
            type=Path,
            metavar="FILE",
            help=f"{verb} {prof.title} profile (default: {stream})",
        )


def _add_resign(p) -> None:
    p.add_argument(
        "--resign", action="store_true", help="re-sign UROT after changing its image configuration"
    )


def _named(args) -> list:
    """The profiles a caller named a file for, with their paths."""
    return [(prof, getattr(args, prof.name)) for prof in PROFILES]


def _profile_list() -> str:
    flags = [f"--{prof.name}" for prof in PROFILES]
    return ", ".join(flags[:-1]) + f" or {flags[-1]}"


# ---------------------------------------------------------------------------
# update-config
# ---------------------------------------------------------------------------
def update(bundle, args) -> int:
    """Set the configuration fields named on the command line."""
    touched = False

    if args.ap_closure is not None:
        changed = be.set_ap_closure(bundle, args.ap_closure)
        label = pb.closure_label(args.ap_closure)
        print(
            f"{PROVISIONING.short_title}: {AP_CLOSURE.label} -> {label}"
            + ("" if changed else " (unchanged)")
        )
        touched = True

    if args.provision_user_seed is not None:
        changed = be.set_ikg_seed(bundle, args.provision_user_seed)
        print(
            f"{PROVISIONING.short_title}: {PROVISION_USER_SEED.label} -> "
            f"{pb.seed_label(args.provision_user_seed)}"
            + ("" if changed else " (unchanged)")
        )
        touched = True

    if args.ap_closure is not None or args.provision_user_seed is not None:
        _report_stale_prov_hexes(bundle)

    sizes = _resolve_partitions(bundle, args)
    if sizes is not None:
        before = be.partitions_kb(bundle)
        pkg = be.set_partitions(bundle, sizes)
        changed = (
            ", ".join(
                f"{name} {before[name] if before else '?'} -> {sizes[name]} kB"
                for name in uicr.PARTITION_NAMES
                if not before or before[name] != sizes[name]
            )
            or "unchanged"
        )
        print(f"{pkg.ident}: {BASE.short_title} -> {fw_bundle.USER_IMAGE_MEMBER} ({changed})")
        _warn_dfu_placement(bundle)
        touched = True

    if args.key_slot_storage_size is not None:
        before = be.key_slot_storage_size(bundle)
        size = be.set_key_slot_storage_size(bundle, args.key_slot_storage_size)
        print(
            f"{bundle.require('urot').ident}: {BASE.short_title} -> {fw_bundle.USER_IMAGE_MEMBER} "
            f"({KEY_SLOT_STORAGE_SIZE.label} {before} -> {size} B, "
            f"{pkr.psa_key_slots(size)} PSA key slots)"
        )
        touched = True

    if args.ikg_seed_select is not None:
        before = be.ikg_seed_select(bundle)
        be.set_ikg_seed_select(bundle, args.ikg_seed_select)
        print(
            f"{bundle.require('urot').ident}: {BASE.short_title} -> {fw_bundle.USER_IMAGE_MEMBER} "
            f"({IKG_SEED_SELECT.label} {before or uicr.IKG_SEED_SELECT_AUTO} -> "
            f"{args.ikg_seed_select})"
        )
        touched = True

    if img_conf_args.given(args):
        pkg = _editable_urot(bundle, args)
        blob = be.edit_image_config(pkg, args)
        print(
            f"{pkg.ident}: {IMAGE.short_title} -> {fw_bundle.USER_IMAGE_MEMBER} "
            f"({img_conf_args.described(blob)})"
        )
        _resign(bundle, pkg, args)
        touched = True

    if not touched:
        raise be.Usage(
            "update-config: specify a partition size, an image-configuration field, "
            "or the access port closure method (see --help)"
        )
    return 0


def _warn_dfu_placement(bundle) -> None:
    """Warn rather than refuse, so the layout and a DFU component built for it can
    be changed in either order. check and installation refuse the mismatch."""
    for line in be.dfu_placement_problems(bundle):
        print(f"warning: {line}")


def _resolve_partitions(bundle, args):
    """Merge the named partition sizes with the bundle's current layout."""
    wanted = {partition: getattr(args, dest) for dest, partition in _PARTITION_BY_DEST.items()}
    if all(size is None for size in wanted.values()):
        return None
    current = be.partitions_kb(bundle)
    if current is None:
        missing = [name for name, size in wanted.items() if size is None]
        if missing:
            raise be.BundleEditError(
                "this bundle has no partition layout. Specify all three sizes: "
                + ", ".join(_FLAG_BY_PARTITION[name] for name in missing)
            )
        current = {}
    return {name: (size if size is not None else current[name]) for name, size in wanted.items()}


def _editable_urot(bundle, args):
    """The UROT package, refusing a signed one unless the caller asked to re-sign."""
    pkg = bundle.require("urot")
    if pkg.signed_image is not None and not args.resign:
        raise be.BundleEditError(
            f"{pkg.ident} is signed. Changing its image configuration "
            "requires --resign and a key source."
        )
    return pkg


def _resign(bundle, pkg, args) -> None:
    if not args.resign:
        return
    result = be.sign(bundle, pkg, keyset=args.keyset, gen=args.gen)
    print(f"  re-signed: {result['report']}")
    for line in result["warnings"]:
        print(f"  warning: {line}")


def _report_stale_prov_hexes(bundle) -> None:
    """Say that a configuration edit left the route firmware behind.

    A configuration edit writes the configuration and nothing else, so a bundle
    already carrying route firmware now carries firmware built at the previous
    values. `check` fails such a bundle; this is what names the repair at the
    moment the edit is made.
    """
    carried = [f"{pkg.ident}/{member}" for pkg, member in be.secret_members(bundle)]
    prov_ns = bundle.by_role(fw_bundle.PROV_NONSECURE)
    if prov_ns is not None and prov_ns.get(fw_bundle.USER_IMAGE_MEMBER) is not None:
        carried.append(f"{prov_ns.ident}/{fw_bundle.USER_IMAGE_MEMBER}")
    if not carried:
        return
    print(
        "  the provisioning firmware this bundle carries was built at the "
        "previous configuration, so it no longer matches. Run "
        "`update-keys --keyset <spec>` to rewrite it:"
    )
    for name in carried:
        print(f"    {name}")


# ---------------------------------------------------------------------------
# export-config and import-config
# ---------------------------------------------------------------------------
def export(bundle, args) -> int:
    """Write the named configuration profiles."""
    named = _named(args)
    if all(path is None for _, path in named) and args.app_header is None:
        flags = ", ".join(f"--{prof.name}" for prof in PROFILES)
        raise be.Usage(f"export-config: name at least one of {flags} or --app-header")
    if sum(path == bp.STDIO for _, path in named) > 1:
        raise be.Usage(
            "export-config: only one profile can use stdout. Specify a file for the rest."
        )
    for prof, path in named:
        if path is None:
            continue
        bp.write_json(path, _exported(bundle, prof))
        if path != bp.STDIO:
            print(f"wrote {path}")
    if args.app_header is not None:
        _write_app_header(bundle, args.app_header, args.bundle.name)
    return 0


def _write_app_header(bundle, path: Path, source: str) -> None:
    cfg = app_config.values(
        bundle.require_soc(),
        be.effective_partitions_kb(bundle),
        be.key_slot_storage_size(bundle),
    )
    text = app_config.render(cfg, source)
    # An unchanged header keeps its timestamp, so a build that exports on every
    # run recompiles only when the configuration changed.
    if path.is_file() and path.read_text() == text:
        print(f"{path} is unchanged")
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    print(f"wrote {path}")


def _exported(bundle, prof) -> dict:
    if prof is BASE:
        return bp.export_base(bundle)
    if prof is IMAGE:
        urot = bundle.by_role("urot")
        return bp.export_image(be.image_config(urot.effective) if urot else None)
    return bp.export_provisioning(bundle)


def apply_profiles(bundle, args) -> int:
    """Apply the named configuration profiles."""
    named = _named(args)
    if all(path is None for _, path in named):
        raise be.Usage(f"import-config: name at least one of {_profile_list()}")
    if sum(path == bp.STDIO for _, path in named) > 1:
        raise be.Usage(
            "import-config: only one profile can use stdin. Specify a file for the rest."
        )
    for prof, path in named:
        if path is None:
            continue
        _apply_one(bundle, prof, path, args)
    return 0


def _apply_one(bundle, prof, path: Path, args) -> None:
    label = bp.profile_label(path)
    if prof is BASE:
        data = bp.load_base(path)
        if not data:
            raise be.Usage(f"import-config: {label} names nothing to apply")
        try:
            bp.apply_base(bundle, data)
        except uicr.UicrError as exc:
            raise bp.ProfileError(str(exc)) from exc
        print(f"{BASE.short_title} from {label}")
        _warn_dfu_placement(bundle)
        return
    if prof is IMAGE:
        data = bp.load_image(path)
        if not data:
            raise be.Usage(f"import-config: {label} names nothing to apply")
        pkg = _editable_urot(bundle, args)
        bp.apply_image(bundle, data)
        blob = be.image_config(pkg.unsigned)
        print(
            f"{pkg.ident}: {IMAGE.short_title} from {label} "
            f"({img_conf_args.described(blob) if blob else 'empty'})"
        )
        _resign(bundle, pkg, args)
        return
    data = bp.load_provisioning(path)
    if not data:
        raise be.Usage(f"import-config: {label} names nothing to apply")
    bp.apply_provisioning(bundle, data)
    carried = bp.export_provisioning(bundle)
    print(
        f"{PROVISIONING.short_title} from {label} "
        f"({prov_conf_args.described(carried[pb.CONFIG_AP_CLOSURE], pb.ikg_seed(carried))})"
    )


# ---------------------------------------------------------------------------
# Listings
# ---------------------------------------------------------------------------
def print_base(base: dict, verbose=False) -> None:
    """The base configuration, split by which image reads each word."""
    print(f"\n{BASE.title}:")
    key_lines = _key_lines if verbose else _key_summary
    for image, roles in (("irot", bp.IROT_KEY_ROLES), ("urot", bp.UROT_KEY_ROLES)):
        print(f"  {image}:")
        for label, value in cf.display(BASE.name, base[image], region=image):
            print(f"    {label + ':':<24} {value}")
        for depth, line in key_lines(base[image].get("keys", {}), roles):
            print(f"    {'  ' * depth}{line}")
    lock = base["urot"].get("authlock")
    if lock:
        print(f"    authlock:        {'provisioned' if lock['provisioned'] else 'not provisioned'}")


def print_image(img: dict) -> None:
    print(f"\nurot {IMAGE.title}:")
    for label, value in cf.display(IMAGE.name, img):
        print(f"  {label + ':':<26} {value}")
    if img.get("supported") is False:
        print(f"  format version {img['formatversion']}  (unsupported)")


def print_ram_layout(ram: dict | None) -> None:
    """The RAM areas the base configuration gives: the secure RAM, whose size
    the key slot storage sets, the non-secure RAM and, where the part has them,
    VPRCTX-RAM and the reserved top of RAM. Nothing here depends on the UROT
    release."""
    if ram is None:
        return
    rot, ns = ram["rot_ram"], ram["ram_ns"]
    print("\nRAM layout:")
    print(
        f"  Reserved (secure):         0x{rot['addr']:08x}  {ns['addr'] - rot['addr']} B "
        f"({ram['psa_key_slots']} key slots)"
    )
    print(f"  APP-RAM (non-secure):      0x{ns['addr']:08x}  {ns['size']} B")
    if ram["vprctx_ram"] is not None:
        vpr, top = ram["vprctx_ram"], ram["reserved_top"]
        print(f"  VPRCTX-RAM (non-secure):   0x{vpr['addr']:08x}  {vpr['size']} B (not linked)")
        print(f"  Reserved:                  0x{top['addr']:08x}  {top['size']} B")


def print_provisioning(prov: dict, verbose=False) -> None:
    """The configured closure and the conditions that prevent programming.

    Verbose output adds the remaining provisioning details.
    """
    print(f"\n{PROVISIONING.title}:")
    if not prov["carried"]:
        print(f"  {bp.show_error(prov)}")
        return
    for label, value in cf.display(PROVISIONING.name, prov):
        print(f"  {label + ':':<28} {value}")
    if not prov["prepared"]:
        print(
            f"  {'not prepared:':<28} run update-keys to provision a keyset "
            "and its provisioning firmware"
        )
        return
    if verbose or not prov["ota-kek"]:
        print(
            f"  {'OTA KEKs:':<28} "
            + (
                f"{pb.abi.KEK_GENERATIONS} generations, in the clear"
                if prov["ota-kek"]
                else "none: this device cannot accept an encrypted update"
            )
        )
    keys = prov["application-keys"]
    key_ids = ", ".join(str(k["id"]) for k in keys)
    print(
        f"  {'application keys:':<28} "
        + (f"{len(keys)}, key ID{'s' if len(keys) > 1 else ''} {key_ids}" if keys else "none")
    )
    space = prov["application-key-space"]
    if verbose:
        for key in keys:
            print(
                f"  {'':<28}   key {key['id']}: {key['bits']}-bit type "
                f"0x{key['type']:04X}, {space['keys'][key['id']]} bytes"
            )
    print(
        f"  {'application key space:':<28} {space['used']} / {space['capacity']} bytes "
        f"({space['free']} free)"
    )
    if verbose:
        print(f"  {'rollback counters:':<28} {counter_line(prov['counters']) or 'not recorded'}")
        print(f"  {'key source:':<28} {prov['keyset'] or 'not recorded'}")
        for member in prov["provisioning-hexes"]:
            print(f"  {'provisioning firmware:':<28} {member}")
    for member in prov["missing-hexes"]:
        print(f"  {'provisioning firmware:':<28} {member}  (absent)")
    for batch_name in prov["unavailable-routes"]:
        print(f"  {'route not available:':<28} {batch_name}  (needs the OTA KEKs)")
    if prov["missing-hexes"]:
        print("  run update-config --ap-closure to write the absent members")
    if prov["stale"]:
        print(f"  warning: {prov['stale']}")


def _key_lines(keys, roles=uicr.ROLES):
    """Yield (indent, line) pairs listing each generation's pubkey and state."""
    for role in roles:
        gens = keys.get(role) or {}
        if not gens:
            yield 0, f"{role} pubkey: none provisioned"
            continue
        yield 0, f"{role} pubkey:"
        for gen in range(uicr.GENERATIONS):
            state = gens.get(gen)
            if state is None:
                yield 1, f"g{gen}: unset"
                continue
            label = be.generation_state_label(state)
            key = state.get("key")
            if key:
                yield 1, f"g{gen}: {key} ({label})"
            else:
                yield 1, f"g{gen}: ({label}, key incomplete)"


def _key_summary(keys, roles=uicr.ROLES):
    """Yield (indent, line) pairs naming each role's generations, without values.

    The concise form of _key_lines(): which generations a device provisioned
    from this bundle will trust is the fact a reader wants from a listing, and
    the 32 bytes behind each one is what -v is for.
    """
    for role in roles:
        gens = keys.get(role) or {}
        if not gens:
            yield 0, f"{role} pubkey: none provisioned"
            continue
        by_label = {}
        for gen in range(uicr.GENERATIONS):
            state = gens.get(gen)
            if state is None:
                label = "unset"
            elif state["provisioned"] and not state["revoked"] and not state.get("key"):
                # _key_lines() spells this out, so the summary must too. A slot
                # marked provisioned with no key words is not a usable key.
                label = "key incomplete"
            else:
                label = be.generation_state_label(state)
            by_label.setdefault(label, []).append(f"g{gen}")
        provisioned = by_label.pop("provisioned", [])
        # Anything not provisioned is named too, so a revoked or half-written
        # generation cannot hide behind a shorter list of the good ones.
        rest = "; ".join(
            f"{', '.join(by_label[label])} {label}"
            for label in ("revoked", "key incomplete", "unset")
            if label in by_label
        )
        summary = ", ".join(provisioned) if provisioned else "none provisioned"
        yield 0, f"{role} pubkey: {summary}" + (f" ({rest})" if rest else "")


# ---------------------------------------------------------------------------
# Rollback counters, which the provisioning configuration records
# ---------------------------------------------------------------------------
def counter_value(value, role) -> str:
    """One counter of a @role image, with what it packs."""
    if value == 0:
        return "0"
    return f"0x{value:08X} ({mci.counter_display(value, role)})"


def counter_line(counters) -> str:
    """The initial rollback counters, with what each one packs.

    Both spellings, because the value is what the device stores and the version
    is what a reader recognizes - and the mapping between them is exact.
    """
    return ", ".join(f"{role}={counter_value(value, role)}" for role, value in counters.items())


def counter_map(values: dict) -> str:
    """{role: counter} as one readable clause."""
    return ", ".join(f"{role} 0x{value:08X}" for role, value in sorted(values.items()))

