#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Manage a keyset -- IronSide TE signing keys, OTA KEKs and application keys.

A keyset holds two unrelated kinds of key material, and the key id says which.
The IronSide keys are a closed set of 18 ids over three generations, one PEM or
raw KEK each:

  updater_g<gen>, app_g<gen>,            ECDSA P-384 signing keys of the DFU
  authopkey_g<gen>                       and App images and of the ADAC root
                                         certificate (lib/ecdsa.py)
  urot_p384_g<gen>, urot_mldsa65_g<gen>  the UROT signing keys: IROT verifies
                                         the UROT image with an ECDSA P-384
                                         and an ML-DSA-65 signature together
                                         (lib/hybrid.py), and a generation is
                                         the pair with that number
  fw_kek_g<gen>                          raw firmware encryption key encryption keys

An application key is `appkey_<id>`, an open set, and holds the pair the device
is provisioned with: the packed psa_key_attributes_t and the key material, both
hex. Application keys are optional -- a keyset with none yields a bundle
carrying none.
"""

import argparse
import base64
import json
import os
import sys
from pathlib import Path
from typing import NoReturn

sys.path.insert(0, str(Path(__file__).resolve().parent))

from lib import app_key_caps as caps  # noqa: E402
from lib import app_keys, ecdsa  # noqa: E402

# Import keyring lazily so directory sources work without the package installed.
_kr = None

SERVICE = "ironside-te-keyset"
# The roles signed with one ECDSA key: the key id prefix, "<prefix><gen>", and the
# curve. UROT is signed with a hybrid key pair instead.
ECDSA_KEYS = {
    "dfu": ("updater_g", ecdsa.P384),
    "app": ("app_g", ecdsa.P384),
    "authopkey": ("authopkey_g", ecdsa.P384),
}
ECDSA_ROLES = tuple(ECDSA_KEYS)
GENERATIONS = (0, 1, 2)
KEK_BYTES = 32

# keyring cannot enumerate entries, so keyset names use a separate index entry.
INDEX_ACCOUNT = "__keysets__"

# An application key is named by its PSA key ID, so the ids are an open set and
# need an index of their own, per keyset. A directory keyset needs none -- it globs.
APPKEY_PREFIX = "appkey_"
APPKEY_INDEX_ACCOUNT = "__appkeys__"

# Hybrid UROT signing keys: one ECDSA and one ML-DSA key per generation. The curve
# is lib.hybrid's ECDSA_CURVE, which needs intelhex to import.
HYBRID_ECDSA_PREFIX = "urot_p384_g"
HYBRID_ECDSA_CURVE = ecdsa.P384
HYBRID_MLDSA_PREFIX = "urot_mldsa65_g"

# How `list` describes the IronSide keys, by key id prefix in listing order: the
# key type and the use of generation 0.
LISTED_KEYS = {
    ECDSA_KEYS["dfu"][0]: (
        "ECDSA P-384 key pair", "Signs and verifies the IronSide TE Updater images"),
    ECDSA_KEYS["app"][0]: (
        "ECDSA P-384 key pair", "Signs and verifies application images"),
    ECDSA_KEYS["authopkey"][0]: (
        "ECDSA P-384 key pair", "Signs and verifies the ADAC authentication of a debugger"),
    HYBRID_ECDSA_PREFIX: (
        "ECDSA P-384 key pair", "Signs and verifies UROT images, with the ML-DSA-65 key"),
    HYBRID_MLDSA_PREFIX: (
        "ML-DSA-65 key pair", "Signs and verifies UROT images, with the ECDSA P-384 key"),
    "fw_kek_g": ("AES-256 key", "Firmware encryption key encryption key"),
}

# keyring's environment variable for selecting a backend.
BACKEND_ENV = "PYTHON_KEYRING_BACKEND"

# The alternative to an OS keyring this session cannot use, in place of keyring's own
# advice. Each message supplies its own lead-in verb.
DIR_KEYSET_REMEDY = (
    "--keyset dir:PATH, as the user guide describes under "
    "\"Generating a keyset into a directory\"."
)


def _keyring():
    """Import keyring on first use and select one backend."""
    global _kr
    if _kr is None:
        try:
            import keyring
            import keyring.backends.fail  # noqa: F401 - submodule, used via keyring.backends
            import keyring.errors  # noqa: F401 - submodule, used via keyring.errors
        except ImportError as exc:
            raise KeyringUnavailable(
                "the `keyring` package is not installed. Install it with "
                "`pip install keyring` or use --keyset dir:PATH."
            ) from exc
        _use_single_backend(keyring)
        _kr = keyring
    return _kr


class KeyringUnavailable(Exception):
    """No usable `keyring` package on this host (see _keyring())."""


def _use_single_backend(kr) -> None:
    """Use the highest-priority backend instead of chaining secret stores.

    Chained backends may open another wallet when a key is absent. An explicit
    PYTHON_KEYRING_BACKEND setting is always respected. Raises KeyringUnavailable
    when the backend in use is keyring's fail backend, which stores nothing.
    """
    try:
        backend = kr.get_keyring()
    except Exception as exc:
        raise KeyringUnavailable(
            f"`keyring` cannot resolve a backend on this host "
            f"({type(exc).__name__}: {exc}). Set {BACKEND_ENV} or use "
            "--keyset dir:PATH."
        ) from exc

    chained = getattr(backend, "backends", None)  # non-None only for a chainer
    if chained and not os.environ.get(BACKEND_ENV):
        backend = chained[0]
        kr.set_keyring(backend)

    if isinstance(backend, kr.backends.fail.Keyring):
        raise KeyringUnavailable(
            "no GNOME Keyring or KWallet answers over D-Bus in this session. "
            f"Use {DIR_KEYSET_REMEDY}"
        )


def backend_name() -> str:
    """Return the backend name without raising."""
    try:
        cls = type(_keyring().get_keyring())
    except Exception as exc:
        return f"unresolvable ({type(exc).__name__}: {exc})"
    return f"{cls.__module__}.{cls.__qualname__}"


def _fail(action: str, acct: str, exc: Exception) -> NoReturn:
    """Exit with keyring backend details and a directory-source alternative."""
    if isinstance(exc, _keyring().errors.KeyringLocked):
        action_hint = (
            "  The keyring is locked, and this session cannot show the unlock prompt "
            "(for example over SSH).\n"
            "  To keep using it, unlock it as the user guide describes under "
            "\"Generating a keyset into the OS keyring\".\n"
            f"  Otherwise, use {DIR_KEYSET_REMEDY}"
        )
    else:
        action_hint = f"  Set {BACKEND_ENV} or use --keyset dir:PATH."
    sys.exit(
        f"OS keyring: cannot {action} {SERVICE}/{acct} "
        f"({type(exc).__name__}: {exc})\n"
        f"  backend: {backend_name()}\n"
        f"{action_hint}"
    )


def _get(acct: str):
    """One entry's stored value, or None if it isn't there."""
    kr = _keyring()
    try:
        return kr.get_password(SERVICE, acct)
    except Exception as exc:
        _fail("read", acct, exc)


def _set(acct: str, value: str) -> None:
    kr = _keyring()
    try:
        kr.set_password(SERVICE, acct, value)
    except Exception as exc:
        _fail("write", acct, exc)


def _delete(acct: str) -> bool:
    """True if an entry was removed, False if there was nothing to remove."""
    kr = _keyring()
    try:
        kr.delete_password(SERVICE, acct)
        return True
    except kr.errors.PasswordDeleteError:  # nothing stored, not a failure -- precedes the catch-all
        return False
    except Exception as exc:
        _fail("delete", acct, exc)


def role_key_ids():
    return [key_id_for(role, gen) for role in ECDSA_ROLES for gen in GENERATIONS]


def kek_key_ids():
    return [f"fw_kek_g{gen}" for gen in GENERATIONS]


def all_key_ids():
    """The ids of a complete keyset."""
    return role_key_ids() + hybrid_key_ids() + kek_key_ids()


# Key ids earlier keyset layouts held, by role: the id prefix and the file suffix of
# a directory keyset. Nothing signs with them; `delete --all` still removes them so
# no orphaned secret stays behind in the keyring, and provisioning refuses a source
# that holds only them for a generation.
_RETIRED_KEYS = {
    "urot": (
        ("urot_g", "_ed25519.pem"),
        ("hybrid_ecdsa_g", "_p256.pem"),
        ("hybrid_mldsa_g", "_mldsa44.pem"),
        ("hybrid_mldsa87_g", "_mldsa87.pem"),
    ),
    "dfu": (("dfu_g", "_ed25519.pem"), ("dfu_ecdsa_g", "_p256.pem")),
    "app": (("app_ecdsa_g", "_p256.pem"),),
    "authopkey": (("authopkey_ecdsa_g", "_p256.pem"),),
}


def retired_key_ids(role: str, gen: int) -> list:
    """The ids an earlier keyset layout held one role/generation under."""
    return [f"{prefix}{gen}" for prefix, _suffix in _RETIRED_KEYS[role]]


_RETIRED_FILENAMES = {
    f"{prefix}{gen}": f"{prefix}{gen}{suffix}"
    for keys in _RETIRED_KEYS.values()
    for prefix, suffix in keys
    for gen in GENERATIONS
}
RETIRED_KEY_IDS = list(_RETIRED_FILENAMES)


def hybrid_key_ids():
    return [
        f"{prefix}{gen}"
        for prefix in (HYBRID_ECDSA_PREFIX, HYBRID_MLDSA_PREFIX)
        for gen in GENERATIONS
    ]


def ecdsa_curve_for(key_id: str):
    """The curve of the ECDSA key @key_id holds, or None if it holds no ECDSA key."""
    prefixes = [(HYBRID_ECDSA_PREFIX, HYBRID_ECDSA_CURVE), *ECDSA_KEYS.values()]
    for prefix, curve in prefixes:
        if key_id in (f"{prefix}{gen}" for gen in GENERATIONS):
            return curve
    return None


def is_hybrid_id(key_id: str) -> bool:
    return any(
        key_id in (f"{prefix}{gen}" for gen in GENERATIONS)
        for prefix in (HYBRID_ECDSA_PREFIX, HYBRID_MLDSA_PREFIX)
    )


def is_kek_id(key_id: str) -> bool:
    return key_id.startswith("fw_kek_g")


def is_appkey_id(key_id: str) -> bool:
    return key_id.startswith(APPKEY_PREFIX)


def appkey_id(psa_key_id: int) -> str:
    """Key id of one application key ("appkey_164"). The PSA key ID is the name."""
    return f"{APPKEY_PREFIX}{psa_key_id}"


def appkey_psa_key_id(key_id: str) -> int:
    """The PSA key ID an application-key id names."""
    return int(key_id[len(APPKEY_PREFIX):])


def filename_for(key_id: str) -> str:
    if is_appkey_id(key_id):
        return f"{key_id}.json"
    if ecdsa_curve_for(key_id) is not None or is_hybrid_id(key_id):
        return f"{key_id}.pem"
    if is_kek_id(key_id):
        return f"{key_id}.bin"
    if key_id in _RETIRED_FILENAMES:  # looked up only to name it
        return _RETIRED_FILENAMES[key_id]
    raise KeySourceError(f"{key_id} is not an IronSide key id")


def account(keyset: str, key_id: str) -> str:
    return f"{keyset}/{key_id}"


def get_value(keyset: str, key_id: str):
    """Return stored PEM or base64 text for one key, or None."""
    return _get(account(keyset, key_id))


def load_index():
    """Return keyset names from this tool's index."""
    raw = _get(INDEX_ACCOUNT)
    return sorted(json.loads(raw)) if raw else []


