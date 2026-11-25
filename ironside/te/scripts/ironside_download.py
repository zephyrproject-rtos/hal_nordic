#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Download published IronSide TE components and create an IronSide bundle."""

from __future__ import annotations

import argparse
import hashlib
import json
import logging as log
import os
import re
import sys
import tempfile
import zipfile
from typing import Callable
from dataclasses import dataclass
from pathlib import Path
from urllib.parse import urlparse

import requests

SCRIPTS = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPTS))
from lib import artifactory_auth, fw_bundle  # noqa: E402

# Overrides the repository base URL a packaged IronSide Integration Kit records in its
# manifest.json. The selected channel is appended to the base.
URL_ENV = "IRONSIDE_RELEASE_URL"

# Published layout:
#   <channel>/<FIRMWARE_DIR>/<soc>/<component identifier>/<package name>.zip
FIRMWARE_DIR = "ironside-te"

# Channels accepted by component manifests and --channel.
CHANNELS = ("release", "nightly", "dev")
# Release packages omit channel, ref, and timestamp from their names.
RELEASE_CHANNEL = "release"
# Dev builds of the IronSide Integration Kit require an explicit channel because dev artifacts are
# not published.
PUBLISHED_CHANNELS = ("release", "nightly")

# A packaged IronSide Integration Kit records its channel and provenance beside scripts/.
SDK_MANIFEST = SCRIPTS.parent / "manifest.json"
# The roles whose release date is reported without --verbose: the two that carry
# the secure firmware.
RELEASE_DATE_ROLES = ("irot", "urot")

# <identifier>_fl<feature_level>_<soc>-<semver>[_<channel>_<timestamp>_<ref>].zip
# The feature level field is optional because packages published before feature levels
# existed carry none.
PACKAGE_RE = re.compile(
    r"^(?P<ident>[^_]+)(?:_fl(?P<feature_level>\d+))?"
    r"_(?P<soc>[^_@/]+)-(?P<semver>\d+\.\d+\.\d+[^_@/]*)"
    r"(?:_(?P<channel>[^_]+)_(?P<timestamp>\d{4}-\d{2}-\d{2})_(?P<ref>[^_]+))?\.zip$"
)
DATE_RE = re.compile(r"^\d{4}-\d{2}-\d{2}$")

# A bundle can contain only one DFU implementation.
DFU_FLAVOURS = {"serial": "ironside-te-dfu-serial", "ble": "ironside-te-dfu-ble"}
DFU_DEFAULT = "serial"

# Requests time out after 10 seconds to connect or 120 seconds without a read.
TIMEOUT = (10, 120)
CHUNK = 1 << 16

# Fixed member timestamps make repeated downloads byte-identical.
FIXED_TIMESTAMP = fw_bundle.FIXED_TIMESTAMP


class DownloadError(Exception):
    """The release could not be downloaded, or is not a self-consistent one."""


@dataclass(frozen=True)
class Package:
    """One published component archive, as the repository listing reports it."""

    ident: str
    semver: str
    soc: str
    name: str
    uri: str  # path under FIRMWARE_DIR, as listed
    size: int
    sha256: str
    # Provenance encoded in non-release package names.
    channel: str | None = None
    ref: str | None = None
    timestamp: str | None = None
    # None for a package published before feature levels existed.
    feature_level: int | None = None
    # Repository channel directory used to build the download URL.
    tree: str = RELEASE_CHANNEL

    @property
    def version(self) -> str:
        """The ABI identifier `<semver>@<soc>`, which is what the manifest carries."""
        return f"{self.semver}@{self.soc}"

    @property
    def feature_level_label(self) -> str:
        return "-" if self.feature_level is None else f"fl{self.feature_level}"

    @property
    def released_on(self) -> str:
        """Return the channel encoded by the package name."""
        return self.channel or RELEASE_CHANNEL

    @property
    def rank(self):
        """Sort by version, build date, then name."""
        return (version_key(self.semver), self.timestamp or "", self.name)

    @property
    def provenance(self) -> str:
        """Return channel, timestamp, and ref as encoded in the package name."""
        return "_".join(field for field in (self.channel, self.timestamp, self.ref) if field)

    @property
    def role(self) -> str:
        return fw_bundle.COMPONENT_ROLES[self.ident]


def sdk_manifest_text(field: str, manifest: Path = SDK_MANIFEST) -> str | None:
    """Read a text @field of the IronSide Integration Kit's manifest, or return None for an
    unpackaged tree or an unreadable field."""
    if not manifest.is_file():
        log.debug("no %s: an unpackaged tree states no %s of its own", manifest, field)
        return None
    try:
        stated = json.loads(manifest.read_text())[field]
    except (OSError, ValueError, TypeError, KeyError) as exc:
        log.warning("%s states no readable %s (%s)", manifest, field, exc)
        return None
    if not isinstance(stated, str):
        log.warning("%s states a %s that is not text (%r)", manifest, field, stated)
        return None
    return stated


