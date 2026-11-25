#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Generate an IronSide base configuration."""

import argparse
import json
import sys
from pathlib import Path

SCRIPTS = Path(__file__).resolve().parent.parent

sys.path.insert(0, str(SCRIPTS))
from lib.python_version import require_python  # noqa: E402

require_python()

from lib import base_conf_args, ecdsa, hybrid, uicr

SDK = SCRIPTS.parent


def _iron_keychain():
    """Import the key-access layer only when keys are read."""
    try:
        import iron_keychain as kc
    except ImportError as exc:
        raise uicr.UicrError(f"cannot load iron_keychain ({exc})") from exc
    return kc


def default_keyset():
    """Return the only keyset in the OS keyring.

    Multiple keysets require an explicit choice because the selected public
    keys are provisioned into write-once storage.
    """
    kc = _iron_keychain()
    try:
        return kc.sole_keyset_spec()
    except kc.KeySourceError as exc:
        raise uicr.UicrError(str(exc)) from exc


def _ecdsa_point_from_pem(curve, pem_bytes, what):
    """The uncompressed public point of a private or public PEM on @curve."""
    for load in (curve.load_private_key, curve.load_public_key):
        try:
            return ecdsa.public_bytes(load(pem_bytes, what))
        except (ValueError, TypeError):
            continue
    raise uicr.UicrError(f"{what}: not an ECDSA {curve.name} private or public key PEM")


class PubkeySource:
    """Provide the 32-byte key value to provision, by role and generation.

    For UROT that is the digest of the generation's hybrid key pair, and for App, DFU
    and AUTHOP the SHA-256 of the generation's ECDSA public key, which may be a
    private or public PEM.

    One ECDSA key serves at most one role. A device does not check which role an
    image is for when it boots it, so a key shared by two roles would let an image
    signed for one boot as the other.
    """

    def __init__(self, spec):
        kc = _iron_keychain()
        try:
            self._src = kc.KeySource.parse(spec)
        except kc.KeySourceError as exc:
            raise uicr.UicrError(str(exc)) from exc
        self._kc = kc
        self._ecdsa_roles = {}  # ECDSA public point -> the role it was read for

    def _claim_ecdsa_point(self, point, role, what):
        owner = self._ecdsa_roles.setdefault(point, role)
        if owner != role:
            raise uicr.UicrError(
                f"{what} is also a {owner} key in {self._src.describe()}; "
                "each role needs keys of its own"
            )

    def describe(self):
        return self._src.describe()

    def _hybrid_digest(self, gen):
        """The pair digest of one hybrid generation, or None if it has neither key."""
        ids = self._kc.hybrid_key_id_pair(gen)
        try:
            pems = [self._src.get_bytes(key_id) for key_id in ids]
        except self._kc.KeySourceError as exc:
            raise uicr.UicrError(str(exc)) from exc
        if all(pem is None for pem in pems):
            self._refuse_retired_only(hybrid.ROLE, gen, ids)
            return None
        absent = [key_id for key_id, pem in zip(ids, pems, strict=True) if pem is None]
        if absent:
            raise uicr.UicrError(
                f"{self._src.describe()} holds only half of hybrid generation {gen}: "
                f"{', '.join(absent)} is missing"
            )
        try:
            ecdsa_key = hybrid.load_ecdsa_key(pems[0], label=self._src.label(ids[0]))
            mldsa_key = hybrid.load_mldsa_key(pems[1], label=self._src.label(ids[1]))
        except hybrid.mci.SignError as exc:
            raise uicr.UicrError(str(exc)) from exc
        self._claim_ecdsa_point(hybrid.ecdsa_public_bytes(ecdsa_key), hybrid.ROLE, ids[0])
        return hybrid.key_pair_digest(ecdsa_key, mldsa_key)

    def _get(self, key_id):
        try:
            return self._src.get_bytes(key_id)
        except self._kc.KeySourceError as exc:
            raise uicr.UicrError(str(exc)) from exc

    def _refuse_retired_only(self, role, gen, key_ids):
        """Refuse a source that holds a generation only under an earlier layout's id.

        Provisioning would otherwise revoke that generation in write-once UICR.
        """
        for retired in self._kc.retired_key_ids(role, gen):
            if self._get(retired) is not None:
                raise uicr.UicrError(
                    f"{self._src.describe()} holds {retired}, a key of an earlier layout, "
                    f"but not {', '.join(key_ids)}: run iron_keychain.py new on the keyset "
                    "to add the keys it lacks"
                )

    def _ecdsa_digest(self, role, gen):
        """SHA-256 of one ECDSA role generation's public key, or None if it is absent."""
        key_id = self._kc.key_id_for(role, gen)
        data = self._get(key_id)
        if data is None:
            self._refuse_retired_only(role, gen, (key_id,))
            return None
        curve = self._kc.ecdsa_curve_for(key_id)
        point = _ecdsa_point_from_pem(curve, data, self._src.label(key_id))
        self._claim_ecdsa_point(point, role, self._src.label(key_id))
        return curve.key_digest(point)

    def pubkey(self, role, gen):
        """The role/generation key value, or None if this source lacks it."""
        if role == hybrid.ROLE:
            return self._hybrid_digest(gen)
        return self._ecdsa_digest(role, gen)


