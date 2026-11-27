# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Edit IronSide bundles atomically without changing released component archives.

Edits use user members and route each UICR word to the image that reads it.
"""

from __future__ import annotations

from .python_version import require_python

require_python()

import io
import json
import os
import subprocess
import sys
import tempfile
import zipfile
from contextlib import contextmanager
from dataclasses import dataclass, field
from pathlib import Path

from intelhex import IntelHex, IntelHexError

from . import app_keys as ak
from . import early_secure_debug_access as esda
from . import conf_img as ci
from . import ecdsa
from . import fw_bundle, img_conf_args, parts, uicr
from . import hybrid as hy
from . import mcuboot_image as mci
from . import prov_batches as pb
from . import psa_key_ram as pkr
from . import urot_storage_space as uss

SCRIPTS = Path(__file__).resolve().parent.parent

# Signable roles: every role whose image carries an MCUboot image id. Not
# IMAGE_ROLES, which answers "what an install programs" and omits the app
# (deployed through the customer's own flow) -- the app is still signable and
# is sign.py's own default role, so using IMAGE_ROLES here made
# sign-component --all silently skip it.
SIGNABLE_ROLES = tuple(
    role for role in fw_bundle.IMAGE_ROLES + ("app",) if role in mci.IMAGE_ID
)

# UROT_PART_START separates what IROT reads from what UROT reads within the
# IronSide UICR region.
REGION_START = uicr.IRONSIDE_UICR_BASE
REGION_END = uicr.IRONSIDE_UICR_BASE + uicr.REGION_SIZE
UROT_PART_START = uicr.IRONSIDE_UICR_BASE + uicr.UROT_OFF


class BundleEditError(Exception):
    """The edit cannot be made, or the bundle cannot be written back."""


class Usage(Exception):
    """A command line that does not say what to do.

    Raised by the bundle tool and by the configuration commands it delegates
    to, which is why it lives beside the edits both of them drive.
    """


@dataclass
class Package:
    """One component archive, held whole so a rewrite loses nothing.

    @files is every member, released and user alike, in the order the archive
    listed them; an edit must carry unrelated members (README.md, resources/)
    through untouched. @raw is the bytes it arrived as, so an unedited package
    is written back exactly as published rather than re-zipped.
    """

    ident: str
    name: str
    files: dict
    unpacked: bool = False
    raw: bytes | None = None
    dirty: bool = False
    # The member set this package arrived with, so a full reset can detect when
    # the members match the release again and write back @raw byte-identical.
    original: dict | None = None

    @property
    def role(self) -> str:
        return fw_bundle.COMPONENT_ROLES[self.ident]

    @property
    def manifest(self) -> dict:
        return self.json_member(fw_bundle.MANIFEST_MEMBER)

    @property
    def user_manifest(self) -> dict:
        return self.json_member(fw_bundle.USER_MANIFEST_MEMBER)

    def json_member(self, member) -> dict:
        """One JSON member of this package, or {} when it carries none."""
        blob = self.files.get(member)
        if blob is None:
            return {}
        try:
            return json.loads(blob)
        except json.JSONDecodeError as exc:
            raise BundleEditError(f"{self.name}: {member} is not valid JSON ({exc})") from exc

    @property
    def version(self) -> str:
        return self.manifest.get("version", "?")

    @property
    def feature_level(self) -> int | None:
        """The IronSide TE feature level of the package's MAJOR.MINOR version; None for an
        application package or a package released before feature levels existed."""
        return self.manifest.get("feature_level")

    @property
    def release_date(self) -> str | None:
        """The release date of the package, as YYYY-MM-DD; None for a package
        not on the release channel or released before release dates existed."""
        return self.manifest.get("release_date")

    @property
    def soc(self):
        return self.manifest.get("soc")

    @property
    def signing_key(self):
        return self.user_manifest.get("signing_key")

    @property
    def customized(self) -> bool:
        return any(m in self.files for m in fw_bundle.USER_MEMBERS)

    def get(self, member):
        return self.files.get(member)

    def set(self, member, blob) -> None:
        if self.files.get(member) == blob:
            return
        self.files[member] = blob
        self.dirty = True

    def drop(self, *members) -> bool:
        """Remove @members if present. True if anything was removed."""
        gone = [m for m in members if m in self.files]
        for member in gone:
            del self.files[member]
        self.dirty = self.dirty or bool(gone)
        return bool(gone)

    @property
    def released_image(self) -> bytes:
        return self.files[fw_bundle.IMAGE_MEMBER]

    @property
    def user_image(self):
        return self.files.get(fw_bundle.USER_IMAGE_MEMBER)

    @property
    def signed_image(self):
        return self.files.get(fw_bundle.USER_SIGNED_MEMBER)

    @property
    def unsigned(self) -> bytes:
        """The unsigned image an edit starts from: the customized one, else the
        released one. Never the signed one: mci.rebuild() refuses a signed image,
        since re-signing would discard the signature."""
        image = self.user_image
        return image if image is not None else self.released_image

    @property
    def effective(self) -> bytes:
        """What a consumer would program: the signature if there is one."""
        signed = self.signed_image
        return signed if signed is not None else self.unsigned


@dataclass
class Editable:
    """A bundle open for editing: its packages, and whatever else it holds."""

    path: Path
    is_directory: bool
    packages: list
    # Bundle-root files that are not component archives. A bundle has no members
    # of its own, so every one of these is also in @unrecognized -- kept only so
    # a save carries someone else's file through rather than deleting it.
    others: dict = field(default_factory=dict)
    unrecognized: list = field(default_factory=list)
    # Tracks bundle-level changes a package's own dirty flag cannot: adding,
    # removing, or replacing a component. Without it, such an edit would report
    # success and save nothing.
    structure_changed: bool = False

    def by_role(self, role):
        return next((p for p in self.packages if p.role == role), None)

    def by_name(self, name):
        """The package @name selects: a role, a component identifier, or the
        identifier with its `ironside-te-` prefix left off."""
        for pkg in self.packages:
            if name in (pkg.role, pkg.ident, pkg.ident.removeprefix("ironside-te-")):
                return pkg
        return None

    def require(self, role) -> Package:
        pkg = self.by_role(role)
        if pkg is None:
            raise BundleEditError(
                f"{self.path} carries no {fw_bundle.identifiers([role])} component, "
                f"which is the one that holds this"
            )
        return pkg

    @property
    def soc(self):
        socs = {p.soc for p in self.packages if p.soc}
        return next(iter(socs)) if len(socs) == 1 else None

    def require_soc(self) -> str:
        """The bundle's SoC, or BundleEditError when its components do not name one."""
        if self.soc is None:
            raise BundleEditError(
                f"{self.path} does not name a single SoC: its components name none, or "
                "several. Rebuild it with ironside_bundle.py init."
            )
        return self.soc

    @property
    def dirty(self) -> bool:
        return any(p.dirty for p in self.packages)


def _members_of_zip(blob, where) -> dict:
    """Every member of a component archive, directory entries included.

    Kept because dropping them would change what the package holds - an empty
    directory such as resources/ is itself a member.
    """
    buf = io.BytesIO(blob)
    if not zipfile.is_zipfile(buf):
        raise BundleEditError(
            f"{where} is not a zip archive -- a bundle's members are the component archives"
        )
    with zipfile.ZipFile(buf) as zf:
        return {
            info.filename: (b"" if info.is_dir() else zf.read(info.filename))
            for info in zf.infolist()
        }


def _package(ident, name, members, where, *, unpacked=False, raw=None) -> Package:
    if fw_bundle.IMAGE_MEMBER not in members:
        raise BundleEditError(
            f"{where} has no {fw_bundle.IMAGE_MEMBER} -- is it an IronSide TE component archive?"
        )
    return Package(
        ident=ident, name=name, files=members, unpacked=unpacked, raw=raw, original=dict(members)
    )


def read(path: Path) -> Editable:
    """Open @path for editing. Accepts both shapes fw_bundle.read() accepts."""
    path = Path(path)
    if path.is_dir():
        return _read_dir(path)
    if not path.exists():
        raise BundleEditError(f"{path} does not exist")
    if not zipfile.is_zipfile(path):
        raise BundleEditError(
            f"{path} is not an IronSide bundle -- that is a zip of "
            f"the component archives, or a directory "
            f"holding those archives"
        )
    packages, others, unrecognized = [], {}, []
    with zipfile.ZipFile(path) as zf:
        for name in sorted(zf.namelist()):
            if name.endswith("/"):
                continue
            blob = zf.read(name)
            ident = fw_bundle.identify(name)
            if ident is None:
                # Carried through rather than dropped -- a bundle is a zip of
                # component archives, so this is someone else's file and not
                # ours to delete -- but reported, because it is not a member.
                others[name] = blob
                unrecognized.append(name)
                continue
            packages.append(
                _package(
                    ident,
                    # A bundle's members are flat and a package name is written
                    # back verbatim, so a member grouped under a directory (as
                    # the CI artifact tree groups the component archives per SoC) is
                    # taken by its own name rather than saved a level deep.
                    Path(name).name,
                    _members_of_zip(blob, f"bundle member {name}"),
                    f"bundle member {name}",
                    raw=blob,
                )
            )
    return _checked(
        Editable(
            path=path,
            is_directory=False,
            packages=packages,
            others=others,
            unrecognized=unrecognized,
        )
    )