def sdk_channel(manifest: Path = SDK_MANIFEST) -> str | None:
    """Read the IronSide Integration Kit's channel, or return None for an unpackaged tree."""
    return sdk_manifest_text("channel", manifest)


def sdk_download_base_url(manifest: Path = SDK_MANIFEST) -> str | None:
    """Read the repository base URL the IronSide Integration Kit was published for, or
    return None for an unpackaged tree."""
    return sdk_manifest_text("download_base_url", manifest)


def sdk_provenance(manifest: Path = SDK_MANIFEST) -> tuple[str | None, str | None]:
    """The build date (YYYY-MM-DD) and git ref this IronSide Integration Kit was cut on, or
    (None, None).

    Read from the same manifest.json as sdk_channel(). A nightly IronSide Integration Kit uses these
    as the default firmware pin when no selector is named on the command line.
    """
    if not manifest.is_file():
        log.debug("no %s: an unpackaged tree states no provenance of its own", manifest)
        return None, None
    try:
        data = json.loads(manifest.read_text())
    except (OSError, ValueError) as exc:
        log.warning("%s states no readable provenance (%s)", manifest, exc)
        return None, None
    timestamp = data.get("timestamp")
    date = timestamp[:10] if isinstance(timestamp, str) and len(timestamp) >= 10 else None
    ref = data.get("ref")
    if not isinstance(ref, str):
        ref = None
    return date, ref


def resolve_channel(asked: str | None, stated: str | None) -> str:
    """Use an explicit channel, the IronSide Integration Kit's channel, or release for a source
    tree."""
    if asked:
        return asked
    if stated is None:
        log.debug("no channel stated for this IronSide Integration Kit; taking %s", RELEASE_CHANNEL)
        return RELEASE_CHANNEL
    if stated not in PUBLISHED_CHANNELS:
        why = (
            f"the {stated} channel is not published"
            if stated in CHANNELS
            else f"{stated} is not a channel this command knows"
        )
        raise DownloadError(
            f"this IronSide Integration Kit uses the {stated} channel, but {why}. "
            f"Select a channel with --channel ({'/'.join(PUBLISHED_CHANNELS)})."
        )
    log.debug("no --channel: taking the %s channel this IronSide Integration Kit was cut on",
              stated)
    return stated


# No version in the name: the components are versioned independently, so no single
# number describes the bundle. What it holds is reported, and recorded in the members.
def bundle_name(soc: str, channel: str) -> str:
    """The default bundle file name for @soc off @channel.

    The release channel keeps the plain name; any other channel is named in the
    file, so a dev bundle cannot quietly take a release bundle's place in a
    directory somebody installs from later.
    """
    infix = "" if channel == RELEASE_CHANNEL else f"_{channel}"
    return f"ironside-te_{soc}{infix}.iron.zip"


def default_output_path(
    *, soc: str, out: Path | str | None = None, channel: str | None = None
) -> Path:
    """Where a bundle is written: @out if named, else `bundle_name(@soc, channel)`."""
    channel = resolve_channel(channel, sdk_channel())
    return Path(out) if out is not None else Path(bundle_name(soc, channel))


def refuse_existing(path: Path, *, force: bool) -> None:
    """Refuse to clobber an existing bundle unless @force is set."""
    if path.exists() and not force:
        raise DownloadError(f"{path} already exists. Use --force to replace it.")


# Every flag ironside_download.py owns. ironside_bundle.py init declares the same
# ones so argparse can parse a full command line; `download_argv` turns them back
# into a subprocess invocation.
DOWNLOAD_OPTION_FLAGS = {
    "latest": "--latest",
    "by_date": "--by-date",
    "by_version": "--by-version",
    "interactive": "-i",
    "ref": "--ref",
    "channel": "--channel",
    "soc": "--soc",
    "url": "--url",
    "auth_token": "--auth-token",
    "login": "--login",
    "forget_login": "--forget-login",
    "dfu": "--dfu",
    "keep_components": "--keep-components",
    "dry_run": "--dry-run",
    "verbose": "-v",
    "force": "--force",
    "out": "-o",
}


def add_selection_arguments(ap, *, by_version=False):
    """Component selectors shared with ironside_bundle.py."""
    selection = ap.add_argument_group("release selection")
    selection.add_argument(
        "--latest",
        action="store_true",
        default=argparse.SUPPRESS,
        help="download the newest build of each component",
    )
    selection.add_argument(
        "-i",
        "--interactive",
        action="store_true",
        default=argparse.SUPPRESS,
        help="choose a build for each component interactively",
    )
    if by_version:
        selection.add_argument(
            "--by-version",
            metavar="VERSION",
            default=argparse.SUPPRESS,
            help="download release packages at VERSION",
        )

    nightly = ap.add_argument_group(
        "nightly selection",
        "These options are used only with --channel nightly.",
    )
    nightly.add_argument(
        "--by-date",
        metavar="DATE",
        default=argparse.SUPPRESS,
        help="download builds from DATE (YYYY-MM-DD, non-release channels only)",
    )
    nightly.add_argument(
        "--ref",
        "--sha",
        dest="ref",
        default=argparse.SUPPRESS,
        metavar="REF",
        help="git ref used to narrow --by-date (short prefixes match)",
    )
    return selection