def collect_keys(source):
    """Return writes and provisioned or revoked role generations.

    Missing generations are revoked so they cannot be filled later.
    """
    writes, provisioned, revoked = [], [], []
    for role in uicr.ROLES:
        for gen in range(uicr.GENERATIONS):
            key = source.pubkey(role, gen)
            if key is None:
                writes += uicr.key_revoke_writes(role, gen)
                revoked.append(f"{role}.gen{gen}")
            else:
                writes += uicr.key_writes(role, gen, key)
                provisioned.append(f"{role}.gen{gen}")
    return writes, provisioned, revoked


def parse_revoke_gen(spec):
    """Parse a --revoke-gen ROLE:GEN value."""
    role, sep, gen_s = spec.partition(":")
    if not sep or role not in uicr.ROLES:
        raise uicr.UicrError(
            f"--revoke-gen {spec!r}: expected ROLE:GEN, ROLE one of {', '.join(uicr.ROLES)}"
        )
    try:
        gen = int(gen_s)
    except ValueError:
        raise uicr.UicrError(f"--revoke-gen {spec!r}: GEN must be an integer") from None
    if not 0 <= gen < uicr.GENERATIONS:
        raise uicr.UicrError(f"--revoke-gen {spec!r}: GEN must be 0..{uicr.GENERATIONS - 1}")
    return role, gen


