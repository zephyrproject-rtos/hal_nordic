# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""UICR layout for the IronSide base configuration and the HW lifecycle registers.

This module contains layout and encoding only. Firmware C structures and static
assertions define the authoritative layout.

The IronSide region at 0xFFD200..0xFFD4FF holds base configuration. Hardware
interprets lifecycle registers at 0xFFD000..0xFFD083. USERCNF entries at
0xFFD100..0xFFD1FF write APB registers before the first CPU instruction.
UICR.OTP at 0xFFD500..0xFFD9FF holds customer data.

RRAM is write-once until ERASEALL. Independent values occupy separate 16-byte
lines. Encoders return absolute-address byte writes for sparse HEX output.
"""

import struct
from pathlib import Path

from intelhex import IntelHex

ERASED_BYTE = 0xFF
ERASED = 0xFFFFFFFF  # erased (never-written) word
DECOMMISSIONED = 0x00000000  # the one DECOMMISSIONED value that means decommissioned

# struct IRONSIDE_UICR_LAYOUT
UICR_HW_BASE = 0x00FFD000  # Hardware register block
IRONSIDE_UICR_BASE = UICR_HW_BASE + 0x200
REGION_SIZE = 0x300

# Each adjacent subregion has its own FORMATVERSION.
IROT_OFF = 0x000
UROT_OFF = 0x0C0

IROT_FORMATVERSION_OFF = IROT_OFF + 0x000
PARTITIONS_OFF = IROT_OFF + 0x0B0
UROT_FORMATVERSION_OFF = UROT_OFF + 0x000
APPROTECTAUTHLOCK_OFF = UROT_OFF + 0x1C0
# KEYSLOTSTORAGESIZE: the PSA key slot storage size in bytes, the base slots at
# the top of ROT-RAM included (lib/psa_key_ram.py). Erased means the base slots.
KEYSLOTSTORAGESIZE_OFF = UROT_OFF + 0x1E0
DECOMMISSIONED_OFF = UROT_OFF + 0x210
IKGSEED_SELECT_OFF = UROT_OFF + 0x220

# IKGSEED.SELECT values, by the IKG SEED entry they select. Any other word
# selects none, so the device does not boot.
IKGSEED_SELECT_NORDIC = 0xA5C3965A
IKGSEED_SELECT_USER = 0x5A3C69A5
IKG_SEED_SELECTIONS = {"nordic": IKGSEED_SELECT_NORDIC, "user": IKGSEED_SELECT_USER}
# Leaves IKGSEED.SELECT erased, so the provisioning firmware writes it from the
# SEED slot states: User SEED when it is locked, else Nordic SEED.
IKG_SEED_SELECT_AUTO = "auto"

# UICR.OTP follows the region and holds customer data that IronSide TE does not
# interpret. The customer decides its layout.
OTP_OFF = 0x300
OTP_BASE = IRONSIDE_UICR_BASE + OTP_OFF
OTP_WORDS = 320
OTP_SIZE = 4 * OTP_WORDS

# Subregion versions use major << 16 | minor.
IROT_FORMATVERSION = 0x00010000
UROT_FORMATVERSION = 0x00010000
SUPPORTED_FORMATVERSIONS = ("1.0",)

# struct IRONSIDE_UICR_KEY is PROVISIONED + REVOKED[3] + KEY[32], KEY holding the
# SHA-256 digest of the generation's verification key.
KEY_PROVISIONED = 0xBD2328A8
KEY_GEN_SIZE = 0x30
REVOKED_OFF = 0x04  # REVOKED[3] follows PROVISIONED
REVOKED_LEN = 12  # 3 u32 words
KEY_BYTES_OFF = 0x10  # KEY follows the validity line
KEY_RECORD_LEN = 32
GENERATIONS = 3  # ROT_KEY_GENERATIONS

# AUTH_LOCK uses KEY_PROVISIONED as its unlocked marker. IronSide later writes
# LOCK words on the device.
AUTHLOCK_PROVISIONED = KEY_PROVISIONED

# IROT stores the UROT key. UROT stores the remaining role keys.
PUBKEY_OFF = {
    "urot": IROT_OFF + 0x010,
    "dfu": UROT_OFF + 0x010,
    "app": UROT_OFF + 0x0A0,
    "authopkey": UROT_OFF + 0x130,
}
ROLES = tuple(PUBKEY_OFF)

# The fixed SIZE4KB word order determines placement down from the end of RRAM.
# SLOT0 occupies the remainder.
PARTITION_NAMES = ("slot1", "appstorage", "urotstorage", "dfuimage")
PARTITION_BLOCK_KB = 4  # SIZE4KB unit
# APPSTORAGE may fold into SLOT1. A zero-size DFUIMAGE disables DFU boot mode.
# SLOT1 cannot be zero because firmware does not define a missing staging bank.
PARTITION_OPTIONAL = ("appstorage", "dfuimage")
# Reference layout used by IROT, UROT, and the installer.
REFERENCE_PARTITIONS_KB = (128, 64, 4, 32)

# These anchors match the RoT partition resolver. Usable NVM size comes from
# device tree on the target and requires a per-part host table.
SLOT0_BASE = 0x00023000
RRAM_SIZE_KB = {
    "nrf54lm20a": 2036,
    "nrf54lm20b": 2036,
    "nrf54lv10a": 1012,
    "nrf54l15": 1524,
}

# Lifecycle register values match nRF54LM20A/B NRF_UICR_Type. Protection
# registers contain redundant copies that must both be written.
HW_PROTECT_COPIES = (0x00, 0x1C)  # PROTECT0, PROTECT1 within a register

# The 54L MDK documents only the unprotected AP value. The protected value follows
# the nRF52 precedent and matches the secure provisioning firmware.
HW_AP_PROTECTED = 0x00000000
HW_ERASEPROTECT_PROTECTED = 0x50FA50FA  # UICR_ERASEPROTECT_PROTECT0_PALL_Protected

# Register offset and protected value.
HW_PROTECT_FIELDS = {
    "approtect": (0x000, HW_AP_PROTECTED),
    "secapprotect": (0x020, HW_AP_PROTECTED),
    "auxapprotect": (0x040, HW_AP_PROTECTED),
    "eraseprotect": (0x060, HW_ERASEPROTECT_PROTECTED),
}
BOOTCONF_OFF = 0x080  # single full-register write

# BOOTCONF SIZE is in kB.
_BC_READ, _BC_EXECUTE, _BC_SECURE = 0, 2, 3
_BC_WRITEONCE, _BC_LOCK, _BC_SIZE = 12, 13, 16
BOOTCONF_LOCK_MSK = 1 << _BC_LOCK
# Hardware drops the high bits of a larger SIZE without an error.
BOOTCONF_SIZE_MAX_KB = {
    "nrf54lm20a": 127,
    "nrf54lm20b": 127,
    "nrf54lv10a": 127,
    "nrf54l15": 31,
}
# IROT is the immutable boot region at address zero, IROT storage follows it, and
# UROT fills the rest of the space up to SLOT0_BASE. The 54L15 IROT carries the
# CRACEN microcode, which the other parts have in ROM.
IROT_SIZE_KB = {
    "nrf54lm20a": 15,
    "nrf54lm20b": 15,
    "nrf54lv10a": 15,
    "nrf54l15": 19,
}
IROT_STORAGE_SIZE_KB = 1

# USERCNF entries are {ADDR, DATA} pairs applied at reset. Addresses match the
# internal UICR and WDT specification.
USERCNF_BASE = UICR_HW_BASE + 0x100
USERCNF_STRIDE = 0x8
USERCNF_COUNT = 32  # USERCNF[n], n = 0..31
# Fill for every slot after the last entry. Hardware also stops at an erased
# slot, but programming 0xFFFFFFFF leaves the word writable, so a later write
# could add an entry that RRAMC applies in secure context at every reset.
USERCNF_TERMINATOR = (0x00000000, 0x00000000)

# WDT30 values match NRF_WDT_Type and the firmware watchdog header.
WDT30_BASE = 0x50108000
WDT_TASKS_START_OFF = 0x000
WDT_CRV_OFF = 0x504
WDT_CONFIG_OFF = 0x50C
# STOPEN lets UROT stop the watchdog before the non-secure jump. SLEEP keeps it
# running while idle. HALT pauses it during debugger stops.
WDT_CONFIG_SECURE_BOOT = (1 << 6) | (1 << 0)
WDT_LFCLK_HZ = 32768  # the WDT counts the 32.768 kHz LFCLK
SECURE_WDT_DEFAULT_TIMEOUT_S = 4.0


class UicrError(Exception):
    """Invalid UICR layout input."""


def _role_off(role):
    try:
        return PUBKEY_OFF[role]
    except KeyError:
        raise UicrError(f"unknown role {role!r} (one of {', '.join(ROLES)})") from None


def pubkey_addr(role, gen):
    """Return the address of a role generation's PROVISIONED word."""
    if not 0 <= gen < GENERATIONS:
        raise UicrError(f"generation {gen} out of range (0..{GENERATIONS - 1})")
    return IRONSIDE_UICR_BASE + _role_off(role) + gen * KEY_GEN_SIZE