def index_add(keyset: str) -> None:
    names = set(load_index())
    if keyset not in names:
        names.add(keyset)
        _set(INDEX_ACCOUNT, json.dumps(sorted(names)))


def index_remove(keyset: str) -> None:
    names = set(load_index())
    if keyset not in names:
        return
    names.discard(keyset)
    if names:
        _set(INDEX_ACCOUNT, json.dumps(sorted(names)))
    else:
        _delete(INDEX_ACCOUNT)


def load_appkey_index(keyset: str) -> list:
    """The PSA key IDs this keyset holds an application key for, lowest first."""
    raw = _get(account(keyset, APPKEY_INDEX_ACCOUNT))
    return sorted(json.loads(raw)) if raw else []


def appkey_ids(keyset: str) -> list:
    """Every application-key id in one keyring keyset."""
    return [appkey_id(psa_key_id) for psa_key_id in load_appkey_index(keyset)]


def appkey_index_add(keyset: str, psa_key_id: int) -> None:
    ids = set(load_appkey_index(keyset))
    if psa_key_id not in ids:
        ids.add(psa_key_id)
        _set(account(keyset, APPKEY_INDEX_ACCOUNT), json.dumps(sorted(ids)))


def appkey_index_remove(keyset: str, psa_key_id: int) -> None:
    ids = set(load_appkey_index(keyset))
    if psa_key_id not in ids:
        return
    ids.discard(psa_key_id)
    if ids:
        _set(account(keyset, APPKEY_INDEX_ACCOUNT), json.dumps(sorted(ids)))
    else:
        _delete(account(keyset, APPKEY_INDEX_ACCOUNT))