def add_arguments(ap, *, out=False, by_version=False):
    """The download flags, shared with ironside_bundle.py init."""
    selection = add_selection_arguments(ap, by_version=by_version)
    selection.add_argument(
        "--channel",
        choices=PUBLISHED_CHANNELS,
        default=argparse.SUPPRESS,
        help=("release channel (default: the IronSide Integration Kit's channel, or release from a "
              "source tree)"),
    )
    selection.add_argument(
        "--soc",
        default=argparse.SUPPRESS,
        metavar="SOC",
        help="SoC to download, such as nrf54lm20a, nrf54lm20b, nrf54lv10a or nrf54l15. "
        "There is no default",
    )
    selection.add_argument(
        "--dfu",
        choices=sorted(DFU_FLAVOURS),
        default=argparse.SUPPRESS,
        help=argparse.SUPPRESS,
    )

    output = ap.add_argument_group("output")
    if out:
        output.add_argument(
            "-o",
            "--out",
            type=Path,
            default=argparse.SUPPRESS,
            metavar="ZIP",
            help="output bundle (default: ironside-te_<soc>[_<channel>].iron.zip)",
        )
    output.add_argument(
        "--keep-components",
        type=Path,
        default=argparse.SUPPRESS,
        metavar="DIR",
        help="also save downloaded component archives",
    )
    output.add_argument(
        "--force",
        action="store_true",
        default=argparse.SUPPRESS,
        help="replace an existing output bundle",
    )

    repository = ap.add_argument_group("repository access")
    repository.add_argument(
        "--url",
        default=argparse.SUPPRESS,
        help=f"repository base URL, with no channel on the end (default: ${URL_ENV}, "
        "else download_base_url in the IronSide Integration Kit's manifest.json)",
    )
    repository.add_argument(
        "--auth-token",
        default=argparse.SUPPRESS,
        metavar="TOKEN",
        help=f"repository bearer token (default: ${artifactory_auth.TOKEN_ENV}, "
        "stored login, ~/.netrc, or browser login)",
    )
    repository.add_argument(
        "--login",
        action="store_true",
        default=argparse.SUPPRESS,
        help="sign in through a browser and store the token",
    )
    repository.add_argument(
        "--forget-login",
        action="store_true",
        default=argparse.SUPPRESS,
        help="delete the stored token and exit",
    )

    execution = ap.add_argument_group("execution")
    execution.add_argument(
        "--dry-run",
        action="store_true",
        default=argparse.SUPPRESS,
        help="show selected components without downloading",
    )
    execution.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=argparse.SUPPRESS,
        help="print requested URLs and download details",
    )


def apply_defaults(args) -> None:
    """Defaults for flags declared with SUPPRESS, after parsing."""
    if not hasattr(args, "latest"):
        args.latest = False
    if not hasattr(args, "by_date"):
        args.by_date = None
    if not hasattr(args, "by_version"):
        args.by_version = None
    if not hasattr(args, "interactive"):
        args.interactive = False
    if not hasattr(args, "ref"):
        args.ref = None
    if not hasattr(args, "channel"):
        args.channel = None
    if not hasattr(args, "soc"):
        args.soc = None
    if not hasattr(args, "out"):
        args.out = None
    if not hasattr(args, "url"):
        args.url = os.environ.get(URL_ENV) or sdk_download_base_url()
    if not hasattr(args, "auth_token"):
        args.auth_token = None
    if not hasattr(args, "login"):
        args.login = False
    if not hasattr(args, "forget_login"):
        args.forget_login = False
    if not hasattr(args, "dfu"):
        args.dfu = None
    if not hasattr(args, "keep_components"):
        args.keep_components = None
    if not hasattr(args, "dry_run"):
        args.dry_run = False
    if not hasattr(args, "verbose"):
        args.verbose = False
    if not hasattr(args, "force"):
        args.force = False


def validate_selection(args) -> None:
    selected = [
        flag
        for flag, value in (
            ("--latest", getattr(args, "latest", False)),
            ("--by-date", getattr(args, "by_date", None)),
            ("--by-version", getattr(args, "by_version", None)),
            ("--interactive", getattr(args, "interactive", False)),
        )
        if value
    ]
    if len(selected) > 1:
        raise DownloadError("choose only one selection option: " + ", ".join(selected))


def require_soc(args) -> None:
    """Refuse a download that names no SoC, since each SoC has its own firmware."""
    if args.soc is None:
        raise DownloadError("name the SoC to download with --soc, such as --soc nrf54lm20b")


def require_url(args) -> None:
    """Refuse a download with no repository base URL, which only an unpackaged tree lacks."""
    if args.url is None:
        raise DownloadError(
            "this IronSide Integration Kit names no download_base_url in a manifest.json. "
            f"Pass --url or set ${URL_ENV}."
        )


