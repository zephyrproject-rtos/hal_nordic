# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Build and parse ordered CBOR operations for the provisioning ABI."""

import dataclasses
import io
import struct

import cbor2

from . import parts

# Keep these values synchronized with the provisioning image layout.
# Both images start with a reset trampoline, followed by the vector table at
# offset 0x800. Their native reset handlers select their own stacks. Secure and
# non-secure modes use different banked VTOR and MSP registers but the same
# values.
#
# The image base is above the largest PSA key-slot reservation on every part. Layouts
# differ by part because UROT grants at most 479 kB of non-secure RAM on
# nRF54LM20 and 159 kB on nRF54LV10.
#
# Entry points call select_soc() once to update the exported addresses. Callers
# must access them as `abi.CMD_ADDR`. A direct import retains the initial value.


@dataclasses.dataclass(frozen=True)
class ProvLayout:
    """Provisioning firmware and buffer locations for one part."""

    prov_entry_pc: int  # Reset trampoline
    prov_vtor: int  # Image base + 0x800
    prov_sp: int  # Must remain non-secure accessible in PRoT Provisioning
    cmd_addr: int  # Input batch
    cmd_size: int  # PROV_CMD_SIZE
    result_addr: int  # Shared result location
    result_size: int  # PROV_RESULT_SIZE


LAYOUTS: dict[str, ProvLayout] = {
    # nRF54LM20A/B have 479 kB of non-secure RAM.
    "nrf54lm20a": ProvLayout(
        0x20012800, 0x20013000, 0x20032800, 0x20061000, 0x10000, 0x20071000, 0x08000
    ),
    # nRF54LV10A has 159 kB. Buffers sit above the 64 kB image window and the DFU
    # agent data region.
    "nrf54lv10a": ProvLayout(
        0x20012800, 0x20013000, 0x20022800, 0x2002A000, 0x2000, 0x2002C000, 0x1000
    ),
    # nRF54L15 has 224 kB. Reuses 54LV10's image/RAM split (the provisioning
    # images' own content does not scale with the part), then gives the DFU
    # agent a 78 kB carve-out above PROV_STACK_TOP, see prov_layout.h.
    "nrf54l15": ProvLayout(
        0x20012800, 0x20013000, 0x20022800, 0x20036000, 0x2000, 0x20038000, 0x1000
    ),
}
LAYOUTS["nrf54lm20b"] = LAYOUTS["nrf54lm20a"]  # Same memory map

# The part select_soc() chose, and its addresses below. None until a caller
# selects one, so an address read before that fails instead of naming some part.
_ACTIVE_SOC = None
PROV_ENTRY_PC = PROV_VTOR = PROV_SP = None
CMD_ADDR = CMD_SIZE = RESULT_ADDR = RESULT_SIZE = None


def layout_for(soc: str) -> ProvLayout:
    try:
        return LAYOUTS[soc.lower()]
    except KeyError:
        raise parts.PartError(f"no provisioning RAM layout for {soc!r}") from None


def active_soc() -> str | None:
    """Return the part selected for the module-level addresses."""
    return _ACTIVE_SOC


def select_soc(soc: str) -> ProvLayout:
    """Select the module-level addresses for one process."""
    global _ACTIVE_SOC, PROV_ENTRY_PC, PROV_VTOR, PROV_SP
    global CMD_ADDR, CMD_SIZE, RESULT_ADDR, RESULT_SIZE
    lay = layout_for(soc)
    _ACTIVE_SOC = soc.lower()
    PROV_ENTRY_PC = lay.prov_entry_pc
    PROV_VTOR = lay.prov_vtor
    PROV_SP = lay.prov_sp
    CMD_ADDR = lay.cmd_addr
    CMD_SIZE = lay.cmd_size
    RESULT_ADDR = lay.result_addr
    RESULT_SIZE = lay.result_size
    return lay


# Bundle entries in this range are loaded through the debug link. Lower addresses
# are programmed as NVM or UICR payloads.
RAM_BASE = 0x20000000
RAM_END = 0x30000000

# --- CBOR provisioning ABI ---
ABI_VERSION = 9

# ABI values for UROT's ironside_approtect_mode.
APPROTECT_MODE = {"hw-lock": 0, "auth-lock": 1}

# ABI provision-counter role indices.
COUNTER_ROLE = {"urot": 0, "app": 1, "dfu": 2}

