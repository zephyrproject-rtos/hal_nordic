# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""What an application key may be, on each SoC IronSide TE supports.

A provisioned key has to clear two gates, and this table is their
intersection. The provisioning firmware has to have the key type and algorithm
compiled in to import the key at all, and IronSide TE has to admit that
algorithm to non-secure callers for the application to use the key afterwards.
A key outside the intersection passes every host-side check and is then refused
by the device partway through a write-once install, so it is refused here
instead.

This is narrower than the run-time key support the user guide describes, which
also covers keys an application creates for itself. Those are not provisioned.

One profile per SoC. A part whose firmware admits a different set is one new
profile and one changed mapping, with no new code.
"""

from __future__ import annotations

from .python_version import require_python

require_python()

import dataclasses  # noqa: E402

from . import app_keys, parts  # noqa: E402


class CapsError(Exception):
    """A key the selected SoC does not provision."""


@dataclasses.dataclass(frozen=True)
class KeyType:
    value: int  # the psa_key_type_t
    material: str  # how the key material is encoded, for material_lengths()


@dataclasses.dataclass(frozen=True)
class Algorithm:
    value: int  # the psa_algorithm_t
    allows: frozenset  # usage groups the algorithm accepts


@dataclasses.dataclass(frozen=True)
class Row:
    """One key an application may provision: a shape, the algorithm the key is
    bound to, and the usage it accepts with it."""

    key_type: str
    key_bits: tuple
    algorithm: str
    allows: frozenset = frozenset()  # narrows the algorithm's own set for this shape
    requires: frozenset = frozenset()  # groups the firmware refuses the key without

    def usage(self) -> frozenset:
        return self.allows or ALGORITHMS[self.algorithm].allows


@dataclasses.dataclass(frozen=True)
class Profile:
    name: str
    rows: tuple

    def key_types(self) -> tuple:
        return tuple(sorted({row.key_type for row in self.rows}))

    def bits_for(self, key_type: str) -> tuple:
        return tuple(
            sorted({b for row in self.rows if row.key_type == key_type for b in row.key_bits})
        )

    def algorithms_for(self, key_type: str, key_bits: int) -> tuple:
        return tuple(
            sorted(
                row.algorithm
                for row in self.rows
                if row.key_type == key_type and key_bits in row.key_bits
            )
        )

    def describe(self) -> list:
        """The whole table, one line per row, for --list."""
        head = ("key type", "bits", "algorithm", "usage", "needs")
        cells = [
            (
                row.key_type,
                ", ".join(str(b) for b in row.key_bits),
                row.algorithm,
                ", ".join(order_usage(row.usage())),
                ", ".join(order_usage(row.requires)),
            )
            for row in self.rows
        ]
        width = [max(len(row[i]) for row in [head, *cells]) for i in range(len(head))]
        return [
            ("  " + "  ".join(t.ljust(width[i]) for i, t in enumerate(row))).rstrip()
            for row in [head, *cells]
        ]


# The psa_key_type_t values, and how psa_import_key encodes the material of each.
KEY_TYPES: dict[str, KeyType] = {
    "AES": KeyType(0x2400, "raw"),
    "CHACHA20": KeyType(0x2004, "raw"),
    "HMAC": KeyType(0x1100, "raw"),
    "ECC_KEY_PAIR_SECP_R1": KeyType(0x7112, "ecc_scalar"),
    "ECC_PUBLIC_KEY_SECP_R1": KeyType(app_keys.ECC_PUBLIC_KEY_SECP_R1, "ecc_point_secp_r1"),
}

# The psa_algorithm_t values. ECDSA_ANY_HASH is PSA_ALG_ECDSA(PSA_ALG_ANY_HASH).
ALGORITHMS: dict[str, Algorithm] = {
    "GCM": Algorithm(0x05500200, frozenset({"ENCRYPT", "DECRYPT"})),
    "KW": Algorithm(0x0B400100, frozenset({"DECRYPT"})),
    "CHACHA20_POLY1305": Algorithm(0x05100500, frozenset({"ENCRYPT", "DECRYPT"})),
    "HMAC_SHA_256": Algorithm(0x03800009, frozenset({"SIGN", "VERIFY"})),
    "ECDSA_SHA_256": Algorithm(0x06000609, frozenset({"SIGN", "VERIFY"})),
    "ECDSA_SHA_384": Algorithm(0x0600060A, frozenset({"SIGN", "VERIFY"})),
    "ECDSA_ANY_HASH": Algorithm(0x060006FF, frozenset({"SIGN", "VERIFY"})),
}

# The usage groups a provisioned key is granted. Each grants every PSA flag it
# covers: a key granted DECRYPT can also unwrap, and one granted SIGN can sign
# both a message and a hash. PSA_KEY_USAGE_VERIFY_DERIVATION and
# PSA_KEY_USAGE_DERIVE_PUBLIC belong to no group and cannot be provisioned.
USAGE: dict[str, int] = {
    "ENCRYPT": 0x00010100,  # PSA_KEY_USAGE_ENCRYPT | WRAP
    "DECRYPT": 0x00020200,  # PSA_KEY_USAGE_DECRYPT | UNWRAP
    "SIGN": 0x1400,  # PSA_KEY_USAGE_SIGN_MESSAGE | SIGN_HASH
    "VERIFY": 0x2800,  # PSA_KEY_USAGE_VERIFY_MESSAGE | VERIFY_HASH
    "DERIVE": 0x4000,  # PSA_KEY_USAGE_DERIVE
}

# Granted only by --allow-usage-export, and never part of an algorithm's own set.
USAGE_EXPORT = 0x0001

# The revocation policy the key is stored under, named and valued as the client
# API names and values it.
PERSISTENCE: dict[str, int] = {name: value for value, name in app_keys.PERSISTENCE_NAMES.items()}

DEFAULT_PERSISTENCE = "ROTATING"

_IRONSIDE_TE_NRF54L = Profile(
    "IronSide TE on nRF54L",
    (
        Row("AES", (128, 256), "GCM"),
        Row("AES", (128, 256), "KW"),
        Row("CHACHA20", (256,), "CHACHA20_POLY1305"),
        Row("HMAC", (128, 192, 256, 384), "HMAC_SHA_256"),
        # An ECC key pair is provisioned only with SIGN, whatever else it is
        # granted. A public key carries the verifying half and only verifies.
        Row("ECC_KEY_PAIR_SECP_R1", (256, 384), "ECDSA_SHA_256", requires=frozenset({"SIGN"})),
        Row("ECC_KEY_PAIR_SECP_R1", (256, 384), "ECDSA_SHA_384", requires=frozenset({"SIGN"})),
        Row("ECC_KEY_PAIR_SECP_R1", (256, 384), "ECDSA_ANY_HASH", requires=frozenset({"SIGN"})),
        Row(
            "ECC_PUBLIC_KEY_SECP_R1", (256, 384), "ECDSA_SHA_256",
            allows=frozenset({"VERIFY"}), requires=frozenset({"VERIFY"}),
        ),
        Row(
            "ECC_PUBLIC_KEY_SECP_R1", (256, 384), "ECDSA_SHA_384",
            allows=frozenset({"VERIFY"}), requires=frozenset({"VERIFY"}),
        ),
        Row(
            "ECC_PUBLIC_KEY_SECP_R1", (256, 384), "ECDSA_ANY_HASH",
            allows=frozenset({"VERIFY"}), requires=frozenset({"VERIFY"}),
        ),
    ),
)

# Every part lib.parts knows has to name a profile here, so a part added there
# without a decision about its crypto fails the self-check.
PROFILES: dict[str, Profile] = dict.fromkeys(parts.PARTS, _IRONSIDE_TE_NRF54L)


def profile(soc: str) -> Profile:
    try:
        return PROFILES[soc.lower()]
    except KeyError:
        raise CapsError(
            f"no application-key profile for SoC {soc!r}, known: {', '.join(sorted(PROFILES))}"
        ) from None


def key_type_names() -> tuple:
    """Every key-type name any profile offers, for an argparse choices list."""
    return tuple(sorted({row.key_type for p in PROFILES.values() for row in p.rows}))


def algorithm_names() -> tuple:
    return tuple(sorted({row.algorithm for p in PROFILES.values() for row in p.rows}))


def usage_names() -> tuple:
    return tuple(USAGE)


def order_usage(names) -> tuple:
    """@names in the order USAGE defines, so a list of them reads the same way twice."""
    wanted = set(names)
    return tuple(name for name in USAGE if name in wanted)


def material_lengths(key_type: str, key_bits: int) -> tuple:
    """The key-material lengths psa_import_key accepts for this shape, in bytes."""
    kind = KEY_TYPES[key_type].material
    if kind == "ecc_point_secp_r1":
        # The uncompressed point 0x04 || x || y.
        return (1 + 2 * (key_bits // 8),)
    return (key_bits // 8,)


def check(soc: str, key_type: str, key_bits: int, algorithm: str, usage) -> Row:
    """The row @soc provisions this key under, or a refusal naming what it does.

    @usage is the set of group names the key is granted, before any export flag.
    """
    caps = profile(soc)
    if key_type not in caps.key_types():
        raise CapsError(
            f"{soc} provisions no {key_type} key. It provisions "
            f"{', '.join(caps.key_types())}. Run --list for the whole table."
        )
    bits = caps.bits_for(key_type)
    if key_bits not in bits:
        raise CapsError(
            f"{soc} provisions no {key_bits}-bit {key_type} key. Its {key_type} keys are "
            f"{_or(str(b) for b in bits)} bits."
        )
    named = caps.algorithms_for(key_type, key_bits)
    if algorithm not in named:
        raise CapsError(
            f"{soc} does not bind a {key_bits}-bit {key_type} key to {algorithm}. "
            f"It binds one to {_or(named)}."
        )
    row = next(
        r
        for r in caps.rows
        if r.key_type == key_type and key_bits in r.key_bits and r.algorithm == algorithm
    )

    granted = frozenset(usage)
    if not granted:
        raise CapsError(
            f"{algorithm} on key type {key_type} accepts "
            f"{_list(order_usage(row.usage()))}, and no usage was given."
        )
    outside = granted - row.usage()
    if outside:
        raise CapsError(
            f"{algorithm} on key type {key_type} accepts {_list(order_usage(row.usage()))}. "
            f"It does not accept {_list(order_usage(outside))}."
        )
    missing = row.requires - granted
    if missing:
        raise CapsError(
            f"{algorithm} on key type {key_type} needs {_list(order_usage(missing))}. "
            f"IronSide TE provisions the key only with it."
        )
    return row


def check_key_id(key_id: int) -> None:
    """Refuse a key ID an application key cannot have.

    Holds the ID against the same ranges lib.app_keys reads a finished file
    against, so a key this accepts cannot be refused there.
    """
    if not app_keys.KEY_ID_MIN <= key_id <= app_keys.KEY_ID_MAX:
        raise CapsError(
            f"key ID {key_id} is outside the PSA user range {app_keys.KEY_ID_MIN} to "
            f"0x{app_keys.KEY_ID_MAX:08X}."
        )
    if app_keys.RESERVED_KEY_ID_MIN <= key_id <= app_keys.RESERVED_KEY_ID_MAX:
        raise CapsError(
            f"key ID {key_id} is reserved for the OTA firmware-encryption KEKs, "
            f"{app_keys.RESERVED_KEY_ID_MIN} to {app_keys.RESERVED_KEY_ID_MAX}. The same "
            f"install batch imports those, so the two writes would collide."
        )


def check_material(key_type: str, key_bits: int, material: bytes) -> None:
    lengths = material_lengths(key_type, key_bits)
    if len(material) in lengths:
        return
    if KEY_TYPES[key_type].material == "ecc_point_secp_r1":
        raise CapsError(
            f"a {key_bits}-bit {key_type} takes the {lengths[0]}-byte uncompressed point "
            f"0x04||x||y, and the key material carries {len(material)} bytes."
        )
    raise CapsError(
        f"a {key_bits}-bit {key_type} key takes {_or(str(n) for n in lengths)} bytes of key "
        f"material, and the key material carries {len(material)} bytes."
    )


def _or(names) -> str:
    """One of these: "a", "a or b", "a, b or c"."""
    items = list(names)
    if len(items) < 2:
        return "".join(items)
    return f"{', '.join(items[:-1])} or {items[-1]}"


def _list(names) -> str:
    """All of these, with no conjunction to read as one-of or all-of."""
    return ", ".join(names)


def _self_check() -> int:
    results = []

    def report(name, got, want) -> None:
        ok = got == want
        results.append(ok)
        print(
            f"  {'ok  ' if ok else 'FAIL'} {name}"
            + ("" if ok else f"\n       got  {got}\n       want {want}")
        )

    def refuses(name, call) -> None:
        try:
            call()
        except CapsError:
            results.append(True)
            print(f"  ok   {name}")
        else:
            results.append(False)
            print(f"  FAIL {name}: accepted")

    rows = [(soc, row) for soc, prof in sorted(PROFILES.items()) for row in prof.rows]

    report("every part lib.parts knows names a profile", sorted(PROFILES), sorted(parts.PARTS))
    # add_application_key.py accepts a key without asking which part it is for,
    # because a keyset is not per-part. That is only honest while every part
    # offers the same keys. Give one part a profile of its own and this fails:
    # the tool then needs a per-part gate, at ironside_bundle.py update-keys
    # where the bundle names the real part.
    first = PROFILES[min(PROFILES)]
    report(
        "every part offers the same keys, so a key needs no part named",
        sorted({soc for soc, prof in PROFILES.items() if prof is not first}),
        [],
    )
    report(
        "every row names a key type that exists",
        sorted({row.key_type for _, row in rows} - set(KEY_TYPES)),
        [],
    )
    report(
        "every row names an algorithm that exists",
        sorted({row.algorithm for _, row in rows} - set(ALGORITHMS)),
        [],
    )
    report(
        "every row names usage groups that exist",
        sorted({n for _, row in rows for n in row.allows | row.requires} - set(USAGE)),
        [],
    )
    report(
        "a row requires nothing it does not also allow",
        [f"{row.key_type}/{row.algorithm}" for _, row in rows if row.requires - row.usage()],
        [],
    )
    report(
        "the argparse choices cover every profile's own names",
        (
            set(key_type_names()) >= {row.key_type for _, row in rows},
            set(algorithm_names()) >= {row.algorithm for _, row in rows},
        ),
        (True, True),
    )

    # A shape whose material does not fit a storage record is a key this table
    # offers and lib.app_keys then refuses.
    report(
        f"every shape's key material fits {app_keys.MATERIAL_MAX} bytes",
        [
            f"{row.key_type} {bits}-bit takes {max(material_lengths(row.key_type, bits))} bytes"
            for _, row in rows
            for bits in row.key_bits
            if max(material_lengths(row.key_type, bits)) > app_keys.MATERIAL_MAX
        ],
        [],
    )

    for soc, row in rows:
        for bits in row.key_bits:
            usage = row.requires or frozenset({order_usage(row.usage())[0]})
            label = f"{row.key_type} {bits}-bit {row.algorithm}"
            report(
                f"{soc} provisions {label}",
                check(soc, row.key_type, bits, row.algorithm, usage),
                row,
            )

    for soc in sorted(PROFILES):
        # The primitives deliberately left out, each with the gate that excludes it.
        for key_type, bits, algorithm, why in (
            ("AES", 192, "GCM", "AES-192 is not supported on these parts"),
            ("AES", 128, "CCM", "the provisioning firmware cannot perform CCM"),
            ("AES", 128, "CTR", "the provisioning firmware cannot perform CTR"),
            ("AES", 128, "CBC_NO_PADDING", "the provisioning firmware cannot perform CBC"),
            ("AES", 128, "CMAC", "CMAC is not reachable from the application"),
            ("AES", 128, "ECB_NO_PADDING", "ECB is not reachable from the application"),
            ("AES", 256, "SP800_108_COUNTER_CMAC", "the provisioning firmware cannot perform it"),
            ("DERIVE", 256, "SP800_108_COUNTER_CMAC", "the key type is not compiled in"),
            ("ECC_KEY_PAIR_TWISTED_EDWARDS", 255, "PURE_EDDSA", "Ed25519 is not compiled in"),
            ("ECC_KEY_PAIR_MONTGOMERY", 255, "ECDH", "X25519 is not compiled in"),
            ("ECC_KEY_PAIR_SECP_R1", 256, "ECDH", "the provisioning firmware cannot perform ECDH"),
            ("ECC_KEY_PAIR_SECP_R1", 521, "ECDSA_SHA_256", "secp521r1 is not supported"),
        ):
            refuses(
                f"{soc}: {key_type} {bits}-bit {algorithm} is refused because {why}",
                lambda t=key_type, b=bits, a=algorithm, soc=soc: check(
                    soc, t, b, a, {"ENCRYPT", "SIGN", "VERIFY"}
                ),
            )

        refuses(
            f"{soc}: a key granted no usage is refused",
            lambda soc=soc: check(soc, "AES", 128, "GCM", set()),
        )
        refuses(
            f"{soc}: a key pair granted only VERIFY is refused",
            lambda soc=soc: check(soc, "ECC_KEY_PAIR_SECP_R1", 256, "ECDSA_SHA_256", {"VERIFY"}),
        )
        refuses(
            f"{soc}: a public key granted SIGN is refused",
            lambda soc=soc: check(
                soc, "ECC_PUBLIC_KEY_SECP_R1", 256, "ECDSA_SHA_256", {"SIGN", "VERIFY"}
            ),
        )
    refuses("an unknown SoC is refused", lambda: profile("nrf54zz99z"))
    check_key_id(app_keys.RESERVED_KEY_ID_MIN - 1)
    check_key_id(app_keys.RESERVED_KEY_ID_MAX + 1)
    refuses("key ID 0 is refused", lambda: check_key_id(0))
    refuses("a key ID above the user range is refused",
            lambda: check_key_id(app_keys.KEY_ID_MAX + 1))
    for key_id in (app_keys.RESERVED_KEY_ID_MIN, app_keys.RESERVED_KEY_ID_MAX):
        refuses(f"reserved key ID {key_id} is refused", lambda k=key_id: check_key_id(k))
    refuses(
        "key material of the wrong length is refused",
        lambda: check_material("AES", 256, bytes(16)),
    )
    refuses(
        "a secp-r1 point without its leading byte is refused",
        lambda: check_material("ECC_PUBLIC_KEY_SECP_R1", 256, bytes(64)),
    )

    bad = results.count(False)
    print(f"{len(results) - bad}/{len(results)} passed")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(_self_check())