def revoked_addr(role, gen):
    """Return the address of a role generation's REVOKED field."""
    return pubkey_addr(role, gen) + REVOKED_OFF


def _check_format_version(version):
    if version not in SUPPORTED_FORMATVERSIONS:
        raise UicrError(
            f"unsupported format version {version!r} "
            f"(supported: {', '.join(SUPPORTED_FORMATVERSIONS)})"
        )


def irot_format_version_writes(version="1.0"):
    """Return the .IROT FORMATVERSION write.

    Each RoT image supplies the version for the write-once subregion it reads.
    """
    _check_format_version(version)
    return [(IRONSIDE_UICR_BASE + IROT_FORMATVERSION_OFF, struct.pack("<I", IROT_FORMATVERSION))]


def urot_format_version_writes(version="1.0"):
    """Return the .UROT FORMATVERSION write."""
    _check_format_version(version)
    return [(IRONSIDE_UICR_BASE + UROT_FORMATVERSION_OFF, struct.pack("<I", UROT_FORMATVERSION))]


def format_version_writes(version="1.0"):
    """Return both FORMATVERSION writes."""
    return irot_format_version_writes(version) + urot_format_version_writes(version)


def key_writes(role, gen, key):
    """Provision a role generation while leaving REVOKED erased."""
    if len(key) != KEY_RECORD_LEN:
        raise UicrError(
            f"{role} gen{gen}: key record is {len(key)} bytes, expected {KEY_RECORD_LEN}"
        )
    base = pubkey_addr(role, gen)
    return [
        (base, struct.pack("<I", KEY_PROVISIONED)),
        (base + KEY_BYTES_OFF, bytes(key)),
    ]


