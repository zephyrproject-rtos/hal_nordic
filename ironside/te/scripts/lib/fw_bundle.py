# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Read and validate IronSide bundles.

Accepts bundle ZIPs and directories of component archives. Customized images
and route-specific provisioning firmware take precedence where defined.
"""

from __future__ import annotations

from .python_version import require_python

require_python()

import io
import json
import logging as log
import zipfile
from dataclasses import dataclass, field
from pathlib import Path

from . import mcuboot_encrypt

# Component identifier -> the image role it fills. The identifiers are the user
# guide's; the roles are the tooling's own vocabulary. Both DFU flavours fill the
# one dfu role -- a bundle for an install carries one of them.
COMPONENT_ROLES = {
    "ironside-te-irot": "irot",
    "ironside-te-urot": "urot",
    "ironside-te-dfu-serial": "dfu",
    "ironside-te-dfu-ble": "dfu",
    "ironside-te-prov-secure": "prov-secure",
    "ironside-te-prov-nonsecure": "prov-nonsecure",
    # The application: the customer's own firmware. Listed so a packaged
    # application is recognized and can travel in a bundle beside the components
    # it runs on.
    "ironside-te-app": "app",
}

# The IronSide TE images, in boot-chain order: what an install requires signed
# and an update programs. The app role is absent because the customer supplies
# the application. A programming artifact, the install's included, takes its
# images from its own merge recipe instead.
IMAGE_ROLES = ("irot", "urot", "dfu")

# Roles whose images carry editable bundle configuration (partition layout in IROT;
# keyset and DFU-entry triggers in UROT). DFU is signable but carries none of it.
CONFIGURABLE_ROLES = ("irot", "urot")

# The provisioning firmware, one role per lifecycle phase it runs in: the secure
# image runs in Assembly and Test, the non-secure one once the secure AP is burned.
PROV_SECURE = "prov-secure"
PROV_NONSECURE = "prov-nonsecure"
PROV_ROLES = (PROV_SECURE, PROV_NONSECURE)

ALL_ROLES = IMAGE_ROLES + PROV_ROLES

# What a component archive holds, of the files read here.
IMAGE_MEMBER = "image.hex"
MANIFEST_MEMBER = "manifest.json"

# The members a CUSTOMIZED bundle carries in addition -- written by
# ironside_bundle.py, never by a published release. Every one is named `*.user.*`,
# which is the whole rule: a release bundle has no such member, so "has this been
# customized" is answerable from the file names alone, and reverting a
# customization is a deletion rather than a restore step.
USER_IMAGE_MEMBER = "image.user.hex"
USER_SIGNED_MEMBER = "image.user.signed.hex"
USER_MANIFEST_MEMBER = "manifest.user.json"

# The production routes a bundle carries programmable provisioning firmware for.
# Each carried member is the released provisioning firmware with that route's
# provisioning batch already at the command address, so everything a programming
# station writes is a bundle member rather than something a tool synthesises.
#
# The non-secure provisioning firmware is only run by the two-phase route's
# second phase, so it uses the plain user image member. This also puts it in the
# precedence rule below, where consumers asking for that role receive the batch
# with it. The secure firmware serves both
# routes, and one command address means one batch per HEX, so its two are named
# apart and are selected by route rather than by precedence.
ROUTE_SINGLE_PHASE = "single-phase"
ROUTE_TWO_PHASE = "two-phase"
USER_ROUTE_MEMBERS = {
    ROUTE_SINGLE_PHASE: "image.user.single-phase.hex",
    ROUTE_TWO_PHASE: "image.user.two-phase.hex",
}

# The configuration is the deployment's own decision: which access port closure
# method and which IKG SEED entry its programming artifacts apply. Separate from
# the record because it takes no key material and is chosen before a bundle is
# prepared, which is what lets an assembly stage with no keyset set it.
#
# Two members, the precedence rule the images already use: the release ships the
# defaults and never modifies them, and an edit writes the `*.user.*` one, which
# displaces it.
PROVISIONING_CONFIG = "provisioning.json"
USER_PROVISIONING_CONFIG = "provisioning.user.json"

USER_MEMBERS = (
    USER_IMAGE_MEMBER,
    USER_SIGNED_MEMBER,
    USER_MANIFEST_MEMBER,
    USER_PROVISIONING_CONFIG,
) + tuple(USER_ROUTE_MEMBERS.values())

# Every member this module and its editor read, released and user alike. A name
# absent here is carried through an edit but never read.
PACKAGE_MEMBERS = (IMAGE_MEMBER, MANIFEST_MEMBER, PROVISIONING_CONFIG) + USER_MEMBERS

# A bundle is a zip of component archives and nothing else: there is no
# bundle-level member. Both halves of the provisioning state live in the
# prov-secure package, beside the route hexes they describe.
#
# The record is what `update-keys` produces: the key source, a digest over the
# verification keys it provisioned, the counter floors, and whether the OTA KEKs
# were withheld. It holds no batch bytes. A batch is built where it is used, so
# those bytes exist once, in the route hex carrying them.
PROVISIONING_RECORD = "provisioning"

# Every zip these tools write is reproducible: one fixed member timestamp and no
# compression, so the same components give the same bundle bytes. Here rather than
# in a writer, because the bundle and the packages inside it are one format.
FIXED_TIMESTAMP = (1980, 1, 1, 0, 0, 0)


class BundleError(Exception):
    """Not an IronSide bundle, or not a self-consistent one."""


@dataclass
class Component:
    """One component archive, as far as a tool taking a bundle cares.

    @image is the unsigned image to use: the customized one where the package
    carries it, else the one the release shipped. @signed is the signed image the
    package carries, or None -- a consumer holding one must not sign again, and
    must program that rather than a signature of its own.
    """

    ident: str
    role: str
    image: Path
    version: str
    soc: str | None
    package: str
    # None for an application package or a package released before feature levels.
    feature_level: int | None = None
    released_image: Path | None = None
    signed: Path | None = None
    signing_key: str | None = None
    # {route: path} for the provisioning firmware members this package carries
    # per production route. Outside the precedence rule on purpose: one package
    # holds several, and which one applies is the route, not a default.
    prov_routes: dict = field(default_factory=dict)

    @property
    def ota_name(self) -> str:
        """The encrypted OTA image filename for this package.

        Mirrors the component archive basename with a .ota.bin suffix, so an export
        directory names the same release the bundle held
        (ironside-te-urot_fl<feature_level>_<soc>-<semver>.ota.bin).
        """
        return f"{self.package}{mcuboot_encrypt.OTA_ARTIFACT_SUFFIX}"

    @property
    def customized(self) -> bool:
        """Does this package carry an image other than the one released?"""
        return self.released_image is not None and self.image != self.released_image

    @property
    def described(self) -> str:
        """This component in a bundle's one-line summary.

        The identifiers lose their common "ironside-te-" prefix and the version
        loses its @<soc> suffix, both of which the summary names once for the
        bundle rather than four times over. Customization is named because it
        changes what gets programmed while leaving the version untouched -- the
        version alone no longer identifies the bytes.
        """
        version = self.version
        if self.soc:
            version = version.removesuffix(f"@{self.soc}")
        marks = [
            m
            for m, on in (("customized", self.customized), ("signed", self.signed is not None))
            if on
        ]
        note = f" ({', '.join(marks)})" if marks else ""
        feature_level = "" if self.feature_level is None else f" fl{self.feature_level}"
        return f"{self.ident.removeprefix('ironside-te-')}{feature_level} {version}{note}"


def prov_route_member(role: str, route: str):
    """The member @role's package carries @route's provisioning firmware in.

    The non-secure firmware is only ever run by the two-phase route's second
    phase, so it uses the plain user image member and no other route names it --
    which also puts it in the precedence rule, so a consumer asking that role for
    an image gets its batch with it. The secure firmware serves both routes, so
    its members carry the route in their names and are selected by route.
    """
    if role == PROV_NONSECURE:
        return USER_IMAGE_MEMBER if route == ROUTE_TWO_PHASE else None
    if role == PROV_SECURE:
        return USER_ROUTE_MEMBERS.get(route)
    return None


def ota_in_dir(directory: Path, role: str) -> Path:
    """The one encrypted OTA image for @role under @directory.

    Files are named like the component archives a release carries
    (ironside-te-urot_<soc>-<semver>{mcuboot_encrypt.OTA_ARTIFACT_SUFFIX}). Both DFU
    flavours share the dfu role, so more than one match is an error.
    """
    idents = [ident for ident, r in COMPONENT_ROLES.items() if r == role]
    matches = sorted(
        p
        for ident in idents
        for p in directory.glob(f"{ident}_*{mcuboot_encrypt.OTA_ARTIFACT_SUFFIX}")
    )
    if not matches:
        raise BundleError(
            f"{directory} has no {mcuboot_encrypt.OTA_ARTIFACT_SUFFIX} for the {role.upper()} component "
            f"({', '.join(idents)}_*{mcuboot_encrypt.OTA_ARTIFACT_SUFFIX})"
        )
    if len(matches) > 1:
        names = ", ".join(p.name for p in matches)
        raise BundleError(
            f"{directory} has several {mcuboot_encrypt.OTA_ARTIFACT_SUFFIX} files for {role.upper()} "
            f"({names}); leave only one"
        )
    return matches[0]


def _json_member_of(package: Path | bytes, member: str, where: str) -> dict:
    """One JSON member of a package, held as a path or as zip bytes.

    An absent member reads as {}, so a caller distinguishes "not carried" by the
    emptiness rather than by knowing which shape the package arrived in.
    """
    if isinstance(package, bytes):
        buf = io.BytesIO(package)
        if not zipfile.is_zipfile(buf):
            return {}
        with zipfile.ZipFile(buf) as czf:
            if member not in set(czf.namelist()):
                return {}
            return _manifest(czf.read(member), where)
    if package.is_dir():
        path = package / member
        return _manifest(path.read_bytes(), str(path)) if path.is_file() else {}
    if not zipfile.is_zipfile(package):
        return {}
    return _json_member_of(package.read_bytes(), member, where)


def provisioning_record(bundle: Path):
    """The provisioning record @bundle carries, or None when it has none.

    Its own read rather than a field off read(): a caller wanting only the
    recorded decisions -- which batch to build, at which closure and seed -- has
    no use for extracted images, and read(roles=()) extracts nothing at all.
    Reads both shapes a bundle arrives in, so a consumer does not care which.
    """
    bundle = Path(bundle)
    for name, package in _packages_for_record(bundle):
        if COMPONENT_ROLES.get(identify(name) or "") != PROV_SECURE:
            continue
        where = f"{bundle} member {name}"
        record = _json_member_of(package, USER_MANIFEST_MEMBER, where).get(PROVISIONING_RECORD)
        if record is None:
            return None
        if not isinstance(record, dict):
            raise BundleError(
                f"{name}: {USER_MANIFEST_MEMBER} names a {PROVISIONING_RECORD!r} that is not a map"
            )
        return record
    return None


def provisioning_config(bundle: Path) -> dict:
    """The provisioning configuration @bundle carries.

    The `*.user.*` member where the bundle has one, else the released defaults,
    else {} for a bundle carrying neither. Field names are the `update-config`
    flags without their leading dashes, so this and an exported configuration
    profile are one format.

    Takes no key material and tolerates an unprepared bundle, which is what lets
    an assembly stage set it before a keyset exists.
    """
    bundle = Path(bundle)
    for name, package in _packages_for_record(bundle):
        if COMPONENT_ROLES.get(identify(name) or "") != PROV_SECURE:
            continue
        where = f"{bundle} member {name}"
        for member in (USER_PROVISIONING_CONFIG, PROVISIONING_CONFIG):
            carried = _json_member_of(package, member, where)
            if carried:
                return carried
        return {}
    return {}


def _packages_for_record(bundle: Path):
    """[(name, package)] for a record or configuration read: a path, or zip bytes."""
    if bundle.is_dir():
        entries = sorted(bundle.iterdir())
        unpacked = {p.name for p in entries if p.is_dir()}
        for entry in entries:
            if entry.is_file() and entry.with_suffix("").name in unpacked:
                continue
            if entry.is_dir() or entry.suffix == ".zip":
                yield entry.name, entry
        return
    if not bundle.is_file() or not zipfile.is_zipfile(bundle):
        return
    with zipfile.ZipFile(bundle) as zf:
        for member in sorted(zf.namelist()):
            if not member.endswith("/"):
                yield member, zf.read(member)


@dataclass
class Bundle:
    """What one IronSide bundle supplies, by role."""

    path: Path
    components: dict  # role -> Component, for the roles that were asked for
    soc: str | None
    # The provisioning record the prov-secure package carries, or None when the
    # bundle was never prepared. A plain dict rather than bytes: it is a handful
    # of recorded decisions, not a container a consumer has to decode.
    provisioning: dict | None = None

    def image(self, role: str):
        """The hex for @role, or None if the bundle carries no such component."""
        component = self.components.get(role)
        return component.image if component else None

    def signed(self, role: str):
        """The signed hex for @role, or None if the bundle carries none.

        A bundle carrying one has already been signed by ironside_bundle.py, from a
        key source that may not be the one the caller holds -- so a consumer
        programs it as-is and cross-checks the key, rather than signing over it.
        """
        component = self.components.get(role)
        return component.signed if component else None

    def prov_hex(self, role: str, route: str):
        """@role's provisioning firmware for @route, batch included, or None.

        None means the bundle was never prepared for that route (or the
        preparation was reset), which a caller reports rather than works around -
        splicing a batch here would put the content back outside the bundle.
        """
        component = self.components.get(role)
        return component.prov_routes.get(route) if component else None

    def images(self) -> dict:
        """{role: hex} for every IMAGE_ROLES role present, in boot-chain order."""
        return {
            role: self.components[role].image for role in IMAGE_ROLES if role in self.components
        }

    @property
    def summary(self) -> str:
        """One line: every component used, and the SoC they all target."""
        used = ", ".join(
            self.components[role].described for role in ALL_ROLES if role in self.components
        )
        return f"{used} ({self.soc})" if self.soc else used


def version_key(semver: str):
    """Sort key for a component version: numeric fields first, prerelease below
    its own release (`1.2.3-rc1` < `1.2.3`), per SemVer 2.0."""
    core = semver.split("+")[0]
    core, _, pre = core.partition("-")
    return (tuple(int(field) for field in core.split(".")), 0 if pre else 1, pre)


def satisfies(have: str, floor: str) -> bool:
    """Whether the ABI identifier @have meets the minimum @floor.

    A `deps` entry is a FLOOR, not a pin (user guide, "Manifest file format"),
    and the `@<soc>` suffixes make identifiers from different SoCs non-comparable
    by construction -- so a differing suffix is not satisfied at any version.

    Here rather than in ironside_download.py, which is where it was: the floors are
    part of the manifest format this module reads, and ironside_bundle.py checks
    them on a bundle it never downloaded. Reaching into the downloader for the
    comparison would have made `requests` -- a dependency requirements.txt calls
    out as needed only for fetching a release -- a requirement for inspecting a
    bundle offline.
    """
    have_semver, _, have_soc = have.partition("@")
    floor_semver, _, floor_soc = floor.partition("@")
    if have_soc != floor_soc:
        return False
    return version_key(have_semver) >= version_key(floor_semver)


def identifiers(roles) -> str:
    """The component identifiers that fill @roles, for naming what a bundle is
    missing in the words the release itself uses."""
    return ", ".join(sorted(ident for ident, role in COMPONENT_ROLES.items() if role in roles))


def package_basename(name: str) -> str:
    """The component archive name without a .zip extension.

    A bundle member is either a zip archive or an unpacked directory; both spell
    the same package identity in their file name.
    """
    stem = Path(name).name
    if stem.endswith(".zip"):
        return stem[: -len(".zip")]
    return stem


def identify(name: str):
    """The component identifier @name (a file or directory name) starts with.

    Longest first: no identifier is a prefix of another today, and a future one
    that is (an `ironside-te-dfu-serial2`) must not be shadowed by its own stem.
    """
    for ident in sorted(COMPONENT_ROLES, key=len, reverse=True):
        if Path(name).name.startswith(ident):
            return ident
    return None


def _manifest(raw: bytes, where: str) -> dict:
    try:
        return json.loads(raw)
    except json.JSONDecodeError as exc:
        raise BundleError(f"{where}: {MANIFEST_MEMBER} is not valid JSON ({exc})") from exc


def _from_zip_bytes(blob: bytes, where: str, ident: str, dest: Path) -> tuple:
    """({member: path}, manifest, user manifest) of a package held in memory.

    The package is read from memory and only the KNOWN file names are taken out of
    it, so nothing an archive names can decide a path on disk. Both the released
    image and any `*.user.*` member are extracted, because a caller that programs
    the customized one still wants the released one beside it to compare against.
    """
    buf = io.BytesIO(blob)
    if not zipfile.is_zipfile(buf):
        raise BundleError(
            f"{where} is not a zip archive -- a bundle's members are the component archives"
        )
    with zipfile.ZipFile(buf) as czf:
        names = set(czf.namelist())
        if IMAGE_MEMBER not in names:
            raise BundleError(
                f"{where} has no {IMAGE_MEMBER} -- is it an IronSide TE component archive?"
            )
        blobs = {name: czf.read(name) for name in PACKAGE_MEMBERS if name in names}
    manifest = _manifest(blobs[MANIFEST_MEMBER], where) if MANIFEST_MEMBER in blobs else {}
    user_manifest = (
        _manifest(blobs[USER_MANIFEST_MEMBER], where) if USER_MANIFEST_MEMBER in blobs else {}
    )
    files = {}
    for name, data in blobs.items():
        if name.endswith(".json"):
            continue
        out = dest / ident / name
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_bytes(data)
        files[name] = out
    return files, manifest, user_manifest


def _from_dir(package: Path, ident: str) -> tuple:
    """({member: path}, manifest, user manifest) of an unpacked package.

    Used in place rather than copied - it is a directory the caller can already
    look at, and copying it would only make a second thing to keep straight.
    """
    image = package / IMAGE_MEMBER
    if not image.is_file():
        raise BundleError(
            f"{package} has no {IMAGE_MEMBER} -- is it an IronSide TE component archive?"
        )
    present = {name: package / name for name in PACKAGE_MEMBERS if (package / name).is_file()}
    manifests = []
    for name in (MANIFEST_MEMBER, USER_MANIFEST_MEMBER):
        path = present.pop(name, None)
        manifests.append(_manifest(path.read_bytes(), str(path)) if path else {})
    return present, manifests[0], manifests[1]


def _members(bundle: Path):
    """[(name, reader)] for the component archives @bundle holds.

    One shape out of two in: the members of a bundle zip, or the entries of a
    directory. @reader takes the destination for extracted images and returns
    (image path, manifest), so which shape it came from stops mattering here.

    A directory can hold both <package>/ and <package>.zip for one component;
    the two are the same package, so the unpacked one is used and its zip is
    not reported as a duplicate.
    """
    if bundle.is_dir():
        entries = sorted(bundle.iterdir())
        unpacked = {p.name for p in entries if p.is_dir()}
        for entry in entries:
            if entry.is_file() and entry.with_suffix("").name in unpacked:
                continue
            if entry.is_dir():
                yield entry.name, (lambda _dest, e=entry, **kw: _from_dir(e, **kw))
            elif entry.suffix == ".zip":
                yield (
                    entry.name,
                    (
                        lambda dest, e=entry, **kw: _from_zip_bytes(
                            e.read_bytes(), str(e), dest=dest, **kw
                        )
                    ),
                )
            else:
                log.debug("  %s: not a component archive, ignored", entry.name)
        return

    if not bundle.exists():
        raise BundleError(f"{bundle} does not exist")
    if not zipfile.is_zipfile(bundle):
        raise BundleError(
            f"{bundle} is not an IronSide bundle -- that is a zip of "
            f"the component archives, or a directory holding "
            f"those archives"
        )
    with zipfile.ZipFile(bundle) as zf:
        for member in sorted(zf.namelist()):
            if member.endswith("/"):
                continue
            where = f"bundle member {member}"
            yield (
                member,
                (
                    lambda dest, m=member, w=where, **kw: _from_zip_bytes(
                        zf.read(m), w, dest=dest, **kw
                    )
                ),
            )


def read(bundle: Path, dest: Path, roles=ALL_ROLES) -> Bundle:
    """Read @bundle and take out the components filling @roles.

    Images extracted from a zip are written to <dest>/<component identifier>/
    image.hex, so what a tool programmed stays inspectable next to what it built
    from it; a component already unpacked on disk is used where it lies.

    Only @roles are taken. A component the caller has no use for is still
    RECOGNIZED -- so it is not reported as an unknown member, and a bundle
    carrying two of it is still an error -- but it is not extracted, and its SoC
    is not held against the others - a release carrying an IROT image says nothing
    about a command that installs no IROT.

    Members are read in whatever order the archive lists them; everything
    returned is ordered along the boot chain, so the summary line, the --image
    arguments and the signing order all read the way the components are talked
    about.
    """
    found, socs, seen = {}, {}, {}
    for name, reader in _members(bundle):
        ident = identify(name)
        if ident is None:
            log.debug("  %s: no known component identifier, ignored", Path(name).name)
            continue
        if ident in seen:
            raise BundleError(
                f"{bundle} has two {ident} components ({seen[ident]} "
                f"and {name}); a bundle carries one version of each"
            )
        seen[ident] = name

        role = COMPONENT_ROLES[ident]
        if role not in roles:
            log.debug("  %s: not used here, skipped", ident)
            continue
        if role in found:
            raise BundleError(
                f"{bundle} carries both {found[role].ident} and {ident}, "
                f"which are two candidates for the same {role} image -- "
                f"a bundle names one"
            )

        files, manifest, user_manifest = reader(dest, ident=ident)
        soc = manifest.get("soc")
        if soc is not None:
            socs.setdefault(soc, []).append(ident)
        # The precedence rule, in the one place every consumer reads a bundle
        # through: a customized image displaces the released one, and a signed
        # image is reported separately because it is not a thing to sign again.
        released = files[IMAGE_MEMBER]
        found[role] = Component(
            ident=ident,
            role=role,
            image=files.get(USER_IMAGE_MEMBER, released),
            version=manifest.get("version", "?"),
            soc=soc,
            package=package_basename(name),
            feature_level=manifest.get("feature_level"),
            released_image=released,
            signed=files.get(USER_SIGNED_MEMBER),
            signing_key=user_manifest.get("signing_key"),
            prov_routes={
                route: files[member]
                for route in (ROUTE_SINGLE_PHASE, ROUTE_TWO_PHASE)
                if (member := prov_route_member(role, route)) in files
            },
        )
        log.debug("  %s: %s -> %s", ident, role, found[role].image)

    if len(socs) > 1:
        raise BundleError(
            f"{bundle} components target different SoCs: "
            + "; ".join(f"{soc} ({', '.join(who)})" for soc, who in sorted(socs.items()))
        )
    return Bundle(
        path=bundle,
        components=found,
        soc=next(iter(socs), None),
        provisioning=provisioning_record(bundle),
    )