def _read_dir(path: Path) -> Editable:
    """A directory of component archives -- zipped, unpacked, or both.

    A directory and a same-stemmed zip are the same package; when both exist the
    unpacked one is used, so an edit lands where a caller can see it.
    """
    packages, others, unrecognized = [], {}, []
    entries = sorted(path.iterdir())
    unpacked = {p.name for p in entries if p.is_dir()}
    for entry in entries:
        if entry.is_file() and entry.with_suffix("").name in unpacked:
            continue
        ident = fw_bundle.identify(entry.name)
        if ident is None:
            # Held in memory like a zip bundle's member, so --out carries it.
            if entry.is_file():
                others[entry.name] = entry.read_bytes()
            unrecognized.append(entry.name)
            continue
        if entry.is_dir():
            members = {
                f.relative_to(entry).as_posix(): f.read_bytes()
                for f in sorted(entry.rglob("*"))
                if f.is_file()
            }
            packages.append(_package(ident, entry.name, members, str(entry), unpacked=True))
        elif entry.suffix == ".zip":
            blob = entry.read_bytes()
            packages.append(
                _package(ident, entry.name, _members_of_zip(blob, str(entry)), str(entry), raw=blob)
            )
        else:
            unrecognized.append(entry.name)
    return _checked(
        Editable(
            path=path,
            is_directory=True,
            packages=packages,
            others=others,
            unrecognized=unrecognized,
        )
    )


def _checked(bundle: Editable) -> Editable:
    """What makes an archive a bundle, checked on the way in: one package per
    component, and one candidate per role (both DFU flavours fill the one dfu
    role). Enforced here too, or an edit starting from an inconsistent bundle
    would write one back.
    """
    seen, roles = {}, {}
    for pkg in bundle.packages:
        if pkg.ident in seen:
            raise BundleEditError(
                f"{bundle.path} has two {pkg.ident} components "
                f"({seen[pkg.ident]} and {pkg.name}); a bundle "
                f"carries one version of each"
            )
        seen[pkg.ident] = pkg.name
        if pkg.role in roles:
            raise BundleEditError(
                f"{bundle.path} carries both {roles[pkg.role]} and {pkg.ident}, "
                f"which are two candidates for the same {pkg.role} image -- a "
                f"bundle names one"
            )
        roles[pkg.role] = pkg.ident
    return bundle


def _zip_package(pkg: Package) -> bytes:
    """A package's members as one zip, member order and names preserved,
    deflated, and stamped with the format's fixed timestamp so the same members
    give the same bytes. Only reached for an edited package; an untouched one is
    written back from @raw.
    """
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED) as zf:
        for name, blob in pkg.files.items():
            info = zipfile.ZipInfo(name, date_time=fw_bundle.FIXED_TIMESTAMP)
            info.external_attr = 0o644 << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            zf.writestr(info, blob)
    return buf.getvalue()


def _package_bytes(pkg: Package) -> bytes:
    """The package's bytes: the ones it arrived with wherever that still holds,
    so an edit undone before saving reproduces the input byte-identically.

    Across separate runs the guarantee is narrower - re-zipping cannot reproduce
    the publisher's original deflate, so a package rewritten once and reset later
    comes back with every released member byte-identical inside a differently
    framed zip. Provenance rests on the members, not the framing.
    """
    if pkg.raw is not None and pkg.files == pkg.original:
        return pkg.raw
    return _zip_package(pkg)


def secret_members(bundle: Editable):
    """[(package, member)] for every member of @bundle holding raw OTA KEKs.

    Only the SECURE provisioning firmware's route members: those carry a batch
    covering "install", and an install batch imports the OTA KEKs in the clear.
    The non-secure route member runs `approtect` alone, which takes no key
    material, and the record itself is a handful of decisions and secret-free.
    """
    pkg = bundle.by_role(fw_bundle.PROV_SECURE)
    if pkg is None:
        return []
    return [
        (pkg, member)
        for member in _prov_members(fw_bundle.PROV_SECURE)
        if pkg.get(member) is not None
    ]


def carries_kek_material(bundle: Editable) -> bool:
    """Whether @bundle holds raw OTA KEKs anywhere in it."""
    return bool(secret_members(bundle))


def _restrict_if_secret(bundle: Editable, path: Path) -> None:
    """Restrict @path when the bundle carries key material.

    A prepared bundle's provisioning firmware imports the OTA KEKs in the clear,
    so it is a secret and must not be world-readable.
    """
    if carries_kek_material(bundle):
        path.chmod(0o600)


def save(bundle: Editable) -> list:
    """Write @bundle back. Returns the package names that were rewritten.

    A zip bundle is rebuilt whole and moved into place with one os.replace(), so
    an interrupted save cannot leave a half-written bundle. A directory bundle
    has no single file to swap, so it is edited in place, package by package.
    """
    # What changed, not what was touched: an edit undone before saving leaves the
    # package as published, so it must not be reported as rewritten.
    rewritten = [
        pkg.name
        for pkg in bundle.packages
        if pkg.dirty and (pkg.raw is None or _package_bytes(pkg) != pkg.raw)
    ]
    if not rewritten and not bundle.structure_changed:
        return []
    if bundle.is_directory:
        secret = {(pkg.ident, member) for pkg, member in secret_members(bundle)}
        for pkg in bundle.packages:
            if not pkg.dirty:
                continue
            if pkg.unpacked:
                root = bundle.path / pkg.name
                for name, blob in pkg.files.items():
                    dest = root / name
                    dest.parent.mkdir(parents=True, exist_ok=True)
                    dest.write_bytes(blob)
                    # An unpacked route hex is a file of its own, so the
                    # restriction has to land on it rather than on one archive.
                    if (pkg.ident, name) in secret:
                        dest.chmod(0o600)
                for existing in sorted(root.rglob("*")):
                    if (
                        existing.is_file()
                        and existing.relative_to(root).as_posix() not in pkg.files
                    ):
                        existing.unlink()
            else:
                dest = bundle.path / pkg.name
                dest.write_bytes(_zip_package(pkg))
                if any(ident == pkg.ident for ident, _member in secret):
                    dest.chmod(0o600)
        return rewritten

    tmp = bundle.path.with_name(bundle.path.name + ".tmp")
    try:
        with zipfile.ZipFile(tmp, "w", zipfile.ZIP_STORED) as zf:
            entries = {pkg.name: _package_bytes(pkg) for pkg in bundle.packages}
            entries.update(bundle.others)
            for name in sorted(entries):
                info = zipfile.ZipInfo(name, date_time=fw_bundle.FIXED_TIMESTAMP)
                info.external_attr = 0o644 << 16
                zf.writestr(info, entries[name])
        os.replace(tmp, bundle.path)
        _restrict_if_secret(bundle, bundle.path)
    finally:
        tmp.unlink(missing_ok=True)
    return rewritten


def write_as(bundle: Editable, out: Path) -> list:
    """Save @bundle to @out instead of where it was read from.

    Every package is written, dirty or not, since @out is a new file with
    nothing to keep. A directory source written to a `.zip` becomes a bundle zip.
    """
    out = Path(out)
    out.parent.mkdir(parents=True, exist_ok=True)
    if out.suffix != ".zip" or out.is_dir():
        raise BundleEditError(f"{out}: --out writes a bundle zip, so it needs a .zip name")
    with zipfile.ZipFile(out, "w", zipfile.ZIP_STORED) as zf:
        entries = {
            (pkg.name if pkg.name.endswith(".zip") else f"{pkg.name}.zip"): _package_bytes(pkg)
            for pkg in bundle.packages
        }
        entries.update(bundle.others)
        for name in sorted(entries):
            info = zipfile.ZipInfo(name, date_time=fw_bundle.FIXED_TIMESTAMP)
            info.external_attr = 0o644 << 16
            zf.writestr(info, entries[name])
    _restrict_if_secret(bundle, out)
    return sorted(entries)