# The two IKG SEED entries and the KMU slots each one occupies. These six slots
# are the ones an ERASEALL does not erase, so every operation on them is one-way.
NORDIC_SEED_INDEX = 0
USER_SEED_INDEX = 1
SEED_SLOTS = {NORDIC_SEED_INDEX: (250, 251, 252), USER_SEED_INDEX: (253, 254, 255)}

# set-secapprotect must be the final secure operation because it closes the
# secure AP at the next reset. set-eraseprotect also restricts recovery.
PROTECT_ORDER = ("auxapprotect", "eraseprotect", "secapprotect")

# Result statuses are PSA status codes. The ITE_ERROR_* codes are the
# provisioning ABI's own, in the -1..-128 range PSA reserves for API-specific
# codes. Idempotent write-once operations may return PSA_ERROR_ALREADY_EXISTS.
PSA_SUCCESS = 0
ITE_ERROR_BAD_CMD = -1
ITE_ERROR_FAULT = -2
ITE_ERROR_ABORTED = -3
PSA_ERROR_NOT_PERMITTED = -133
PSA_ERROR_NOT_SUPPORTED = -134
PSA_ERROR_BAD_STATE = -137
PSA_ERROR_ALREADY_EXISTS = -139
PSA_ERROR_CORRUPTION_DETECTED = -151

STATUS_NAMES = {
    0: "PSA_SUCCESS",
    -1: "ITE_ERROR_BAD_CMD",
    -2: "ITE_ERROR_FAULT",
    -3: "ITE_ERROR_ABORTED",
    -132: "PSA_GENERIC_ERROR",
    -133: "PSA_NOT_PERMITTED",
    -134: "PSA_NOT_SUPPORTED",
    -135: "PSA_INVALID_ARGUMENT",
    -136: "PSA_INVALID_HANDLE",
    -137: "PSA_BAD_STATE",
    -138: "PSA_BUFFER_TOO_SMALL",
    -139: "PSA_ALREADY_EXISTS",
    -140: "PSA_DOES_NOT_EXIST",
    -141: "PSA_INSUFFICIENT_MEMORY",
    -142: "PSA_INSUFFICIENT_STORAGE",
    -143: "PSA_INSUFFICIENT_DATA",
    -144: "PSA_SERVICE_FAILURE",
    -145: "PSA_COMMUNICATION_FAILURE",
    -146: "PSA_STORAGE_FAILURE",
    -147: "PSA_HARDWARE_FAILURE",
    -148: "PSA_INSUFFICIENT_ENTROPY",
    -149: "PSA_INVALID_SIGNATURE",
    -150: "PSA_INVALID_PADDING",
    -151: "PSA_CORRUPTION_DETECTED",
    -152: "PSA_DATA_CORRUPT",
    -153: "PSA_DATA_INVALID",
}


def status_name(status: int) -> str:
    """A result status as its number and, when known, its name."""
    name = STATUS_NAMES.get(status)
    return f"{status} {name}" if name else str(status)

# OTA KEKs are provisioned for every generation or not at all.
KEK_GENERATIONS = 3

# SEC1 uncompressed secp256r1 point, 0x04 || X || Y.
IAK_PUBKEY_MAX_SIZE = 65

# The zcbor decoder rejects an entire batch above its maximum quantity.
MAX_BATCH_OPS = 64

# A batch also has to fit the part's command buffer, which is the smaller cap on
# the parts with 8 kB of it: 64 operations of application-key imports do not fit
# there. check_batch_size() holds an encoded batch against it.

# The packed psa_key_attributes_t an import-key operation carries. The device
# refuses any other length.
KEY_ATTRS_SIZE = 28


# cbor2 preserves the CDDL key insertion order.
def encode_batch(operations: list[dict]) -> bytes:
    """Wrap an ordered list of operation maps into a prov-input batch."""
    if len(operations) > MAX_BATCH_OPS:
        raise ValueError(
            f"batch has {len(operations)} operations, maximum {MAX_BATCH_OPS}. "
            "Split the transition into smaller batches."
        )
    return cbor2.dumps({"version": ABI_VERSION, "operations": operations})


# How a batch treats the IKG SEED. "check" writes nothing and requires the
# device to be seeded already. "user" enrolls User SEED, which is permanent, and
# leaves Nordic SEED as the device has it.
SEED_CHECK = "check"
SEED_USER = "user"
SEED_MODES = (SEED_CHECK, SEED_USER)