def keyset_has_any_entry(keyset: str) -> bool:
    """Whether a keyset holds anything at all, of either kind.

    Both kinds count: a keyset holding only application keys must stay indexed,
    or `list` would stop naming it.
    """
    if load_appkey_index(keyset):
        return True
    return any(_get(account(keyset, k)) is not None for k in all_key_ids() + RETIRED_KEY_IDS)


def appkey_text(metadata: bytes, material: bytes) -> str:
    """One application key as a keyset stores it.

    The packed psa_key_attributes_t and the key material, hex, which is the pair
    the provisioning batch carries and the device is provisioned with. One text
    for both backends, so an export and an import round-trip byte for byte.
    """
    return (
        json.dumps(
            {"metadata": "0x" + metadata.hex().upper(), "value": "0x" + material.hex()},
            indent=4,
        )
        + "\n"
    )


def appkey_entry(text: str, where: str) -> dict:
    """The {metadata, value} pair @text holds, or a refusal naming @where."""
    try:
        entry = json.loads(text)
    except ValueError as exc:
        raise app_keys.AppKeyError(f"{where} is not readable JSON ({exc})") from None
    if not isinstance(entry, dict):
        raise app_keys.AppKeyError(f"{where} is not a JSON object")
    return entry


def appkey_entries(ids, read, label) -> list:
    """Every application key @ids names, each validated under its own name.

    Validated one at a time so a refusal names the key that caused it rather
    than its position in a list. The caller validates the whole list afterwards,
    which is what catches an ID two keys share.
    """
    entries = []
    for key_id in ids:
        where = label(key_id)
        entry = appkey_entry(read(key_id), where)
        app_keys.parse_entries([entry], where)
        entries.append(entry)
    return entries


def load_bytes(path: Path, key_id: str) -> bytes:
    """Read and validate a signing key, a raw KEK or an application key."""
    data = path.read_bytes()
    if is_appkey_id(key_id):
        text = data.decode("utf-8", errors="replace")
        entry = appkey_entry(text, str(path))
        (key,) = app_keys.parse_entries([entry], str(path))
        if key.key_id != appkey_psa_key_id(key_id):
            sys.exit(
                f"{path}: names key ID {appkey_psa_key_id(key_id)}, and its attributes "
                f"carry key ID {key.key_id}"
            )
        return data
    try:
        check_key_material(key_id, data)
    except KeySourceError as exc:
        sys.exit(f"{path}: {exc}")
    return data