def download_options_named(args) -> list[str]:
    """Download flags named on the command line (for rejecting them with --from)."""
    skip = {"out"}
    return [
        flag
        for attr, flag in DOWNLOAD_OPTION_FLAGS.items()
        if attr not in skip and hasattr(args, attr)
    ]


def download_using_remote(args) -> bool:
    """True when @args names a remote fetch rather than a local source."""
    return bool(download_options_named(args))


def download_argv(args, *, out: Path) -> list[str]:
    """Rebuild a download.py argv from parsed init/download arguments."""
    argv = ["-o", str(out)]
    if getattr(args, "latest", False):
        argv.append("--latest")
    if getattr(args, "by_date", None):
        argv.extend(["--by-date", args.by_date])
    if getattr(args, "by_version", None):
        argv.extend(["--by-version", args.by_version])
    if getattr(args, "interactive", False):
        argv.append("-i")
    if getattr(args, "ref", None):
        argv.extend(["--ref", args.ref])
    if hasattr(args, "channel"):
        argv.extend(["--channel", args.channel])
    if getattr(args, "soc", None) is not None:
        argv.extend(["--soc", args.soc])
    if hasattr(args, "url"):
        argv.extend(["--url", args.url])
    if hasattr(args, "auth_token"):
        argv.extend(["--auth-token", args.auth_token])
    if getattr(args, "login", False):
        argv.append("--login")
    if getattr(args, "forget_login", False):
        argv.append("--forget-login")
    if hasattr(args, "dfu"):
        argv.extend(["--dfu", args.dfu])
    if hasattr(args, "keep_components"):
        argv.extend(["--keep-components", str(args.keep_components)])
    if getattr(args, "dry_run", False):
        argv.append("--dry-run")
    if getattr(args, "verbose", False):
        argv.append("-v")
    if getattr(args, "force", False):
        argv.append("--force")
    return argv


# Version ordering and the `deps` floor rule live in lib/fw_bundle.py - they are
# part of the manifest format, and ironside_bundle.py checks the same floors on a
# bundle it never downloaded.
version_key = fw_bundle.version_key
satisfies = fw_bundle.satisfies


def endpoints(base: str):
    """Return storage API and download URLs for an Artifactory repository."""
    root, sep, repo_path = base.rstrip("/").partition("/artifactory/")
    if not sep or not repo_path:
        raise DownloadError(
            f"{base} is not an Artifactory repository URL "
            f"(expected <host>/artifactory/<repo>/<path>)"
        )
    head, _, tail = repo_path.rstrip("/").rpartition("/")
    if tail in CHANNELS and head:
        log.debug("%s names the %s channel; using the repository base %s", base, tail, head)
        repo_path = head
    return f"{root}/artifactory/api/storage/{repo_path}", f"{root}/artifactory/{repo_path}"


def _check(resp, url: str) -> None:
    """Turn a failed request into one sentence about what went wrong."""
    if resp.status_code in (401, 403):
        raise DownloadError(
            f"{resp.status_code} for {url}. Use --login, --auth-token, or a "
            f"`machine {urlparse(url).hostname}` entry in ~/.netrc. Clear a "
            "rejected stored token with --forget-login."
        )
    if resp.status_code == 404:
        raise DownloadError(f"404: nothing is published at {url}")
    if not resp.ok:
        raise DownloadError(f"HTTP {resp.status_code} for {url}")


def listing(session: requests.Session, storage: str, channel: str):
    """List component archives published on a channel."""
    url = f"{storage}/{channel}/{FIRMWARE_DIR}?list&deep=1&listFolders=0"
    log.debug("$ GET %s", url)
    resp = session.get(url, timeout=TIMEOUT)
    if resp.status_code == 404:
        raise DownloadError(f"nothing is published on the {channel} channel ({url})")
    _check(resp, url)
    try:
        files = resp.json()["files"]
    except (ValueError, KeyError, TypeError) as exc:
        raise DownloadError(f"{url} did not answer with a file listing ({exc})") from exc

    published = []
    for entry in files:
        uri = entry.get("uri", "")
        match = PACKAGE_RE.match(Path(uri).name)
        if match is None:
            log.debug("  %s: not a component archive, ignored", uri)
            continue
        if match["ident"] not in fw_bundle.COMPONENT_ROLES:
            log.debug("  %s: no known component identifier, ignored", uri)
            continue
        published.append(
            Package(
                ident=match["ident"],
                semver=match["semver"],
                soc=match["soc"],
                feature_level=(None if match["feature_level"] is None
                               else int(match["feature_level"])),
                name=Path(uri).name,
                uri=uri,
                size=int(entry.get("size", 0)),
                sha256=entry.get("sha2", ""),
                channel=match.groupdict().get("channel"),
                ref=match.groupdict().get("ref"),
                timestamp=match.groupdict().get("timestamp"),
                tree=channel,
            )
        )
    return published


