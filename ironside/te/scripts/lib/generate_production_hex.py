# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Build provisioning HEX files from prepared IronSide bundles.

The library-only Open flow leaves access ports open for development.
"""

from __future__ import annotations

from .python_version import require_python

require_python()

import tempfile  # noqa: E402
from dataclasses import dataclass  # noqa: E402
from pathlib import Path  # noqa: E402

from . import app_keys as ak  # noqa: E402
from . import bundle_edit as be  # noqa: E402
from . import fw_bundle, uicr  # noqa: E402
from . import parts as soc_parts  # noqa: E402
from . import prov_abi as abi  # noqa: E402
from . import prov_batches as pb  # noqa: E402

FLOW_OPEN = "open"
FLOW_SINGLE = fw_bundle.ROUTE_SINGLE_PHASE
FLOW_TWO_PHASE = fw_bundle.ROUTE_TWO_PHASE
FLOW_PROTECT = "protect"
FLOW_PROT_OPEN = "prot-keep-ns-open"

# Production firmware and batches are carried in the bundle for inspection
# before programming. Developer flows splice their batches here so the bundle
# does not carry production-like artifacts that leave a device debuggable.
CARRIED_FLOWS = (FLOW_SINGLE, FLOW_TWO_PHASE)


class ProdhexError(Exception):
    """The artifacts cannot be written from this bundle."""


@dataclass(frozen=True)
class Phase:
    """One programming step: the HEX it writes and what that HEX does."""

    out: str
    # Operation groups use the bundle's configured access port closure.
    kinds: tuple
    # Lifecycle recipe selecting the images and provisioning firmware.
    lcs: str
    what: str
    # Must match the provisioning firmware selected by the lifecycle recipe.
    prov: str = fw_bundle.PROV_SECURE
    # Production route carrying the firmware and batch. Developer flows use None.
    route: str | None = None
    # Whether the HEX carries the firmware images as well as the provisioning
    # firmware. The untrusted phase does not - it runs after the trusted phase has
    # already programmed and sealed them, so re-programming them there would hand
    # a second entity a write it has no reason to make.
    images: bool = True

    def batch_name(self, closure: str, seed: str) -> str:
        """The batch this phase runs, at @closure and SEED mode @seed."""
        return pb.name(
            self.kinds,
            closure if "approtect" in self.kinds else None,
            seed if "seed" in self.kinds else None,
        )


FLOWS = {
    FLOW_OPEN: (
        Phase(
            "production_open.hex",
            ("seed", "install"),
            "assembly-and-test",
            "one entity: Unseeded -> Open, access ports left open",
        ),
    ),
    FLOW_SINGLE: (
        Phase(
            "production_single_phase.hex",
            ("seed", "install", "approtect", "protect"),
            "assembly-and-test",
            "one entity: Unseeded -> Secured",
            route=FLOW_SINGLE,
        ),
    ),
    FLOW_TWO_PHASE: (
        Phase(
            "production_two_phase_1.hex",
            ("seed", "install", "protect"),
            "assembly-and-test",
            "trusted entity: Unseeded -> PRoT Provisioning",
            route=FLOW_TWO_PHASE,
        ),
        Phase(
            "production_two_phase_2.hex",
            ("approtect",),
            "prot-provisioning",
            "non-trusted entity: PRoT Provisioning -> Secured",
            prov=fw_bundle.PROV_NONSECURE,
            route=FLOW_TWO_PHASE,
            images=False,
        ),
    ),
    # The two developer flows ironside_protect.py runs on a part that is already
    # installed, which is why neither carries the firmware images.
    FLOW_PROTECT: (
        Phase(
            "protect_secured.hex",
            ("approtect", "protect"),
            "assembly-and-test",
            "developer: Open -> Secured",
            images=False,
        ),
    ),
    FLOW_PROT_OPEN: (
        Phase(
            "protect_prot_provisioning.hex",
            ("protect",),
            "assembly-and-test",
            "developer: Open -> PRoT Provisioning, non-secure AP left open",
            images=False,
        ),
    ),
}


def resolve_soc(bundle: be.Editable, soc=None) -> str:
    """The part the artifacts target.

    The provisioning command address is part-specific. @soc can confirm the
    bundle value but cannot replace it. A bundle with no part requires @soc.
    """
    if bundle.soc is not None:
        if soc is not None and soc != bundle.soc:
            raise ProdhexError(f"{soc} disagrees with the bundle, which declares {bundle.soc}")
        return bundle.soc

    if soc is None:
        raise ProdhexError(
            "the bundle declares no SoC, so the provisioning command address "
            "cannot be placed. Name the part explicitly."
        )
    return soc


def _require_prov_hex(bundle: be.Editable, phase: Phase, batch_name: str) -> None:
    """Raise unless the bundle carries @phase's provisioning firmware.

    Naming the repair matters here - re-materializing the members runs an install
    batch through the splicer, so it needs the key source the bundle was
    prepared with.
    """
    pkg = bundle.by_role(phase.prov)
    member = fw_bundle.prov_route_member(phase.prov, phase.route)
    if pkg is None:
        raise ProdhexError(
            f"{bundle.path} carries no {fw_bundle.identifiers([phase.prov])} "
            f"component, which runs this flow's {batch_name!r} batch"
        )
    if pkg.get(member) is not None:
        return
    rec = be.provisioning_record(bundle)
    if rec is not None and unavailable(rec, batch_name):
        raise ProdhexError(
            f"this bundle provisions no OTA KEK, so it cannot carry the "
            f"{batch_name!r} batch and the {phase.route} route cannot be "
            f"programmed from it. Prepare the bundle with the OTA KEKs to use "
            f"this flow."
        )
    raise ProdhexError(
        f"{pkg.ident} carries no {member}, so this bundle holds no "
        f"programmable provisioning firmware for the {phase.route} route. "
        f"Run ironside_bundle.py update-keys --keyset to write it."
    )


def carried_hexes(closure: str, seed: str) -> tuple:
    """(role, member, batch name) for every provisioning hex a bundle carries.

    Derived from FLOWS rather than listed again, so a route cannot carry a
    different batch from the one the flow that programs it runs.
    """
    out = []
    for flow in CARRIED_FLOWS:
        for phase in FLOWS[flow]:
            if phase.route is None:
                continue
            member = fw_bundle.prov_route_member(phase.prov, phase.route)
            out.append((phase.prov, member, phase.batch_name(closure, seed)))
    return tuple(out)


def unavailable(rec: dict, batch_name: str) -> bool:
    """Whether the bundle @rec describes deliberately supplies no such batch.

    A bundle without OTA KEKs cannot combine seed enrollment and protection.
    The KEK imports prevent an unseeded device from being locked, so these
    batches and their routes are omitted.
    """
    return not pb.available(rec, batch_name)


def set_prov_hexes(bundle: be.Editable, keyset, *, report=None) -> tuple:
    """Write each production route's provisioning firmware into its package.

    The member is the released provisioning firmware with that route's batch at
    the command address, so `build-production-hex` merges bundle members rather
    than synthesising the batch half of what a station programs -- and so the
    batch bytes exist once, here, rather than in a second copy beside them.

    @keyset is required: every production route runs a batch covering "install",
    which imports the OTA KEKs. Re-run after anything that changes the record,
    the configured closure or seed, or a provisioning component.

    Returns the written and skipped routes. A KEK-free bundle skips routes that
    combine seed enrollment and protection.
    """
    rec = be.provisioning_record(bundle)
    if rec is None:
        raise ProdhexError(
            f"{bundle.path} is not prepared for provisioning. "
            f"Run ironside_bundle.py update-keys first."
        )
    cfg = be.provisioning_config(bundle)
    closure = pb.ap_closure(cfg)
    seed = pb.ikg_seed(cfg)
    # Refuse a missing part because the batch address is part-specific.
    try:
        soc = resolve_soc(bundle)
        cmd_addr = abi.layout_for(soc).cmd_addr
    except soc_parts.PartError as exc:
        raise ProdhexError(str(exc)) from exc
    keks = be.ota_kek_material(keyset) if pb.has_ota_kek(rec) else ()
    app_keys = be.app_key_material(keyset) if pb.has_ota_kek(rec) else ()

    written, skipped = [], []
    for role, member, batch_name in carried_hexes(closure, seed):
        pkg = bundle.by_role(role)
        if pkg is None:
            raise ProdhexError(
                f"{bundle.path} carries no "
                f"{fw_bundle.identifiers([role])} component, which runs the "
                f"{batch_name!r} batch"
            )
        if unavailable(rec, batch_name):
            # Dropped rather than left - a stale member here would be a route that
            # looks programmable on a bundle that cannot provision it.
            pkg.drop(member)
            skipped.append((pkg.ident, member, batch_name))
            continue
        try:
            blob = pb.build_from(rec, batch_name, keks=keks, app_keys=ak.ops(app_keys))
        except pb.BatchError as exc:
            raise ProdhexError(str(exc)) from exc
        # The batch has to fit the buffer the provisioning firmware reads it
        # from, which is 8 kB on the smaller parts. Checked here because the
        # alternative is a station finding out after the device is erased.
        try:
            abi.check_batch_size(blob, soc)
        except ValueError as exc:
            raise ProdhexError(f"{pkg.ident}: {batch_name!r}: {exc}") from exc
        try:
            pkg.set(member, be.splice_batch(pkg.released_image, blob, cmd_addr))
        except be.BundleEditError as exc:
            raise ProdhexError(f"{pkg.ident}: {exc}") from exc
        written.append((pkg.ident, member, batch_name))
        if report is not None:
            report(f"{pkg.ident}: {member} ({batch_name})")
    return written, skipped


def needs_keyset(flow: str) -> bool:
    """Whether @flow can only be written from the bundle's key source.

    True when one of its phases runs a batch covering "install": that is the
    group importing the OTA KEKs, and the KEKs live in the keyset rather than
    in the bundle. Every other batch -- `protect`, `approtect` and their
    combinations -- is built from the record alone, which is what keeps
    ironside_protect.py free of key material.

    The production routes are carried members, so they need nothing at write
    time; it is only a developer flow that builds its batch here.
    """
    return any(phase.route is None and "install" in phase.kinds for phase in FLOWS.get(flow, ()))


def write(
    bundle: be.Editable,
    *,
    flow: str,
    out_dir: Path,
    keyset=None,
    ap_closure: str | None = None,
    ikg_seed: str | None = None,
    soc=None,
    report=print,
) -> list:
    """Write @flow's HEX files into @out_dir. Returns them in programming order.

    The bundle must already be prepared: the keyset, the provisioning record
    and the per-route provisioning firmware from update-keys, and a signature on
    every signable component. Nothing here signs or provisions keys.

    Production routes merge firmware and batches already carried in the bundle.
    Developer flows build their batch here, from the record -- and @keyset is
    required exactly when that batch covers "install", because that is the group
    importing the OTA KEKs (needs_keyset() answers it per flow).

    @ap_closure lets a development tool avoid permanent hardware-fixed closure
    from production configuration, and @ikg_seed overrides the configured SEED
    mode for the same reason. Both default to what the bundle configures.
    """
    if flow not in FLOWS:
        raise ProdhexError(f"unknown flow {flow!r}. Expected one of: {', '.join(FLOWS)}")

    rec = be.provisioning_record(bundle)
    if rec is None:
        raise ProdhexError(
            f"{bundle.path} is not prepared for provisioning. "
            f"Run ironside_bundle.py update-keys first."
        )
    provisioned = be.provisioned_keys(bundle)
    if not provisioned:
        raise ProdhexError(
            f"{bundle.path} provisions no verification keys. "
            f"Run ironside_bundle.py update-keys first."
        )
    # An install batch imports the OTA KEKs of the keyset the bundle was
    # prepared with. A bundle re-keyed since then would provision one keyset and
    # import another's KEKs, which is what this refuses -- for the batch built
    # here and for the route hexes built earlier alike.
    try:
        pb.check_digest(rec, provisioned)
    except pb.BatchError as exc:
        raise ProdhexError(str(exc)) from exc
    keks = ()
    app_keys = ()
    if needs_keyset(flow) and pb.has_ota_kek(rec):
        try:
            # None resolves to the sole keyset in the keyring; the refusal below
            # names what the key source is for when nothing resolves.
            keks = be.ota_kek_material(keyset)
            app_keys = be.app_key_material(keyset)
        except be.BundleEditError as exc:
            raise ProdhexError(
                f"the {flow} flow provisions the OTA KEKs, so it can only be "
                f"written from the keyset this bundle was prepared with "
                f"({rec.get('keyset')}): {exc}"
            ) from exc

    resolved = resolve_soc(bundle, soc)
    if any(phase.images for phase in FLOWS[flow]):
        problems = be.dfu_placement_problems(bundle)
        if problems:
            raise ProdhexError("; ".join(problems))
    cfg = be.provisioning_config(bundle)
    closure = ap_closure or pb.ap_closure(cfg)
    seed = ikg_seed or pb.ikg_seed(cfg)
    if any(phase.images for phase in FLOWS[flow]):
        problems = be.ikg_seed_select_problems(be.ikg_seed_select(bundle), seed)
        if problems:
            raise ProdhexError("; ".join(problems))
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    written = []
    with tempfile.TemporaryDirectory(prefix="ironside-prodhex-") as tmp:
        work = Path(tmp)
        for phase in FLOWS[flow]:
            try:
                batch_name = phase.batch_name(closure, seed)
            except pb.BatchError as exc:
                raise ProdhexError(str(exc)) from exc
            batch_file = None
            if phase.route is None:
                # A developer flow: the batch is not a member, so build and
                # splice it. Only an install batch consumes @keks.
                try:
                    blob = pb.build_from(
                        rec, batch_name, keks=keks, app_keys=ak.ops(app_keys)
                    )
                except pb.BatchError as exc:
                    raise ProdhexError(str(exc)) from exc
                batch_file = work / f"{batch_name.replace('@', '_').replace(pb.SEED_SEP, '_')}.bin"
                batch_file.write_bytes(blob)
                batch_file.chmod(0o600)
            else:
                _require_prov_hex(bundle, phase, batch_name)
            out = out_dir / phase.out
            report(f"{phase.out}: {phase.what}")
            report(f"  batch: {batch_name} ({pb.describe(batch_name)})")
            be.build_prov_artifacts(
                bundle.path,
                lcs=phase.lcs,
                batch=batch_file,
                prov_route=phase.route,
                soc=resolved,
                out=out,
                work=work / phase.lcs,
                images=phase.images,
            )
            # Validate the merged artifact before programming write-once words.
            # A layout mismatch can set DECOMMISSIONED and permanently consume
            # the device, so remove invalid output instead of warning.
            problems = be.region_layout_problems(
                uicr.decode_file(out)["region"],
                soc=resolved,
                carriers=be.SLICE_CARRIERS if phase.images else (),
            )
            if problems:
                out.unlink(missing_ok=True)
                raise ProdhexError(f"{phase.out} was not written: " + "; ".join(problems))
            written.append(out)

    report(f"production artifacts for {resolved}: {out_dir}")
    for out in written:
        mode = "0600" if out.stat().st_mode & 0o077 == 0 else "0644"
        report(f"  {out.name} ({out.stat().st_size} bytes, {mode})")
    if pb.has_ota_kek(rec):
        secret = [o.name for o in written if o.stat().st_mode & 0o077 == 0]
        if secret:
            verb = "contains" if len(secret) == 1 else "contain"
            report(f"warning: {' and '.join(secret)} {verb} raw OTA KEKs")
    return written