def check_key_material(key_id: str, data: bytes) -> None:
    """Raise KeySourceError unless @data is the kind of key IronSide id @key_id holds.

    That is a raw KEK, or an ECDSA or ML-DSA private key as unencrypted PEM.
    """
    if is_kek_id(key_id):
        if len(data) != KEK_BYTES:
            raise KeySourceError(f"expected {KEK_BYTES} raw bytes for {key_id}, got {len(data)}")
        return
    curve = ecdsa_curve_for(key_id)
    if curve is not None:
        try:
            curve.load_private_key(data, label=key_id)
        except (ValueError, TypeError) as exc:
            raise KeySourceError(str(exc)) from exc
        return
    if is_hybrid_id(key_id):
        from lib import hybrid  # noqa: PLC0415 - lib.hybrid needs intelhex; key handling does not

        try:
            hybrid.load_mldsa_key(data, label=key_id)
        except (ValueError, TypeError) as exc:
            raise KeySourceError(f"not an unencrypted PEM private key ({exc})") from exc
        except hybrid.mci.SignError as exc:
            raise KeySourceError(str(exc)) from exc
        return
    raise KeySourceError(f"{key_id} is not an IronSide key id")


def decode_value(key_id: str, value: str) -> bytes:
    """Decode stored text into key-file bytes.

    An application key is stored as its JSON text, like a PEM and unlike a raw
    KEK, so it reads back as the file an export writes.
    """
    return base64.b64decode(value) if is_kek_id(key_id) else value.encode("ascii")


def store_value(keyset: str, key_id: str, data: bytes) -> None:
    """Encode and store key bytes under keyset/key-id."""
    value = base64.b64encode(data).decode("ascii") if is_kek_id(key_id) else data.decode("ascii")
    _set(account(keyset, key_id), value)
    index_add(keyset)
    if is_appkey_id(key_id):
        appkey_index_add(keyset, appkey_psa_key_id(key_id))


# `new` targets may not exist yet, so they are parsed separately from KeySource.


def generate_bytes(key_id: str) -> bytes:
    """Fresh material for one key id: a raw OTA KEK, or a signing key."""
    curve = ecdsa_curve_for(key_id)
    if curve is not None:
        return curve.generate_private_pem()
    if is_hybrid_id(key_id):
        from lib import hybrid  # noqa: PLC0415 - lib.hybrid needs intelhex; key handling does not

        return hybrid.generate_mldsa_pem()
    if is_kek_id(key_id):
        return os.urandom(KEK_BYTES)
    raise KeySourceError(f"{key_id} is not an IronSide key id")


class _KeyringTarget:
    """A keyset created in the OS keyring."""

    def __init__(self, keyset: str):
        self.keyset = keyset

    def describe(self) -> str:
        return f"the OS keyring, keyset {self.keyset!r}"

    def has(self, key_id: str) -> bool:
        return _get(account(self.keyset, key_id)) is not None

    def put(self, key_id: str, data: bytes) -> None:
        store_value(self.keyset, key_id, data)

    def remove(self, key_id: str) -> bool:
        """True if the key was removed, False if the keyset did not hold it."""
        if not _delete(account(self.keyset, key_id)):
            return False
        if is_appkey_id(key_id):
            appkey_index_remove(self.keyset, appkey_psa_key_id(key_id))
        return True

    def appkey_ids(self) -> list:
        return appkey_ids(self.keyset)

    def read_appkey(self, key_id: str) -> str:
        return _get(account(self.keyset, key_id)) or ""

    def finish(self) -> None:
        """Index the keyset while it holds anything, and drop it once it holds nothing.

        Also indexes complete keysets created before the index existed.
        """
        if keyset_has_any_entry(self.keyset):
            index_add(self.keyset)
        else:
            index_remove(self.keyset)

    def hints(self):
        return [
            f"sign an image    : scripts/sign.py [--role urot|app|dfu] <image> "
            f"-o <signed.hex> --keyset keyring:{self.keyset}",
            f"base config      : scripts/ironside_bundle.py update-config <bundle.zip> "
            f"--slot1-size-kb 128 --storage-size-kb 64 --dfuimage-size-kb 32",
            f"export to files  : scripts/iron_keychain.py export {self.keyset} --out-dir <dir>",
        ]


class _DirTarget:
    """A keyset created as files in a directory."""

    def __init__(self, path: Path):
        self.path = path

    def describe(self) -> str:
        return f"directory {self.path}"

    def has(self, key_id: str) -> bool:
        return (self.path / filename_for(key_id)).exists()

    def put(self, key_id: str, data: bytes) -> None:
        self.path.mkdir(parents=True, exist_ok=True)
        path = self.path / filename_for(key_id)
        path.write_bytes(data)
        path.chmod(0o600)

    def remove(self, key_id: str) -> bool:
        """True if the key file was removed, False if the directory did not hold it."""
        try:
            (self.path / filename_for(key_id)).unlink()
        except FileNotFoundError:
            return False
        return True

    def appkey_ids(self) -> list:
        return dir_appkey_ids(self.path)

    def read_appkey(self, key_id: str) -> str:
        return (self.path / filename_for(key_id)).read_text(encoding="utf-8")

    def finish(self) -> None:
        pass

    def hints(self):
        return [
            f"sign an image    : scripts/sign.py [--role urot|app|dfu] <image> "
            f"-o <signed.hex> --keyset dir:{self.path}",
            f"base config      : scripts/ironside_bundle.py update-config <bundle.zip> "
            f"--slot1-size-kb 128 --storage-size-kb 64 --dfuimage-size-kb 32",
            f"store in keyring : scripts/iron_keychain.py new "
            f"--keyset <keyset-name> --from-dir {self.path}",
        ]