# The check-ikg-seed seeding codes. SEEDING_NORDIC_FACTORY is for a part given
# Nordic SEED as the Nordic factory does; no SEED mode uses it.
SEEDING_ANY = 0
SEEDING_NORDIC_FACTORY = 1
SEEDING_USER = 2


def seed_mode_checked(code: int) -> str | None:
    """The SEED mode a check-ikg-seed code checks for, or None for another code."""
    return {SEEDING_ANY: SEED_CHECK, SEEDING_USER: SEED_USER}.get(code)


def _check_seed_op(seeding: int, *, unseeded_ok: bool = False) -> dict:
    """Build a check-ikg-seed operation for the seeding code @seeding.

    Carries no continue-on-error, so a device the batch does not fit stops the
    batch before the operations after the check run.
    """
    return {"check-ikg-seed": {"seeding": seeding, "unseeded-ok": unseeded_ok}}


def _seed_ops(seed: str) -> list:
    """The operations a batch runs for the IKG SEED mode @seed.

    The User SEED enrollment is checked twice. The first check, before the
    write, refuses a device whose User SEED is revoked, so nothing is written to
    it. The second confirms the write landed before any IKG consumer runs. The
    write carries continue-on-error, because a device that already holds User
    SEED reports PSA_ERROR_ALREADY_EXISTS.
    """
    if seed == SEED_CHECK:
        return [_check_seed_op(SEEDING_ANY)]
    if seed != SEED_USER:
        raise ValueError(f"IKG SEED mode must be one of {', '.join(SEED_MODES)}, got {seed!r}")
    return [
        _check_seed_op(SEEDING_USER, unseeded_ok=True),
        {"enroll-ikg-seed": USER_SEED_INDEX, "continue-on-error": True},
        _check_seed_op(SEEDING_USER),
    ]


def seed_op_names(seed: str) -> tuple:
    """The operation names the SEED mode @seed contributes to a batch, in order."""
    return tuple(op_name(op) for op in _seed_ops(seed))


# The select-ikg-seed entry codes. auto selects User SEED when it is locked, else
# Nordic SEED.
SELECT_ENTRIES = {"nordic": NORDIC_SEED_INDEX, "user": USER_SEED_INDEX, "auto": 2}


def select_seed_op(entry: str = "auto", *, commit: bool = True) -> dict:
    """Build a select-ikg-seed operation.

    The operation selects the IKG SEED entry the rest of the run derives from.
    With @commit, it writes an erased IKG SEED selection in the base
    configuration; without it, nothing is written and the selection holds for
    this run only. A programmed selection is never replaced: the operation
    checks that it names a locked entry, and @entry if it is not auto. Every IKG
    consumer depends on the selection, so it precedes all of them. Carries no
    continue-on-error, so a device with no entry to select stops the batch.
    """
    if entry not in SELECT_ENTRIES:
        raise ValueError(
            f"IKG SEED selection must be one of {', '.join(SELECT_ENTRIES)}, got {entry!r}"
        )
    return {"select-ikg-seed": {"entry": SELECT_ENTRIES[entry], "commit": commit}}


def _iak_export_op() -> dict:
    """Build an export-iak-pubkey operation.

    Hardware testing shows that this operation hangs without a seed and leaves no
    result. Callers must place it after an operation that fails without a seed.
    """
    return {"export-iak-pubkey": None}


def build_provision_ikg_seed(seed: str) -> bytes:
    """Apply the IKG SEED mode @seed and export the IAK public key.

    UROT does not permit exporting the IAK public key through the runtime NSC,
    so this is the only way to learn it.

    The SEED is selected for this run only. This batch can run before the
    Assembly-and-Test bundle programs the base configuration, and a selection
    it wrote would collide with that write-once programming. The install batch
    that follows writes the selection, and on a device that carries one the
    selection wins over the run-only one.
    """
    return encode_batch([*_seed_ops(seed), select_seed_op(commit=False), _iak_export_op()])


def _u32(name: str, value: int) -> int:
    if not (0 <= value <= 0xFFFFFFFF):
        raise ValueError(f"{name} must fit in a uint32 (0..4294967295), got {value}")
    return value