def _package_date(pkg: Package) -> str | None:
    """YYYY-MM-DD from the filename timestamp field, if the name carries one."""
    if not pkg.timestamp:
        return None
    return pkg.timestamp[:10]


def _match_ref(got: str | None, wanted: str) -> bool:
    """True when @got equals @wanted or starts with it (short git sha)."""
    if not got:
        return False
    return got == wanted or got.startswith(wanted)


def _filter_soc_channel(published, soc: str, channel: str):
    """Packages for @soc whose file name claims @channel."""
    for_soc = [pkg for pkg in published if pkg.soc == soc]
    if not for_soc:
        offered = sorted({pkg.soc for pkg in published})
        raise DownloadError(
            f"nothing is published for {soc}; the SoCs on offer are "
            + (", ".join(offered) if offered else "none")
        )
    on_channel = [pkg for pkg in for_soc if pkg.released_on == channel]
    if not on_channel:
        claimed = sorted({pkg.released_on for pkg in for_soc})
        raise DownloadError(
            f"nothing published for {soc} in the {channel} tree is named as a "
            f"{channel} package; what is there names itself "
            + (", ".join(claimed) if claimed else "nothing")
        )
    return on_channel


def candidates(published, soc: str, channel: str, idents: list[str] | None = None):
    """Published packages for @soc on @channel, grouped by component identifier."""
    on_channel = _filter_soc_channel(published, soc, channel)
    if idents is not None:
        want = set(idents)
        on_channel = [pkg for pkg in on_channel if pkg.ident in want]
        missing = want - {pkg.ident for pkg in on_channel}
        if missing:
            raise DownloadError(
                f"nothing published for {', '.join(sorted(missing))} on the "
                f"{channel} channel for {soc}"
            )
    grouped: dict[str, list[Package]] = {}
    for pkg in on_channel:
        grouped.setdefault(pkg.ident, []).append(pkg)
    return grouped


def _resolve_dfu(newest: dict[str, Package], soc: str, channel: str, dfu):
    """Keep the requested DFU transport or the default."""
    flavours = [ident for ident in DFU_FLAVOURS.values() if ident in newest]
    if dfu is not None:
        wanted = DFU_FLAVOURS[dfu]
        if wanted not in newest:
            raise DownloadError(
                f"--dfu {dfu} names {wanted}, which is not published for {soc} on "
                f"the {channel} channel" + (f". Available: {flavours[0]}" if flavours else "")
            )
        for ident in flavours:
            if ident != wanted:
                del newest[ident]
    elif len(flavours) > 1:
        keep = DFU_FLAVOURS[DFU_DEFAULT]
        log.debug("both DFU flavours are published; taking %s (--dfu selects the other)", keep)
        for ident in flavours:
            if ident != keep:
                del newest[ident]


def _finalize_selection(newest: dict[str, Package], soc: str, channel: str):
    """Role exclusivity and stable ordering for a one-package-per-ident map."""
    by_role = {}
    for pkg in newest.values():
        clash = by_role.setdefault(pkg.role, pkg)
        if clash is not pkg:
            raise DownloadError(
                f"{clash.ident} and {pkg.ident} are both selected for {soc} on the "
                f"{channel} channel and are two candidates for the same {pkg.role} "
                f"image; a bundle names one"
            )
    order = {role: i for i, role in enumerate(fw_bundle.ALL_ROLES)}
    return sorted(newest.values(), key=lambda p: (order.get(p.role, len(order)), p.ident))


def latest(published, soc: str, channel: str, dfu, idents: list[str] | None = None):
    """The latest package of each component published for @soc on @channel."""
    grouped = candidates(published, soc, channel, idents)
    newest = {ident: max(pkgs, key=lambda p: p.rank) for ident, pkgs in grouped.items()}
    log.debug(
        "published for %s on %s: %s",
        soc,
        channel,
        ", ".join(f"{i} {p.semver}" for i, p in sorted(newest.items())),
    )
    _resolve_dfu(newest, soc, channel, dfu)
    return _finalize_selection(newest, soc, channel)


