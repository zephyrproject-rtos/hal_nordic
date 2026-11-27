# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""The space a device has for the application keys a bundle provisions.

The device refuses a key that would leave less than a fixed reserve free, so a
key that does not fit here is one the install batch stops at. The constants
are the device's own space accounting, which IronSide pins at build time.
"""

from __future__ import annotations

from .python_version import require_python

require_python()

import dataclasses  # noqa: E402

from . import app_key_caps as caps  # noqa: E402
from . import app_keys as ak  # noqa: E402

# The partition is used in blocks, one of which the device keeps empty.
BLOCK_SIZE = 1024
BLOCK_CAPACITY = 944

# A record costs its data and a fixed overhead, rounded up to the alignment.
RECORD_OVERHEAD = 45
RECORD_ALIGN = 16

# A stored key is this header and its key material.
KEY_HEADER_SIZE = 36



def record_cost(data_length: int) -> int:
    """The space a record of @data_length bytes takes."""
    return -(-(data_length + RECORD_OVERHEAD) // RECORD_ALIGN) * RECORD_ALIGN


# The space IronSide keeps for its own records (the format record and the App
# and DFU counters, 4 bytes each) and a reserve of two of the largest keys, and
# the space the OTA KEKs take when a bundle provisions them.
IRONSIDE_RESERVED = 3 * record_cost(4) + 2 * record_cost(KEY_HEADER_SIZE + ak.MATERIAL_MAX)
OTA_KEKS_SIZE = 384


@dataclasses.dataclass(frozen=True)
class Usage:
    """The space application keys have, in bytes.

    @application_keys maps the key ID of each application key to the space it
    takes, and @capacity is the space all application keys share.
    """

    application_keys: dict
    capacity: int

    @property
    def used(self) -> int:
        return sum(self.application_keys.values())

    @property
    def free(self) -> int:
        return max(self.capacity - self.used, 0)

    @property
    def is_exceeded(self) -> bool:
        return self.used > self.capacity


def application_key_space(partition_size: int, app_keys, *, ota_kek: bool) -> Usage:
    """The space the application keys take of a @partition_size-byte partition.

    @app_keys is the inventory a bundle records, one {"id", "type", "bits"}
    entry per key.
    """
    blocks = partition_size // BLOCK_SIZE - 1
    capacity = blocks * BLOCK_CAPACITY - IRONSIDE_RESERVED - (OTA_KEKS_SIZE if ota_kek else 0)
    return Usage(
        application_keys={
            key["id"]: record_cost(KEY_HEADER_SIZE + key_material_length(key["type"], key["bits"]))
            for key in app_keys
        },
        capacity=max(capacity, 0),
    )


def key_material_length(key_type: int, key_bits: int) -> int:
    """Bytes of key material the device stores for a key of this shape.

    A key type that no SoC provisions is counted at the largest material a
    key can have.
    """
    for name, known in caps.KEY_TYPES.items():
        if known.value == key_type:
            return caps.material_lengths(name, key_bits)[0]
    return ak.MATERIAL_MAX


def _self_check() -> int:
    failures = 0

    def check(name, got, want) -> None:
        nonlocal failures
        if got != want:
            print(f"FAIL {name}: got {got!r}, want {want!r}")
            failures += 1

    check("AES-256 key", record_cost(KEY_HEADER_SIZE + 32), 128)
    check("largest key", record_cost(KEY_HEADER_SIZE + ak.MATERIAL_MAX), 240)
    check("IronSide's records and reserve", IRONSIDE_RESERVED, 672)

    use = application_key_space(4096, (), ota_kek=True)
    check("4 KB capacity", use.capacity, 3 * 944 - 672 - 384)
    use = application_key_space(4096, (), ota_kek=False)
    check("4 KB capacity without the KEKs", use.capacity, 3 * 944 - 672)

    keys = [{"id": i, "type": 0x2400, "bits": 256} for i in range(1, 15)]
    use = application_key_space(4096, keys[:13], ota_kek=True)
    check("4 KB holds 13 AES-256 keys", use.is_exceeded, False)
    use = application_key_space(4096, keys, ota_kek=True)
    check("4 KB refuses a 14th", use.is_exceeded, True)

    check("P-256 public key", key_material_length(ak.ECC_PUBLIC_KEY_SECP_R1, 256), 65)
    check("unknown key type", key_material_length(0x7001, 2048), ak.MATERIAL_MAX)

    print("ok" if not failures else f"{failures} failure(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(_self_check())