def dir_appkey_ids(path: Path) -> list:
    """Every application-key id a directory keyset holds, lowest key ID first.

    A directory needs no index: the filenames are the enumeration. A name whose
    key ID is not a number is left out rather than failing the listing -- the
    directory is not exclusively ours.
    """
    ids = []
    for found in path.glob(f"{APPKEY_PREFIX}*.json"):
        try:
            psa_key_id = appkey_psa_key_id(found.stem)
        except ValueError:
            continue
        ids.append((psa_key_id, found.stem))
    return [key_id for _, key_id in sorted(ids)]


def new_target(spec: str):
    """Parse a writable keyset target. A bare name selects the OS keyring.

    Separate from KeySource because a target need not exist yet: `new` creates
    one, and add_application_key.py may add the first key a keyset holds.
    """
    if ":" not in spec:
        return _KeyringTarget(spec)
    backend, _, rest = spec.partition(":")
    if backend not in ("keyring", "dir") or not rest:
        sys.exit(f"keyset {spec!r}: expected <keyset>, keyring:<keyset> or dir:<path>")
    return _KeyringTarget(rest) if backend == "keyring" else _DirTarget(Path(rest))


# Consumers access keyring and directory sources through the same KeySource API.
# New keysets default to "dev". Readers default to the sole indexed keyset.
DEFAULT_KEYSET = "dev"
DEFAULT_KEYSET_SPEC = f"keyring:{DEFAULT_KEYSET}"


class KeySourceError(Exception):
    """A key source could not be opened, or lacks keys a caller requires."""


def sole_keyset_spec() -> str:
    """Return the sole indexed keyring keyset or raise if ambiguous."""
    try:
        names = load_index()
    except KeyringUnavailable as exc:
        raise KeySourceError(f"no --keyset given and the OS keyring cannot be read: {exc}") from exc
    if len(names) == 1:
        return f"keyring:{names[0]}"
    if not names:
        raise KeySourceError(
            f"no --keyset given and no keyset is indexed in the OS keyring "
            f"({backend_name()}). Create one with `iron_keychain.py new` or use "
            "--keyset keyring:NAME or --keyset dir:PATH."
        )
    raise KeySourceError(
        f"no --keyset given and the OS keyring holds {len(names)} keysets "
        f"({', '.join(names)}). Select one with --keyset keyring:NAME."
    )


def key_id_for(role: str, gen: int) -> str:
    """Key id of the single signing key of one role/generation ("app_g0")."""
    if role in ECDSA_KEYS:
        return f"{ECDSA_KEYS[role][0]}{gen}"
    raise ValueError(f"{role!r} has no single signing key (one of {', '.join(ECDSA_ROLES)})")


def signing_key_ids(role: str, gen: int) -> tuple:
    """Key ids that sign one role/generation: the hybrid pair for urot, else one id."""
    return hybrid_key_id_pair(gen) if role == "urot" else (key_id_for(role, gen),)


def hybrid_key_id_pair(gen: int) -> tuple:
    """Key ids of one hybrid generation: (ECDSA, ML-DSA)."""
    return f"{HYBRID_ECDSA_PREFIX}{gen}", f"{HYBRID_MLDSA_PREFIX}{gen}"


def kek_key_id(gen: int) -> str:
    """Key id of one OTA-KEK generation ("fw_kek_g0")."""
    return f"fw_kek_g{gen}"


class KeySource:
    """Base class; use KeySource.parse() rather than instantiating a subclass."""

    @staticmethod
    def parse(spec: str) -> "KeySource":
        """Resolve a --keyset spec. The backend is explicit in the spelling so one
        flag covers both kinds and a bare name cannot be ambiguous."""
        backend, sep, rest = spec.partition(":")
        if not sep:
            raise KeySourceError(f"--keyset {spec!r}: expected keyring:<keyset> or dir:<path>")
        if backend == "keyring":
            if not rest:
                raise KeySourceError("--keyset keyring:<keyset>: keyset name is empty")
            return KeyringKeySource(rest)
        if backend == "dir":
            return DirKeySource(rest)
        raise KeySourceError(f"--keyset {spec!r}: unknown backend {backend!r} (keyring | dir)")

    def describe(self) -> str:
        raise NotImplementedError

    def label(self, key_id: str) -> str:
        """Where one key lives, for a message naming it."""
        raise NotImplementedError

    def get_bytes(self, key_id: str):
        """Return PEM or raw KEK bytes, or None when absent."""
        raise NotImplementedError

    def path_for(self, key_id: str):
        """The key's own path, for a dir source; None for a keyring source (its
        keys have no path until materialize() gives them one)."""
        return None

    def get_required(self, key_id: str) -> bytes:
        """get_bytes(), raising rather than returning None."""
        data = self.get_bytes(key_id)
        if data is None:
            raise KeySourceError(f"no {key_id} in {self.describe()}")
        return data

    def missing(self, key_ids) -> list:
        """Which of @key_ids this source does not hold, as human labels."""
        return [self.label(k) for k in key_ids if self.get_bytes(k) is None]

    def validate(self, key_ids, what: str = "key") -> None:
        """Raise KeySourceError listing every required key that is absent.

        This validation never generates keys.
        """
        absent = self.missing(key_ids)
        if absent:
            raise KeySourceError(
                f"{self.describe()} is missing {len(absent)} required {what}(s):\n  "
                + "\n  ".join(absent)
            )

    def appkey_ids(self) -> list:
        """Every application-key id this source holds, lowest key ID first."""
        raise NotImplementedError

    def read_appkey(self, key_id: str) -> str:
        """The stored JSON text of one application key."""
        raise NotImplementedError

    def app_key_entries(self) -> list:
        """The {metadata, value} pair of every application key this source holds.

        Not validated here: the caller runs app_keys.parse_entries over the whole
        list, which is what refuses a key whose span another one covers.
        """
        return appkey_entries(self.appkey_ids(), self.read_appkey, self.label)

    def materialize(self, key_id: str, out_dir: Path) -> Path:
        """Return a key path, writing keyring material with mode 0600 if needed."""
        existing = self.path_for(key_id)
        if existing is not None:
            return existing
        out_dir.mkdir(parents=True, exist_ok=True)
        path = out_dir / filename_for(key_id)
        path.write_bytes(self.get_required(key_id))
        path.chmod(0o600)
        return path


