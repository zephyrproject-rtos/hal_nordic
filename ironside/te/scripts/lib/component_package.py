# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Write reproducible IronSide TE component ZIPs from Intel HEX images."""

from __future__ import annotations

import json
import os
import re
import sys
import tempfile
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import fw_bundle, parts  # noqa: E402


def _mcuboot_image():
    """Import the image reader only when a hex is being checked.

    It needs intelhex. Package writing and archive stamping use only the
    standard library.
    """
    from lib import mcuboot_image

    return mcuboot_image


# One fixed member timestamp, shared with the bundle writer: the bundle and the
# packages inside it are one format, so they answer to one reproducibility rule.
FIXED_TIMESTAMP = fw_bundle.FIXED_TIMESTAMP

# The component name a --role may be given as, mapped to the identifier that
# names the package. Derived from the identifiers themselves so a new component
# needs no edit here. There is deliberately no bare `dfu` key - two identifiers
# fill the dfu role, and a package name has to say which one.
ROLE_MAP = {ident.removeprefix("ironside-te-"): ident for ident in fw_bundle.COMPONENT_ROLES}

# Three or four decimal fields. The version reaches the package name and the
# manifest, where the bundle tools order and compare it.
VERSION_RE = re.compile(r"^\d+\.\d+\.\d+(?:\.\d+)?$")

RESOURCES_DIR = "resources"


class ComponentPackageError(Exception):
    """The package could not be built from the inputs given."""


def validate_hex(path) -> Path:
    """Check that @path is a readable Intel hex holding firmware.

    Refuses anything but a .hex - the member it becomes is image.hex, and a raw
    binary carries no addresses to place it at.
    """
    path = Path(path)
    if path.suffix.lower() != ".hex":
        raise ComponentPackageError(f"{path}: expected an Intel HEX file with a .hex suffix")
    if not path.is_file():
        raise ComponentPackageError(f"image not found: {path}")
    mci = _mcuboot_image()
    try:
        mci.read_image(path)
    except mci.SignError as exc:
        raise ComponentPackageError(str(exc)) from exc
    return path


def validate_component_name(name: str) -> str:
    """Resolve @name to a component identifier.

    Takes either the identifier (ironside-te-urot) or its short form (urot), the
    same two spellings the bundle tools accept for a component.
    """
    if name in fw_bundle.COMPONENT_ROLES:
        return name
    ident = ROLE_MAP.get(name)
    if ident is None:
        raise ComponentPackageError(
            f"unknown component {name!r}. Expected one of: {', '.join(sorted(ROLE_MAP))}"
        )
    return ident


def validate_version(version: str) -> str:
    """Check that @version is MAJOR.MINOR.PATCH[.PATCH2] and can be ordered."""
    if not VERSION_RE.match(version):
        raise ComponentPackageError(
            f"version {version!r}: need 3 or 4 dot-separated decimal fields, "
            f"MAJOR.MINOR.PATCH[.PATCH2]"
        )
    try:
        fw_bundle.version_key(version)
    except ValueError as exc:
        raise ComponentPackageError(f"version {version!r} cannot be ordered ({exc})") from exc
    return version


def validate_soc(soc: str) -> str:
    """Resolve @soc to a supported SoC name."""
    try:
        return parts.by_soc(soc).soc
    except parts.PartError as exc:
        raise ComponentPackageError(str(exc)) from exc


def create_manifest(ident: str, soc: str, version: str, *, name: str | None = None) -> dict:
    """The manifest for one package, in the order the user guide lists the fields.

    `version` carries the @<soc> suffix that makes it an ABI identifier, which
    lets a bundle tool compare it with a dependency floor. The file name carries
    the SoC separately. `deps` is an object rather
    than a list because an ABI identifier does not name its own component.

    The channel, git reference and build time a published package records are
    absent - a package built from a hex has no source for them, and a manifest
    stating a provenance it does not have is worse than one that is silent.
    """
    return {
        "name": name or ident,
        "soc": soc,
        "version": f"{version}@{soc}",
        "deps": {},
    }


def _fix_zip_timestamps(path) -> Path:
    """Rewrite @path so its members carry the fixed timestamp and fixed modes.

    For an archive some other writer produced: a zip stamped with file mtimes and
    filesystem modes is not reproducible, and there is no way to ask
    shutil.make_archive() for either. Member order and compression are preserved,
    so only what varies between two builds of one input is replaced.

    Underscored but not private to this file: the packaging tooling calls it. It
    is not part of the documented interface a package consumer uses.
    """
    path = Path(path)
    with zipfile.ZipFile(path) as src:
        members = [(info, src.read(info.filename)) for info in src.infolist()]
    tmp = path.with_name(path.name + ".tmp")
    with zipfile.ZipFile(tmp, "w") as out:
        for info, blob in members:
            fixed = zipfile.ZipInfo(info.filename, date_time=FIXED_TIMESTAMP)
            fixed.compress_type = info.compress_type
            if info.is_dir():
                fixed.external_attr = (0o755 << 16) | 0x10
            else:
                fixed.external_attr = 0o644 << 16
            out.writestr(fixed, blob)
    os.replace(tmp, path)
    return path


