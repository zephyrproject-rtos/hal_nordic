#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Run a provisioning RAM image with an encoded operation batch.

Reading a batch from standard input keeps raw KEKs off disk. NVM and UICR data
must be programmed before running a factory bundle.
"""

import argparse
import hashlib
import json
import struct
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import parts
from lib import prov_batches as pb

# Provisioning RAM addresses are per-part and must be read through `abi` after
# main() calls select_soc(). Importing them by name would retain reference-part
# values.
from lib import prov_abi as abi
from lib.prov_abi import (
    ABI_VERSION,
    APPROTECT_MODE,
    KEK_GENERATIONS,
    PROTECT_ORDER,
    PSA_ERROR_ALREADY_EXISTS,
    RAM_BASE,
    RAM_END,
    build_provision_counters,
    build_provision_fw_kek,
    build_provision_ikg_seed,
    build_provision_install,
    build_provision_protect,
    build_set_approtect,
    parse_input_batch,
    parse_result,
)

# This device-identity inventory is excluded from version control.
DEFAULT_IAK_REGISTRY = Path(__file__).resolve().parent.parent.parent / "iak_registry.json"


# IAK public-key registry
def register_iak_pubkey(registry_path: Path, pubkey: bytes, *, serial=None) -> str:
    """Register an IAK public key by its SHA-256 hash.

    The registry key matches the token instance-id after removing the leading
    UEID type byte from `0x01 || SHA-256(pubkey)` (EAT UEID label 256).
    """
    key_hex = hashlib.sha256(pubkey).hexdigest()

    registry = {}
    if registry_path.is_file():
        registry = json.loads(registry_path.read_text())

    registry[key_hex] = {"pubkey": pubkey.hex(), "serial": serial}
    registry_path.parent.mkdir(parents=True, exist_ok=True)
    registry_path.write_text(json.dumps(registry, indent=2, sort_keys=True) + "\n")
    return key_hex


# J-Link device control
def _read_words(hexfile: Path, *, ram_only=False):
    """Yield word-aligned data for each selected Intel HEX segment.

    With ram_only, omit NVM and UICR data that was programmed separately.
    """
    from intelhex import IntelHex

    ih = IntelHex(str(hexfile))
    for start, end in ih.segments():
        if ram_only and not (RAM_BASE <= start < RAM_END):
            continue
        data = ih.tobinarray(start=start, size=end - start)
        # The linker word-aligns RAM segments.
        pad = (-len(data)) % 4
        data = bytes(data) + b"\xff" * pad
        words = list(struct.unpack(f"<{len(data) // 4}I", data))
        yield start, words


def _staged_batch(hexfile: Path) -> bytes:
    """Return the batch stored at CMD_ADDR, or b"" if none is present."""
    from intelhex import IntelHex

    ih = IntelHex(str(hexfile))
    for start, end in ih.segments():
        if start <= abi.CMD_ADDR < end:
            return bytes(ih.tobinarray(start=abi.CMD_ADDR, size=end - abi.CMD_ADDR))
    return b""


def _reset_and_catch(jlink, ns):
    """Reset and halt in the required security state.

    Cortex-M33 resets into secure state. The non-secure path must reset without
    halting, allow the boot chain to hand off, then halt in non-secure flash.
    """
    import pylink

    RESET_ATTEMPTS = 5
    NS_SETTLE_MS = (0.02, 0.05, 0.1, 0.25, 0.5)
    for attempt in range(1, RESET_ATTEMPTS + 1):
        try:
            if ns:
                jlink.reset(ms=10, halt=False)
                time.sleep(NS_SETTLE_MS[min(attempt - 1, len(NS_SETTLE_MS) - 1)])
                jlink.halt()
            else:
                jlink.reset(ms=10, halt=True)
            # A halt request can return before the core stops.
            settle = time.time() + 3
            while time.time() < settle and not jlink.halted():
                time.sleep(0.05)
            if not jlink.halted():
                raise pylink.errors.JLinkException("core did not halt after reset")
            pc = jlink.register_read(15)
            if ns and not (0x00023000 <= pc <= 0x00200000):
                # PC must be in the resident App or DFU image.
                raise pylink.errors.JLinkException(
                    f"did not catch a clean NS entry (PC={pc:#010x})"
                )
            return pc
        except pylink.errors.JLinkException as exc:
            if attempt == RESET_ATTEMPTS:
                raise SystemExit(
                    f"prov_run: could not halt the non-secure image after reset: {exc}"
                )
            time.sleep(0.5)


def _halt_live_ns(jlink, attempts=5):
    """Halt a running App or DFU core and return its PC.

    Retry transient halt failures and halts during secure NSC calls.
    """
    import pylink

    last = "no attempt"
    for attempt in range(1, attempts + 1):
        try:
            jlink.halt()
            settle = time.time() + 3
            while time.time() < settle and not jlink.halted():
                time.sleep(0.05)
            if not jlink.halted():
                raise pylink.errors.JLinkException("core did not halt")
            pc = jlink.register_read(15)
            if not (0x00023000 <= pc <= 0x00200000):
                raise pylink.errors.JLinkException(f"halted outside NS flash (PC={pc:#010x})")
            return pc
        except pylink.errors.JLinkException as exc:
            last = str(exc)
            if attempt < attempts:
                try:
                    jlink.restart()
                except pylink.errors.JLinkException:
                    pass
                time.sleep(0.1)
    raise SystemExit(
        f"prov_run: could not halt the non-secure image: {last}. "
        "Power-cycle the board or use --reset."
    )


STALL_REGISTER_NAMES = (
    "R15 (PC)", "R14", "R13 (SP)", "XPSR",
    "MSP", "PSP", "MSP_S", "PSP_S", "MSP_NS", "PSP_NS",
    "CONTROL", "CONTROL_S", "CONTROL_NS",
)

SCS_BASE = 0xE000E000
CFSR_ADDR = SCS_BASE + 0xD28
HFSR_ADDR = SCS_BASE + 0xD2C
XPSR_IPSR_MASK = 0x1FF


def _stall_diagnosis(jlink, mode, loaded) -> str:
    """Describe a provisioning image that did not finish, from the halted core.

    `loaded` is the (first, last) span the image occupies. A PC outside it is
    ambiguous and is called out: a device that faulted and reset reads the same
    as one that was never redirected, since the reset clears CFSR and HFSR and
    restores the resident image.

    A probe that cannot halt the core reports that rather than replacing the
    timeout with an error of its own.
    """
    import pylink

    try:
        jlink.halt()
        settle = time.time() + 3
        while time.time() < settle and not jlink.halted():
            time.sleep(0.05)
        if not jlink.halted():
            return "    core did not halt for diagnosis"

        idx_map = _register_indices(jlink)
        regs = {}
        lines = []
        for name in STALL_REGISTER_NAMES:
            idx = idx_map.get(name)
            if idx is None:
                lines.append(f"    {name:12s} = <unavailable on this probe>")
                continue
            regs[name] = jlink.register_read(idx)
            lines.append(f"    {name:12s} = {regs[name]:#010x}")

        if "XPSR" in regs:
            ipsr = regs["XPSR"] & XPSR_IPSR_MASK
            lines.append(
                f"    {'IPSR':12s} = {ipsr} "
                + ("(Thread mode)" if ipsr == 0 else f"(exception #{ipsr} active)")
            )

        cfsr = jlink.memory_read32(CFSR_ADDR, 1)[0]
        hfsr = jlink.memory_read32(HFSR_ADDR, 1)[0]
        lines.append(f"    CFSR = {cfsr:#010x}   HFSR = {hfsr:#010x}")
        lines.append(
            f"    entered at PC={abi.PROV_ENTRY_PC:#010x} SP={abi.PROV_SP:#010x} "
            f"VTOR={abi.PROV_VTOR:#010x}, {mode} mode"
        )
        lines.append(
            f"    {abi.active_soc()} layout: batch at {abi.CMD_ADDR:#010x} "
            f"({abi.CMD_SIZE:#x} bytes), result at {abi.RESULT_ADDR:#010x}"
        )

        pc = regs.get("R15 (PC)")
        if pc is not None and loaded and not (loaded[0] <= (pc & ~1) <= loaded[1]):
            lines.append(
                f"    The core is outside the provisioning image "
                f"({loaded[0]:#010x}..{loaded[1]:#010x}), so either the redirect "
                f"never took or the device faulted and reset and this is the "
                f"resident image. A reset clears CFSR and HFSR, so the two read "
                f"alike here. Read the boot report on the console for a recorded "
                f"fault and the reset reason before suspecting the halt."
            )
        return "\n".join(lines)
    except pylink.errors.JLinkException as exc:
        return f"    could not read the halted core: {exc}"


def _register_indices(jlink) -> dict[str, int]:
    """Map each core register name the probe lists to its index.

    pylink resolves a name to an index of its own, which from MSP_NS upward is
    not the probe's: register_write("MSP_NS") writes FPS31. Access a register
    by name only through this map.
    """
    return {jlink.register_name(i): i for i in jlink.register_list()}


def run_on_device(
    cmd_buf: bytes,
    prov_image: Path,
    *,
    serial,
    mode,
    device,
    timeout_s,
    dry_run,
    reset_first=False,
    ram_only=False,
    inspect=None,
) -> dict:
    """Run a provisioning image and return its decoded output.

    An empty cmd_buf preserves a batch already loaded from a bundle. ram_only
    restricts bundle loading to the provisioning RAM window. inspect, if given,
    is called with the J-Link while the image is still halted.
    """
    from lib.jlink_util import create_jlink, resolve_jlink_serial

    serial = resolve_jlink_serial(serial)
    if dry_run:
        print(
            f"load {prov_image} at {abi.PROV_ENTRY_PC:#010x}, stage "
            f"{len(cmd_buf)} bytes at {abi.CMD_ADDR:#010x}, start {mode} mode"
        )
        result = {"version": ABI_VERSION, "results": []}
        return result

    import pylink

    if len(cmd_buf) > abi.CMD_SIZE:
        raise SystemExit(
            f"prov_run: command buffer is {len(cmd_buf)} bytes (maximum: {abi.CMD_SIZE})"
        )

    ns = mode == "ns"

    jlink = create_jlink()
    jlink.open(serial_no=serial)
    jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
    jlink.connect(device)
    try:
        # Halt in the required security state.
        if ns and not reset_first:
            # UROT has already configured the non-secure state and NSC gateway.
            _halt_live_ns(jlink)
        else:
            _reset_and_catch(jlink, ns)
        # The span is kept so a timeout can say whether the core is still inside
        # the image it was handed.
        loaded = None
        for addr, words in _read_words(prov_image, ram_only=ram_only):
            jlink.memory_write32(addr, words)
            end = addr + 4 * len(words) - 1
            loaded = (addr, end) if loaded is None else (min(loaded[0], addr),
                                                         max(loaded[1], end))

        # Pad CBOR before unpacking into words. Clear the result header so stale
        # magic cannot indicate completion.
        if cmd_buf:
            padded = cmd_buf + b"\x00" * ((-len(cmd_buf)) % 4)
            jlink.memory_write32(abi.CMD_ADDR, list(struct.unpack(f"<{len(padded) // 4}I", padded)))
        jlink.memory_write32(abi.RESULT_ADDR, [0, 0, 0, 0])

        # Both images use fixed PC, VTOR, and SP ABI addresses. Only the banked
        # VTOR and stack registers differ by security state.
        VTOR_NS, VTOR_S = 0xE002ED08, 0xE000ED08
        jlink.memory_write32(VTOR_NS if ns else VTOR_S, [abi.PROV_VTOR])
        reg = _register_indices(jlink)
        if ns:
            # The live image's stack pointers and stack limits stay set across
            # the redirect, and a PROV_SP below a limit faults the first push.
            for name, value in (("MSP_NS", abi.PROV_SP), ("PSP_NS", abi.PROV_SP),
                                ("MSPLIM_NS", 0), ("PSPLIM_NS", 0)):
                jlink.register_write(reg[name], value)
        else:
            jlink.register_write(reg["MSP"], abi.PROV_SP)
        jlink.register_write(13, abi.PROV_SP)
        jlink.register_write(15, abi.PROV_ENTRY_PC & ~1)
        jlink.register_write(reg["XPSR"], 0x01000000)  # Thumb bit
        jlink.restart()

        # The ABI has no in-band completion marker. Final BKPT signals completion.
        deadline = time.time() + timeout_s
        while time.time() < deadline:
            if jlink.halted():
                break
            time.sleep(0.02)
        else:
            raise SystemExit(
                f"prov_run: image did not finish within {timeout_s}s\n"
                + _stall_diagnosis(jlink, mode, loaded)
            )

        # The decoder stops at the end of the CBOR map.
        raw = bytes(bytearray(jlink.memory_read8(abi.RESULT_ADDR, 2048)))
        if inspect is not None:
            # Closing the session resumes the core.
            inspect(jlink)
        return parse_result(raw)
    finally:
        jlink.close()


def _kek_material(args, gens) -> list:
    """Read raw KEKs from files or a key source.

    Return an empty list when no KEK operations were requested. Key-source data
    remains in memory and is placed only in the device-side batch.
    """
    # provision-fw-kek uses one path. provision-install accepts repeated paths.
    files = args.kek_file
    files = [] if files is None else ([files] if isinstance(files, Path) else list(files))
    if args.keyset and files:
        raise ValueError("give either --kek-file or --keyset, not both")
    if files:
        return [f.read_bytes() for f in files]
    if not args.keyset:
        return []
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
    import iron_keychain as kc

    try:
        source = kc.KeySource.parse(args.keyset)
        out = []
        for gen in gens:
            key_id = kc.kek_key_id(gen)
            data = source.get_bytes(key_id)
            if data is None:
                raise ValueError(f"no {key_id} in {source.describe()}")
            out.append(data)
        return out
    except kc.KeySourceError as exc:
        raise ValueError(str(exc)) from exc


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        description="Load IronSide provisioning firmware and a provisioning "
        "batch into device RAM over J-Link, then return the results. "
        "Choose a command to create a standard batch or run a "
        "prepared batch or bundle."
    )
    sub = ap.add_subparsers(dest="cmd", required=True, title="commands", metavar="COMMAND")

    sa = sub.add_parser("set-approtect", help="set the APPROTECT mode")
    sa.add_argument("lock", choices=sorted(APPROTECT_MODE), help="protection mode")

    seed = sub.add_parser(
        "provision-ikg-seed",
        help="check or seed the IKG SEED and register the IAK public key",
    )
    seed.add_argument(
        "--provision-user-seed",
        choices=tuple(pb.USER_SEED_CHOICES),
        default="no",
        help="no requires the device to be seeded already and writes no SEED slot; "
        "yes enrolls User SEED permanently (default: %(default)s)",
    )
    seed.add_argument(
        "--registry",
        type=Path,
        default=DEFAULT_IAK_REGISTRY,
        help=f"IAK public key registry (default: {DEFAULT_IAK_REGISTRY})",
    )

    kek = sub.add_parser("provision-fw-kek", help="provision an OTA KEK")
    kek.add_argument("--kek-file", type=Path, help="raw 32-byte binary KEK file")
    kek.add_argument(
        "--gen", type=int, choices=(0, 1, 2), default=0, help="KEK generation (default: 0)"
    )

    counters = sub.add_parser("provision-counters", help="provision rollback counters")
    counters.add_argument(
        "--urot-count", type=int, default=0, help="initial UROT counter (default: 0)"
    )
    counters.add_argument(
        "--app-count", type=int, default=0, help="initial App counter (default: 0)"
    )
    counters.add_argument(
        "--dfu-count", type=int, default=0, help="initial DFU counter (default: 0)"
    )

    install = sub.add_parser("provision-install", help="provision rollback counters and OTA KEKs")
    install.add_argument(
        "--urot-count", type=int, default=0, help="initial UROT counter (default: 0)"
    )
    install.add_argument(
        "--app-count", type=int, default=0, help="initial App counter (default: 0)"
    )
    install.add_argument(
        "--dfu-count", type=int, default=0, help="initial DFU counter (default: 0)"
    )
    install.add_argument(
        "--kek-file",
        type=Path,
        action="append",
        default=[],
        metavar="FILE",
        help=f"raw 32-byte binary KEK file in generation order (repeat {KEK_GENERATIONS} times)",
    )

    # No default key source is used because absent KEK input means counters only.
    for p in (kek, install):
        p.add_argument("--keyset", dest="keyset", metavar="SPEC", help="key source as keyring:NAME or dir:PATH")

    protect = sub.add_parser("provision-protect", help="set UICR protection registers")
    for f in PROTECT_ORDER:
        protect.add_argument(f"--{f}", action="store_true", help=f"set {f.upper()}")

    # run-batch accepts a batch composed from lifecycle and runtime inputs.
    batch = sub.add_parser("run-batch", help="run an encoded provisioning batch")
    batch.add_argument(
        "--batch-file",
        type=Path,
        required=True,
        metavar="FILE",
        help="CBOR batch file (- reads standard input)",
    )
    batch.add_argument("--json", action="store_true", help="print results as JSON")

    # Factory bundles already carry their batch at CMD_ADDR.
    bundle = sub.add_parser("run-bundle", help="run the provisioning batch in a bundle")
    bundle.add_argument(
        "--bundle",
        type=Path,
        required=True,
        metavar="HEX",
        help="bundle HEX file from ironside_bundle.py build-production-hex",
    )
    bundle.add_argument("--json", action="store_true", help="print results as JSON")

    # APPROTECT defaults to the non-secure backend. Other operations default to
    # the secure Assembly and Test backend.
    for p, default_mode in (
        (sa, "ns"),
        (seed, "secure"),
        (kek, "secure"),
        (counters, "secure"),
        (install, "secure"),
        (protect, "secure"),
        (batch, "secure"),
        (bundle, "secure"),
    ):
        if p is not bundle:
            p.add_argument(
                "--prov-image", type=Path, required=True, help="provisioning firmware HEX file"
            )
        p.add_argument("--serial", "--serial-number", type=int, default=None, help="J-Link serial number")
        # Provisioning RAM addresses differ by part. Detect the part from the
        # selected board unless --soc is given.
        p.add_argument(
            "--soc",
            default=None,
            help=f"SoC ({', '.join(sorted(abi.LAYOUTS))}, default: detected from board)",
        )
        p.add_argument(
            "--mode",
            choices=("auto", "secure", "ns"),
            default=default_mode,
            help=f"provisioning mode (default: {default_mode})",
        )
        p.add_argument(
            "--device", default="nRF54LM20A_M33", help="J-Link device name (default: %(default)s)"
        )
        p.add_argument(
            "--timeout",
            type=float,
            default=10.0,
            metavar="S",
            help="timeout in seconds (default: %(default)s)",
        )
        p.add_argument(
            "--dry-run", action="store_true", help="print actions without accessing the device"
        )
        p.add_argument(
            "--reset", action="store_true", help="reset before starting non-secure provisioning"
        )

    args = ap.parse_args(argv)

    # Select the per-part address layout before reading any ABI address.
    try:
        soc = args.soc or parts.for_serial(getattr(args, "serial", None)).soc
        abi.select_soc(soc)
    except parts.PartError as exc:
        hint = "" if args.soc else "\nName the part with --soc."
        raise SystemExit(f"prov_run: {exc}{hint}") from None

    accept = {0}
    # The decoded batch, for a command that runs one with a SEED check.
    probe = None
    # A bare image loads in full. A bundle loads only its RAM window.
    prov_image, ram_only = getattr(args, "prov_image", None), False

    if args.cmd == "set-approtect":
        cmd_buf = build_set_approtect(args.lock)
        label = f"set-approtect {args.lock}"
    elif args.cmd == "provision-ikg-seed":
        cmd_buf = build_provision_ikg_seed(pb.USER_SEED_CHOICES[args.provision_user_seed])
        probe = parse_input_batch(cmd_buf)
        label = f"provision-ikg-seed (provision-user-seed {args.provision_user_seed})"
    elif args.cmd == "provision-fw-kek":
        try:
            keks = _kek_material(args, [args.gen])
        except (ValueError, OSError) as exc:
            raise SystemExit(f"prov_run: {exc}")
        if len(keks) != 1:
            raise SystemExit("prov_run: pass one --kek-file or --keyset")
        cmd_buf = build_provision_fw_kek(keks[0], args.gen)
        label = f"provision-fw-kek (gen={args.gen})"
        accept.add(PSA_ERROR_ALREADY_EXISTS)
    elif args.cmd == "provision-counters":
        try:
            cmd_buf = build_provision_counters(args.urot_count, args.app_count, args.dfu_count)
        except ValueError as exc:
            raise SystemExit(f"prov_run: {exc}")
        label = (
            f"provision-counters (urot={args.urot_count}, "
            f"app={args.app_count}, dfu={args.dfu_count})"
        )
        # Counter pairs are write-once. A failed batch requires returning the
        # device to Empty and repeating installation.
        accept.add(PSA_ERROR_ALREADY_EXISTS)
    elif args.cmd == "provision-install":
        try:
            keks = _kek_material(args, range(KEK_GENERATIONS))
            cmd_buf = build_provision_install(args.urot_count, args.app_count, args.dfu_count, keks)
        except (ValueError, OSError) as exc:
            raise SystemExit(f"prov_run: {exc}")
        label = (
            f"provision-install (urot={args.urot_count}, app={args.app_count}, "
            f"dfu={args.dfu_count}, kek-gens={len(keks)})"
        )
        # The batch covers counters and all KEK generations. A failure requires
        # returning the device to Empty and repeating installation.
        accept.add(PSA_ERROR_ALREADY_EXISTS)
    elif args.cmd == "provision-protect":
        fields = [f for f in PROTECT_ORDER if getattr(args, f.replace("-", "_"))]
        try:
            cmd_buf = build_provision_protect(fields)
        except ValueError as exc:
            raise SystemExit(f"prov_run: {exc}")
        label = f"provision-protect ({', '.join(fields)})"
        # The image writes only erased protection registers.
    elif args.cmd == "run-batch":
        if str(args.batch_file) == "-":
            cmd_buf = sys.stdin.buffer.read()
        else:
            try:
                cmd_buf = args.batch_file.read_bytes()
            except OSError as exc:
                raise SystemExit(f"prov_run: {exc}")
        if not cmd_buf:
            raise SystemExit("prov_run: empty batch")
        # Validate the batch before sending it to the device.
        try:
            probe = parse_input_batch(cmd_buf)
        except ValueError as exc:
            raise SystemExit(f"prov_run: --batch-file {exc}")
        label = f"run-batch ({len(probe['operations'])} ops, {len(cmd_buf)} B)"
        # All write-once ABI operations treat ALREADY_EXISTS as idempotent.
        accept.add(PSA_ERROR_ALREADY_EXISTS)
    elif args.cmd == "run-bundle":
        # Loading the bundle RAM window delivers both firmware and batch. Do not
        # restage a decoded copy of the batch.
        if not args.bundle.is_file():
            raise SystemExit(f"prov_run: bundle not found: {args.bundle}")
        prov_image, ram_only = args.bundle, True
        try:
            staged = _staged_batch(args.bundle)
        except (OSError, ValueError) as exc:
            raise SystemExit(f"prov_run: --bundle {exc}")
        # Decode the stored batch for validation and reporting.
        try:
            probe = parse_input_batch(staged)
        except ValueError as exc:
            raise SystemExit(
                f"prov_run: {args.bundle} has no provisioning batch at {abi.CMD_ADDR:#010x}: {exc}"
            )
        cmd_buf = b""
        label = (
            f"run-bundle ({len(probe['operations'])} ops, {len(staged)} B "
            f"already at {abi.CMD_ADDR:#010x})"
        )
        accept.add(PSA_ERROR_ALREADY_EXISTS)
    else:
        return 2

    if not prov_image.is_file() and not args.dry_run:
        raise SystemExit(f"prov_run: provisioning image not found: {prov_image}")
    res = run_on_device(
        cmd_buf,
        prov_image,
        serial=args.serial,
        mode=args.mode,
        device=args.device,
        timeout_s=args.timeout,
        dry_run=args.dry_run,
        reset_first=args.reset,
        ram_only=ram_only,
    )
    results = res["results"]

    seed_problem = None
    if probe is not None and not args.dry_run:
        seed_problem = pb.seed_failure(probe["operations"], results)
    if seed_problem is not None:
        print(f"prov_run: {seed_problem}", file=sys.stderr)

    if args.cmd == "provision-ikg-seed":
        if args.dry_run:
            return 0
        # The SEED writes are idempotent. Public-key export must return success.
        statuses = [r["status"] for r in results]
        pubkey = results[-1].get("pubkey") if results else None
        ok = (
            len(statuses) == len(probe["operations"])
            and all(s in (0, PSA_ERROR_ALREADY_EXISTS) for s in statuses[:-1])
            and statuses[-1] == 0
        )
        print(f"{label}: {', '.join(abi.status_name(s) for s in statuses)}")
        if ok and pubkey:
            registry_key = register_iak_pubkey(args.registry, pubkey, serial=args.serial)
            print(f"registered IAK public key {registry_key} in {args.registry}")
        elif ok:
            print("warning: IAK public key export returned no data")
        return 0 if ok else 1

    statuses = [r["status"] for r in results]
    ok = args.dry_run or (bool(statuses) and all(s in accept for s in statuses))

    # Emit JSON last and encode byte strings as hexadecimal.
    if getattr(args, "json", False):

        def _jsonable(value):
            return value.hex() if isinstance(value, bytes) else value

        print(
            json.dumps(
                {
                    "version": res["version"],
                    "ok": ok,
                    "results": [{k: _jsonable(v) for k, v in r.items()} for r in results],
                }
            )
        )
    elif not args.dry_run:
        state = "passed" if ok else "failed"
        print(f"{label}: {state} ({', '.join(str(s) for s in statuses)})")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