@contextmanager
def _scratch():
    with tempfile.TemporaryDirectory(prefix="ironside-bundle-") as tmp:
        yield Path(tmp)


@contextmanager
def as_file(blob, name="image.hex"):
    """@blob as a file, for readers that take paths because a released image.hex
    is a file everywhere else. A bundle's images are members, so they are staged
    for the length of one call.
    """
    with _scratch() as tmp:
        path = tmp / name
        path.write_bytes(blob)
        yield path


def image_parts(blob):
    """(firmware bytes, base address, aux IntelHex) of one image hex.

    The staging path is stripped from any failure - it names a temporary directory
    this module made up, not the member that is actually malformed. The caller
    names that.
    """
    with as_file(blob) as path:
        try:
            return mci.read_image(path) + (mci.read_aux(path),)
        except mci.SignError as exc:
            raise mci.SignError(str(exc).replace(f"{path}: ", "", 1)) from exc


def _load_hex(path: Path) -> IntelHex:
    """@path as an IntelHex, with a malformed member reported as a
    BundleEditError rather than an intelhex exception, so `check` can report it
    instead of crashing.
    """
    loaded = IntelHex()
    try:
        loaded.loadhex(str(path))
    except (IntelHexError, ValueError) as exc:
        raise BundleEditError(f"not a readable Intel HEX ({exc})") from exc
    return loaded


def splice_batch(image: bytes, blob: bytes, addr: int) -> bytes:
    """@image, an Intel HEX member, with @blob placed at @addr: the firmware and
    the batch it reads at the command address become one hex, so nothing
    downstream has to synthesise the batch half.
    """
    with _scratch() as tmp:
        src = tmp / "image.hex"
        src.write_bytes(image)
        merged = _load_hex(src)
        if addr in set(merged.addresses()):
            raise BundleEditError(
                f"the provisioning firmware already carries data at 0x{addr:08X}, "
                f"where its batch goes"
            )
        for offset, byte in enumerate(blob):
            merged[addr + offset] = byte
        out = tmp / "out.hex"
        merged.write_hex_file(str(out))
        return out.read_bytes()


def batch_at(image: bytes, addr: int):
    """The contiguous blob @image carries at @addr, or None.

    The inverse of splice_batch(), for the checks that hold a carried member
    against the batch it should be.
    """
    with _scratch() as tmp:
        src = tmp / "image.hex"
        src.write_bytes(image)
        loaded = _load_hex(src)
    for start, end in loaded.segments():
        if start == addr:
            return loaded.tobinstr(start=start, end=end - 1)
    return None


def _assemble(data, base, aux) -> bytes:
    with _scratch() as tmp:
        out = tmp / "out.hex"
        mci.write_image(out, data, base, aux=aux)
        return out.read_bytes()


def _apply_writes(aux, writes) -> None:
    for addr, payload in writes:
        for offset, byte in enumerate(bytes(payload)):
            aux[addr + offset] = byte


def image_config(blob):
    """The image-configuration TLV of an image, or None if it carries none."""
    with as_file(blob) as path:
        return mci.find_protected_tlv(path, mci.TLV_IRONSIDE_CONF)


def region_carriers(bundle: Editable) -> list:
    """The packages whose images can carry UICR words: the boot-chain images.

    The provisioning firmware is excluded - it links into RAM at 0x2xxxxxxx, above
    the UICR base the hex splitter divides firmware from aux on, so reading it as
    an image hex reports no firmware at all.
    """
    return [pkg for pkg in bundle.packages if pkg.role in fw_bundle.IMAGE_ROLES]


def uicr_carriers(bundle: Editable) -> list:
    """The packages whose images can carry any UICR word: the boot-chain images,
    and the App, which carries UICR.OTP.
    """
    app = bundle.by_role("app")
    return [*region_carriers(bundle), *([app] if app else [])]


def aux_lookup(bundle: Editable):
    """A uicr.decode()-style byte lookup over the images that carry region words:
    the region is split across the packages, so only their union is a region to
    decode.

    Reads the effective image deliberately - it is what a consumer programs, so a
    stale signed image shows up as a stale decode rather than being papered over.
    """
    merged = {}
    for pkg in region_carriers(bundle):
        _, _, aux = image_parts(pkg.effective)
        for addr in aux.addresses():
            merged[addr] = aux[addr]

    def get(addr):
        return merged.get(addr)

    return get


def region_report(bundle: Editable) -> dict:
    """What the bundle's UICR words decode to, through the shared decoder."""
    return uicr.decode(aux_lookup(bundle))


# The components whose released image carries a base-config slice, and whose
# absence therefore makes a slice check meaningless rather than failed.
SLICE_CARRIERS = ("irot", "urot")


def region_layout_problems(region: dict, *, soc, carriers=SLICE_CARRIERS) -> list:
    """Why a decoded base-config region does not fit the layout this tool knows.

    Each RoT image carries its own slice of the region at offsets fixed when that
    image was built, so components from opposite sides of a layout change can
    merge into a region where words land in adjacent fields. FORMATVERSION cannot
    detect every such change - a misplaced PARTITIONS value can reach the
    write-once, terminal DECOMMISSIONED field.

    @carriers is which slices to expect, so an artifact carrying no images (the
    untrusted phase, both protect flows) is not failed for holding none. @soc is
    the part the partition layout is placed for; None reports that it cannot be.
    """
    problems = []
    if region.get("decommissioned"):
        problems.append(
            f"the base configuration writes DECOMMISSIONED "
            f"(0x{uicr.IRONSIDE_UICR_BASE + uicr.DECOMMISSIONED_OFF:08X}), which is "
            f"terminal and write-once. No installation provisions that word, so "
            f"these components carry their base-config slice at different offsets "
            f"than these scripts read: the components and the tooling are from "
            f"either side of a UICR layout change. Use components built from the "
            f"same release as this checkout."
        )
    for role, reported, off, want in (
        ("irot", "irot_formatversion", uicr.IROT_FORMATVERSION_OFF, uicr.IROT_FORMATVERSION),
        ("urot", "urot_formatversion", uicr.UROT_FORMATVERSION_OFF, uicr.UROT_FORMATVERSION),
    ):
        if role not in carriers:
            continue
        word = region.get(reported)
        addr = uicr.IRONSIDE_UICR_BASE + off
        if word is None or word == uicr.ERASED:
            problems.append(
                f"nothing carries the .{role.upper()} FORMATVERSION word at "
                f"0x{addr:08X}, which every accessor of that sub-region is "
                f"gated on. The {role} component was built against a different "
                f"UICR layout than these scripts."
            )
        elif word != want:
            problems.append(
                f".{role.upper()} FORMATVERSION at 0x{addr:08X} is "
                f"0x{word:08X}, and these scripts read 0x{want:08X}"
            )
    if "irot" in carriers:
        if region.get("partitions_kb") is None:
            problems.append(
                f"the base configuration carries no readable partition layout at "
                f"0x{uicr.IRONSIDE_UICR_BASE + uicr.PARTITIONS_OFF:08X}. IROT "
                f"refuses to boot without one."
            )
        elif soc is None:
            problems.append(
                "the components name no single SoC, so the partition layout cannot be "
                "placed. Rebuild the bundle with ironside_bundle.py init."
            )
        else:
            sizes = region["partitions_kb"]
            try:
                uicr.partition_addresses([sizes[n] for n in uicr.PARTITION_NAMES], soc=soc)
            except uicr.UicrError as exc:
                problems.append(f"the partition layout does not place: {exc}")
    return problems


def base_conf_problems(bundle: Editable) -> list:
    """region_layout_problems() over what @bundle's components carry."""
    return region_layout_problems(
        region_report(bundle)["region"],
        soc=bundle.soc,
        carriers=tuple(r for r in SLICE_CARRIERS if bundle.by_role(r) is not None),
    )


def partitions_kb(bundle: Editable):
    """The partition layout the bundle carries, or None if it carries none."""
    return region_report(bundle)["region"].get("partitions_kb")


def effective_partitions_kb(bundle: Editable) -> dict:
    """The partition layout a device provisioned from @bundle has.

    The reference layout where the bundle carries none.
    """
    sizes = partitions_kb(bundle)
    if sizes is None:
        sizes = dict(zip(uicr.PARTITION_NAMES, uicr.REFERENCE_PARTITIONS_KB, strict=True))
    return sizes


def application_key_space(bundle: Editable, rec: dict) -> uss.Usage:
    """The space the application keys provisioning record @rec installs take and have."""
    return uss.application_key_space(
        effective_partitions_kb(bundle)["urotstorage"] * 1024,
        pb.app_keys(rec),
        ota_kek=pb.has_ota_kek(rec),
    )