def _kek_op(kek: bytes, gen: int) -> dict:
    """Build an import-fw-kek operation for a revocable AES-KW unwrap key."""
    if len(kek) != 32:
        raise ValueError(f"KEK gen {gen} must be 32 bytes (AES-256), got {len(kek)}")
    if not (0 <= gen <= 2):
        raise ValueError(f"KEK generation must be 0, 1, or 2, got {gen}")
    return {"import-fw-kek": {"generation": gen, "kek": kek}}


def _import_key_op(attributes: bytes, material: bytes) -> dict:
    """Build an import-key operation for one persistent application key.

    @attributes is the packed psa_key_attributes_t, passed through unchanged: the
    key ID, the persistence and the policy are all inside it.

    Carries no continue-on-error, unlike the seed operations. A key ID already
    holding a key returns PSA_ERROR_ALREADY_EXISTS, which the result readers
    accept as idempotent success -- so a failure that did not stop the batch
    would leave a key ID holding material the host cannot identify, and would
    report clean. Aborting is how the operator finds out.
    """
    if len(attributes) != KEY_ATTRS_SIZE:
        raise ValueError(
            f"key attributes must be {KEY_ATTRS_SIZE} bytes, got {len(attributes)}"
        )
    if not material:
        raise ValueError("import-key carries no key material")
    return {"import-key": {"attributes": bytes(attributes), "key-material": bytes(material)}}


def _validate_kek_count(count: int) -> None:
    """Require all OTA KEK generations or none."""
    if count not in (0, KEK_GENERATIONS):
        raise ValueError(
            f"OTA KEK provisioning accepts exactly {KEK_GENERATIONS} generations "
            f"or none, got {count}"
        )


def _urot_storage_op() -> dict:
    """Build the URoT storage initialization.

    UROT refuses to boot without it. The records are authenticated under the
    MKEK, so it must follow the SEED operations. Carries no
    continue-on-error: an initialized storage reports ALREADY_EXISTS and stops
    the batch.
    """
    return {"init-urot-storage": None}


def _counter_ops(urot_count: int, app_count: int, dfu_count: int) -> list[dict]:
    """Build rollback-counter operations for UROT, App, and DFU.

    Existing valid counters are not reduced.
    """
    return [
        {
            "provision-counter": {
                "role": COUNTER_ROLE["urot"],
                "value": _u32("--urot-count", urot_count),
            }
        },
        {
            "provision-counter": {
                "role": COUNTER_ROLE["app"],
                "value": _u32("--app-count", app_count),
            }
        },
        {
            "provision-counter": {
                "role": COUNTER_ROLE["dfu"],
                "value": _u32("--dfu-count", dfu_count),
            }
        },
    ]


def batch_counters(blob: bytes) -> dict | None:
    """The rollback counters @blob provisions, or None when it provisions none.

    The counter values are the one thing in a built batch a later edit changes,
    so they are readable as well as writable -- a caller holding a batch (a
    spliced provisioning hex, say) can say what floors it would write without
    rebuilding it.

    A batch carrying only some of the roles is refused - no builder here emits
    that shape, and a partial set would leave a floor at whatever it was baked
    with while looking like it had been set.
    """
    role_of = {index: role for role, index in COUNTER_ROLE.items()}
    out = {}
    for op in parse_input_batch(blob)["operations"]:
        counter = op.get("provision-counter")
        if counter is None:
            continue
        role = role_of.get(counter.get("role"))
        if role is None:
            raise ValueError(f"unknown provision-counter role {counter.get('role')!r}")
        out[role] = counter["value"]
    if not out:
        return None
    if sorted(out) != sorted(COUNTER_ROLE):
        raise ValueError(
            f"a batch provisioning any rollback counter provisions all of "
            f"{', '.join(COUNTER_ROLE)}, got {', '.join(sorted(out))}"
        )
    return out


def batch_app_key_ids(blob: bytes) -> tuple:
    """The key IDs the import-key operations in @blob target, in order.

    Read out of the attributes blob rather than taken on trust, so a carried
    batch can be held against the inventory a bundle records beside it: the two
    are written by one command, and a disagreement means one of them was
    patched.
    """
    out = []
    for op in parse_input_batch(blob)["operations"]:
        params = op.get("import-key")
        if params is None:
            continue
        attrs = params["attributes"]
        if len(attrs) != KEY_ATTRS_SIZE:
            raise ValueError(
                f"an import-key operation carries {len(attrs)} bytes of key "
                f"attributes, and the device requires {KEY_ATTRS_SIZE}"
            )
        (key_id,) = struct.unpack_from("<I", attrs, 20)
        out.append(key_id)
    return tuple(out)