def key_revoke_writes(role, gen):
    """Revoke a role generation by writing zeros to REVOKED.

    Firmware treats any non-erased value as revoked. Key bytes remain unchanged.
    """
    return [(revoked_addr(role, gen), b"\x00" * REVOKED_LEN)]


def kb_to_blocks(size_kb, what, allow_zero=False):
    """Convert a partition size to aligned SIZE4KB blocks."""
    if size_kb < 0 or (size_kb == 0 and not allow_zero):
        raise UicrError(
            f"{what}: size must be "
            + ("0 or positive" if allow_zero else "positive")
            + f", got {size_kb} kB"
        )
    if size_kb % PARTITION_BLOCK_KB:
        raise UicrError(f"{what}: {size_kb} kB is not a multiple of {PARTITION_BLOCK_KB} kB")
    return size_kb // PARTITION_BLOCK_KB


def partition_writes(sizes_kb, *, soc):
    """Encode partition sizes in PARTITION_NAMES order.

    Placement is validated before encoding because the words are write-once.
    This prevents layouts that IROT would refuse as an invalid partition layout.
    """
    if len(sizes_kb) != len(PARTITION_NAMES):
        raise UicrError(
            f"partitions: expected {len(PARTITION_NAMES)} sizes "
            f"({', '.join(PARTITION_NAMES)}), got {len(sizes_kb)}"
        )
    blocks = [
        kb_to_blocks(kb, name, allow_zero=name in PARTITION_OPTIONAL)
        for kb, name in zip(sizes_kb, PARTITION_NAMES)
    ]
    partition_addresses(sizes_kb, soc=soc)
    return [(IRONSIDE_UICR_BASE + PARTITIONS_OFF, struct.pack(f"<{len(blocks)}I", *blocks))]


def resolve_soc(soc):
    """Match a board-qualified SoC name to the longest supported part prefix."""
    for family in sorted(RRAM_SIZE_KB, key=len, reverse=True):
        if str(soc).startswith(family):
            return family
    raise UicrError(f"partitions: unknown soc {soc!r} (known: {', '.join(sorted(RRAM_SIZE_KB))})")


def partition_addresses(sizes_kb, *, soc, slot0_base=SLOT0_BASE, rram_end=None):
    """Place `sizes_kb` (PARTITION_NAMES order) and return {name: (addr, size)}.

    Partitions are packed from the end of NVM in reverse word order. SLOT0 occupies
    the remainder from `slot0_base` to SLOT1. Rejections match the RoT resolver.
    """
    if len(sizes_kb) != len(PARTITION_NAMES):
        raise UicrError(
            f"partitions: expected {len(PARTITION_NAMES)} sizes "
            f"({', '.join(PARTITION_NAMES)}), got {len(sizes_kb)}"
        )
    if rram_end is None:
        rram_end = RRAM_SIZE_KB[resolve_soc(soc)] * 1024

    sizes = {n: kb * 1024 for n, kb in zip(PARTITION_NAMES, sizes_kb)}
    out = {}
    base = rram_end
    for name in reversed(PARTITION_NAMES):
        if sizes[name] > base:
            raise UicrError(f"partitions: {name} ({sizes[name]} B) does not fit below {base:#x}")
        base -= sizes[name]
        out[name] = (base, sizes[name])

    # SLOT0 must be non-empty.
    if base <= slot0_base:
        raise UicrError(
            f"partitions: no room for SLOT0. SLOT1 starts at "
            f"{base:#x}, at or below the SLOT0 base {slot0_base:#x}"
        )
    out["slot0"] = (slot0_base, base - slot0_base)
    return out