def select_by_date(
    published,
    soc: str,
    channel: str,
    date: str,
    ref: str | None,
    dfu,
    idents: list[str] | None = None,
):
    """One package per component from builds on @date, optionally pinned to @ref."""
    if channel == RELEASE_CHANNEL:
        raise DownloadError(
            "release packages are not dated in their names; use --latest, --by-version, or -i"
        )
    if not DATE_RE.match(date):
        raise DownloadError(f"--by-date: {date!r} is not YYYY-MM-DD")

    grouped = candidates(published, soc, channel, idents)
    on_date = {
        ident: [pkg for pkg in pkgs if _package_date(pkg) == date]
        for ident, pkgs in grouped.items()
    }
    missing = [ident for ident, pkgs in on_date.items() if not pkgs]
    if missing:
        raise DownloadError(
            f"nothing published for {', '.join(sorted(missing))} on the "
            f"{channel} channel for {soc} on {date}"
        )

    flat = [pkg for pkgs in on_date.values() for pkg in pkgs]
    refs = sorted({pkg.ref for pkg in flat if pkg.ref})
    if len(refs) > 1 and not ref:
        raise DownloadError(f"multiple builds on {date}: {', '.join(refs)}. Pass --ref to choose")
    if ref:
        filtered = [pkg for pkg in flat if _match_ref(pkg.ref, ref)]
        if not filtered:
            raise DownloadError(
                f"no packages on {date} match ref {ref!r} on the {channel} channel for {soc}"
            )
        flat = filtered

    newest: dict[str, Package] = {}
    for pkg in flat:
        best = newest.get(pkg.ident)
        if best is None or pkg.rank > best.rank:
            newest[pkg.ident] = pkg
    _resolve_dfu(newest, soc, channel, dfu)
    return _finalize_selection(newest, soc, channel)


def select_by_version(
    published, soc: str, channel: str, version: str, dfu, idents: list[str] | None = None
):
    """One release-channel package per component at @version."""
    if channel != RELEASE_CHANNEL:
        raise DownloadError(
            "--by-version is for the release channel only; on nightly use --by-date or --latest"
        )
    grouped = candidates(published, soc, channel, idents)
    selected: dict[str, Package] = {}
    missing = []
    for ident, pkgs in grouped.items():
        matches = [pkg for pkg in pkgs if pkg.semver == version]
        if not matches:
            missing.append(ident)
            continue
        selected[ident] = matches[0] if len(matches) == 1 else max(matches, key=lambda p: p.rank)
    if missing:
        raise DownloadError(
            f"nothing published for {', '.join(sorted(missing))} at version "
            f"{version} on the release channel for {soc}"
        )
    _resolve_dfu(selected, soc, channel, dfu)
    return _finalize_selection(selected, soc, channel)


def select_interactive(
    published,
    soc: str,
    channel: str,
    dfu,
    idents: list[str] | None = None,
    *,
    input_fn: Callable[[str], str] = input,
    isatty: Callable[[], bool] = sys.stdin.isatty,
):
    """Ask on a terminal which build to take for each component."""
    if not isatty():
        raise DownloadError("-i needs a terminal. Use --by-date, --by-version, or --latest.")

    grouped = candidates(published, soc, channel, idents)
    selected: dict[str, Package] = {}
    print(f"{channel} channel, {soc}. Select one build per component:\n")
    for ident in sorted(grouped):
        options = sorted(grouped[ident], key=lambda p: p.rank, reverse=True)
        print(f"{ident}:")
        for index, pkg in enumerate(options, 1):
            latest_mark = "  (latest)" if index == 1 else ""
            print(
                f"  [{index}] {pkg.feature_level_label:<5} {pkg.version:<20} {pkg.provenance:<28} "
                f"{pkg.size / 1000:8.1f} kB{latest_mark}"
            )
        print("  [s] skip")
        while True:
            raw = input_fn(f"Choice [1]: ").strip() or "1"
            if raw.lower() == "s":
                break
            if raw.isdigit() and 1 <= int(raw) <= len(options):
                selected[ident] = options[int(raw) - 1]
                break
            print(f"  enter 1-{len(options)}, or s to skip")
        print()

    if not selected:
        raise DownloadError("nothing selected")
    answer = input_fn(f"Selected {len(selected)} component(s). Proceed? [Y/n]: ").strip()
    if answer.lower() in ("n", "no"):
        raise DownloadError("cancelled")
    _resolve_dfu(selected, soc, channel, dfu)
    return _finalize_selection(selected, soc, channel)


def _resolve_selection_params(args, channel: str):
    """Selection mode: latest, by_date, by_version, or interactive."""
    if args.interactive:
        return "interactive"
    if args.by_date:
        return "by_date"
    if getattr(args, "by_version", None):
        return "by_version"
    if args.latest:
        return "latest"
    stated = sdk_channel()
    if channel == "nightly" and stated == "nightly":
        date, ref = sdk_provenance()
        if date:
            return "by_date"
    return "latest"


def resolve_selection(
    args, published, channel: str, *, idents: list[str] | None = None
) -> tuple[list[Package], str]:
    """Pick packages and the one-line report heading for the selection."""
    mode = _resolve_selection_params(args, channel)
    if mode == "interactive":
        selected = select_interactive(published, args.soc, channel, args.dfu, idents)
        heading = f"selected on the {channel} channel for {args.soc}:"
    elif mode == "by_date":
        date = args.by_date or sdk_provenance()[0]
        ref = args.ref
        if not args.by_date and sdk_channel() == "nightly" and channel == "nightly":
            _, ref = sdk_provenance()
        selected = select_by_date(published, args.soc, channel, date, ref, args.dfu, idents)
        if not args.by_date and sdk_channel() == "nightly" and channel == "nightly":
            sdk_date, sdk_ref = sdk_provenance()
            ref_note = f", ref {sdk_ref}" if sdk_ref else ""
            heading = (
                f"this IronSide Integration Kit's nightly ({sdk_date}{ref_note}) on the "
                f"{channel} channel for {args.soc}:"
            )
        else:
            ref_note = f", ref {ref}" if ref else ""
            heading = f"builds from {date}{ref_note} on the {channel} channel for {args.soc}:"
    elif mode == "by_version":
        selected = select_by_version(
            published, args.soc, channel, args.by_version, args.dfu, idents
        )
        heading = f"release {args.by_version} on the {channel} channel for {args.soc}:"
    else:
        selected = latest(published, args.soc, channel, args.dfu, idents)
        heading = f"latest on the {channel} channel for {args.soc}:"
    return selected, heading


