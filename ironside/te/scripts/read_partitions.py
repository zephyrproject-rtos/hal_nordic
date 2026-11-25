#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Report the IronSide TE partition and RAM layout as a map or JSON.

Read the provisioned PARTITIONS and KEYSLOTSTORAGESIZE words from a device or
decode them from base configuration artifacts. UICR stores the SLOT1,
APPSTORAGE, and DFUIMAGE sizes in 4 kB blocks. Partition addresses, the
application size, and the RAM split are derived using the same placement rules
as the RoT.

Device reads require secure debug access and are unavailable from PRoT
Provisioning onward.

--ram reports the RAM split instead: the non-secure RAM window the last
completed boot used, and the PSA key-slot count it gives, read from the boot report over
ADAC, which needs the AUTHOP key and works in every lifecycle state that answers
ADAC.
"""

import argparse
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from lib import parts, uicr  # noqa: E402
from lib import psa_key_ram as pkr  # noqa: E402

FROM_UICR = "uicr"
FROM_DERIVED = "derived"
FROM_FIXED = "fixed"

# The split within this span belongs to the images, not the partition layout.
RESERVED_NAME = "reserved"


class ReadError(Exception):
    """Partition source read failure."""


def _word(get, addr):
    """Read a u32 from a byte lookup, or None if any byte is absent."""
    b = [get(addr + i) for i in range(4)]
    if any(v is None for v in b):
        return None
    return int.from_bytes(bytes(b), "little")


def read_device_words(addr, nwords, serial=None):
    """Read NWORDS u32 values at ADDR using nrfutil."""
    select = ("--serial-number", str(serial)) if serial is not None else ()
    cmd = [
        "nrfutil",
        "device",
        "read",
        "--address",
        hex(addr),
        "--bytes",
        str(4 * nwords),
        "--width",
        "32",
        "--direct",
        "--json",
        "--skip-overhead",
        *select,
    ]
    # The first access to an idle board can fail before the probe identifies it.
    for attempt in (1, 2):
        try:
            out = subprocess.run(cmd, capture_output=True, text=True, check=True)
            break
        except FileNotFoundError:
            raise ReadError("nrfutil is not on PATH") from None
        except subprocess.CalledProcessError as exc:
            if attempt == 2:
                detail = (exc.stderr or exc.stdout or "").strip().splitlines()
                raise ReadError(
                    "nrfutil device read failed: "
                    + (detail[-1] if detail else f"exit {exc.returncode}")
                    + "\nDevice reads require secure debug access, which "
                    "is unavailable from PRoT Provisioning onward."
                ) from None
    try:
        values = json.loads(out.stdout)["devices"][0]["memoryData"][0]["values"]
    except (ValueError, KeyError, IndexError):
        raise ReadError(f"no memory-read data for {addr:#x}: {out.stdout.strip()}") from None
    if len(values) < 4 * nwords:
        raise ReadError(f"short read at {addr:#x}: {len(values)} of {4 * nwords} bytes")
    return [int.from_bytes(bytes(values[i * 4 : i * 4 + 4]), "little") for i in range(nwords)]


def source_from_device(serial):
    """Read partition sizes, the IROT format version, and the key slot storage
    size word from a device."""
    words = read_device_words(
        uicr.IRONSIDE_UICR_BASE + uicr.PARTITIONS_OFF, len(uicr.PARTITION_NAMES), serial
    )
    # RoT rejects partition data with an unsupported format version.
    version = read_device_words(uicr.IRONSIDE_UICR_BASE + uicr.IROT_FORMATVERSION_OFF, 1, serial)[0]
    storage = read_device_words(uicr.IRONSIDE_UICR_BASE + uicr.KEYSLOTSTORAGESIZE_OFF, 1, serial)[0]
    return words, version, storage


def source_from_artifacts(paths):
    """Read partition sizes, the IROT format version, and the key slot storage
    size word from artifacts, later ones taking precedence."""
    try:
        get = uicr.files_reader(paths)
    except (OSError, ValueError, uicr.UicrError) as exc:
        raise ReadError(str(exc)) from None
    words = [
        _word(get, uicr.IRONSIDE_UICR_BASE + uicr.PARTITIONS_OFF + 4 * i)
        for i in range(len(uicr.PARTITION_NAMES))
    ]
    return (
        words,
        _word(get, uicr.IRONSIDE_UICR_BASE + uicr.IROT_FORMATVERSION_OFF),
        _word(get, uicr.IRONSIDE_UICR_BASE + uicr.KEYSLOTSTORAGESIZE_OFF),
    )


def format_version(word):
    """Decode an IROT format-version word."""
    if word is None:
        return None
    if word == uicr.ERASED:
        return {"raw": word, "version": None, "supported": False}
    version = f"{word >> 16}.{word & 0xFFFF}"
    return {"raw": word, "version": version, "supported": version in uicr.SUPPORTED_FORMATVERSIONS}


def resolve(words, soc):
    """Resolve partition rows and sizes in ascending address order."""
    missing = [n for n, w in zip(uicr.PARTITION_NAMES, words) if w is None]
    if missing:
        raise ReadError(
            f"no PARTITIONS words for {', '.join(missing)} "
            f"(the artifact does not carry the partition layout)"
        )
    erased = [n for n, w in zip(uicr.PARTITION_NAMES, words) if w == uicr.ERASED]
    if erased:
        raise ReadError(
            f"partitions are not provisioned ({', '.join(erased)} "
            f"still erased). Install the base configuration first."
        )

    sizes_kb = [w * uicr.PARTITION_BLOCK_KB for w in words]
    try:
        placed = uicr.partition_addresses(sizes_kb, soc=soc)
    except uicr.UicrError as exc:
        raise ReadError(str(exc)) from None

    blocks = dict(zip(uicr.PARTITION_NAMES, words))
    rows = [
        {
            "name": RESERVED_NAME,
            "addr": 0,
            "size": uicr.SLOT0_BASE,
            "from": FROM_FIXED,
            "detail": "irot + IronSide TE image slots",
        },
        {
            "name": "slot0",
            "addr": placed["slot0"][0],
            "size": placed["slot0"][1],
            "from": FROM_DERIVED,
            "detail": "implicit remainder, up to slot1",
        },
    ]
    for name in uicr.PARTITION_NAMES:
        addr, size = placed[name]
        rows.append(
            {
                "name": name,
                "addr": addr,
                "size": size,
                "size4kb": blocks[name],
                "from": FROM_UICR,
                "detail": f"SIZE4KB={blocks[name]}",
            }
        )
    for row in rows:
        row["end"] = row["addr"] + row["size"]
        row["size_kb"] = row["size"] // 1024
        row["disabled"] = row["size"] == 0
    return rows, sizes_kb


def resolve_ram(storage_word: int | None, soc: str) -> dict:
    """The RAM layout that the key slot storage size word @storage_word gives on
    @soc. An erased, absent, or invalid word gives the base slots, as in UROT."""
    part = parts.by_soc(soc)
    stored = None if storage_word in (None, uicr.ERASED) else storage_word
    size = pkr.applied_size(stored, part.ram_split_granule)
    ram = pkr.ram_layout(size, part.ram_kb, part.ram_physical_kb)
    ram["key_slot_storage_word"] = storage_word
    return ram


def report(source, rows, sizes_kb, version, soc, ram):
    """Return the machine-readable partition and RAM report."""
    rram_end = uicr.RRAM_SIZE_KB[soc] * 1024

    def hexed(row):
        # Include numeric addresses for tools and hexadecimal addresses for display.
        out = dict(row)
        out["addr_hex"] = f"0x{row['addr']:08X}"
        out["end_hex"] = f"0x{row['end']:08X}"
        return out

    return {
        "source": source,
        "soc": soc,
        "nvm": {
            "base": 0,
            "end": rram_end,
            "end_hex": f"0x{rram_end:08X}",
            "size_kb": uicr.RRAM_SIZE_KB[soc],
        },
        "slot0_base": uicr.SLOT0_BASE,
        "uicr": {
            "partitions_addr": uicr.IRONSIDE_UICR_BASE + uicr.PARTITIONS_OFF,
            "partitions_addr_hex": f"0x{uicr.IRONSIDE_UICR_BASE + uicr.PARTITIONS_OFF:08X}",
            "block_kb": uicr.PARTITION_BLOCK_KB,
            "size4kb": dict(
                zip(uicr.PARTITION_NAMES, [kb // uicr.PARTITION_BLOCK_KB for kb in sizes_kb])
            ),
            "irot_formatversion": format_version(version),
        },
        "partitions": [hexed(r) for r in rows],
        "ram": ram,
    }


def render(rep):
    """Render the partition report as a map."""
    src = rep["source"]
    where = f"device {src['serial']}" if src["kind"] == "device" else f"artifact {src['path']}"
    if src["kind"] == "device" and src["serial"] is None:
        where = "device (sole attached probe)"
    lines = [
        "IronSide TE partition layout",
        f"  source: {where}",
        f"  SoC: {rep['soc']}, NVM {rep['nvm']['end_hex']} ({rep['nvm']['size_kb']} kB usable)",
    ]

    version = rep["uicr"]["irot_formatversion"]
    if version is None:
        lines.append("  IROT format version: not in artifact")
    elif version["version"] is None:
        lines.append("  IROT format version: erased (unsupported)")
    else:
        lines.append(
            f"  IROT format version: {version['version']}"
            + ("" if version["supported"] else " (unsupported)")
        )
    lines.append("")

    # Uniform boxes keep small partitions visible.
    width = 26
    name_w = max(len(r["name"]) for r in rep["partitions"])
    from_w = max(len(r["from"]) for r in rep["partitions"])
    # Report zero-size partitions below the map to avoid overlapping boundaries.
    drawn = [r for r in rep["partitions"] if not r["disabled"]]
    for row in drawn:
        lines.append(
            f"  {row['addr_hex']} +{'-' * width}+"
            f" {row['size_kb']:>6} kB  {row['name']:<{name_w}}"
            f"  {row['from']:<{from_w}}  {row['detail']}"
        )
    lines.append(f"  {drawn[-1]['end_hex']} +{'-' * width}+ {'':>9}  end of NVM")
    lines.append("")

    blocks = rep["uicr"]["size4kb"]
    lines.append(
        f"  UICR PARTITIONS at {rep['uicr']['partitions_addr_hex']}: "
        + ", ".join(f"{n}={b}" for n, b in blocks.items())
        + f" ({rep['uicr']['block_kb']} kB blocks)"
    )

    disabled = [r["name"] for r in rep["partitions"] if r["disabled"]]
    if disabled:
        lines.append(f"  Not present (size 0): {', '.join(disabled)}")

    ram = rep["ram"]
    lines += ["", "IronSide TE RAM layout", f"  {describe_storage_word(ram)}", ""]
    lines += ram_map_lines(ram)
    return "\n".join(lines)


def describe_storage_word(ram: dict) -> str:
    """Where the key slot storage size of @ram comes from."""
    word = ram["key_slot_storage_word"]
    size = ram["key_slot_storage_size"]
    if word is None:
        return (
            f"key slot storage size: {size} B, the base slots (KEYSLOTSTORAGESIZE not in artifact)"
        )
    if word == uicr.ERASED:
        return f"key slot storage size: {size} B, the base slots (KEYSLOTSTORAGESIZE erased)"
    if word != size:
        return (
            f"key slot storage size: {size} B, the base slots "
            f"(KEYSLOTSTORAGESIZE {word} B is invalid for this part)"
        )
    return f"key slot storage size: {size} B (KEYSLOTSTORAGESIZE)"


def ram_map_lines(ram: dict) -> list[str]:
    """Render the RAM areas of a psa_key_ram.ram_layout() report as a map."""
    rot, ns = ram["rot_ram"], ram["ram_ns"]
    box = f"+{'-' * 26}+"
    lines = [
        f"  0x{rot['addr']:08X} {box} {size_text(ns['addr'] - rot['addr'])}  reserved"
        f"    {ram['psa_key_slots']} key slots",
        f"  0x{ns['addr']:08X} {box} {size_text(ns['size'])}  APP-RAM",
    ]
    end = ns["addr"] + ns["size"]
    if ram["vprctx_ram"] is not None:
        vpr, top = ram["vprctx_ram"], ram["reserved_top"]
        lines += [
            f"  0x{vpr['addr']:08X} {box} {size_text(vpr['size'])}  VPRCTX-RAM  not linked",
            f"  0x{top['addr']:08X} {box} {size_text(top['size'])}  reserved",
        ]
        end = top["addr"] + top["size"]
    lines.append(f"  0x{end:08X} {box} {'':>9}  end of RAM")
    return lines


def size_text(size: int) -> str:
    """@size bytes as a map column: whole kB where it is one, bytes otherwise."""
    if size % 1024 == 0:
        return f"{size // 1024:>6} kB"
    return f"{size:>6} B "


def ram_report(serial, keyset):
    """The RAM split of the last completed boot, from the boot report over ADAC."""
    cmd = [sys.executable, str(Path(__file__).resolve().parent / "adac.py"), "--json"]
    if serial is not None:
        cmd += ["--serial-number", str(serial)]
    cmd.append("boot-report")
    if keyset is not None:
        cmd += ["--keyset", keyset]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        detail = (proc.stderr or proc.stdout).strip().splitlines()
        raise ReadError("adac.py boot-report failed: " + (detail[-1] if detail else "no output"))
    try:
        report = json.loads(proc.stdout)["report"]
        ram_ns = report["layout"]["ram_ns"]
    except (ValueError, KeyError, TypeError):
        raise ReadError(f"no RAM split in the boot report: {proc.stdout.strip()}") from None
    # The boot report carries only the split; the storage size and slot count follow from it.
    ram_end_kb = (ram_ns["addr"] + ram_ns["size"] - pkr.RAM_BASE) // 1024
    try:
        ram_physical_kb = parts.for_serial(serial).ram_physical_kb
    except parts.PartError as exc:
        raise ReadError(str(exc)) from None
    ram = pkr.ram_layout(pkr.size_at(ram_ns["addr"]), ram_end_kb, ram_physical_kb)
    ram["ram_ns"] = ram_ns
    return {"source": {"kind": "device", "serial": serial}, **ram}


def render_ram(rep):
    src = rep["source"]
    where = (
        f"device {src['serial']}" if src["serial"] is not None else "device (sole attached probe)"
    )
    lines = ["IronSide TE RAM layout (last completed boot)", f"  source: {where}"]
    return "\n".join(lines + ram_map_lines(rep))


def main(argv=None):
    p = argparse.ArgumentParser(
        description="Read the IronSide TE partition and RAM layout from a device or base "
        "configuration artifacts. Device reads do not modify the device."
    )
    p.add_argument(
        "artifacts",
        nargs="*",
        type=Path,
        metavar="HEX|BIN",
        help="base configuration artifacts, merged with later ones taking precedence; "
        "the IROT image carries the partition sizes and the UROT image the key slot "
        "storage size (default: read device)",
    )
    p.add_argument(
        "--serial-number",
        dest="serial",
        help="J-Link serial number (default: sole attached board)",
    )
    p.add_argument("--serial", dest="serial", help=argparse.SUPPRESS)  # older spelling
    p.add_argument(
        "--soc",
        choices=sorted(uicr.RRAM_SIZE_KB),
        help="SoC used to calculate partition addresses (required with an artifact; a "
        "device read takes it from the device)",
    )
    p.add_argument("--json", action="store_true", help="print JSON instead of the partition map")
    p.add_argument("--json-out", type=Path, metavar="FILE", help="also write the layout as JSON")
    p.add_argument(
        "--ram",
        action="store_true",
        help="report the RAM split of the last completed boot instead, from the boot "
        "report over ADAC (device only)",
    )
    p.add_argument(
        "--keyset",
        dest="keyset",
        metavar="SPEC",
        help="key source holding the AUTHOP key --ram authenticates with (default: the "
        "only keyset in the OS keyring)",
    )
    args = p.parse_args(argv)

    if args.artifacts and args.serial is not None:
        p.error("give artifacts or --serial-number, not both")
    if args.ram and args.artifacts:
        p.error("--ram reads a device, not an artifact")
    if args.artifacts and args.soc is None:
        p.error("an artifact does not name its SoC; give --soc")

    if args.ram:
        try:
            rep = ram_report(args.serial, args.keyset)
        except ReadError as exc:
            print(f"read_partitions: {exc}", file=sys.stderr)
            return 3
        if args.json_out is not None:
            args.json_out.write_text(json.dumps(rep, indent=2) + "\n")
        print(json.dumps(rep, indent=2) if args.json else render_ram(rep))
        return 0

    try:
        if args.artifacts:
            source = {"kind": "artifact", "path": ", ".join(map(str, args.artifacts))}
            words, version, storage = source_from_artifacts(args.artifacts)
        else:
            source = {"kind": "device", "serial": args.serial}
            device_soc = parts.for_serial(args.serial).soc
            if args.soc is not None and args.soc != device_soc:
                p.error(f"--soc {args.soc} disagrees with the device, which is {device_soc}")
            args.soc = device_soc
            words, version, storage = source_from_device(args.serial)
        rows, sizes_kb = resolve(words, args.soc)
        ram = resolve_ram(storage, args.soc)
    except (ReadError, parts.PartError) as exc:
        print(f"read_partitions: {exc}", file=sys.stderr)
        return 3

    rep = report(source, rows, sizes_kb, version, args.soc, ram)
    if args.json_out is not None:
        args.json_out.write_text(json.dumps(rep, indent=2) + "\n")
    print(json.dumps(rep, indent=2) if args.json else render(rep))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