# The partition each signable role's image is programmed into. UROT's slot is
# not in the editable layout, so sign.py's per-role default stands for it.
SLOT_PARTITION = {"dfu": "dfuimage", "app": "slot0"}


def slot_size(bundle: Editable, role: str):
    """The size in bytes of the partition @role's image is signed against.

    Taken from the bundle's own layout, not a constant - after shrinking a
    partition, signing against the old reference size would let an over-slot
    image through the one guard meant to catch it. None where the layout does
    not decide it, leaving sign.py's per-role default in charge.
    """
    partition = SLOT_PARTITION.get(role)
    if partition is None:
        return None
    sizes = partitions_kb(bundle)
    if sizes is None:
        return None
    try:
        placed = uicr.partition_addresses(
            [sizes[name] for name in uicr.PARTITION_NAMES], soc=bundle.require_soc()
        )
    except uicr.UicrError:
        return None
    return placed[partition][1] or None


def dfu_slot_size(bundle: Editable):
    """The DFUIMAGE partition's size in bytes, or None if unresolvable."""
    return slot_size(bundle, "dfu")


def dfu_placement_problems(bundle: Editable) -> list:
    """Why @bundle's DFU image would not start on the partition layout @bundle provisions.

    UROT boots the DFU image from the DFUIMAGE base, and the DFUIMAGE size sets that
    base. The image's hex start address is the address it is linked for. Empty when
    the bundle carries no DFU image, no layout, or a disabled DFUIMAGE.
    """
    pkg = bundle.by_role("dfu")
    sizes = partitions_kb(bundle)
    if pkg is None or sizes is None or sizes["dfuimage"] == 0:
        return []
    soc = bundle.require_soc()
    try:
        placed = uicr.partition_addresses([sizes[name] for name in uicr.PARTITION_NAMES], soc=soc)
        rram_end = uicr.RRAM_SIZE_KB[uicr.resolve_soc(soc)] * 1024
        _, image_base, _ = image_parts(pkg.effective)
    except (uicr.UicrError, mci.SignError):
        # Reported by the layout and image checks.
        return []
    dfuimage_base = placed["dfuimage"][0]
    if image_base == dfuimage_base:
        return []
    problem = (
        f"{pkg.ident} starts at 0x{image_base:08X}, but DFUIMAGE {sizes['dfuimage']} kB "
        f"starts at 0x{dfuimage_base:08X}. UROT boots the DFU image from the DFUIMAGE base."
    )
    linked_kb, remainder = divmod(rram_end - image_base, 1024)
    if remainder == 0 and linked_kb > 0 and linked_kb % uicr.PARTITION_BLOCK_KB == 0:
        problem += f" Set --dfuimage-size-kb {linked_kb}, or add"
    else:
        problem += " Add"
    return [problem + f" a DFU image linked at 0x{dfuimage_base:08X}."]


def _ram_split_granule(bundle: Editable) -> int:
    """The bundle's part's RAM split granule, or the finest of any part's."""
    return parts.by_soc(bundle.soc).ram_split_granule if bundle.soc else pkr.RAM_SPLIT_GRANULE_MIN


def key_slot_storage_size(bundle: Editable) -> int:
    """The PSA key slot storage size a device provisioned from @bundle has, in
    bytes: the base configuration's, or the base slots' when the bundle carries
    no usable one."""
    size = region_report(bundle)["region"].get("key_slot_storage_size")
    return pkr.applied_size(size, _ram_split_granule(bundle))


def set_key_slot_storage_size(bundle: Editable, size: int) -> int:
    """Write a key slot storage size of @size bytes, checked against the bundle's
    part, into the component that reads it (UROT); the base slots' size leaves
    the word erased. Returns the size."""
    granule = parts.by_soc(bundle.require_soc()).ram_split_granule
    try:
        pkr.check_key_slot_storage_size(size, granule)
    except pkr.PsaKeyRamError as exc:
        raise BundleEditError(f"{bundle.soc}: {exc}") from None
    word = uicr.IRONSIDE_UICR_BASE + uicr.KEYSLOTSTORAGESIZE_OFF
    writes = (
        [] if size == pkr.KEY_SLOT_STORAGE_SIZE_MIN else uicr.key_slot_storage_size_writes(size)
    )

    def mutate(aux, _released):
        held = set(aux.addresses())
        for addr in range(word, word + 4):
            if addr in held:
                del aux[addr]
        _apply_writes(aux, writes)

    _edit_aux(bundle.require("urot"), mutate)
    return size


def otp_words(pkg: Package) -> dict[int, int]:
    """{index: word} for every whole UICR.OTP word @pkg's effective image programs."""
    _, _, aux = image_parts(pkg.effective)
    return uicr.otp_words(_byte_lookup(aux))


def otp_problems(bundle: Editable, skip=()) -> list:
    """Why the UICR.OTP data the bundle carries cannot be programmed.

    The App component carries UICR.OTP, because the application is its only
    reader, and it carries no other UICR word. Packages named in @skip are not
    read.
    """
    problems = []
    for pkg in uicr_carriers(bundle):
        if pkg.ident in skip:
            continue
        _, _, aux = image_parts(pkg.effective)
        held = set(aux.addresses())
        otp = {a for a in held if uicr.OTP_BASE <= a < uicr.OTP_BASE + uicr.OTP_SIZE}
        if pkg.role != "app":
            if otp:
                problems.append(
                    f"{pkg.ident} writes UICR.OTP at 0x{min(otp):08X}, and only the App "
                    f"component carries OTP data"
                )
            continue
        other = held - otp
        if other:
            problems.append(
                f"{pkg.ident} writes UICR at 0x{min(other):08X}, outside UICR.OTP "
                f"(0x{uicr.OTP_BASE:08X}..0x{uicr.OTP_BASE + uicr.OTP_SIZE - 1:08X}), "
                f"the only UICR words an App component carries"
            )
        partial = sorted({a & ~3 for a in otp if not all((a & ~3) + i in otp for i in range(4))})
        if partial:
            problems.append(
                f"{pkg.ident} writes only part of the UICR.OTP word at 0x{partial[0]:08X}. "
                f"Give all 4 bytes of each OTP word in the application HEX"
            )
    return problems


def ikg_seed_select(bundle: Editable) -> str | None:
    """The IKG SEED entry @bundle's base configuration selects, "nordic" or
    "user", or None when the provisioning firmware selects it."""
    entry = region_report(bundle)["region"].get("ikg_seed_select")
    return entry if entry in uicr.IKG_SEED_SELECTIONS else None


def set_ikg_seed_select(bundle: Editable, entry: str) -> bool:
    """Write the IKG SEED selection @entry into the component that reads it
    (UROT), or remove it for auto. Returns whether the selection changed."""
    before = ikg_seed_select(bundle)
    auto = entry == uicr.IKG_SEED_SELECT_AUTO
    writes = [] if auto else uicr.ikg_seed_select_writes(entry)
    word = uicr.IRONSIDE_UICR_BASE + uicr.IKGSEED_SELECT_OFF

    def mutate(aux, _released):
        held = set(aux.addresses())
        for addr in range(word, word + 4):
            if addr in held:
                del aux[addr]
        _apply_writes(aux, writes)

    _edit_aux(bundle.require("urot"), mutate)
    return before != (None if auto else entry)


def ikg_seed_select_problems(selected: str | None, seed: str) -> list:
    """Why the IKG SEED selection @selected cannot boot a device provisioned
    with the SEED mode @seed."""
    problem = pb.selection_problem(selected, seed)
    if problem is None:
        return []
    permitted = "|".join(pb.SEED_SELECTIONS[seed])
    return [
        f"{problem}. Run ironside_bundle.py update-config --ikg-seed-select {permitted}, "
        "or set --provision-user-seed no."
    ]


def _edit_aux(pkg: Package, mutate) -> None:
    """Apply @mutate(aux, released_aux) to every image the package carries,
    signed image included. UICR words sit outside the signed region, so a
    signature stays valid across this edit; skipping the signed image would
    leave a consumer programming the release's stale words instead.
    """
    _, _, released_aux = image_parts(pkg.released_image)
    for member, blob in (
        (fw_bundle.USER_IMAGE_MEMBER, pkg.unsigned),
        (fw_bundle.USER_SIGNED_MEMBER, pkg.signed_image),
    ):
        if blob is None:
            continue
        data, base, aux = image_parts(blob)
        mutate(aux, released_aux)
        pkg.set(member, _assemble(data, base, aux))
    _drop_if_pristine(pkg)


