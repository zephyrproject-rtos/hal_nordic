# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""The PSA key slot storage: the base slots at the top of ROT-RAM and the secure
RAM the base configuration reserves above it for more.

The slot array starts at PSA_KEY_ARRAY_ADDR, PSA_BASE_KEY_SLOTS slots below the
end of ROT-RAM, and spans the key slot storage size, base slots included: at
least KEY_SLOT_STORAGE_SIZE_MIN, and anything above that is secure RAM reserved
above ROT-RAM, a multiple of the part's RAM split granule. UROT offers every
whole PSA_KEY_SLOT_BUDGET-byte slot that fits, and the non-secure RAM starts
where the storage ends, so the size is an ABI towards the App, which is linked
for one. The size is a base-configuration word in UICR, written once at install
and stored as given, and UROT reads one out of range or off the part's granule
as the minimum. psa_key_slots() gives the slot count a size holds.
"""

from __future__ import annotations

PSA_BASE_KEY_SLOTS = 8
PSA_KEY_SLOT_BUDGET = 192
KEY_SLOT_STORAGE_SIZE_MIN = PSA_BASE_KEY_SLOTS * PSA_KEY_SLOT_BUDGET
KEY_SLOT_STORAGE_SIZE_MAX = KEY_SLOT_STORAGE_SIZE_MIN + 40 * 1024
# The finest RAM split granule of any part: what a size is checked against when
# the part is not known.
RAM_SPLIT_GRANULE_MIN = 32
ROT_RAM_SIZE = 32 * 1024
RAM_BASE = 0x20000000
PSA_KEY_ARRAY_ADDR = RAM_BASE + ROT_RAM_SIZE - KEY_SLOT_STORAGE_SIZE_MIN
# The top of physical RAM that a hardware MPC override opens only to the KMU and
# CRACEN, on a part whose physical RAM extends past the App RAM: the 256 B CRACEN
# ProtectedRAM and the 32 B guard below it. VPRCTX-RAM is the RAM between the App
# RAM end and this span.
RESERVED_TOP_SIZE = 0x120


class PsaKeyRamError(ValueError):
    pass


def psa_key_slots(size: int) -> int:
    """The PSA key slots a key slot storage size of @size bytes holds."""
    return size // PSA_KEY_SLOT_BUDGET


def ns_addr(size: int) -> int:
    """The non-secure RAM start a key slot storage size of @size bytes gives."""
    return PSA_KEY_ARRAY_ADDR + size


def size_at(ns_ram_addr: int) -> int:
    """The key slot storage size that puts the non-secure RAM at @ns_ram_addr."""
    return ns_ram_addr - PSA_KEY_ARRAY_ADDR


def is_usable(size: int, granule: int = RAM_SPLIT_GRANULE_MIN) -> bool:
    """Whether UROT keeps a stored key slot storage size on a part with @granule."""
    return (
        KEY_SLOT_STORAGE_SIZE_MIN <= size <= KEY_SLOT_STORAGE_SIZE_MAX
        and (size - KEY_SLOT_STORAGE_SIZE_MIN) % granule == 0
    )


def applied_size(stored: int | None, granule: int = RAM_SPLIT_GRANULE_MIN) -> int:
    """The key slot storage size UROT applies for a stored size of @stored bytes,
    None when the word is erased, on a part with @granule."""
    if stored is not None and is_usable(stored, granule):
        return stored
    return KEY_SLOT_STORAGE_SIZE_MIN


def _describe(size: int) -> str:
    return f"{size} B ({psa_key_slots(size)} slots)"


def check_key_slot_storage_size(size: int, granule: int = RAM_SPLIT_GRANULE_MIN) -> int:
    """Validate a key slot storage size for a part whose RAM split moves in
    @granule bytes. Raises on bad input, naming the nearest valid sizes."""
    if not KEY_SLOT_STORAGE_SIZE_MIN <= size <= KEY_SLOT_STORAGE_SIZE_MAX:
        raise PsaKeyRamError(
            f"key slot storage size {size} B out of range "
            f"({KEY_SLOT_STORAGE_SIZE_MIN}..{KEY_SLOT_STORAGE_SIZE_MAX}); "
            f"the {PSA_BASE_KEY_SLOTS} base slots take {KEY_SLOT_STORAGE_SIZE_MIN} B"
        )
    off = (size - KEY_SLOT_STORAGE_SIZE_MIN) % granule
    if off:
        below = size - off
        above = below + granule
        nearest = _describe(below)
        if above <= KEY_SLOT_STORAGE_SIZE_MAX:
            nearest += f" and {_describe(above)}"
        raise PsaKeyRamError(
            f"key slot storage size {size} B must be {KEY_SLOT_STORAGE_SIZE_MIN} B plus a "
            f"multiple of the part's {granule} B RAM split granule; the nearest are {nearest}"
        )
    return size


def ram_layout(size: int, ram_kb: int, ram_physical_kb: int) -> dict:
    """The RAM areas a key slot storage size of @size bytes gives on a part with
    @ram_kb usable and @ram_physical_kb physical RAM.

    What the bundle tool prints and what an App built for @size is linked
    against: ROT-RAM, the key slot storage from the top of ROT-RAM, the part of
    it reserved above ROT-RAM, and the non-secure RAM. On a part whose physical
    RAM extends past the usable RAM, also VPRCTX-RAM and the reserved top of RAM above
    it, which no image links; None on a part without them.
    """
    ns_base = ns_addr(size)
    ram_end = RAM_BASE + ram_kb * 1024
    physical_end = RAM_BASE + ram_physical_kb * 1024
    vprctx_ram = reserved_top = None
    if physical_end > ram_end:
        reserved_base = physical_end - RESERVED_TOP_SIZE
        vprctx_ram = {"addr": ram_end, "size": reserved_base - ram_end}
        reserved_top = {"addr": reserved_base, "size": RESERVED_TOP_SIZE}
    return {
        "psa_key_slots": psa_key_slots(size),
        "key_slot_storage_size": size,
        "rot_ram": {"addr": RAM_BASE, "size": ROT_RAM_SIZE},
        "key_slot_storage": {"addr": PSA_KEY_ARRAY_ADDR, "size": size},
        "extra_key_ram": {
            "addr": RAM_BASE + ROT_RAM_SIZE,
            "size": size - KEY_SLOT_STORAGE_SIZE_MIN,
        },
        "ram_ns": {"addr": ns_base, "size": ram_end - ns_base},
        "vprctx_ram": vprctx_ram,
        "reserved_top": reserved_top,
    }