class DirKeySource(KeySource):
    """A directory of key files (iron_keychain new --keyset dir: / export)."""

    def __init__(self, path):
        self.path = Path(path)
        if not self.path.is_dir():
            raise KeySourceError(f"--keyset dir:{self.path}: not a directory")

    def describe(self):
        return f"directory {self.path}"

    def label(self, key_id):
        return str(self.path / filename_for(key_id))

    def path_for(self, key_id):
        p = self.path / filename_for(key_id)
        return p if p.is_file() else None

    def appkey_ids(self):
        return dir_appkey_ids(self.path)

    def read_appkey(self, key_id):
        return (self.path / filename_for(key_id)).read_text(encoding="utf-8")

    def get_bytes(self, key_id):
        p = self.path_for(key_id)
        if p is None:
            return None
        data = p.read_bytes()
        if is_kek_id(key_id) and len(data) != KEK_BYTES:
            raise KeySourceError(
                f"{p}: expected {KEK_BYTES} raw bytes for {key_id}, got {len(data)}"
            )
        return data


class KeyringKeySource(KeySource):
    """An OS-keyring keyset stored by this module."""

    def __init__(self, keyset):
        self.keyset = keyset
        try:
            _keyring()  # fail here, not on the first key read
        except KeyringUnavailable as exc:
            raise KeySourceError(f"--keyset keyring:{keyset}: {exc}") from exc

    def describe(self):
        return f"keyring keyset {self.keyset!r}"

    def label(self, key_id):
        return account(self.keyset, key_id)

    def appkey_ids(self):
        return appkey_ids(self.keyset)

    def read_appkey(self, key_id):
        return get_value(self.keyset, key_id) or ""

    def get_bytes(self, key_id):
        value = get_value(self.keyset, key_id)
        return None if value is None else decode_value(key_id, value)


def cmd_list_keysets() -> int:
    """Print indexed keyset names."""
    names = load_index()
    print(f"store: {backend_name()}")
    if not names:
        print("no indexed keysets")
        return 0
    print("known keysets:")
    for name in names:
        print(f"  {name}")
    return 0


def cmd_new(args) -> int:
    """Generate or import all keys into a keyset."""
    if args.from_dir is not None and not args.from_dir.is_dir():
        sys.exit(f"{args.from_dir}: not a directory")
    importing = args.from_dir is not None

    target = new_target(args.keyset or DEFAULT_KEYSET_SPEC)

    wanted = list(all_key_ids())
    if importing:
        # Application keys are an open set, so the directory is the enumeration.
        wanted += dir_appkey_ids(args.from_dir)

    filled, kept, absent = [], [], []
    for key_id in wanted:
        if target.has(key_id) and not args.force:
            kept.append(key_id)
            continue
        if importing:
            path = args.from_dir / filename_for(key_id)
            if not path.exists():
                absent.append(key_id)
                continue
            data = load_bytes(path, key_id)
        else:
            data = generate_bytes(key_id)
        target.put(key_id, data)
        filled.append(key_id)
    target.finish()

    if importing and not filled and not kept:
        sys.exit(f"{args.from_dir}: none of the {len(wanted)} known keyset files found")

    verb = "imported" if importing else "generated"
    print(
        f"keyset in {target.describe()}: {verb} {len(filled)}, kept {len(kept)}"
        + (f", absent {len(absent)}" if absent else "")
    )
    if filled:
        print(f"  {verb}: {', '.join(filled)}")
    if kept:
        print(
            f"  kept (use --force to {'overwrite' if importing else 'regenerate'}): "
            f"{', '.join(kept)}"
        )
    if absent:
        print(f"  not in {args.from_dir}: {', '.join(absent)}")
    return 0


def cmd_list(args) -> int:
    if args.keyset is None:
        return cmd_list_keysets()
    rows = listed_key_rows()
    id_width = max(len(key_id) for key_id, _type, _use in rows)
    type_width = max(len(key_type) for _id, key_type, _use in rows)
    print("IronSide keys:")
    for key_id, key_type, use in rows:
        present = _get(account(args.keyset, key_id)) is not None
        print(f"  {'x' if present else '.'} {key_id:<{id_width}}  {key_type:<{type_width}}  {use}")
    print("application keys:")
    ids = appkey_ids(args.keyset)
    if not ids:
        print("  (none)")
        return 0
    for line in describe_appkeys(
        [(key_id, _get(account(args.keyset, key_id)) or "") for key_id in ids],
        args.keyset,
    ):
        print(f"  {line}")
    return 0