def key_slot_storage_size_writes(size):
    """Write the PSA key slot storage size. The caller validates @size for the
    part, and leaves the word erased for the base slots' size."""
    return [(IRONSIDE_UICR_BASE + KEYSLOTSTORAGESIZE_OFF, struct.pack("<I", size))]


def authlock_provision_writes():
    """Return the unlocked APPROTECTAUTHLOCK provisioning marker."""
    return [(IRONSIDE_UICR_BASE + APPROTECTAUTHLOCK_OFF, struct.pack("<I", AUTHLOCK_PROVISIONED))]


def ikg_seed_select_writes(entry):
    """Return the IKGSEED.SELECT write selecting @entry, "nordic" or "user"."""
    try:
        value = IKG_SEED_SELECTIONS[entry]
    except KeyError:
        raise UicrError(
            f"unknown IKG SEED selection {entry!r} (expected {' or '.join(IKG_SEED_SELECTIONS)})"
        ) from None
    return [(IRONSIDE_UICR_BASE + IKGSEED_SELECT_OFF, struct.pack("<I", value))]


def decommission_writes(value=DECOMMISSIONED):
    """Return the terminal DECOMMISSIONED flag write."""
    return [(IRONSIDE_UICR_BASE + DECOMMISSIONED_OFF, struct.pack("<I", value))]


def irot_size_kb(soc):
    """Return the size of the IROT partition, the immutable boot region, in kB."""
    return IROT_SIZE_KB[resolve_soc(soc)]


def urot_slot_size(soc):
    """Return the size of the UROT slot, from the end of IROT storage to SLOT0_BASE, in bytes."""
    return SLOT0_BASE - (irot_size_kb(soc) + IROT_STORAGE_SIZE_KB) * 1024


def bootconf_value(size_kb, soc):
    """Return immutable RXS BOOTCONF with WRITEONCE and LOCK set."""
    size_max_kb = BOOTCONF_SIZE_MAX_KB[resolve_soc(soc)]
    if not 0 <= size_kb <= size_max_kb:
        raise UicrError(f"bootconf size {size_kb} kB out of range for {soc} (0..{size_max_kb})")
    return (
        (1 << _BC_READ)
        | (1 << _BC_EXECUTE)
        | (1 << _BC_SECURE)
        | (1 << _BC_WRITEONCE)
        | (1 << _BC_LOCK)
        | (size_kb << _BC_SIZE)
    )


def bootconf_writes(value):
    """Return one full BOOTCONF register write."""
    return [(UICR_HW_BASE + BOOTCONF_OFF, struct.pack("<I", value))]


def hw_protect_writes(field):
    """Set both copies of a hardware lifecycle protection register."""
    try:
        base_off, value = HW_PROTECT_FIELDS[field]
    except KeyError:
        raise UicrError(
            f"unknown HW field {field!r} (one of {', '.join(HW_PROTECT_FIELDS)})"
        ) from None
    return [
        (UICR_HW_BASE + base_off + copy, struct.pack("<I", value)) for copy in HW_PROTECT_COPIES
    ]


def usercnf_writes(entries, start=0):
    """Encode ordered {ADDR, DATA} entries starting at a USERCNF index."""
    if start + len(entries) > USERCNF_COUNT:
        raise UicrError(
            f"USERCNF: {len(entries)} entries from index {start} "
            f"exceeds USERCNF[0..{USERCNF_COUNT - 1}]"
        )
    out = []
    for i, (addr, data) in enumerate(entries, start=start):
        slot = USERCNF_BASE + i * USERCNF_STRIDE
        out.append((slot, struct.pack("<I", addr)))
        out.append((slot + 0x4, struct.pack("<I", data)))
    return out


def usercnf_full_writes(entries):
    """Encode @entries followed by USERCNF_TERMINATOR in every remaining slot."""
    if len(entries) > USERCNF_COUNT:
        raise UicrError(
            f"USERCNF: {len(entries)} entries exceed USERCNF[0..{USERCNF_COUNT - 1}]"
        )
    for addr, data in entries:
        if is_usercnf_terminator((addr, data)):
            raise UicrError(
                f"USERCNF entry 0x{addr:08X}=0x{data:08X} reads as a terminator "
                f"and would end the list"
            )
    return usercnf_writes(entries + [USERCNF_TERMINATOR] * (USERCNF_COUNT - len(entries)))


