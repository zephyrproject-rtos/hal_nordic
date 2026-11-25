# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Validate an application's own persistent keys as a key source holds them.

One entry per key, the pair `add_application_key.py` stores in a keyset:
"metadata" (the packed psa_key_attributes_t, as hex) and "value" (the key
material, as hex). Every entry becomes one import-key provisioning operation,
which stores the key in the URoT storage under the key ID inside the attributes.

A key that would be unusable or would collide is refused here rather than on the
bench. The device reports a second key with the same ID as
PSA_ERROR_ALREADY_EXISTS, which the result readers accept as idempotent success,
and the device confirms nothing about an import it did perform -- so these
checks are the only ones there are.
"""

from __future__ import annotations

from .python_version import require_python

require_python()

import dataclasses  # noqa: E402
import struct  # noqa: E402

# The packed psa_key_attributes_t: type, bits, lifetime, usage, two algorithms,
# key id, and a reserved word the device ignores. Unsigned, because that is what
# the fields mean; the nRF Connect SDK generator's signed spelling of the first
# two agrees for every value it emits.
ATTRS_PACK = "<HHIIIIII"
ATTRS_BLOB_SIZE = 28

# The PSA user key ID range. ID 0 is PSA_KEY_ID_NULL and names no key.
KEY_ID_MIN = 1
KEY_ID_MAX = 0x3FFFFFFF

# Reserved inside the user range for the OTA firmware-encryption KEKs. The same
# install batch imports them, so a key here would collide with a write this tool
# makes itself. Mirrors the client API's reserved key ID range.
RESERVED_KEY_ID_MIN = 228
RESERVED_KEY_ID_MAX = 239

# The lifetime's top 24 bits are the key's location, and a provisioned key is in
# local storage, so its lifetime is its persistence.
LOCATION_LOCAL_STORAGE = 0

# The persistence levels IronSide TE admits, as the client API values them.
PERSISTENCE_VOLATILE = 0x00
PERSISTENCE_ROTATING = 0x01
PERSISTENCE_REVOKABLE = 0x80
PERSISTENCE_LOCKED = 0x81

PERSISTENCE_NAMES: dict[int, str] = {
    PERSISTENCE_ROTATING: "ROTATING",
    PERSISTENCE_REVOKABLE: "REVOKABLE",
    PERSISTENCE_LOCKED: "LOCKED",
}

# The largest key material a persistent key holds, the PSA core's key slot. A
# URoT storage record holds this and the PSA core's 36-byte key file header.
MATERIAL_MAX = 145

ECC_PUBLIC_KEY_SECP_R1 = 0x4112

# What the nRF Connect SDK generator writes for a key the device generates itself.
TRNG_PREFIX = "TRNG:"


class AppKeyError(Exception):
    """An application key cannot be used as given."""


@dataclasses.dataclass(frozen=True)
class AppKey:
    """One key to import."""

    key_id: int
    attributes: bytes
    material: bytes
    key_type: int
    key_bits: int
    lifetime: int
    usage: int
    alg: int

    @property
    def persistence(self) -> int:
        """The revocation policy the key is stored under."""
        return self.lifetime & 0xFF

    def label(self) -> str:
        """The key's shape, for a line a reader holds against the file."""
        names = {
            0x2400: "AES",
            0x2004: "CHACHA20",
            0x1100: "HMAC",
            0x7112: "ECC secp-r1 key pair",
            ECC_PUBLIC_KEY_SECP_R1: "ECC secp-r1 public",
            0x7142: "ECC Edwards key pair",
            0x4142: "ECC Edwards public",
            0x1001: "raw data",
        }
        kind = names.get(self.key_type, f"key type 0x{self.key_type:04X}")
        return f"{kind} {self.key_bits}-bit"


def parse_entries(entries, where: str) -> tuple:
    """Every key @entries provisions, validated and ordered by key ID.

    @where names the source the entries came from, for a diagnostic an operator
    can act on. Ordered so the batch's operations run in key ID order and a
    result batch reads against a report positionally.
    """
    # A single entry is named by @where alone: the caller validating one key has
    # already said which, and "key 1" of one adds nothing.
    sole = len(entries) == 1
    keys = [_one(where, index, entry, sole=sole) for index, entry in enumerate(entries)]
    keys.sort(key=lambda k: k.key_id)
    _check_duplicates(where, keys)
    return tuple(keys)


def ops(keys) -> tuple:
    """The (attributes, material) pairs a batch builder takes."""
    return tuple((k.attributes, k.material) for k in keys)


def inventory(keys) -> list:
    """What a bundle records about the keys it provisions.

    The shape of each key and its ID, and no key material: the batches carry
    the material already, and a second copy would double what a leaked bundle
    leaks.
    """
    return [{"id": k.key_id, "type": k.key_type, "bits": k.key_bits} for k in keys]