def write_package_zip(out, image_hex, manifest: dict, resources=None) -> Path:
    """Write the package @out: image.hex, manifest.json, and @resources.

    Flat, because that is where the user guide's layout puts those names --
    rooting them under a directory would make every documented path one level
    deeper than it says. Deflated, as a published package is, and stamped so two
    runs over one input give identical bytes.

    @resources is an iterable of paths, each copied to resources/<file name>.
    The archive is written beside @out and moved into place, so an interrupted
    run leaves no half-written package.
    """
    out, image_hex = Path(out), Path(image_hex)
    members = {
        fw_bundle.IMAGE_MEMBER: image_hex.read_bytes(),
        fw_bundle.MANIFEST_MEMBER: (json.dumps(manifest, indent=2) + "\n").encode(),
    }
    for resource in resources or ():
        resource = Path(resource)
        if not resource.is_file():
            raise ComponentPackageError(f"resource not found: {resource}")
        name = f"{RESOURCES_DIR}/{resource.name}"
        if name in members:
            raise ComponentPackageError(f"two resources are named {resource.name}")
        members[name] = resource.read_bytes()

    tmp = out.with_name(out.name + ".tmp")
    with zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED) as zf:
        for name in sorted(members):
            info = zipfile.ZipInfo(name, date_time=FIXED_TIMESTAMP)
            info.external_attr = 0o644 << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            zf.writestr(info, members[name])
    os.replace(tmp, out)
    return out


def create_package(
    hex_path,
    role: str,
    version: str,
    soc: str,
    *,
    out_dir=None,
    name: str | None = None,
    resources=None,
) -> Path:
    """Package @hex_path as the @role component and return the file written.

    The name is derived rather than given: <identifier>_<soc>-<version>.zip, the
    form the bundle tools locate a component by. Every input is validated before
    anything is written.
    """
    image_hex = validate_hex(hex_path)
    ident = validate_component_name(role)
    version = validate_version(version)
    soc = validate_soc(soc)
    manifest = create_manifest(ident, soc, version, name=name)
    out_dir = Path(out_dir) if out_dir is not None else Path()
    out = out_dir / f"{ident}_{soc}-{version}.zip"
    with tempfile.TemporaryDirectory(prefix="component-archive-") as tmp:
        image_hex = with_unsigned_metadata(image_hex, ident, version, Path(tmp))
        return write_package_zip(out, image_hex, manifest, resources)


def with_unsigned_metadata(image_hex: Path, ident: str, version: str, work_dir: Path) -> Path:
    """@image_hex, or a copy in @work_dir with unsigned MCUboot metadata added.

    A signable image needs the metadata, and the security counter in it, before
    a bundle accepts it. Raw firmware receives it here with @version, as the
    IronSide TE release images receive it at build time. The copy keeps the UICR
    words @image_hex carries beside the firmware. An image that already carries
    metadata is returned unchanged.
    """
    mci = _mcuboot_image()
    image_role = fw_bundle.COMPONENT_ROLES[ident]
    if image_role not in mci.IMAGE_ID or mci.has_metadata(image_hex):
        return image_hex
    try:
        unsigned, base = mci.build_unsigned(
            image_hex,
            version=version,
            pad_header=mci.PAD_HEADER_DEFAULT[image_role],
            image_id=mci.IMAGE_ID[image_role],
        )
    except mci.SignError as exc:
        raise ComponentPackageError(f"{image_hex}: {exc}") from exc
    out = work_dir / fw_bundle.IMAGE_MEMBER
    mci.write_image(out, unsigned, base, aux=mci.read_aux(image_hex))
    return out


