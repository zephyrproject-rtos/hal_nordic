# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Pack an application key's PSA attributes the way the device stores them.

One function, and it is the only place the 28-byte psa_key_attributes_t blob is
built. The layout is app_keys.ATTRS_PACK, so the writer and the reader cannot
disagree about a field's offset, and the provisioning firmware reads the same
offsets on the device.

The blob is packed only for a key the named SoC can actually provision:
app_key_caps holds the shape against the three gates a provisioned key clears,
and the checks run here rather than in a caller, so no path reaches the struct
with a shape the factory floor would refuse.

The key location is not an argument: a provisioned key is in local storage.
"""

from __future__ import annotations

from .python_version import require_python

require_python()

import struct  # noqa: E402

from . import app_key_caps as caps  # noqa: E402
from . import app_keys  # noqa: E402


def attributes(soc: str, key_type: str, key_bits: int, algorithm: str, usage, key_id: int,
               persistence: str, *, allow_export: bool = False) -> bytes:
    """The packed psa_key_attributes_t for one provisioned application key.

    @usage is the set of usage-group names the key is granted, before any export
    flag. Raises app_key_caps.CapsError for a key @soc does not provision, for a
    usage the algorithm does not accept, and for a key ID an application key cannot
    have.
    """
    row = caps.check(soc, key_type, key_bits, algorithm, usage)
    caps.check_key_id(key_id)

    flags = caps.USAGE_EXPORT if allow_export else 0
    for name in usage:
        flags |= caps.USAGE[name]
    lifetime = (app_keys.LOCATION_LOCAL_STORAGE << 8) | caps.PERSISTENCE[persistence]
    return struct.pack(
        app_keys.ATTRS_PACK,
        caps.KEY_TYPES[row.key_type].value,
        key_bits,
        lifetime,
        flags,
        caps.ALGORITHMS[row.algorithm].value,
        0,  # the second algorithm, which IronSide TE does not use
        key_id,
        0,  # the owner id, which this provisioning path does not support
    )


# ---------------------------------------------------------------------------
# Self-check
# ---------------------------------------------------------------------------
def _self_check() -> int:
    from . import parts

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
        except caps.CapsError:
            results.append(True)
            print(f"  ok   {name}")
        else:
            results.append(False)
            print(f"  FAIL {name}: accepted")

    for soc in sorted(parts.PARTS):
        # The vector the host-side tests pin with literals, so a moved field fails
        # here too rather than only on a device.
        report(
            f"{soc}: an AES-128 GCM key at ID 134 packs to the vector the tests pin",
            attributes(soc, "AES", 128, "GCM", {"ENCRYPT", "DECRYPT"}, 134, "ROTATING").hex().upper(),
            "00248000010000000003030000025005000000008600000000000000",
        )
        report(
            f"{soc}: the packed fields are type, bits, lifetime, usage, algorithm, id",
            struct.unpack(
                app_keys.ATTRS_PACK, attributes(soc, "AES", 128, "GCM", {"ENCRYPT"}, 52, "LOCKED")
            ),
            (0x2400, 128, 0x81, 0x00010100, 0x05500200, 0, 52, 0),
        )
        report(
            f"{soc}: the export flag adds PSA_KEY_USAGE_EXPORT and nothing else",
            struct.unpack(
                app_keys.ATTRS_PACK,
                attributes(soc, "AES", 128, "GCM", {"ENCRYPT"}, 52, "ROTATING", allow_export=True),
            )[3],
            0x00010101,
        )

    # Every shape every profile offers, at both ends of the key ID range, read
    # back through the reader that consumes the blob.
    for profile_soc, row in [
        (s, r) for s, prof in sorted(caps.PROFILES.items()) for r in prof.rows
    ]:
        for bits in row.key_bits:
            usage = row.requires or frozenset({caps.order_usage(row.usage())[0]})
            label = f"{row.key_type} {bits}-bit {row.algorithm}"
            for key_id in (app_keys.KEY_ID_MIN, app_keys.KEY_ID_MAX):
                blob = attributes(
                    profile_soc, row.key_type, bits, row.algorithm, usage, key_id,
                    caps.DEFAULT_PERSISTENCE,
                )
                material = bytes(max(caps.material_lengths(row.key_type, bits)))
                (key,) = app_keys.parse_entries(
                    [{"metadata": "0x" + blob.hex().upper(), "value": "0x" + material.hex()}],
                    "the self-check",
                )
                report(
                    f"{profile_soc} {label} at key ID {key_id} round-trips through the reader",
                    (key.key_id, key.key_type, key.key_bits, key.alg, key.persistence),
                    (
                        key_id,
                        caps.KEY_TYPES[row.key_type].value,
                        bits,
                        caps.ALGORITHMS[row.algorithm].value,
                        app_keys.PERSISTENCE_ROTATING,
                    ),
                )

    # The checks run here, so a shape the table refuses never reaches the struct.
    refuses(
        "a key type the part does not provision is refused",
        lambda: attributes(soc, "AES", 192, "GCM", {"ENCRYPT"}, 164, "ROTATING"),
    )
    refuses(
        "a usage the algorithm does not accept is refused",
        lambda: attributes(soc, "AES", 128, "KW", {"ENCRYPT"}, 164, "ROTATING"),
    )
    refuses(
        "a key ID in the OTA KEK reserved range is refused",
        lambda: attributes(soc, "AES", 128, "GCM", {"ENCRYPT"}, 228, "ROTATING"),
    )

    bad = results.count(False)
    print(f"{len(results) - bad}/{len(results)} passed")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(_self_check())