def check_batch_size(blob: bytes, soc: str) -> None:
    """Raise unless @blob fits the command buffer of the part @soc names.

    The operation count is not the only cap: the staged batch has to fit the
    buffer the provisioning firmware reads it from, and that is 8 kB on the
    smaller parts. Checked where a batch is built or spliced, because the
    alternative is finding out on the bench after the device has been erased.
    """
    size = layout_for(soc).cmd_size
    if len(blob) > size:
        raise ValueError(
            f"the batch is {len(blob)} bytes and {soc}'s command buffer holds "
            f"{size}. Provision fewer keys in one batch."
        )


def set_batch_counters(blob: bytes, counters: dict) -> bytes:
    """@blob with the rollback counters it provisions set to @counters.

    This edit requires no key material. The import-fw-kek operations are kept
    unchanged, so the result matches a rebuild from the same key source.

    A batch that provisions no counter comes back unchanged.
    """
    batch = parse_input_batch(blob)
    present = batch_counters(blob)
    if present is None:
        return blob
    unknown = sorted(role for role in counters if role not in COUNTER_ROLE)
    if unknown:
        raise ValueError(
            f"unknown counter role(s): {', '.join(unknown)}. "
            f"Expected: {', '.join(COUNTER_ROLE)}"
        )
    wanted = dict(present)
    wanted.update(counters)
    for op in batch["operations"]:
        counter = op.get("provision-counter")
        if counter is not None:
            role = next(r for r, i in COUNTER_ROLE.items() if i == counter["role"])
            counter["value"] = _u32(f"{role} rollback counter", wanted[role])
    return encode_batch(batch["operations"])


def build_provision_fw_kek(kek: bytes, gen: int = 0) -> bytes:
    """Build one firmware KEK import.

    DFU tries generations 2, 1, then 0 and revokes older generations after a
    complete image write.
    """
    return encode_batch([_kek_op(kek, gen)])


def build_provision_counters(urot_count: int, app_count: int, dfu_count: int) -> bytes:
    """Build one rollback-counter operation per firmware role."""
    return encode_batch(_counter_ops(urot_count, app_count, dfu_count))


def build_provision_install(
    urot_count: int, app_count: int, dfu_count: int, keks: list[bytes], app_keys=()
) -> bytes:
    """Build the secure Empty-to-Open install batch.

    The URoT storage precedes the counters, which precede KEKs, which precede
    the application keys. The device stops at the first failure and wipes the
    key-bearing input buffer once.
    """
    _validate_kek_count(len(keks))
    ops = [_urot_storage_op()] + _counter_ops(urot_count, app_count, dfu_count)
    ops += [_kek_op(kek, gen) for gen, kek in enumerate(keks)]
    ops += [_import_key_op(attrs, material) for attrs, material in app_keys]
    return encode_batch(ops)


def _approtect_op(mode: str) -> dict:
    """Build a set-approtect operation and validate direct API callers."""
    if mode not in APPROTECT_MODE:
        raise ValueError(
            f"AP-closure mode must be one of {'/'.join(sorted(APPROTECT_MODE))}, got {mode!r}"
        )
    return {"set-approtect": {"mode": APPROTECT_MODE[mode]}}


def build_set_approtect(mode: str) -> bytes:
    """Build one set-approtect operation."""
    return encode_batch([_approtect_op(mode)])


def build_provision_protect(fields) -> bytes:
    """Build protection operations in the required order.

    set-secapprotect must remain last because it closes the secure AP at reset.
    """
    unknown = [f for f in fields if f not in PROTECT_ORDER]
    if unknown:
        raise ValueError(f"unknown protection register(s): {', '.join(unknown)}")
    if not fields:
        raise ValueError("no protection register selected")
    ordered = [f for f in PROTECT_ORDER if f in fields]
    return encode_batch([{f"set-{f}": None} for f in ordered])