def describe(keys) -> list:
    """One line per key, for a report a reader holds against the file."""
    if not keys:
        return []
    id_width = max(len(str(k.key_id)) for k in keys)
    width = max(len(k.label()) for k in keys)
    return [
        f"key {k.key_id:<{id_width}}  {k.label():<{width}}  "
        f"{PERSISTENCE_NAMES[k.persistence]}"
        for k in keys
    ]


def permanent(keys) -> tuple:
    """The keys whose ID a destroy does not free.

    A REVOKABLE key's destroy revokes its ID for the life of the part, and a
    LOCKED key cannot be destroyed. Either is a legitimate choice an application
    makes, so it is reported rather than refused.
    """
    return tuple(k for k in keys if k.persistence != PERSISTENCE_ROTATING)


# ---------------------------------------------------------------------------
# One entry
# ---------------------------------------------------------------------------
def _one(source: str, index: int, entry, *, sole: bool = False) -> AppKey:
    where = source if sole else f"{source}, key {index + 1}"
    if not isinstance(entry, dict):
        raise AppKeyError(f"{where} is not a JSON object")
    for field in ("metadata", "value"):
        if not isinstance(entry.get(field), str):
            raise AppKeyError(f"{where} carries no {field!r} string")

    value = entry["value"]
    if value.startswith(TRNG_PREFIX):
        raise AppKeyError(
            f"{where} names {value!r}, so the device would have to generate the "
            "key itself, and the provisioning operation imports only material "
            "the host supplies. Generate the key with an explicit value instead."
        )

    attrs = _hex(where, "metadata", entry["metadata"])
    if len(attrs) != ATTRS_BLOB_SIZE:
        raise AppKeyError(
            f"{where} carries {len(attrs)} bytes of key attributes, and the "
            f"device requires {ATTRS_BLOB_SIZE}"
        )
    material = _hex(where, "value", value)
    if not material:
        raise AppKeyError(f"{where} carries no key material")

    key_type, key_bits, lifetime, usage, alg, _alg2, key_id, reserved = struct.unpack(
        ATTRS_PACK, attrs
    )
    if reserved:
        raise AppKeyError(
            f"{where} carries a non-zero owner id, which this provisioning path "
            "does not support"
        )
    _check_lifetime(where, lifetime)
    _check_id(where, key_id)
    if len(material) > MATERIAL_MAX:
        raise AppKeyError(
            f"{where} carries {len(material)} bytes of key material, and a "
            f"provisioned key holds at most {MATERIAL_MAX}"
        )

    return AppKey(
        key_id=key_id,
        attributes=attrs,
        material=material,
        key_type=key_type,
        key_bits=key_bits,
        lifetime=lifetime,
        usage=usage,
        alg=alg,
    )


def _hex(where: str, field: str, text: str) -> bytes:
    body = text[2:] if text[:2].lower() == "0x" else text
    try:
        return bytes.fromhex(body)
    except ValueError:
        raise AppKeyError(f"{where} has a {field!r} that is not hex: {text!r}") from None


def _check_lifetime(where: str, lifetime: int) -> None:
    location = lifetime >> 8
    if location != LOCATION_LOCAL_STORAGE:
        raise AppKeyError(
            f"{where} names key location 0x{location:06X}, and a provisioned key "
            "is in local storage. Generate it for PSA_KEY_LOCATION_LOCAL_STORAGE."
        )
    persistence = lifetime & 0xFF
    if persistence == PERSISTENCE_VOLATILE:
        raise AppKeyError(f"{where} is a volatile key, which provisioning cannot store")
    if persistence not in PERSISTENCE_NAMES:
        levels = ", ".join(f"0x{v:02X} ({n})" for v, n in PERSISTENCE_NAMES.items())
        raise AppKeyError(
            f"{where} names persistence 0x{persistence:02X}, and IronSide TE admits "
            f"{levels}"
        )


def _check_id(where: str, key_id: int) -> None:
    if not KEY_ID_MIN <= key_id <= KEY_ID_MAX:
        raise AppKeyError(
            f"{where} carries key ID 0x{key_id:08X}, outside the PSA user range "
            f"{KEY_ID_MIN} to 0x{KEY_ID_MAX:08X}"
        )
    if RESERVED_KEY_ID_MIN <= key_id <= RESERVED_KEY_ID_MAX:
        raise AppKeyError(
            f"{where} carries key ID {key_id}, inside {RESERVED_KEY_ID_MIN} to "
            f"{RESERVED_KEY_ID_MAX}, which IronSide TE reserves for the OTA "
            "firmware-encryption KEKs. The same batch imports those, so the two "
            "writes would collide."
        )