def listed_key_rows() -> list[tuple[str, str, str]]:
    """The IronSide key rows of `list`: (key id, key type, use)."""
    return [
        (f"{prefix}{gen}", key_type, use if gen == 0 else "Next generation")
        for prefix, (key_type, use) in LISTED_KEYS.items()
        for gen in GENERATIONS
    ]


def describe_appkeys(stored, keyset: str) -> list:
    """Two lines per application key: its shape, then the policy it carries.

    The PSA attributes are read back out of the stored blob rather than out of
    anything the tool remembers separately, so a listing cannot describe a key
    differently from how the device will be provisioned with it. A value with no
    name prints as a number rather than being hidden.
    """
    types = {caps.KEY_TYPES[n].value: n for n in caps.KEY_TYPES}
    algorithms = {caps.ALGORITHMS[n].value: n for n in caps.ALGORITHMS}
    persistences = {caps.PERSISTENCE[n]: n for n in caps.PERSISTENCE}

    lines = []
    for key_id, text in stored:
        where = account(keyset, key_id)
        try:
            (key,) = app_keys.parse_entries([appkey_entry(text, where)], where)
        except app_keys.AppKeyError as exc:
            lines.append(f"key {appkey_psa_key_id(key_id):<6}unreadable: {exc}")
            continue
        usage = [name for name, bit in caps.USAGE.items() if key.usage & bit == bit]
        if key.usage & caps.USAGE_EXPORT:
            usage.append("EXPORT")
        shape = types.get(key.key_type, f"key type 0x{key.key_type:04X}")
        lines.append(f"key {key.key_id:<6}{shape} {key.key_bits}-bit")
        lines.append(
            f"          {algorithms.get(key.alg, f'algorithm 0x{key.alg:08X}')}"
            f"  usage {', '.join(usage) or '(none)'}"
            f"  {persistences.get(key.persistence, f'persistence {key.persistence}')}"
        )
    return lines


def store_appkey(target, metadata: bytes, material: bytes, *, force: bool = False) -> int:
    """Store one application key in @target. Returns its PSA key ID.

    The key ID comes out of the attributes rather than from a separate argument,
    so a caller cannot name one key ID and provision another. The whole keyset
    is then validated together.
    """
    (incoming,) = app_keys.parse_entries(
        [appkey_entry(appkey_text(metadata, material), "the given key")], "the given key"
    )
    key_id = appkey_id(incoming.key_id)
    if target.has(key_id) and not force:
        raise app_keys.AppKeyError(
            f"{target.describe()} already holds a key with key ID {incoming.key_id}. "
            f"Pass --force to replace it, or choose another key ID."
        )

    held = appkey_entries(
        [held_id for held_id in target.appkey_ids() if held_id != key_id],
        target.read_appkey,
        lambda held_id: f"{target.describe()}, {held_id}",
    )
    app_keys.parse_entries(
        [*held, appkey_entry(appkey_text(metadata, material), "the given key")],
        target.describe(),
    )

    target.put(key_id, appkey_text(metadata, material).encode("ascii"))
    target.finish()
    return incoming.key_id


def cmd_add_app_key(args) -> int:
    """Store an application key from its packed attributes and its material."""
    target = new_target(args.keyset or DEFAULT_KEYSET_SPEC)
    try:
        metadata = _hex_arg("--metadata", args.metadata)
        material = _hex_arg("--value", args.value)
        psa_key_id = store_appkey(target, metadata, material, force=args.force)
    except app_keys.AppKeyError as exc:
        sys.exit(f"add-app-key: {exc}")
    print(f"{target.describe()}: application key with key ID {psa_key_id}")
    return 0


def _hex_arg(flag: str, text: str) -> bytes:
    body = text[2:] if text[:2].lower() == "0x" else text
    try:
        return bytes.fromhex(body)
    except ValueError:
        sys.exit(f"add-app-key: {flag} is not hex: {text!r}")


def cmd_get(args) -> int:
    if args.keyset is None:
        return cmd_list_keysets()
    if args.key_id is None:
        sys.exit(
            f"get: KEY-ID required, choose an IronSide key "
            f"({', '.join(all_key_ids())}) or an application key "
            f"({APPKEY_PREFIX}<id>; `list {args.keyset}` names the ones held)"
        )
    acct = account(args.keyset, args.key_id)
    value = _get(acct)
    if value is None:
        sys.exit(f"no keyring entry for {acct}")

    if is_kek_id(args.key_id) and args.out is None:
        sys.exit(f"{args.key_id} is binary; pass -o to write it to a file")
    if args.out is not None:
        args.out.write_bytes(decode_value(args.key_id, value))
        args.out.chmod(0o600)
        print(f"wrote {args.out}")
    else:
        sys.stdout.write(value)
    return 0