def set_partitions(bundle: Editable, sizes_kb) -> Package:
    """Write a partition layout into the component that reads it (IROT)."""
    writes = uicr.partition_writes(
        [sizes_kb[name] for name in uicr.PARTITION_NAMES], soc=bundle.require_soc()
    )
    pkg = bundle.require("irot")
    _edit_aux(pkg, lambda aux, _released: _apply_writes(aux, writes))
    return pkg


def set_early_secure_debug_access(bundle: Editable, mode: str) -> Package:
    """Write the early secure debug access USERCNF entries for @mode into IROT.

    The IROT USERCNF array is first restored to the released one, so a new mode
    replaces an earlier one and mode off leaves the release's array.
    """
    pkg = bundle.require("irot")
    new_entries = esda.entries(pkg.soc, mode)

    def mutate(aux, released_aux):
        _restore_usercnf(aux, released_aux)
        if new_entries:
            slots = uicr.usercnf_slots(_byte_lookup(aux))
            _apply_writes(aux, uicr.usercnf_append_writes(slots, new_entries))

    _edit_aux(pkg, mutate)
    return pkg


def early_secure_debug_access_mode(pkg: Package) -> str:
    """esda.classify() over the USERCNF entries the IROT package programs."""
    _, _, aux = image_parts(pkg.effective)
    return esda.classify(uicr.usercnf_slots(_byte_lookup(aux)), pkg.soc)


def _restore_usercnf(aux, released_aux) -> None:
    usercnf_end = uicr.USERCNF_BASE + uicr.USERCNF_COUNT * uicr.USERCNF_STRIDE
    released, edited = set(released_aux.addresses()), set(aux.addresses())
    for addr in range(uicr.USERCNF_BASE, usercnf_end):
        if addr in released:
            aux[addr] = released_aux[addr]
        elif addr in edited:
            del aux[addr]


def _byte_lookup(ih: IntelHex):
    """A uicr.decode()-style lookup that reports bytes @ih does not hold as None."""
    present = set(ih.addresses())
    return lambda addr: ih[addr] if addr in present else None


def set_image_config(pkg: Package, blob: bytes) -> None:
    """Replace the image configuration inside the package's unsigned image.

    Rebuilds rather than appends, since every value here lives inside the signed
    region - the result is byte-identical to an image built with this
    configuration from the start. Any signature the package carries is left in
    place and becomes stale; the caller decides whether to re-sign, since that
    needs a key source it may not have.
    """
    data, base, aux = image_parts(pkg.unsigned)
    pkg.set(fw_bundle.USER_IMAGE_MEMBER, _assemble(mci.rebuild(data, conf_img=blob), base, aux))
    _drop_if_pristine(pkg)


def edit_image_config(pkg: Package, args) -> bytes:
    """Apply the image-configuration flags in @args to the package. Returns the
    new blob.

    Field granularity: the current configuration is decoded, the named fields are
    replaced, and the rest are kept. A component with no configuration starts
    from an empty one.
    """
    blob = img_conf_args.apply(args, current=image_config(pkg.unsigned))
    set_image_config(pkg, blob)
    return blob


def _drop_if_pristine(pkg: Package) -> None:
    """Delete a user image that has become the released one again, so reverting
    the last edit leaves a package that no longer claims to be customized.
    """
    if pkg.user_image == pkg.released_image and pkg.signed_image is None:
        pkg.drop(fw_bundle.USER_IMAGE_MEMBER, fw_bundle.USER_MANIFEST_MEMBER)


def _run(argv, what):
    """Run one of the sibling tools and return its stdout.

    A subprocess, not an import: the tool owns its own checks and reporting, so
    there stays one implementation of "generate this region" or "sign this
    image", not a second copy here.
    """
    proc = subprocess.run([sys.executable, *[str(a) for a in argv]], capture_output=True, text=True)
    if proc.returncode != 0:
        detail = (proc.stderr or proc.stdout or "").strip()
        raise BundleEditError(f"{what} failed:\n{detail}")
    # A successful tool writes only warnings to stderr, and the user must see them.
    sys.stderr.write(proc.stderr)
    return proc.stdout


def split_region_writes(get, present) -> dict:
    """Route region words to the package that reads them: {role: [(addr, byte)]}.

    The struct's own boundary is also the reader boundary - below .UROT is what
    IROT reads (format version, UROT verification key, partition layout); at or
    above it is what UROT reads.
    """
    routed = {"irot": [], "urot": []}
    for addr in sorted(present):
        if not REGION_START <= addr < REGION_END:
            raise BundleEditError(
                f"0x{addr:08X} is outside the IronSide UICR region "
                f"(0x{REGION_START:08X}..0x{REGION_END - 1:08X}); a bundle carries "
                f"the region and nothing else"
            )
        routed["urot" if addr >= UROT_PART_START else "irot"].append((addr, bytes([get(addr)])))
    return routed


def authop_generations_problem(keys: dict) -> str | None:
    """Why the AUTHOP keys in a decoded key report are not an installable set.

    A device holds every AUTHOP generation or none, so generation 2 is the last
    generation on every device. With fewer, a setting that revokes the newest
    generation the device happens to hold would leave it with no AUTHOP key, and
    signing warns only about a setting that revokes generation 2.
    """
    gens = keys.get("authopkey", {})
    valid = [g for g, state in gens.items() if state["provisioned"] and not state["revoked"]]
    if len(valid) in (0, uicr.GENERATIONS):
        return None
    held = ", ".join(f"gen{g}" for g in sorted(valid))
    return (
        f"provides AUTHOP {held} only. Provide all {uicr.GENERATIONS} AUTHOP generations, "
        "or none to leave ADAC authentication off for good."
    )


def set_keys(bundle: Editable, keyset) -> dict:
    """Provision the keyset @keyset holds into the bundle. Returns the manifest
    the base config generator reported, naming what was provisioned and what was
    revoked by omission.

    The base config generator derives and revokes; this only routes each word to
    its package. A plain --keyset run writes the keys and the auth-lock marker
    only, not the format versions or partition layout, which the images already
    carry.
    """
    for role in ("irot", "urot"):
        bundle.require(role)
    with _scratch() as tmp:
        out = tmp / "base_config.hex"
        # raw_decode, not loads: --manifest-json prints the manifest and then says
        # where it wrote the hex, so stdout is JSON followed by a line of prose.
        # Reading the object off the front leaves that report alone.
        manifest, _ = json.JSONDecoder().raw_decode(
            _run(
                [
                    SCRIPTS / "internal" / "gen_base_conf.py",
                    "--keyset",
                    keyset,
                    "--manifest-json",
                    "-o",
                    out,
                ],
                "base config generator",
            )
        )
        get = uicr.file_reader(out)
        present = IntelHex(str(out)).addresses()
    problem = authop_generations_problem(uicr.decode(get)["region"].get("keys", {}))
    if problem:
        raise BundleEditError(f"{keyset}: {problem}")
    routed = split_region_writes(get, present)
    for role, writes in routed.items():
        if not writes:
            continue
        pkg = bundle.require(role)
        _edit_aux(pkg, lambda aux, _released, w=writes: _apply_writes(aux, w))
    return manifest


def provisioned_keys(bundle: Editable) -> dict:
    """{role: {gen: state}} for the keys the bundle provisions."""
    return region_report(bundle)["region"].get("keys", {})


def generation_state_label(state: dict) -> str:
    """provisioned, revoked or unset for one key generation's decoded state."""
    if state["revoked"]:
        return "revoked"
    if state["provisioned"]:
        return "provisioned"
    return "unset"


def ota_kek_material(keyset) -> list:
    """The three OTA KEK generations a key source holds.

    All three or none: the KEK slots are write-once and sit in the range the
    application owns, so a generation left unfilled at install can never be
    filled afterwards.
    """
    sys.path.insert(0, str(SCRIPTS))
    import iron_keychain as kc

    try:
        # None resolves to the sole keyset in the keyring, as everywhere else a
        # --keyset is optional (sign(), sign.py, adac.py).
        source = kc.KeySource.parse(keyset or kc.sole_keyset_spec())
    except kc.KeySourceError as exc:
        raise BundleEditError(str(exc)) from exc
    keks = []
    for gen in range(pb.abi.KEK_GENERATIONS):
        key_id = kc.kek_key_id(gen)
        data = source.get_bytes(key_id)
        if data is None:
            raise BundleEditError(
                f"{keyset} holds no {key_id}. All {pb.abi.KEK_GENERATIONS} OTA KEK "
                f"generations are required. Fill the keyset with "
                f"iron_keychain.py new."
            )
        keks.append(data)
    return keks