def usercnf_slots(get) -> list[tuple[int, int]]:
    """Read all USERCNF slots as (ADDR, DATA) through a byte lookup.

    A word the lookup does not fully expose reads as erased, which is what an
    unprogrammed device holds.
    """
    slots = []
    for i in range(USERCNF_COUNT):
        slot = USERCNF_BASE + i * USERCNF_STRIDE
        addr, data = _word(get, slot), _word(get, slot + 0x4)
        slots.append((ERASED if addr is None else addr, ERASED if data is None else data))
    return slots


def is_usercnf_terminator(entry: tuple[int, int]) -> bool:
    """Whether hardware stops applying USERCNF at this (ADDR, DATA) pair."""
    return entry in ((0, 0), (ERASED, ERASED))


def usercnf_end(slots: list[tuple[int, int]]) -> int:
    """Return the index of the first slot hardware does not apply."""
    return next((i for i, entry in enumerate(slots) if is_usercnf_terminator(entry)), len(slots))


def usercnf_append_writes(
    slots: list[tuple[int, int]], entries: list[tuple[int, int]]
) -> list[tuple[int, bytes]]:
    """Encode the complete USERCNF array with @entries after the applied ones.

    This edits an image before it is programmed. Hardware stops at the first
    terminator, so a programmed slot after it would never be applied and is
    refused.
    """
    end = usercnf_end(slots)
    stray = [i for i in range(end, len(slots)) if not is_usercnf_terminator(slots[i])]
    if stray:
        raise UicrError(
            f"USERCNF[{stray[0]}] is programmed after the terminator at "
            f"USERCNF[{end}], so the list cannot be extended"
        )
    return usercnf_full_writes(slots[:end] + entries)


def secure_wdt_crv(timeout_s):
    """Convert a secure watchdog timeout using (CRV + 1) / WDT_LFCLK_HZ."""
    crv = round(timeout_s * WDT_LFCLK_HZ) - 1
    if not 0 <= crv <= ERASED:
        raise UicrError(f"secure watchdog timeout {timeout_s}s: CRV {crv} out of range")
    return crv


def secure_wdt_entries(timeout_s=SECURE_WDT_DEFAULT_TIMEOUT_S):
    """Return ordered entries that start WDT30 before IROT.

    CONFIG and CRV become read-only after TASKS_START. UROT stops WDT30 before
    the non-secure jump.
    """
    return [
        (WDT30_BASE + WDT_CONFIG_OFF, WDT_CONFIG_SECURE_BOOT),
        (WDT30_BASE + WDT_CRV_OFF, secure_wdt_crv(timeout_s)),
        (WDT30_BASE + WDT_TASKS_START_OFF, 1),
    ]


def emit_hex(writes, path):
    """Write sparse HEX for programming with ERASE_NONE."""
    ih = IntelHex()
    for addr, data in writes:
        ih.frombytes(bytes(data), offset=addr)
    path.parent.mkdir(parents=True, exist_ok=True)
    ih.write_hex_file(str(path))


def emit_bin(writes, path):
    """Write a dense 0xff-filled IronSide region image.

    Reject out-of-region writes because binary output carries no addresses.
    """
    buf = bytearray([ERASED_BYTE]) * REGION_SIZE
    for addr, data in writes:
        off = addr - IRONSIDE_UICR_BASE
        if off < 0 or off + len(data) > REGION_SIZE:
            raise UicrError(
                f"0x{addr:08X}+{len(data)} is outside the IronSide "
                f"region. A bin file can only hold region content"
            )
        buf[off : off + len(data)] = bytes(data)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(bytes(buf))


def _word(get, addr):
    """Read a little-endian u32, or None if any byte is absent."""
    b = [get(addr + i) for i in range(4)]
    if any(v is None for v in b):
        return None
    return int.from_bytes(bytes(b), "little")