def build_merged(
    *,
    seed=None,
    counters=None,
    keks=(),
    app_keys=(),
    export_iak=False,
    approtect_mode=None,
    protect_fields=(),
) -> bytes:
    """Build an ordered secure batch across lifecycle transitions.

    The secure backend handles set-approtect directly, so the single-phase flow
    does not need the non-secure provisioning image.

    @seed is one of SEED_MODES, or None for a batch with no SEED operations.

    Operations follow these ordering rules:

    * The SEED operations, which end in a check that the device is seeded,
      and then the SEED selection precede every IKG consumer (URoT storage
      initialization, KEK imports, IAK export).
    * KEK imports precede IAK export and irreversible protection. Without a
      seed, KEK import fails cleanly and aborts the batch; IAK export hangs.
    * AP protection precedes erase protection. Secure AP protection is last.
    """
    unknown = [f for f in protect_fields if f not in PROTECT_ORDER]
    if unknown:
        raise ValueError(f"unknown protection register(s): {', '.join(unknown)}")
    _validate_kek_count(len(keks))
    if export_iak and not keks:
        raise ValueError(
            "export-iak-pubkey requires OTA KEK imports in the same batch. "
            "The imports stop the batch safely if no IKG seed is present."
        )
    if seed is not None and protect_fields and not keks:
        raise ValueError(
            "a merged batch cannot enroll the IKG seed and enable protection "
            "without OTA KEK imports. The imports prevent an unseeded device "
            "from being locked."
        )

    ops = [] if seed is None else _seed_ops(seed)
    if counters is not None or keks or app_keys or export_iak:
        ops.append(select_seed_op())
    if counters is not None:
        ops.append(_urot_storage_op())
        ops += _counter_ops(*counters)
    ops += [_kek_op(kek, gen) for gen, kek in enumerate(keks)]
    ops += [_import_key_op(attrs, material) for attrs, material in app_keys]
    if export_iak:
        ops.append(_iak_export_op())
    if approtect_mode is not None:
        ops.append(_approtect_op(approtect_mode))
    ops += [{f"set-{f}": None} for f in PROTECT_ORDER if f in protect_fields]
    if not ops:
        raise ValueError("merged batch would be empty")
    return encode_batch(ops)


def op_name(op: dict) -> str:
    """Return the operation name, excluding optional flags."""
    names = [k for k in op if k != "continue-on-error"]
    return names[0] if len(names) == 1 else "?(%s)" % ",".join(sorted(op))


def parse_input_batch(buf: bytes) -> dict:
    """Decode and validate a provisioning input batch."""
    try:
        batch = cbor2.loads(buf)
    except Exception as exc:  # noqa: BLE001 - any undecodable input is an error
        raise ValueError(f"not decodable CBOR ({exc})") from exc
    if not isinstance(batch, dict) or "operations" not in batch:
        raise ValueError(
            "not a provisioning input batch: missing 'operations'. The file may be a result batch."
        )
    if batch.get("version") != ABI_VERSION:
        raise ValueError(f"batch ABI version {batch.get('version')} != {ABI_VERSION}")
    if not isinstance(batch["operations"], list) or not batch["operations"]:
        raise ValueError("batch has no operations")
    return batch


def parse_result(buf: bytes) -> dict:
    """Decode one provisioning result from RESULT_ADDR.

    Bytes after the top-level CBOR item may contain stale data. Results align
    positionally with input operations.
    """
    try:
        out = cbor2.CBORDecoder(io.BytesIO(buf)).decode()
    except Exception as exc:  # noqa: BLE001 - any malformed buffer is a hard error
        raise RuntimeError(
            f"result buffer is not decodable CBOR ({exc}). "
            "The provisioning image may not have finished, or the "
            "wrong buffer was read."
        ) from exc
    if not isinstance(out, dict) or "version" not in out or "results" not in out:
        raise RuntimeError(f"result is not a prov-output batch: {out!r}")
    if out["version"] != ABI_VERSION:
        raise RuntimeError(f"result ABI version {out['version']} != {ABI_VERSION}")
    return out