def _self_test() -> None:
    import shutil
    import tempfile

    from intelhex import IntelHex

    assert ROLE_MAP["urot"] == "ironside-te-urot"
    assert ROLE_MAP["app"] == "ironside-te-app"
    assert "dfu" not in ROLE_MAP, "a bare dfu role cannot name a package"
    assert validate_component_name("urot") == "ironside-te-urot"
    assert validate_component_name("ironside-te-app") == "ironside-te-app"
    assert validate_version("1.2.3") == "1.2.3"
    assert validate_version("1.2.3.4") == "1.2.3.4"
    assert validate_soc("NRF54LM20B") == "nrf54lm20b"

    def refuses(what, call):
        try:
            call()
        except ComponentPackageError:
            return
        raise AssertionError(f"{what} must be refused")

    refuses("an unknown component", lambda: validate_component_name("dfu"))
    refuses("a two-field version", lambda: validate_version("1.2"))
    refuses("a non-decimal version", lambda: validate_version("1.2.x"))
    refuses("an unsupported SoC", lambda: validate_soc("nrf52840"))

    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        image = tmp / "image.hex"
        composed = IntelHex()
        composed.frombytes(bytes(range(256)), offset=0x4000)
        composed.tofile(str(image), "hex")

        refuses("a missing image", lambda: validate_hex(tmp / "absent.hex"))
        refuses("a non-hex image", lambda: validate_hex(image.with_suffix(".bin")))
        empty = tmp / "empty.hex"
        empty.write_text(":00000001FF\n")
        refuses("a hex with no firmware", lambda: validate_hex(empty))

        resource = tmp / "notes.txt"
        resource.write_text("resource\n")
        out_dir = tmp / "out"
        out_dir.mkdir()
        pkg = create_package(
            image, "app", "1.0.0", "nrf54lm20b", out_dir=out_dir, resources=[resource]
        )
        assert pkg.name == "ironside-te-app_nrf54lm20b-1.0.0.zip", pkg.name
        with zipfile.ZipFile(pkg) as zf:
            assert zf.namelist() == ["image.hex", "manifest.json", "resources/notes.txt"]
            manifest = json.loads(zf.read(fw_bundle.MANIFEST_MEMBER))
            assert all(info.date_time == FIXED_TIMESTAMP for info in zf.infolist())
        assert manifest == {
            "name": "ironside-te-app",
            "soc": "nrf54lm20b",
            "version": "1.0.0@nrf54lm20b",
            "deps": {},
        }, manifest

        # Raw firmware is archived with unsigned metadata that carries the
        # version's security counter, which a bundle requires before it signs.
        mci = _mcuboot_image()
        archived = tmp / "archived.hex"
        with zipfile.ZipFile(pkg) as zf:
            archived.write_bytes(zf.read(fw_bundle.IMAGE_MEMBER))
        data, _ = mci.read_image(archived)
        _header, _version, _body, tlvs = mci.read_unsigned(data)
        counter = int.from_bytes(tlvs[mci.TLV_SEC_CNT], "little")
        assert counter == mci.security_counter_from_version("1.0.0"), hex(counter)
        assert tlvs[mci.TLV_IROT_IMAGE_ID][0] == mci.IMAGE_ID["app"]
        repacked = create_package(archived, "app", "1.0.0", "nrf54lm20b", out_dir=tmp)
        with zipfile.ZipFile(repacked) as zf:
            assert zf.read(fw_bundle.IMAGE_MEMBER) == archived.read_bytes(), (
                "an image with metadata must be archived unchanged"
            )

        # The same inputs give the same bytes, which is what the fixed timestamp
        # is for.
        first = pkg.read_bytes()
        os.utime(image, (0, 0))
        create_package(image, "app", "1.0.0", "nrf54lm20b", out_dir=out_dir, resources=[resource])
        assert pkg.read_bytes() == first, "a package must be reproducible"

        # The reader recognizes it, and reports the role the identifier names.
        bundle = fw_bundle.read(out_dir, tmp / "extract", roles=("app",))
        assert set(bundle.components) == {"app"}, bundle.components
        assert bundle.components["app"].ident == "ironside-te-app"
        assert bundle.components["app"].soc == "nrf54lm20b"
        assert bundle.components["app"].version == "1.0.0@nrf54lm20b"

        # An archive another writer produced is stamped on the way through.
        loose = tmp / "loose"
        (loose / RESOURCES_DIR).mkdir(parents=True)
        (loose / fw_bundle.IMAGE_MEMBER).write_bytes(image.read_bytes())
        (loose / RESOURCES_DIR / "notes.txt").write_text("resource\n")
        made = Path(shutil.make_archive(str(tmp / "made"), "zip", root_dir=loose))
        with zipfile.ZipFile(made) as zf:
            stamps = {info.date_time for info in zf.infolist()}
        assert stamps != {FIXED_TIMESTAMP}, "make_archive is expected to use real mtimes"
        _fix_zip_timestamps(made)
        with zipfile.ZipFile(made) as zf:
            infos = zf.infolist()
            assert all(info.date_time == FIXED_TIMESTAMP for info in infos)
            assert all(
                info.external_attr >> 16 == (0o755 if info.is_dir() else 0o644) for info in infos
            )
            assert zf.testzip() is None

    print("component_package self-test: OK")


if __name__ == "__main__":
    _self_test()