def _require_soc(args) -> str:
    """The --soc that a partition layout is placed for, or exit without one."""
    if args.soc is None:
        sys.exit("gen_base_conf: writing partitions needs --soc to check them against the part")
    return args.soc


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate an IronSide base configuration.")
    ap.add_argument(
        "--keyset",
        dest="keyset",
        metavar="SPEC",
        help="key source as keyring:NAME or dir:PATH (default: only keyset in the OS keyring)",
    )
    # Share base-configuration arguments with ironside_bundle.py.
    base_conf_args.add_arguments(ap, group_title=None)
    ap.add_argument(
        "--dry-run", action="store_true", help="print the manifest without writing the output file"
    )
    ap.add_argument(
        "--manifest-json", action="store_true", help="print the configuration manifest as JSON"
    )
    ap.add_argument("--dump", type=Path, metavar="FILE", help="decode a configuration file")
    ap.add_argument(
        "--soc",
        choices=sorted(uicr.RRAM_SIZE_KB),
        help="target SoC, which the partition layout is checked against (required when "
        "partitions are written)",
    )
    ap.add_argument(
        "-o",
        "--output",
        "--out",
        type=Path,
        dest="output",
        help="output HEX or binary file (default: build/base_config.hex)",
    )

    image = ap.add_argument_group("Image defaults")
    image.add_argument(
        "--irot-defaults",
        action="store_true",
        help="include the IROT format version and default partitions",
    )
    image.add_argument(
        "--urot-defaults", action="store_true", help="include the UROT format version"
    )

    patch = ap.add_argument_group("Patch mode (HEX output only)")
    patch.add_argument(
        "--revoke-gen",
        action="append",
        default=[],
        metavar="ROLE:GEN",
        help="revoke one key generation (may be repeated)",
    )
    patch.add_argument(
        "--revoke",
        action="append",
        default=[],
        metavar="ROLE",
        choices=list(uicr.ROLES),
        help="revoke all generations of a role (may be repeated)",
    )
    patch.add_argument(
        "--decommission", action="store_true", help="mark the device as decommissioned"
    )
    args = ap.parse_args()

    if args.dump is not None:
        print(json.dumps(uicr.format_report(uicr.decode_file(args.dump, args.soc)), indent=2))
        return 0

    patching = bool(args.revoke_gen or args.revoke or args.decommission)
    if patching and args.keyset is not None:
        sys.exit("gen_base_conf: patch mode cannot be used with --keyset")

    image_slices = bool(args.irot_defaults or args.urot_defaults)
    if image_slices and (patching or args.keyset is not None):
        sys.exit("gen_base_conf: image defaults cannot be combined with --keyset or patch options")

    writes = []
    manifest = {}

    if image_slices:
        # Image slices contain build-wide values. Per-device keys remain in the
        # generated base configuration.
        parts = []
        if args.irot_defaults:
            writes += uicr.irot_format_version_writes(args.format_version)
            parts.append("irot.formatversion")
            # IROT requires a partition layout, so its image carries the default.
            sizes = base_conf_args.partitions_kb(args)
            if sizes is not None:
                writes += uicr.partition_writes(sizes, soc=_require_soc(args))
                manifest["partitions_kb"] = dict(zip(uicr.PARTITION_NAMES, sizes, strict=True))
                parts.append("irot.partitions")
        if args.urot_defaults:
            writes += uicr.urot_format_version_writes(args.format_version)
            parts.append("urot.formatversion")
        manifest.update(mode="image-slice", parts=parts, format_version=args.format_version)
    elif patching:
        revoked = []
        for role in dict.fromkeys(args.revoke):  # de-dup, keep order
            for gen in range(uicr.GENERATIONS):
                writes += uicr.key_revoke_writes(role, gen)
            revoked.append(f"{role}.all")
        for spec in dict.fromkeys(args.revoke_gen):
            role, gen = parse_revoke_gen(spec)
            if role in args.revoke:
                sys.exit(
                    f"gen_base_conf: --revoke-gen {spec}: {role} is already "
                    f"fully revoked via --revoke"
                )
            writes += uicr.key_revoke_writes(role, gen)
            revoked.append(f"{role}.gen{gen}")
        if args.decommission:
            writes += uicr.decommission_writes()
            manifest["decommissioned"] = True
        manifest.update(mode="patch", revoke=revoked)
    else:
        source = PubkeySource(args.keyset or default_keyset())
        key_writes, provisioned, revoked = collect_keys(source)
        writes += key_writes
        if not any(p.startswith("urot.") for p in provisioned) and not args.allow_no_urot:
            sys.exit(
                f"gen_base_conf: {source.describe()} has no UROT key, so "
                "the device would not boot. Use --allow-no-urot to continue."
            )
        # RoT images carry their own format versions and the default partition
        # layout. Duplicating those write-once words would make bundle merging
        # fail. This file contains per-device keys, revocations and AUTHLOCK. A
        # custom partition layout overrides the default carried by IROT.
        sizes = (
            base_conf_args.partitions_kb(args) if base_conf_args.partitions_given(args) else None
        )
        if sizes is not None:
            writes += uicr.partition_writes(sizes, soc=_require_soc(args))
            manifest["partitions_kb"] = dict(zip(uicr.PARTITION_NAMES, sizes))
        if not args.no_authlock:
            writes += uicr.authlock_provision_writes()
        manifest.update(
            mode="generate",
            source=source.describe(),
            format_version=args.format_version,
            provision=provisioned,
            revoke=revoked,
            authlock_marker=not args.no_authlock,
        )

    out = args.output or (SDK / "build" / "base_config.hex")
    if patching and out.suffix == ".bin":
        sys.exit("gen_base_conf: patch mode requires HEX output")

    if args.manifest_json:
        print(json.dumps(manifest, indent=2))
    elif args.dry_run:
        print("base configuration:")
        if manifest.get("mode") == "generate":
            print(f"  source:    {manifest['source']}")
            print(f"  provision: {', '.join(manifest['provision']) or '(none)'}")
        if "format_version" in manifest:
            print(f"  version:   {manifest['format_version']}")
        if manifest.get("mode") == "image-slice":
            print(f"  slice:     {', '.join(manifest['parts'])}")
        if "revoke" in manifest:
            print(f"  revoke:    {', '.join(manifest['revoke']) or '(none)'}")
        if "partitions_kb" in manifest:
            print(
                "  partitions: "
                + ", ".join(f"{k}={v} kB" for k, v in manifest["partitions_kb"].items())
            )
        if manifest.get("decommissioned"):
            print("  decommissioned: yes")

    if args.dry_run:
        return 0

    if out.suffix == ".bin":
        uicr.emit_bin(writes, out)
    else:
        uicr.emit_hex(writes, out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except uicr.UicrError as e:
        sys.exit(f"gen_base_conf: {e}")
    except ValueError as e:
        sys.exit(f"gen_base_conf: {e}")