def _self_check() -> int:
    """Check batch ordering and fail-closed guards."""
    keks = [bytes([g]) * 32 for g in range(KEK_GENERATIONS)]
    counters = (0, 0, 0)
    results = []

    def names(buf: bytes) -> list:
        return [op_name(o) for o in parse_input_batch(buf)["operations"]]

    def value(buf: bytes, name: str):
        for op in parse_input_batch(buf)["operations"]:
            if name in op:
                return op[name]
        raise AssertionError(f"{name} is not in the batch")

    def check(name: str, got, want) -> None:
        ok = got == want
        results.append(ok)
        print(
            f"  {'ok  ' if ok else 'FAIL'} {name}"
            + ("" if ok else f"\n       got  {got}\n       want {want}")
        )

    def raises(name: str, fn) -> None:
        try:
            fn()
        except ValueError:
            results.append(True)
            print(f"  ok   {name}")
        else:
            results.append(False)
            print(f"  FAIL {name}: no ValueError")

    def seed_args(buf: bytes) -> list:
        return [
            (op_name(o), o[op_name(o)])
            for o in parse_input_batch(buf)["operations"]
            if op_name(o) in ("check-ikg-seed", "enroll-ikg-seed")
        ]

    check(
        "check writes no SEED slot and accepts a device seeded either way",
        seed_args(build_provision_ikg_seed("check")),
        [("check-ikg-seed", {"seeding": 0, "unseeded-ok": False})],
    )
    check(
        "a standalone seed batch exports the IAK last",
        names(build_provision_ikg_seed("check"))[-1],
        "export-iak-pubkey",
    )
    check(
        "user checks it can be reached, enrolls User SEED, then checks it landed",
        seed_args(build_provision_ikg_seed("user")),
        [
            ("check-ikg-seed", {"seeding": 2, "unseeded-ok": True}),
            ("enroll-ikg-seed", 1),
            ("check-ikg-seed", {"seeding": 2, "unseeded-ok": False}),
        ],
    )
    check(
        "user: only the enrollment continues on error",
        [
            op_name(o)
            for o in parse_input_batch(build_provision_ikg_seed("user"))["operations"]
            if o.get("continue-on-error")
        ],
        ["enroll-ikg-seed"],
    )
    for code, mode in ((0, "check"), (1, None), (2, "user")):
        check(f"seeding code {code} names its SEED mode", seed_mode_checked(code), mode)
    raises("an unknown SEED mode is refused", lambda: build_provision_ikg_seed("bogus"))

    # KEK imports separate the SEED from IAK export and protection.
    full = build_merged(
        seed="user",
        counters=counters,
        keks=keks,
        export_iak=True,
        approtect_mode="hw-lock",
        protect_fields=PROTECT_ORDER,
    )
    check(
        "merged Unseeded->Secured is 16 ops in guide order",
        names(full),
        list(seed_op_names("user"))
        + ["select-ikg-seed", "init-urot-storage"]
        + ["provision-counter"] * 3
        + ["import-fw-kek"] * 3
        + ["export-iak-pubkey", "set-approtect"]
        + [f"set-{f}" for f in PROTECT_ORDER],
    )
    check(
        "the IAK export follows the last KEK import",
        names(full).index("export-iak-pubkey")
        > max(i for i, n in enumerate(names(full)) if n == "import-fw-kek"),
        True,
    )
    # The IKG consumers derive from the active SEED, so the check that it is
    # seeded runs before all of them.
    ikg_consumers = ("init-urot-storage", "import-fw-kek", "export-iak-pubkey")
    for seed in SEED_MODES:
        merged = names(
            build_merged(seed=seed, counters=counters, keks=keks, export_iak=True)
        )
        last_check = max(i for i, n in enumerate(merged) if n == "check-ikg-seed")
        first_consumer = min(i for i, n in enumerate(merged) if n in ikg_consumers)
        check(
            f"{seed}: the SEED is checked, then selected, before every IKG consumer",
            last_check < merged.index("select-ikg-seed") < first_consumer,
            True,
        )
    check(
        "an install batch selects from the slot states and commits the selection",
        value(build_merged(counters=counters, keks=keks), "select-ikg-seed"),
        {"entry": 2, "commit": True},
    )
    check(
        "a standalone seed batch selects from the slot states for its run only",
        value(build_provision_ikg_seed("check"), "select-ikg-seed"),
        {"entry": 2, "commit": False},
    )
    check(
        "a run-only selection names its entry and commits nothing",
        select_seed_op("nordic", commit=False),
        {"select-ikg-seed": {"entry": 0, "commit": False}},
    )
    raises("an unknown selection entry is refused", lambda: select_seed_op("both"))
    check(
        "a standalone seed batch selects the SEED before the IAK export",
        names(build_provision_ikg_seed("user"))[-2:],
        ["select-ikg-seed", "export-iak-pubkey"],
    )

    # Caller order is normalized so set-secapprotect remains last.
    check(
        "scrambled protect fields are re-ordered",
        names(build_merged(protect_fields=("secapprotect", "auxapprotect", "eraseprotect"))),
        [f"set-{f}" for f in PROTECT_ORDER],
    )
    check(
        "build_provision_protect orders a scrambled subset too",
        names(build_provision_protect(("secapprotect", "auxapprotect"))),
        ["set-auxapprotect", "set-secapprotect"],
    )
    check(
        "install-only span selects the SEED and carries no other identity ops",
        names(build_merged(counters=counters, keks=keks)),
        ["select-ikg-seed", "init-urot-storage"]
        + ["provision-counter"] * 3
        + ["import-fw-kek"] * 3,
    )
    check(
        "a protection-only batch does not select the SEED",
        names(build_merged(protect_fields=PROTECT_ORDER)),
        [f"set-{f}" for f in PROTECT_ORDER],
    )
    raises(
        "IAK export without the KEK gate is refused",
        lambda: build_merged(seed="check", counters=counters, export_iak=True),
    )
    raises(
        "sealing a seed batch without the KEK gate is refused",
        lambda: build_merged(seed="check", counters=counters, protect_fields=("secapprotect",)),
    )
    raises("partial KEK generations are refused", lambda: build_merged(keks=keks[:2]))
    raises(
        "a batch longer than the device decoder is refused",
        lambda: encode_batch([{"set-auxapprotect": None}] * (MAX_BATCH_OPS + 1)),
    )
    raises("an empty merged batch is refused", build_merged)

    # --- application keys, the third bake input ---
    app_attrs = struct.pack("<HHIIIIII", 0x2400, 256, 0x01, 0x00030300, 0, 0, 0x50, 0)
    app_keys = ((app_attrs, bytes(32)),)
    with_keys = build_merged(
        seed="user", counters=counters, keks=keks, app_keys=app_keys,
        export_iak=True, approtect_mode="auth-lock", protect_fields=PROTECT_ORDER,
    )
    # After the KEKs: those are the write-once block DFU cannot work without, and
    # the batch stops at the first failure, so an application-key authoring error
    # leaves every Nordic-owned write already done. Before the export and the
    # irreversible protection operations, so a failed import cannot reach them.
    check(
        "application keys import after the KEKs and before the IAK export",
        names(with_keys),
        [
            *seed_op_names("user"), "select-ikg-seed", "init-urot-storage",
            "provision-counter", "provision-counter", "provision-counter",
            "import-fw-kek", "import-fw-kek", "import-fw-kek",
            "import-key",
            "export-iak-pubkey", "set-approtect",
            "set-auxapprotect", "set-eraseprotect", "set-secapprotect",
        ],
    )
    check(
        "an import-key operation names its two blobs and tolerates no failure",
        value(with_keys, "import-key"),
        {"attributes": app_attrs, "key-material": bytes(32)},
    )
    check(
        "the key ID an import-key targets is read back out of its attributes",
        batch_app_key_ids(with_keys),
        (0x50,),
    )
    check(
        "a batch with no application keys is the batch it was before",
        build_merged(counters=counters, keks=keks, app_keys=()),
        build_merged(counters=counters, keks=keks),
    )
    raises(
        "key attributes of the wrong length are refused",
        lambda: build_merged(counters=counters, app_keys=((app_attrs[:-1], bytes(32)),)),
    )
    raises(
        "an import-key operation with no material is refused",
        lambda: build_merged(counters=counters, app_keys=((app_attrs, b""),)),
    )
    # The operation count is not the only cap. A full batch of the widest keys
    # (a secp384r1 public point is 97 bytes of material) overruns the 8 kB
    # command buffer the smaller parts read the batch from.
    wide = ((app_attrs, bytes(97)),) * (MAX_BATCH_OPS - 5)
    raises(
        "a batch too large for the smallest command buffer is refused",
        lambda: check_batch_size(
            build_merged(counters=counters, app_keys=wide), "nrf54l15"
        ),
    )
    check(
        "the largest command buffer holds what the smallest cannot",
        check_batch_size(build_merged(counters=counters, app_keys=wide), "nrf54lm20b"),
        None,
    )

    bad = results.count(False)
    print(f"{len(results) - bad}/{len(results)} passed")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(_self_check())