def decode(get, soc=None):
    """Decode whatever a byte lookup exposes into a report of named fields.

    The lookup returns a byte or None. Absent fields are omitted. Partition
    addresses depend on the part, so they are placed only when @soc is given.
    """
    region, hw, usercnf = {}, {}, []

    for name, off in (
        ("irot_formatversion", IROT_FORMATVERSION_OFF),
        ("urot_formatversion", UROT_FORMATVERSION_OFF),
    ):
        word = _word(get, IRONSIDE_UICR_BASE + off)
        if word is not None:
            region[name] = word

    keys = {}
    for role in ROLES:
        gens = {}
        for gen in range(GENERATIONS):
            base = pubkey_addr(role, gen)
            prov = _word(get, base)
            revoked = [_word(get, base + REVOKED_OFF + 4 * i) for i in range(REVOKED_LEN // 4)]
            key = [get(base + KEY_BYTES_OFF + i) for i in range(KEY_RECORD_LEN)]
            if prov is None and all(w is None for w in revoked) and all(b is None for b in key):
                continue
            gens[gen] = {
                "provisioned": prov == KEY_PROVISIONED,
                # Firmware treats any non-erased REVOKED word as revoked.
                "revoked": any(w is not None and w != ERASED for w in revoked),
                "key": (bytes(key).hex() if all(b is not None for b in key) else None),
            }
        if gens:
            keys[role] = gens
    if keys:
        region["keys"] = keys

    parts = [
        _word(get, IRONSIDE_UICR_BASE + PARTITIONS_OFF + 4 * i) for i in range(len(PARTITION_NAMES))
    ]
    if all(w is not None for w in parts):
        sizes_kb = [blocks * PARTITION_BLOCK_KB for blocks in parts]
        region["partitions_kb"] = dict(zip(PARTITION_NAMES, sizes_kb))
        # Report invalid stored layouts instead of rejecting device readback.
        if soc is not None:
            try:
                region["partitions"] = {
                    name: {"addr": addr, "size": size}
                    for name, (addr, size) in partition_addresses(sizes_kb, soc=soc).items()
                }
            except UicrError as exc:
                region["partitions_error"] = str(exc)

    storage = _word(get, IRONSIDE_UICR_BASE + KEYSLOTSTORAGESIZE_OFF)
    if storage is not None and storage != ERASED:
        region["key_slot_storage_size"] = storage

    authlock = _word(get, IRONSIDE_UICR_BASE + APPROTECTAUTHLOCK_OFF)
    if authlock is not None:
        lock = [
            _word(get, IRONSIDE_UICR_BASE + APPROTECTAUTHLOCK_OFF + 4 * (i + 1)) for i in range(3)
        ]
        region["authlock"] = {
            "provisioned": authlock == AUTHLOCK_PROVISIONED,
            "locked": any(w is not None and w != ERASED for w in lock),
        }

    select = _word(get, IRONSIDE_UICR_BASE + IKGSEED_SELECT_OFF)
    if select is not None and select != ERASED:
        names = {value: name for name, value in IKG_SEED_SELECTIONS.items()}
        region["ikg_seed_select"] = names.get(select, f"invalid (0x{select:08X})")

    decomm = _word(get, IRONSIDE_UICR_BASE + DECOMMISSIONED_OFF)
    if decomm is not None:
        region["decommissioned"] = decomm == DECOMMISSIONED

    for field, (off, protected) in HW_PROTECT_FIELDS.items():
        copies = [_word(get, UICR_HW_BASE + off + c) for c in HW_PROTECT_COPIES]
        if all(w is None for w in copies):
            continue
        hw[field] = {
            "protected": all(w == protected for w in copies),
            "copies": copies,
        }
    bootconf = _word(get, UICR_HW_BASE + BOOTCONF_OFF)
    if bootconf is not None:
        hw["bootconf"] = {
            "value": bootconf,
            "size_kb": bootconf >> _BC_SIZE,
            "locked": bool(bootconf & BOOTCONF_LOCK_MSK),
        }

    for i in range(USERCNF_COUNT):
        slot = USERCNF_BASE + i * USERCNF_STRIDE
        addr, data = _word(get, slot), _word(get, slot + 0x4)
        if addr is None and data is None:
            continue
        usercnf.append({"index": i, "addr": addr, "data": data})

    return {"region": region, "hw": hw, "usercnf": usercnf}


def format_report(report):
    """Render register words in a decoded report as hexadecimal strings."""

    def conv(value, key=None):
        if isinstance(value, bool) or value is None:
            return value
        if isinstance(value, int):
            return f"0x{value:08X}"
        if isinstance(value, dict):
            return {k: conv(v, k) for k, v in value.items()}
        if isinstance(value, list):
            return [conv(v) for v in value]
        return value

    # Keep generation indices and partition sizes as counts.
    out = conv(report)
    keys = report.get("region", {}).get("keys")
    if keys:
        out["region"]["keys"] = {
            role: {gen: conv(state) for gen, state in gens.items()} for role, gens in keys.items()
        }
    parts = report.get("region", {}).get("partitions_kb")
    if parts:
        out["region"]["partitions_kb"] = dict(parts)
    for entry, raw in zip(out.get("usercnf", []), report.get("usercnf", [])):
        entry["index"] = raw["index"]
    if "bootconf" in out.get("hw", {}):
        out["hw"]["bootconf"]["size_kb"] = report["hw"]["bootconf"]["size_kb"]
    return out


def file_reader(path):
    """Create a byte lookup over sparse HEX or a dense region image."""
    if path.suffix == ".bin":
        buf = path.read_bytes()
        if len(buf) != REGION_SIZE:
            raise UicrError(f"{path}: {len(buf)} bytes, expected a {REGION_SIZE}-byte region image")

        def get(addr):
            off = addr - IRONSIDE_UICR_BASE
            return buf[off] if 0 <= off < len(buf) else None
    else:
        ih = IntelHex()
        ih.loadhex(str(path))
        present = set(ih.addresses())

        def get(addr):
            return ih[addr] if addr in present else None

    return get


def files_reader(paths):
    """Create a byte lookup across files, with later files taking precedence."""
    readers = [file_reader(Path(path)) for path in paths]

    def get(addr):
        for reader in reversed(readers):
            byte = reader(addr)
            if byte is not None:
                return byte
        return None

    return get


def decode_file(path, soc=None):
    """Decode sparse HEX or a dense region image, placing partitions for @soc."""
    return decode(file_reader(path), soc)


def otp_words(get) -> dict[int, int]:
    """Return {index: word} for every UICR.OTP word a byte lookup holds whole."""
    words = {}
    for index in range(OTP_WORDS):
        word = _word(get, OTP_BASE + 4 * index)
        if word is not None:
            words[index] = word
    return words


def _self_check():
    """Check partition policy, placement, and encoding round trips."""
    bad = 0

    def case(name, ok, detail=""):
        nonlocal bad
        bad += not ok
        print(f"  {'ok  ' if ok else 'FAIL'} {name}" + (f": {detail}" if detail else ""))

    def refuses(name, sizes_kb, soc):
        try:
            partition_writes(sizes_kb, soc=soc)
        except UicrError as exc:
            case(f"{soc}: {name}", True, str(exc))
        else:
            case(f"{soc}: {name}", False, f"{sizes_kb} was accepted")

    for soc in RRAM_SIZE_KB:

        def pcase(name, ok, detail="", soc=soc):
            case(f"{soc}: {name}", ok, detail)

        # Check zero-size policy for each partition.
        ref = tuple(REFERENCE_PARTITIONS_KB)
        for i, name in enumerate(PARTITION_NAMES):
            sizes = list(ref)
            sizes[i] = 0
            if name in PARTITION_OPTIONAL:
                blocks = struct.unpack(f"<{len(sizes)}I", partition_writes(sizes, soc=soc)[0][1])
                pcase(f"{name}=0 accepted (optional)", blocks[i] == 0, f"blocks {blocks}")
            else:
                refuses(f"{name}=0 refused (no firmware meaning)", sizes, soc)
        refuses("negative size refused", (128, -4, 4, 32), soc)
        refuses("misaligned size refused", (128, 66, 4, 32), soc)
        refuses("wrong number of sizes refused", (128, 64), soc)

        # Check encoding address and block counts.
        addr, payload = partition_writes(ref, soc=soc)[0]
        pcase("written at PARTITIONS", addr == IRONSIDE_UICR_BASE + PARTITIONS_OFF, f"{addr:#x}")
        pcase(
            "encoded as 4 kB block counts",
            struct.unpack(f"<{len(ref)}I", payload) == tuple(kb // PARTITION_BLOCK_KB for kb in ref),
            payload.hex(),
        )

        # Check placement from the end of NVM.
        rram_end = RRAM_SIZE_KB[soc] * 1024
        placed = partition_addresses(ref, soc=soc)
        top = rram_end
        for name in reversed(PARTITION_NAMES):
            a, size = placed[name]
            pcase(f"{name} packed below {top:#x}", a + size == top, f"{a:#x}+{size}")
            top = a
        pcase(
            "slot0 is the remainder up to the lowest partition",
            placed["slot0"] == (SLOT0_BASE, top - SLOT0_BASE),
            f"{placed['slot0'][0]:#x}+{placed['slot0'][1]}",
        )
        # A zero-size partition retains the boundary address.
        nodfu = partition_addresses((128, 64, 4, 0), soc=soc)
        pcase(
            "a 0-size partition sits on the NVM end",
            nodfu["dfuimage"] == (rram_end, 0),
            f"{nodfu['dfuimage']}",
        )
        pcase(
            "omitting it grows slot0 by that much",
            nodfu["slot0"][1] - placed["slot0"][1] == ref[3] * 1024,
            f"{placed['slot0'][1]} -> {nodfu['slot0'][1]}",
        )

        # Both placement and encoding must reject layouts that do not fit.
        for name, sizes in (
            ("a layout larger than NVM", (128, 64, 4, RRAM_SIZE_KB[soc])),
            ("a layout leaving no room for slot0", (RRAM_SIZE_KB[soc] - 64, 28, 4, 32)),
        ):
            try:
                partition_addresses(sizes, soc=soc)
            except UicrError as exc:
                pcase(f"{name} refused", True, str(exc))
            else:
                pcase(f"{name} refused", False, f"{sizes} placed")
            refuses(f"{name} not encoded either", sizes, soc)

        # Check encode and decode round trips.
        written = dict(enumerate(partition_writes(ref, soc=soc)))
        buf = {}
        for _, (base, data) in written.items():
            buf.update({base + i: b for i, b in enumerate(bytes(data))})
        report = decode(lambda a: buf.get(a), soc)["region"]
        pcase(
            "decode() reads back the written sizes",
            report["partitions_kb"] == dict(zip(PARTITION_NAMES, ref)),
            str(report.get("partitions_kb")),
        )
        pcase(
            "decode() places them where the writer meant",
            all(report["partitions"][n]["addr"] == placed[n][0] for n in PARTITION_NAMES),
            str({n: hex(report["partitions"][n]["addr"]) for n in PARTITION_NAMES}),
        )

    # Check the per-part IROT/UROT split and the BOOTCONF SIZE limit.
    for soc in RRAM_SIZE_KB:
        # UROT's vector table follows its 0x800-byte header and needs 2 kB alignment.
        urot_base = SLOT0_BASE - urot_slot_size(soc)
        case(f"{soc}: UROT starts on a 2 kB boundary", urot_base % 0x800 == 0, f"{urot_base:#x}")
        case(
            f"{soc}: BOOTCONF covers IROT",
            bootconf_value(irot_size_kb(soc), soc) >> _BC_SIZE == irot_size_kb(soc),
            f"{bootconf_value(irot_size_kb(soc), soc):#010x}",
        )
    try:
        bootconf_value(32, "nrf54l15")
    except UicrError as exc:
        case("a BOOTCONF size the SIZE field truncates is refused", True, str(exc))
    else:
        case("a BOOTCONF size the SIZE field truncates is refused", False, "32 kB accepted")

    # Check full USERCNF output and the zero fill after the last entry.
    wdt = secure_wdt_entries()
    padded = usercnf_full_writes(wdt)
    last_slot = USERCNF_BASE + (USERCNF_COUNT - 1) * USERCNF_STRIDE
    case(
        "USERCNF array spans the full register block",
        len(padded) == 2 * USERCNF_COUNT
        and padded[0][0] == USERCNF_BASE
        and padded[-1][0] == last_slot + 0x4,
        f"0x{padded[0][0]:08X}..0x{padded[-1][0]:08X}",
    )
    buf = {}
    for addr, data in padded:
        buf.update({addr + i: b for i, b in enumerate(bytes(data))})
    read = usercnf_slots(lambda a: buf.get(a))
    case("usercnf_slots reads the watchdog entries", read[:3] == wdt, str(read[:3]))
    case(
        "unused USERCNF slots read back as zero",
        all(entry == USERCNF_TERMINATOR for entry in read[3:]),
    )
    case("the applied list ends after the watchdog", usercnf_end(read) == 3)

    # Check that appended entries follow the applied ones and the rest stays zero.
    extra = [(0x50000000, 1), (0x50000004, 2)]
    appended = usercnf_append_writes(read, extra)
    case(
        "usercnf_append_writes places entries after the applied list",
        appended == usercnf_full_writes(wdt + extra),
        f"0x{appended[0][0]:08X}",
    )
    erased_tail = wdt + [(ERASED, ERASED)] * (USERCNF_COUNT - len(wdt))
    case(
        "usercnf_append_writes seals an erased tail with zero",
        usercnf_append_writes(erased_tail, extra) == appended,
    )
    case(
        "absent words read as erased",
        usercnf_slots(lambda a: None) == [(ERASED, ERASED)] * USERCNF_COUNT,
    )
    for name, bad_slots, bad_entries in (
        (
            "an entry after a terminator refused",
            read[:5] + [(0x50000000, 1)] + read[6:],
            extra,
        ),
        ("an erased terminator entry refused", read, [(ERASED, ERASED)]),
        ("a zero terminator entry refused", read, [USERCNF_TERMINATOR]),
        ("an overflowing list refused", read, extra * USERCNF_COUNT),
    ):
        try:
            usercnf_append_writes(bad_slots, bad_entries)
        except UicrError as exc:
            case(name, True, str(exc))
        else:
            case(name, False, "accepted")

    _self_check_otp(case)

    print("uicr self-test: " + ("OK" if not bad else f"{bad} FAILED"))
    return 1 if bad else 0


def _self_check_otp(case) -> None:
    """Check that otp_words reads whole little-endian words and skips partial ones."""
    buf = {OTP_BASE + i: b for i, b in enumerate(bytes.fromhex("78563412"))}
    buf.update({OTP_BASE + 8 + i: 0 for i in range(4)})
    buf.update({OTP_BASE + 12 + i: 0 for i in range(2)})
    case(
        "otp_words reads OTP[0] little-endian and skips the partial OTP[3]",
        otp_words(buf.get) == {0: 0x12345678, 2: 0},
        str(otp_words(buf.get)),
    )


if __name__ == "__main__":
    raise SystemExit(_self_check())