def app_key_material(keyset) -> tuple:
    """The application's own persistent keys a key source holds, ordered by key ID.

    From the key source the verification keys and the OTA KEKs come from, so a
    device cannot be provisioned with application keys belonging to one
    deployment and verification keys belonging to another. They are optional - a
    keyset holding none yields a bundle importing none.
    """
    sys.path.insert(0, str(SCRIPTS))
    import iron_keychain as kc

    try:
        source = kc.KeySource.parse(keyset or kc.sole_keyset_spec())
        return ak.parse_entries(source.app_key_entries(), source.describe())
    except (ak.AppKeyError, kc.KeySourceError) as exc:
        raise BundleEditError(str(exc)) from exc


def _read_counter(pkg: Package) -> int:
    data, _base, _aux = image_parts(pkg.effective)
    region, _signature = mci.split_signed(data)
    return int.from_bytes(mci.read_unsigned(region)[3][mci.TLV_SEC_CNT], "little")


def package_counter(pkg: Package):
    """The protected-TLV counter @pkg's image presents, or None if it has none.

    The value the device compares (`rot_counter_check()`) and the only source
    for a rollback floor. The header version is not a fallback: every device
    gate refuses an image without the TLV, so a floor derived from its header
    would describe firmware that never boots.
    """
    try:
        return _read_counter(pkg)
    except (mci.SignError, ValueError):
        return None


def counter_problem(pkg: Package):
    """Why @pkg's image carries no usable security counter, or None if it does.

    Every device rollback gate requires the protected counter TLV, so an image
    without one never installs and never boots.
    """
    try:
        _read_counter(pkg)
    except (mci.SignError, ValueError) as exc:
        return f"{pkg.ident}: {exc}. The device refuses an image without a security counter."
    return None


def image_counters(bundle: Editable) -> dict:
    """The initial rollback counter floor for each counter role.

    Taken from the images the bundle carries, not defaulted to zero - the store is
    write-once and refuses an image whose counter is below it
    (`rot_counter_check()`), so a floor above the image's own counter would
    refuse that same image and could never be lowered. The image's own counter is
    the highest floor that cannot brick the device.

    A role the bundle carries no image for keeps 0 - the deployment says nothing
    about a version it does not ship. Each floor comes from package_counter().
    """
    out = dict(pb.DEFAULT_COUNTERS)
    for role in pb.COUNTER_ROLES:
        pkg = bundle.by_role(role)
        if pkg is None:
            continue
        counter = package_counter(pkg)
        if counter is not None:
            out[role] = counter
    return out


def stale_counters(bundle: Editable) -> list:
    """Roles whose recorded counter floor no longer matches their image.

    `sign-component` moves the floors itself without a key source. This instead
    catches commands that change which images a bundle carries - adding,
    replacing, or removing one leaves the recorded floor describing an image
    that is no longer there. Signing or `update-keys` settles it.

    Returns (role, recorded, derived, ident) per role that differs.
    """
    try:
        rec = provisioning_record(bundle)
    except BundleEditError:
        return []
    if rec is None:
        return []
    recorded = rec.get("counters") or {}
    out = []
    for role, derived in image_counters(bundle).items():
        have = recorded.get(role, 0)
        if have != derived:
            pkg = bundle.by_role(role)
            out.append((role, have, derived, pkg.ident if pkg else None))
    return out


def check_counters(bundle: Editable, counters: dict) -> list:
    """Roles whose requested floor would refuse the image in this bundle.

    The counter store is write-once, so a floor above the image's own counter
    is a device that never boots the firmware it was provisioned with.
    """
    over = []
    for role, wanted in counters.items():
        pkg = bundle.by_role(role)
        if pkg is None:
            continue
        counter = package_counter(pkg)
        if counter is not None and wanted > counter:
            over.append((role, wanted, counter, pkg.ident))
    return over


def _refuse_high_counters(bundle: Editable, counters: dict) -> None:
    """Raise when a floor in @counters would refuse the image it ships beside.

    Shared by both writers of the floors, because it is a property of the values
    rather than of the command that sets them.
    """
    over = check_counters(bundle, counters)
    if over:
        raise BundleEditError(
            "these initial rollback counters are above the image's own, so the "
            "device would refuse the firmware it is provisioned with, and the "
            "counter store cannot be lowered: "
            + "; ".join(
                f"{role} 0x{wanted:08X} > {ident} 0x{have:08X} "
                f"({mci.counter_display(have, role)})"
                for role, wanted, have, ident in over
            )
        )


def _record_home(bundle: Editable) -> Package:
    """The package carrying the provisioning record, or a refusal naming why.

    The prov-secure package: the record describes the route hexes, those are its
    members, and a bundle with no secure provisioning firmware has nothing to
    provision with in the first place.
    """
    pkg = bundle.by_role(fw_bundle.PROV_SECURE)
    if pkg is None:
        raise BundleEditError(
            f"{bundle.path} carries no "
            f"{fw_bundle.identifiers([fw_bundle.PROV_SECURE])} component, which "
            f"is what holds a bundle's provisioning configuration and runs its "
            f"batches. Add it with ironside_bundle.py add-component."
        )
    return pkg


def provisioning_config(bundle: Editable) -> dict:
    """The provisioning configuration @bundle carries, filled in from the defaults.

    The `*.user.*` member where the bundle has one, else the released defaults.
    Never refuses an unprepared bundle: the configuration is chosen before a key
    set exists, which is what lets an assembly stage holding no keyset set it.
    """
    pkg = bundle.by_role(fw_bundle.PROV_SECURE)
    if pkg is None:
        return pb.config()
    for member in (fw_bundle.USER_PROVISIONING_CONFIG, fw_bundle.PROVISIONING_CONFIG):
        carried = pkg.json_member(member)
        if carried:
            try:
                return pb.validate_config(carried)
            except pb.BatchError as exc:
                raise BundleEditError(f"{pkg.ident}: {member}: {exc}") from exc
    return pb.config()


def _write_config(bundle: Editable, cfg: dict) -> None:
    """Write @cfg as the user configuration member, displacing the released one."""
    pkg = _record_home(bundle)
    pkg.set(
        fw_bundle.USER_PROVISIONING_CONFIG,
        json.dumps(cfg, indent=2, sort_keys=True).encode() + b"\n",
    )


def provisioning_record(bundle: Editable):
    """The provisioning record @bundle carries, or None when it has none."""
    pkg = bundle.by_role(fw_bundle.PROV_SECURE)
    if pkg is None:
        return None
    rec = pkg.user_manifest.get(fw_bundle.PROVISIONING_RECORD)
    if rec is None:
        return None
    try:
        return pb.validate(rec)
    except pb.BatchError as exc:
        raise BundleEditError(f"{pkg.ident}: {exc}") from exc


def _write_record(bundle: Editable, rec: dict) -> None:
    """Put @rec into the prov-secure package's user manifest.

    Merged into whatever that manifest already holds rather than replacing it, so
    a field another command owns is not collateral of a provisioning edit.
    """
    pkg = _record_home(bundle)
    manifest = dict(pkg.user_manifest)
    manifest[fw_bundle.PROVISIONING_RECORD] = rec
    pkg.set(
        fw_bundle.USER_MANIFEST_MEMBER,
        json.dumps(manifest, indent=2, sort_keys=True).encode() + b"\n",
    )


def _require_record(bundle: Editable, what: str) -> dict:
    """The record, or a refusal naming @what the caller wanted to change."""
    rec = provisioning_record(bundle)
    if rec is None:
        raise BundleEditError(
            f"{bundle.path} is not prepared for provisioning, so it has no "
            f"{what}. Run ironside_bundle.py update-keys first."
        )
    return rec


def set_provisioning_record(
    bundle: Editable,
    keyset,
    *,
    counters=None,
    without_ota_kek=False,
    app_keys=(),
) -> dict:
    """Record what @bundle's provisioning batches are to be built at.

    Written here, not by the provisioning tool, so one command settles everything
    a device is trusted with - the verification keys written into UICR, the OTA
    KEKs a later install batch imports, and the application's own keys in
    @app_keys all come from the same `--keyset`. The recorded digest says which
    key source that must be, which is what makes a later divergence detectable.

    Call after the keys are written, so the digest describes them. Returns what
    was recorded, for the caller to report.
    """
    if counters is None:
        counters = image_counters(bundle)
    _refuse_high_counters(bundle, counters)
    # Opened even though nothing here holds the material: a --keyset that cannot
    # supply the KEKs must fail now, not at the first install batch built from it.
    if not without_ota_kek:
        ota_kek_material(keyset)
    try:
        rec = pb.validate(
            pb.record(
                keyset=keyset,
                counters=counters,
                ota_kek=not without_ota_kek,
                digest=pb.keys_digest(provisioned_keys(bundle)),
                app_keys=ak.inventory(app_keys),
            )
        )
    except pb.BatchError as exc:
        raise BundleEditError(str(exc)) from exc
    _write_record(bundle, rec)
    return {
        "keyset": keyset,
        "counters": dict(counters or pb.DEFAULT_COUNTERS),
        "ota_kek": not without_ota_kek,
        "app_keys": tuple(app_keys),
    }