def report_packages(packages: list[Package], heading: str) -> None:
    """The standard one-line-per-component selection report."""
    log.info("%s", heading.rstrip(":"))
    for pkg in packages:
        log.info(
            "  %-30s %-5s %-20s %-28s %8s",
            pkg.ident,
            pkg.feature_level_label,
            pkg.version,
            pkg.provenance,
            f"{pkg.size / 1000:.1f} kB",
        )


def report_release_dates(packages: list[Package], manifests: dict, verbose: bool) -> None:
    """The release date of the irot and urot packages, or of every package with @verbose."""
    shown = [pkg for pkg in packages if verbose or pkg.role in RELEASE_DATE_ROLES]
    levels = [
        f"{pkg.ident if verbose else pkg.role} "
        f"{manifests[pkg.ident].get('release_date') or 'not recorded'}"
        for pkg in shown
    ]
    if levels:
        log.info("release date: %s", ", ".join(levels))


def fetch(session: requests.Session, download: str, pkg: Package, dest: Path) -> Path:
    """Download one component archive and verify it against the repository."""
    url = f"{download}/{pkg.tree}/{FIRMWARE_DIR}{pkg.uri}"
    out = dest / pkg.name
    digest = hashlib.sha256()
    log.debug("$ GET %s", url)
    with session.get(url, stream=True, timeout=TIMEOUT) as resp:
        _check(resp, url)
        with out.open("wb") as fh:
            for chunk in resp.iter_content(CHUNK):
                digest.update(chunk)
                fh.write(chunk)
    got = digest.hexdigest()
    if not pkg.sha256:
        log.warning("warning: %s has no published SHA-256 checksum", pkg.name)
    elif got != pkg.sha256:
        raise DownloadError(f"{pkg.name}: SHA-256 mismatch. Expected {pkg.sha256}, got {got}.")
    return out


def inspect(path: Path, pkg: Package) -> dict:
    """The manifest of a downloaded package, once it agrees with its own name.

    A package that describes itself as something other than what it was published
    as is a mis-published artifact; catching it here keeps it from becoming a
    bundle member whose contents contradict its file name.
    """
    if not zipfile.is_zipfile(path):
        raise DownloadError(f"{pkg.name} is not a zip archive")
    with zipfile.ZipFile(path) as zf:
        names = set(zf.namelist())
        for member in (fw_bundle.IMAGE_MEMBER, fw_bundle.MANIFEST_MEMBER):
            if member not in names:
                raise DownloadError(
                    f"{pkg.name} has no {member}. It is not an IronSide TE component archive."
                )
        try:
            manifest = json.loads(zf.read(fw_bundle.MANIFEST_MEMBER))
        except json.JSONDecodeError as exc:
            raise DownloadError(
                f"{pkg.name}: {fw_bundle.MANIFEST_MEMBER} is not valid JSON ({exc})"
            ) from exc

    # The channel the name says it is on -- a word off the release channel, the
    # absence of one on it -- is checked like every other field - every name states
    # one, so there is no case in which there is nothing to hold the manifest to.
    agree = [
        ("name", pkg.ident),
        ("soc", pkg.soc),
        ("version", pkg.version),
        ("feature_level", pkg.feature_level),
        ("channel", pkg.released_on),
    ]
    for field, expected in agree:
        got = manifest.get(field)
        if got != expected:
            raise DownloadError(
                f"{pkg.name}: {fw_bundle.MANIFEST_MEMBER} says {field} is "
                f"{got!r}, the package is published as {expected!r}"
            )
    return manifest


def check_deps(manifests: dict) -> None:
    """Check minimum component versions declared by the selected packages."""
    versions = {ident: manifest.get("version") for ident, manifest in manifests.items()}
    for ident, manifest in sorted(manifests.items()):
        for dep, floor in sorted((manifest.get("deps") or {}).items()):
            have = versions.get(dep)
            if have is None:
                log.debug("  %s needs %s %s: not in this bundle, not checked", ident, dep, floor)
                continue
            if not satisfies(have, floor):
                raise DownloadError(
                    f"{ident} {manifest.get('version')} needs {dep} {floor} or newer, "
                    f"but the latest published {dep} is {have}"
                )
            log.debug("  %s needs %s %s: satisfied by %s", ident, dep, floor, have)


