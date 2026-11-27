# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""IronSide boot report: the wire layout, its field names and a text rendering.

The layout is the 180-byte little-endian struct ironside_boot_report that the
boot-report service returns and that ADAC BOOT_REPORT_GET returns unwrapped. The
SMP transport carries the same fields as a CBOR map with the keys to_dict()
produces.
"""

import ctypes as c

BOOT_REPORT_KEYS = (
    "update_applied",
    "update_target",
    "update_status",
    "booted_image",
    "dfu_reason",
    "lifecycle",
    "irot_version",
    "urot_version",
    "app_version",
    "dfu_version",
    "reset_reason",
)
VERSION_KEYS = ("irot_version", "urot_version", "app_version", "dfu_version")

BOOTED_IMAGE = {
    0: "none",
    1: "app",
    2: "dfu",
}

DFU_REASON = {
    0: "none",  # App booted
    1: "adac_boot_mode",
    2: "request_service",
    3: "app_signature",  # App signature/header rejected
    4: "app_rollback",
    5: "app_unavailable",
    6: "trigger_resetreas",
    7: "trigger_gpio",
    8: "trigger_fault",
}

# Matches adac.py PSA_LIFECYCLE.
PSA_LIFECYCLE = {
    0x1000: "assembly",
    0x2000: "prot-provisioning",
    0x3000: "secured",
    0x4000: "non-prot-debug",
    0x6000: "decommissioned",
}

UPDATE_APPLIED = {
    0: "no",
    1: "yes",
}

UPDATE_STATUS = {
    0: "none",
    1: "applied",
    2: "rejected_signature",
    3: "rejected_rollback",
    4: "rejected_size",
    5: "rejected_sec_cnt",
    6: "counter_store",
    7: "write_failed",
}


class ImageVersion(c.LittleEndianStructure):
    """struct ironside_image_version."""

    _pack_ = 1
    _fields_ = [
        ("major", c.c_uint8),
        ("minor", c.c_uint8),
        ("patch", c.c_uint8),
        ("tweak", c.c_uint8),
    ]


class FaultReport(c.LittleEndianStructure):
    """struct ironside_fault_report."""

    _pack_ = 1
    _fields_ = [
        ("present", c.c_uint32),
        ("source", c.c_uint32),
        ("flags", c.c_uint32),
        ("count", c.c_uint32),
        ("exception", c.c_uint32),
        ("cfsr", c.c_uint32),
        ("bfar", c.c_uint32),
        ("mmfar", c.c_uint32),
        ("sfsr", c.c_uint32),
        ("sfar", c.c_uint32),
        ("cfsr_ns", c.c_uint32),
        ("mmfar_ns", c.c_uint32),
        ("frame", c.c_uint32 * 8),
        ("frame_sp", c.c_uint32),
        ("mpc_address", c.c_uint32),
        ("mpc_access", c.c_uint32),
    ]


class BootRegion(c.LittleEndianStructure):
    """struct ironside_boot_region."""

    _pack_ = 1
    _fields_ = [
        ("addr", c.c_uint32),
        ("size", c.c_uint32),
    ]


LAYOUT_KEYS = ("slot0", "slot1", "appstorage", "dfuimage", "ram_ns")


class BootReport(c.LittleEndianStructure):
    """struct ironside_boot_report."""

    _pack_ = 1
    _fields_ = [
        ("update_applied", c.c_uint32),
        ("update_target", c.c_uint32),
        ("update_status", c.c_uint32),
        ("booted_image", c.c_uint32),
        ("dfu_reason", c.c_uint32),
        ("lifecycle", c.c_uint32),
        ("irot_version", ImageVersion),
        ("urot_version", ImageVersion),
        ("app_version", ImageVersion),
        ("dfu_version", ImageVersion),
        ("fault", FaultReport),
        ("reset_reason", c.c_uint32),
        ("slot0", BootRegion),
        ("slot1", BootRegion),
        ("appstorage", BootRegion),
        ("dfuimage", BootRegion),
        ("ram_ns", BootRegion),
        ("fault_image", c.c_uint32),
    ]


SIZE = c.sizeof(BootReport)


def parse(data: bytes) -> BootReport:
    """Decode a raw boot report.

    Raises ValueError unless data is exactly SIZE bytes. A shorter report comes
    from firmware older than this layout, and a longer one from firmware newer
    than it; neither is a prefix of fields this layout can name.
    """
    if len(data) != SIZE:
        raise ValueError(f"boot report is {len(data)} bytes, expected {SIZE}")
    return BootReport.from_buffer_copy(data)


def to_dict(report: BootReport) -> dict:
    """Return the report with the SMP map's keys, plus "fault", "fault_image" and "layout".

    Versions are [major, minor, patch, tweak] lists, as in the SMP map. The SMP
    map carries no layout, so "layout" appears only in a decoded raw report.
    """
    result = {}
    for key in BOOT_REPORT_KEYS:
        value = getattr(report, key)
        if key in VERSION_KEYS:
            value = [value.major, value.minor, value.patch, value.tweak]
        result[key] = value
    fault = {}
    for name, _ in FaultReport._fields_:
        value = getattr(report.fault, name)
        fault[name] = list(value) if name == "frame" else value
    result["fault"] = fault
    result["fault_image"] = report.fault_image
    result["layout"] = {
        key: {"addr": getattr(report, key).addr, "size": getattr(report, key).size}
        for key in LAYOUT_KEYS
    }
    return result


def enum_name(table: dict, value: int) -> str:
    """Return an enum name or unknown(N)."""
    return table.get(value, f"unknown({value})")


def fmt_version(parts: list) -> str:
    if not isinstance(parts, list) or len(parts) != 4:
        return str(parts)
    return f"{parts[0]}.{parts[1]}.{parts[2]}+{parts[3]}"


def format_human(rsp: dict) -> str:
    """Render a boot report dict with named enum values."""
    applied = enum_name(UPDATE_APPLIED, rsp["update_applied"])
    target = rsp["update_target"]
    target_s = "0x0" if target == 0 else f"0x{target:x}"

    lines = [
        f"update_applied={applied} update_target={target_s} "
        f"update_status={enum_name(UPDATE_STATUS, rsp['update_status'])}",
        (
            f"booted_image={enum_name(BOOTED_IMAGE, rsp['booted_image'])} "
            f"dfu_reason={enum_name(DFU_REASON, rsp['dfu_reason'])} "
            f"lifecycle={enum_name(PSA_LIFECYCLE, rsp['lifecycle'])} "
            f"reset_reason=0x{rsp['reset_reason']:08x}"
        ),
    ]
    lines.extend(f"{key}={fmt_version(rsp[key])}" for key in VERSION_KEYS)
    layout = rsp.get("layout")
    if layout is not None:
        lines.extend(
            f"{key}: 0x{layout[key]['addr']:x} size=0x{layout[key]['size']:x}"
            for key in LAYOUT_KEYS
        )
    fault = rsp.get("fault")
    if fault is not None and fault["present"]:
        image = enum_name(BOOTED_IMAGE, rsp["fault_image"])
        lines.append(
            f"fault: image={image} source={fault['source']} count={fault['count']} "
            "(--json for fields)"
        )
    return "\n".join(lines)