def set_ap_closure(bundle: Editable, closure: str) -> bool:
    """Configure the access port closure method. True when the bundle changed.

    Only the record moves here. The caller re-materializes the provisioning
    route hexes afterwards, carrying the change into what a station programs --
    which needs the key source, because those routes run an install batch and an
    install batch imports the OTA KEKs.
    """
    cfg = provisioning_config(bundle)
    try:
        changed = pb.set_ap_closure(cfg, closure)
    except pb.BatchError as exc:
        raise BundleEditError(str(exc)) from exc
    if changed or fw_bundle.USER_PROVISIONING_CONFIG not in _record_home(bundle).files:
        _write_config(bundle, cfg)
    return changed


def set_ikg_seed(bundle: Editable, entry: str) -> bool:
    """Configure the SEED mode @entry. True when the bundle changed.

    A record edit like set_ap_closure(), and re-materialized the same way.
    """
    cfg = provisioning_config(bundle)
    try:
        changed = pb.set_ikg_seed(cfg, entry)
    except pb.BatchError as exc:
        raise BundleEditError(str(exc)) from exc
    if changed or fw_bundle.USER_PROVISIONING_CONFIG not in _record_home(bundle).files:
        _write_config(bundle, cfg)
    return changed


def prov_state(bundle: Editable) -> dict:
    """Everything a provisioning edit touches, for putting it back.

    The record says what the route hexes hold, and `check` holds the two
    against each other, so an edit that cannot finish the re-splice must leave
    neither moved -- the route hexes are written one at a time, and a failure
    part-way through would otherwise leave a mix. The record lives in a prov
    package too, which is why snapshotting those covers both halves.
    """
    return {
        "structure_changed": bundle.structure_changed,
        "packages": {
            pkg.ident: (dict(pkg.files), pkg.dirty)
            for pkg in bundle.packages
            if pkg.role in fw_bundle.PROV_ROLES
        },
    }


def restore_prov_state(bundle: Editable, state: dict) -> None:
    """Undo the edits prov_state() was taken before."""
    bundle.structure_changed = state["structure_changed"]
    for pkg in bundle.packages:
        held = state["packages"].get(pkg.ident)
        if held is not None:
            pkg.files, pkg.dirty = held


def set_counters(bundle: Editable, counters=None) -> dict:
    """Set the initial rollback counter floors. Returns {role: (before, after)}
    for each floor that changed.

    Defaults to image_counters(): the floors the carried images justify, which
    is what `update-keys` derives with no `--counters` and what signing
    re-derives -- the image's own counter is the highest floor that cannot
    refuse it. A record edit like set_ap_closure(), re-materialized the same way.
    """
    rec = _require_record(bundle, "rollback counters to set")
    if counters is None:
        counters = image_counters(bundle)
    _refuse_high_counters(bundle, counters)
    before = dict(rec.get("counters") or {})
    try:
        changed = pb.set_counters(rec, counters)
    except pb.BatchError as exc:
        raise BundleEditError(str(exc)) from exc
    if changed:
        _write_record(bundle, rec)
    after = rec["counters"]
    return {
        role: (before.get(role, 0), value)
        for role, value in after.items()
        if before.get(role, 0) != value
    }


def _prov_members(role: str) -> tuple:
    """The provisioning firmware members @role's package can carry."""
    return tuple(
        member
        for route in (fw_bundle.ROUTE_SINGLE_PHASE, fw_bundle.ROUTE_TWO_PHASE)
        if (member := fw_bundle.prov_route_member(role, route)) is not None
    )


def build_prov_artifacts(
    bundle: Path,
    *,
    lcs,
    soc,
    out: Path,
    work: Path,
    batch=None,
    prov_route=None,
    images: bool = True,
) -> None:
    """Merge a bundle into one programmable HEX a station can run.

    Composed over the tool that owns the per-lifecycle merge recipe, so there is
    one implementation of which images and firmware a programming artifact
    holds. With @images false the HEX carries the provisioning firmware alone.

    Exactly one of @prov_route and @batch says where the provisioning commands
    come from: a production route takes the bundle's carried member, and a
    developer flow hands over a batch file to splice.
    """
    if (prov_route is None) == (batch is None):
        raise BundleEditError("name either a provisioning route or a batch to splice, not both")
    argv = [
        SCRIPTS / "internal" / "build_prov_artifacts.py",
        bundle,
        "--lcs",
        lcs,
        "--soc",
        soc,
        "-o",
        work,
        "--bundle",
        out,
    ]
    argv += ["--prov-route", prov_route] if prov_route else ["--batch", batch]
    if not images:
        argv.append("--prov-only")
    _run(argv, f"building the {lcs} artifact")


def check_key_source(keyset) -> None:
    """Raise unless @keyset names a key source that can be opened: the spelling
    and, for a `dir:` source, the directory. Nothing about the keys themselves.

    Checked early - a typo here otherwise surfaces only after an expensive
    download.
    """
    sys.path.insert(0, str(SCRIPTS))
    import iron_keychain as kc

    try:
        kc.KeySource.parse(keyset)
    except kc.KeySourceError as exc:
        raise BundleEditError(str(exc)) from exc


def signing_pubkey(role, gen, *, keyset=None) -> bytes:
    """The 32-byte value a device stores for the key the signer will sign with.

    For UROT that is the digest of the generation's hybrid key pair. For App and
    DFU it is the SHA-256 of the ECDSA public key, derived here from the same
    (source, role, generation) the signer is given. A consumer needs the value to
    check the bundle was signed with a key the bundle also provisions.
    """
    sys.path.insert(0, str(SCRIPTS))
    import iron_keychain as kc

    if role == hy.ROLE:
        return _hybrid_pair_digest(kc, gen, keyset)

    try:
        source = kc.KeySource.parse(keyset or kc.sole_keyset_spec())
        key_id = kc.key_id_for(role, gen)
        pem = source.get_bytes(key_id)
    except kc.KeySourceError as exc:
        raise BundleEditError(str(exc)) from exc
    if pem is None:
        raise BundleEditError(
            f"no {key_id} in {source.describe()} "
            f"(run scripts/iron_keychain.py new, or point --keyset "
            f"elsewhere)"
        )
    private = mci.load_signing_key_from_pem(pem, label=key_id)
    return mci.CURVE.key_digest(ecdsa.public_bytes(private))


def _hybrid_pair_digest(kc, gen, keyset) -> bytes:
    """The digest of hybrid generation @gen's key pair in @keyset."""
    try:
        source = kc.KeySource.parse(keyset or kc.sole_keyset_spec())
        ids = kc.hybrid_key_id_pair(gen)
        pems = [source.get_bytes(key_id) for key_id in ids]
    except kc.KeySourceError as exc:
        raise BundleEditError(str(exc)) from exc
    absent = [key_id for key_id, pem in zip(ids, pems, strict=True) if pem is None]
    if absent:
        raise BundleEditError(
            f"no {', '.join(absent)} in {source.describe()} "
            f"(run scripts/iron_keychain.py new, or point --keyset elsewhere)"
        )
    try:
        return hy.key_pair_digest(
            hy.load_ecdsa_key(pems[0], label=ids[0]),
            hy.load_mldsa_key(pems[1], label=ids[1]),
        )
    except mci.SignError as exc:
        raise BundleEditError(str(exc)) from exc


def _unsigned_form(signed: bytes) -> bytes:
    """@signed with its signature removed: exactly the bytes that were signed."""
    data, base, aux = image_parts(signed)
    region = mci.split_signed(data)[0]
    return _assemble(region, base, aux)


def authop_revoke_gen(image: bytes | None) -> int | None:
    """The newest AUTHOP generation the configuration of @image revokes, with
    every older one, or None when it sets no revocation."""
    conf = image_config(image) if image is not None else None
    if conf is None or not ci.is_supported(conf):
        return None
    return ci.unpack(conf)["authop"]["revoke_gen"]