def cmd_export(args) -> int:
    if args.keyset is None:
        return cmd_list_keysets()
    if args.out_dir is None:
        sys.exit("export: --out-dir is required")

    args.out_dir.mkdir(parents=True, exist_ok=True)

    exported, absent, skipped_exists = [], [], []
    for key_id in all_key_ids() + appkey_ids(args.keyset):
        value = _get(account(args.keyset, key_id))
        if value is None:
            absent.append(key_id)
            continue
        data = decode_value(key_id, value)
        path = args.out_dir / filename_for(key_id)
        if path.exists() and not args.force:
            skipped_exists.append(key_id)
            continue
        path.write_bytes(data)
        path.chmod(0o600)
        exported.append(key_id)

    if not exported and not skipped_exists:
        sys.exit(f"keyset {args.keyset!r}: no entries to export")

    print(
        f"keyset {args.keyset!r}: exported {len(exported)} to {args.out_dir}, "
        f"absent {len(absent)}, already present {len(skipped_exists)}"
    )
    if skipped_exists:
        print(f"  already present (use --force to overwrite): {', '.join(skipped_exists)}")
    return 1 if skipped_exists else 0


def cmd_delete(args) -> int:
    if args.keyset is None:
        return cmd_list_keysets()
    if bool(args.key_id) == bool(args.all):
        sys.exit("delete: specify either KEY-ID or --all")
    target = new_target(args.keyset)
    ids = all_key_ids() + RETIRED_KEY_IDS + target.appkey_ids() if args.all else [args.key_id]

    removed = [key_id for key_id in ids if target.remove(key_id)]
    target.finish()
    if not removed:
        sys.exit(f"{target.describe()}: nothing to delete")

    print(f"{target.describe()}: deleted {', '.join(removed)}")
    return 0


def _key_id_arg(text: str) -> str:
    """One key id of either kind.

    The IronSide ids are a closed set and an application key's is `appkey_<id>`
    for any PSA key ID, so argparse `choices` cannot state both. Named here instead,
    so a typo is refused with a message that says which spaces exist.
    """
    if text in all_key_ids():
        return text
    if is_appkey_id(text):
        try:
            appkey_psa_key_id(text)
        except ValueError:
            raise argparse.ArgumentTypeError(
                f"{text!r}: an application key is {APPKEY_PREFIX}<id>, and "
                f"{text[len(APPKEY_PREFIX):]!r} is not a key ID"
            ) from None
        return text
    raise argparse.ArgumentTypeError(
        f"{text!r}: choose an IronSide key ({', '.join(all_key_ids())}) or an "
        f"application key ({APPKEY_PREFIX}<id>)"
    )


def main() -> int:
    # Consumers import this module without running the CLI version check.
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from lib.python_version import require_python

    require_python()

    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="command", required=True, title="commands", metavar="COMMAND")

    new = sub.add_parser("new", help="generate or import a keyset")
    new.add_argument(
        "--keyset",
        dest="keyset",
        metavar="SPEC",
        help=f"target as keyring:NAME, dir:PATH, or a keyring name "
        f"(default: {DEFAULT_KEYSET_SPEC})",
    )
    new.add_argument(
        "--from-dir",
        type=Path,
        metavar="DIR",
        help="import key files from DIR instead of generating them",
    )
    new.add_argument(
        "--force", action="store_true", help="replace existing keys (default: keep them)"
    )
    new.set_defaults(func=cmd_new)

    add = sub.add_parser(
        "add-app-key",
        help="store one application key from its packed attributes and material",
    )
    add.add_argument(
        "--keyset",
        dest="keyset",
        metavar="SPEC",
        help=f"target as keyring:NAME, dir:PATH, or a keyring name "
        f"(default: {DEFAULT_KEYSET_SPEC})",
    )
    add.add_argument(
        "--metadata",
        required=True,
        metavar="HEX",
        help="the packed psa_key_attributes_t, which carries the key ID. "
        "add_application_key.py builds it",
    )
    add.add_argument(
        "--value", required=True, metavar="HEX", help="the key material"
    )
    add.add_argument(
        "--force", action="store_true", help="replace a key with the same key ID"
    )
    add.set_defaults(func=cmd_add_app_key)

    lst = sub.add_parser("list", help="list keysets or keys")
    lst.add_argument("keyset", nargs="?", help="keyset name (default: list keysets)")
    lst.set_defaults(func=cmd_list)

    get = sub.add_parser("get", help="read one keyring key")
    get.add_argument("keyset", nargs="?", help="keyset name (default: list keysets)")
    get.add_argument("key_id", nargs="?", metavar="KEY-ID", type=_key_id_arg)
    get.add_argument(
        "-o",
        "--out",
        type=Path,
        help="write to FILE with mode 0600 instead of stdout",
        metavar="FILE",
    )
    get.set_defaults(func=cmd_get)

    export = sub.add_parser("export", help="export a keyset to a directory")
    export.add_argument("keyset", nargs="?", help="keyset name (default: list keysets)")
    export.add_argument("--out-dir", type=Path, help="output directory")
    export.add_argument(
        "--force", action="store_true", help="overwrite existing files in --out-dir"
    )
    export.set_defaults(func=cmd_export)

    delete = sub.add_parser("delete", help="remove one or all entries for a keyset")
    delete.add_argument(
        "keyset",
        nargs="?",
        help="keyset as keyring:NAME, dir:PATH, or a keyring name (default: list keysets)",
    )
    delete.add_argument("key_id", nargs="?", metavar="KEY-ID", type=_key_id_arg)
    delete.add_argument("--all", action="store_true", help="delete every present key id")
    delete.set_defaults(func=cmd_delete)

    args = ap.parse_args()
    try:
        return args.func(args)
    except KeyringUnavailable as exc:
        sys.exit(f"OS keyring: {exc}")


if __name__ == "__main__":
    sys.exit(main())