def write_bundle(paths, out: Path) -> None:
    """Write a flat ZIP containing the selected component archives."""
    with zipfile.ZipFile(out, "w", zipfile.ZIP_STORED) as zf:
        for path in sorted(paths, key=lambda p: p.name):
            info = zipfile.ZipInfo(path.name, date_time=FIXED_TIMESTAMP)
            info.external_attr = 0o644 << 16
            zf.writestr(info, path.read_bytes())


def open_session(args) -> tuple[requests.Session, str, str, str]:
    """An authorized session and (storage, download, channel) for @args."""
    channel = resolve_channel(args.channel, sdk_channel())
    storage, download = endpoints(args.url)
    session = requests.Session()
    source = artifactory_auth.authorize(session, args.url, token=args.auth_token, login=args.login)
    log.debug("authenticating with %s", source)
    return session, storage, download, channel


def download_selected(
    session: requests.Session, download: str, packages: list[Package], dest: Path
) -> tuple[list[Path], dict]:
    """Fetch @packages into @dest, inspect each, and verify deps across the set."""
    paths, manifests = [], {}
    for pkg in packages:
        path = fetch(session, download, pkg, dest)
        manifests[pkg.ident] = inspect(path, pkg)
        paths.append(path)
    check_deps(manifests)
    return paths, manifests


def fetch_packages_to_dir(args, dest: Path, *, idents: list[str] | None = None) -> list[Path]:
    """List, select, download into @dest, and return the local paths."""
    if not log.getLogger().handlers:
        log.basicConfig(
            format="%(message)s", level=log.DEBUG if getattr(args, "verbose", False) else log.INFO
        )
    apply_defaults(args)
    validate_selection(args)
    require_soc(args)
    require_url(args)
    session, storage, download, channel = open_session(args)
    log.debug("listing %s for %s on %s", args.url, args.soc, channel)
    published = listing(session, storage, channel)
    selected, heading = resolve_selection(args, published, channel, idents=idents)
    report_packages(selected, heading)
    if args.dry_run:
        log.info("dry run complete, nothing downloaded")
        return []
    dest.mkdir(parents=True, exist_ok=True)
    paths, manifests = download_selected(session, download, selected, dest)
    report_release_dates(selected, manifests, getattr(args, "verbose", False))
    return paths


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    add_arguments(ap, out=True)
    args = ap.parse_args(argv)
    apply_defaults(args)
    try:
        validate_selection(args)
        require_url(args)
    except DownloadError as exc:
        ap.error(str(exc))

    log.basicConfig(format="%(message)s", level=log.DEBUG if args.verbose else log.INFO)

    # Forgetting a credential does not require repository access.
    if args.forget_login:
        try:
            host = urlparse(artifactory_auth.platform_root(args.url)).hostname
            if artifactory_auth.forget(args.url):
                log.info("forgot the stored token for %s", host)
            else:
                log.info("no token was stored for %s", host)
        except artifactory_auth.AuthError as exc:
            log.error("%s", exc)
            return 1
        return 0

    try:
        require_soc(args)
        channel = resolve_channel(args.channel, sdk_channel())
        out = default_output_path(out=args.out, channel=channel, soc=args.soc)
        if not args.dry_run:
            refuse_existing(out, force=args.force)

        storage, download = endpoints(args.url)
        session = requests.Session()
        source = artifactory_auth.authorize(
            session, args.url, token=args.auth_token, login=args.login
        )
        log.debug("authenticating with %s", source)

        log.debug("listing %s for %s on %s", args.url, args.soc, channel)
        selected, heading = resolve_selection(args, listing(session, storage, channel), channel)
        report_packages(selected, heading)

        if args.dry_run:
            log.info("dry run complete, nothing downloaded")
            return 0

        # Keep component archives only when requested.
        with tempfile.TemporaryDirectory(prefix="ironside-download-") as scratch:
            dest = args.keep_components or Path(scratch)
            paths, manifests = download_selected(session, download, selected, dest)
            report_release_dates(selected, manifests, args.verbose)

            out.parent.mkdir(parents=True, exist_ok=True)
            write_bundle(paths, out)
            with tempfile.TemporaryDirectory(prefix="ironside-verify-") as probe:
                release = fw_bundle.read(out.resolve(), Path(probe), fw_bundle.ALL_ROLES)
            log.debug("%s", release.summary)
            if args.keep_components:
                log.debug("components: %s", dest)
    except (DownloadError, fw_bundle.BundleError, artifactory_auth.AuthError) as exc:
        return _fail(str(exc))
    except requests.RequestException as exc:
        return _fail(f"could not reach {args.url}: {exc}")

    log.info("created bundle")
    log.info("  %s", out)
    return 0


def _fail(msg: str) -> int:
    log.error("ironside_download: %s", msg)
    return 2


if __name__ == "__main__":
    sys.exit(main())