def check_authop_setting(pkg: Package) -> list:
    """Warnings for a UROT whose AUTHOP revocation setting is lower than the
    outgoing signed image's, and for one whose setting revokes the last generation.

    The setting belongs in every later release, because a device that has not
    applied it yet applies whatever the next release it receives sets. A lower
    setting only warns, so a development bundle can still lower it.
    """
    gen = authop_revoke_gen(pkg.unsigned)
    signed_gen = authop_revoke_gen(pkg.signed_image)
    warnings = []
    if signed_gen is not None and (gen is None or gen < signed_gen):
        revoked = "no AUTHOP generation" if gen is None else f"AUTHOP generation {gen}"
        warnings.append(
            f"{pkg.ident} revokes {revoked}, but its signed image revokes generation "
            f"{signed_gen} and every older one. A device that has not applied the earlier "
            f"setting no longer revokes those generations. Set --authop-revoke-gen "
            f"{signed_gen} or higher to keep it."
        )
    if gen == ci.AUTHOP_GENERATIONS - 1:
        warnings.append(
            f"{pkg.ident} revokes the last AUTHOP generation. A device that applies it has "
            "no AUTHOP key left, so it can never again be debug-unlocked, erased or "
            "decommissioned over ADAC."
        )
    return warnings


def sign(
    bundle: Editable,
    pkg: Package,
    *,
    keyset=None,
    gen=0,
    version=None,
    security_counter=None,
    keep_security_counter=False,
) -> dict:
    """Sign the package's unsigned image into `image.user.signed.hex`.

    sign.py does the signing, as a subprocess - it owns the image-id check, the
    slot-fits guard, and the report, so a second signer here would duplicate
    those. What is decided here is the slot size, taken from the bundle's own
    partition layout rather than sign.py's per-role default, since editing the
    layout moves it.

    With neither @version nor @security_counter named, signing keeps the
    image's existing version and counter.
    """
    if pkg.role not in SIGNABLE_ROLES:
        raise BundleEditError(
            f"{pkg.ident} is not a signable image: {', '.join(SIGNABLE_ROLES)} are "
            f"(IROT carries no signature -- it is BOOTCONF-protected and has no "
            f"image id -- and the provisioning firmware is not a bootable image)"
        )
    warnings = check_authop_setting(pkg) if pkg.role == "urot" else []
    slot = slot_size(bundle, pkg.role)
    with _scratch() as tmp:
        # Named for what they are inside the package, so sign.py's report reads
        # in the bundle's terms once the staging directory is stripped from it.
        src = tmp / (
            fw_bundle.USER_IMAGE_MEMBER if pkg.user_image is not None else fw_bundle.IMAGE_MEMBER
        )
        out = tmp / fw_bundle.USER_SIGNED_MEMBER
        src.write_bytes(pkg.unsigned)
        argv = [SCRIPTS / "sign.py", "--role", pkg.role, src, "-o", out, "--gen", str(gen)]
        argv += ["--soc", bundle.require_soc()]
        argv += ["--keyset", keyset] if keyset else []
        if version is not None:
            argv += ["-v", version]
        if security_counter is not None:
            argv += ["-s", str(security_counter)]
        if keep_security_counter:
            argv.append("--keep-security-counter")
        if slot is not None:
            argv += ["-S", str(slot)]
        report = _run(argv, "sign.py").strip().replace(f"{tmp}/", "")
        signed = out.read_bytes()

    pubkey = signing_pubkey(pkg.role, gen, keyset=keyset)
    pkg.set(fw_bundle.USER_SIGNED_MEMBER, signed)
    # Kept beside the signature even when unedited - it is what the signature was
    # made over, so a later base-config edit has something to re-apply to, and
    # `check` can compare the two for a stale signature.
    #
    # Taken from the signed image with its signature stripped, not from the
    # input - `--version` and `--security-counter` are rewritten by sign.py
    # inside the signed region, so the input is no longer what was signed.
    # Using the input instead would make `check` report a stale signature after
    # every versioned sign.
    pkg.set(fw_bundle.USER_IMAGE_MEMBER, _unsigned_form(signed))
    pkg.set(
        fw_bundle.USER_MANIFEST_MEMBER,
        json.dumps({"signing_key": pubkey.hex()}, indent=2).encode() + b"\n",
    )
    return {
        "report": report,
        "signing_key": pubkey.hex(),
        "slot_size": slot,
        "role": pkg.role,
        "warnings": warnings,
    }


def packages_from(path: Path) -> Editable:
    """What @path offers as component archives: a bundle, a directory, or one
    package.

    An Editable, not a list, so a caller also gets what was not recognized -
    otherwise building a bundle from a working directory could quietly leave a
    component behind, shipping a release without its DFU image.
    """
    path = Path(path)
    if not path.exists():
        raise BundleEditError(f"{path} does not exist")
    ident = fw_bundle.identify(path.name)
    if path.is_dir() and ident is not None and (path / fw_bundle.IMAGE_MEMBER).is_file():
        return Editable(
            path=path,
            is_directory=True,
            packages=[
                _package(
                    ident,
                    path.name,
                    {
                        f.relative_to(path).as_posix(): f.read_bytes()
                        for f in sorted(path.rglob("*"))
                        if f.is_file()
                    },
                    str(path),
                    unpacked=True,
                )
            ],
        )
    if path.is_dir():
        return _read_dir(path)
    if not zipfile.is_zipfile(path):
        raise BundleEditError(f"{path} is not a zip archive")
    blob = path.read_bytes()
    members = _members_of_zip(blob, str(path))
    if ident is not None and fw_bundle.IMAGE_MEMBER in members:
        return Editable(
            path=path,
            is_directory=False,
            packages=[_package(ident, path.name, members, str(path), raw=blob)],
        )
    return read(path)


def add_package(bundle: Editable, new: Package, *, replace=False) -> str:
    """Put @new into @bundle. Returns what happened, for the report.

    Replacing drops the displaced package's customizations rather than carrying
    them over - they were made against other firmware, and a configuration that
    silently survives an update is how a stale one ships. `--reapply-config` asks
    for the old configuration back explicitly.

    Refuses a signable image without a security counter TLV, which no device
    gate accepts.
    """
    if new.role in SIGNABLE_ROLES:
        problem = counter_problem(new)
        if problem is not None:
            raise BundleEditError(problem)
    existing = bundle.by_role(new.role)
    if existing is not None and not replace:
        raise BundleEditError(
            f"{bundle.path} already carries {existing.ident} "
            f"{existing.version} for the {new.role} role; pass --force to replace "
            f"it with {new.ident} {new.version}"
        )
    if existing is not None:
        bundle.packages.remove(existing)
        if not bundle.is_directory:
            bundle.others.pop(existing.name, None)
    new.dirty = True
    bundle.packages.append(new)
    bundle.structure_changed = True
    _checked(bundle)
    return "added" if existing is None else f"replaced {existing.ident} {existing.version}"


def remove_package(bundle: Editable, pkg: Package) -> None:
    bundle.packages.remove(pkg)
    bundle.structure_changed = True
    bundle.others.pop(pkg.name, None)
    if bundle.is_directory:
        target = bundle.path / pkg.name
        if target.is_dir():
            for f in sorted(target.rglob("*"), reverse=True):
                f.unlink() if f.is_file() else f.rmdir()
            target.rmdir()
        else:
            target.unlink(missing_ok=True)


def check_deps(bundle: Editable) -> tuple:
    """(unsatisfied floors, floors naming a component the bundle lacks).

    The components are versioned independently, so the set in a bundle is not
    automatically consistent, and an edit that swaps or removes one is exactly
    when that stops being true. Uses fw_bundle.satisfies(), the same comparison
    the downloader uses.

    The two lists are separate because only the first is always wrong - a bundle
    may legitimately omit a component (a second-phase bundle carries only the
    non-secure provisioning firmware), so the downloader ignores a floor naming
    an absent one. For an edit that silence is too much - deleting IROT from a
    full bundle leaves UROT declaring a floor on it, worth reporting without
    calling a partial bundle broken.
    """
    versions = {p.ident: p.version for p in bundle.packages}
    unsatisfied, absent = [], []
    for pkg in sorted(bundle.packages, key=lambda p: p.ident):
        for dep, floor in sorted((pkg.manifest.get("deps") or {}).items()):
            have = versions.get(dep)
            if have is None:
                absent.append(
                    f"{pkg.ident} {pkg.version} declares a dependency on "
                    f"{dep} {floor}, which this bundle does not carry"
                )
            elif not fw_bundle.satisfies(have, floor):
                unsatisfied.append(
                    f"{pkg.ident} {pkg.version} needs {dep} {floor} "
                    f"or newer, but this bundle carries {dep} {have}"
                )
    return unsatisfied, absent