def _check_duplicates(source: str, keys) -> None:
    """Refuse two keys with the same ID. @keys is sorted by key ID."""
    for earlier, later in zip(keys, keys[1:], strict=False):
        if earlier.key_id == later.key_id:
            raise AppKeyError(
                f"{source} provisions two keys with key ID {earlier.key_id}. One ID "
                "names one key."
            )


# ---------------------------------------------------------------------------
# Self-check
# ---------------------------------------------------------------------------
def _pack(key_id, key_type, key_bits, *, alg=0, usage=0x0300,
          location=LOCATION_LOCAL_STORAGE, persistence=PERSISTENCE_ROTATING,
          reserved=0) -> str:
    blob = struct.pack(
        ATTRS_PACK, key_type, key_bits, (location << 8) | persistence, usage,
        alg, 0, key_id, reserved,
    )
    return "0x" + blob.hex().upper()


def _self_check() -> int:
    results = []

    def check(name, got, want) -> None:
        ok = got == want
        results.append(ok)
        print(
            f"  {'ok  ' if ok else 'FAIL'} {name}"
            + ("" if ok else f"\n       got  {got}\n       want {want}")
        )

    def parse(*entries) -> tuple:
        return parse_entries(list(entries), "the self-check")

    def refuses(name, *entries) -> None:
        try:
            parse(*entries)
        except AppKeyError:
            results.append(True)
            print(f"  ok   {name}")
        else:
            results.append(False)
            print(f"  FAIL {name}: accepted")

    def entry(key_id, key_type=0x2400, key_bits=128, material=None, **kw) -> dict:
        body = bytes(key_bits // 8 if material is None else material)
        return {"metadata": _pack(key_id, key_type, key_bits, **kw), "value": "0x" + body.hex()}

    for key_id in (KEY_ID_MIN, 52, 134, RESERVED_KEY_ID_MIN - 1, RESERVED_KEY_ID_MAX + 1,
                   KEY_ID_MAX):
        (key,) = parse(entry(key_id))
        check(f"key ID {key_id} round-trips", key.key_id, key_id)

    keys = parse(entry(167), entry(164))
    check("keys are accepted in key ID order", [k.key_id for k in keys], [164, 167])
    check("the batch takes one pair per key", len(ops(keys)), 2)
    check(
        "the recorded inventory carries the shape and no material",
        inventory(keys)[0],
        {"id": 164, "type": 0x2400, "bits": 128},
    )
    check("a rotating key is not permanent", permanent(keys), ())
    check(
        "REVOKABLE and LOCKED keys are reported as permanent",
        [k.key_id for k in permanent(parse(
            entry(164, persistence=PERSISTENCE_REVOKABLE),
            entry(165, persistence=PERSISTENCE_LOCKED),
            entry(166),
        ))],
        [164, 165],
    )
    check(
        "a key of the widest material is accepted",
        len(parse(entry(164, 0x1001, 8 * MATERIAL_MAX))[0].material),
        MATERIAL_MAX,
    )

    refuses("two keys with one ID are refused", entry(164), entry(164))
    refuses("a TRNG placeholder is refused",
            {"metadata": _pack(164, 0x2400, 128), "value": "TRNG:16"})
    refuses("attributes of the wrong length are refused",
            {"metadata": _pack(164, 0x2400, 128)[:-2], "value": "0x" + "00" * 16})
    refuses("attributes that are not hex are refused",
            {"metadata": "0xnothex", "value": "0x" + "00" * 16})
    refuses("empty key material is refused",
            {"metadata": _pack(164, 0x2400, 128), "value": "0x"})
    refuses("key ID 0 is refused", entry(0))
    refuses("a key ID above the user range is refused", entry(KEY_ID_MAX + 1))
    refuses("a CRACEN KMU key ID is refused", entry(0x7FFF20A4))
    for key_id in (RESERVED_KEY_ID_MIN, 232, RESERVED_KEY_ID_MAX):
        refuses(f"reserved key ID {key_id} is refused", entry(key_id))
    refuses("the CRACEN KMU location is refused", entry(164, location=0x804E4B))
    refuses("a volatile key is refused", entry(164, persistence=PERSISTENCE_VOLATILE))
    for level in (0x02, 0x03, 0x82, 0xFF):
        refuses(f"persistence 0x{level:02X} is refused", entry(164, persistence=level))
    refuses("a non-zero owner id is refused", entry(164, reserved=1))
    refuses("more key material than a record holds is refused",
            entry(164, 0x1001, 8 * (MATERIAL_MAX + 1)))
    refuses("an entry that is not an object is refused", "not an object")

    bad = results.count(False)
    print(f"{len(results) - bad}/{len(results)} passed")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(_self_check())
