# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Decode the BOOTFAIL word IronSide TE publishes in the CTRL-AP mailbox.

IROT and UROT write one 32-bit word, only when a stage halts: which stage
halted and why. Nothing is written on a boot that does not halt.

    31..28  MAGIC     0xB
    27      STAGE     0 = IROT, 1 = UROT
    26..8   reserved, 0
     7..0   ERROR     the reason the stage halted

The tables below are a hand-written copy of the firmware's error enums. IROT's
error values are frozen once released; UROT's are append-only.
"""

from __future__ import annotations

from typing import Any

MAGIC = 0xB

STAGE_IROT = 0
STAGE_UROT = 1
STAGE_NAMES = {STAGE_IROT: "IROT", STAGE_UROT: "UROT"}

FAULTS = {
    0xF0: ("HARDFAULT", "HardFault"),
    0xF1: ("MEMMANAGE", "MemManage fault"),
    0xF2: ("BUSFAULT", "BusFault"),
    0xF3: ("USAGEFAULT", "UsageFault"),
    0xF4: ("SECUREFAULT", "SecureFault"),
    0xF5: ("NMI", "non-maskable interrupt"),
    0xF6: ("SW_FATAL", "fatal software error"),
    0xFE: ("WATCHDOG", "watchdog"),
}

IROT_ERRORS = {
    0x80: ("CRYPTO_INIT", "crypto init failed"),
    0x81: ("BASE_CONFIG_VERSION", "IROT base configuration (UICR) version unsupported"),
    0x82: ("LAYOUT_INVALID", "UICR partition layout invalid or unprovisioned"),
    0x83: ("UROT_UPDATE", "a UROT update failed"),
    0x84: ("RRAMC_LOCK", "RRAMC region lock read-back failed"),
    0x85: ("UROT_SIGNATURE", "UROT signature invalid"),
    0x86: ("UROT_SEC_CNT_MISSING", "UROT security counter unreadable"),
    0x87: ("UROT_ROLLBACK", "UROT rollback counter gate failed"),
    0x89: ("UROT_BOOT_RETURNED", "UROT's reset handler returned"),
}

UROT_ERRORS = {
    0x80: ("IROT_BASE_CONFIG_FORMAT", "IROT base configuration format unsupported"),
    0x81: ("UROT_BASE_CONFIG_FORMAT", "UROT base configuration format unsupported"),
    0x82: ("LAYOUT_INVALID", "partition layout invalid"),
    0x83: ("CRYPTO_INIT", "crypto init failed"),
    0x84: ("HANDOVER", "handover from IROT invalid (bad magic or unsupported format version)"),
    0x85: (
        "DECOMMISSIONED",
        "the device is decommissioned, its normal end state and not a failure",
    ),
    0x88: ("NO_BOOT_IMAGE", "no App or IronSide DFU image validated"),
    0x89: ("ADAC_EXIT", "ADAC session exited unexpectedly"),
    0x8A: ("CONFIG_ASSERT", "exception configuration assertion refused"),
    0x8B: ("IKG_SEED_INVALID", "base configuration selects an IKG SEED entry that is not locked"),
    0x8D: ("IKG_SEED_SELECTION_INVALID", "base configuration selects no IKG SEED entry"),
    0x8E: ("BOARD_UNSET", "UROT image configuration unreadable or lacks a board word"),
    0x8F: ("UROT_STORAGE_MOUNT", "URoT storage does not mount"),
    0x90: (
        "UROT_STORAGE_NO_FORMAT",
        "URoT storage format record absent: provisioning did not finish",
    ),
    0x91: ("UROT_STORAGE_UNAUTHENTIC", "URoT storage format record does not authenticate"),
    0x92: (
        "UROT_STORAGE_VERSION",
        "URoT storage format record declares an unsupported layout version",
    ),
    0x93: ("UROT_STORAGE_READ", "URoT storage format record unreadable"),
    0x94: (
        "UROT_STORAGE_BLOCK",
        "URoT storage MPC blocking override does not read back as programmed, "
        "or the storage is mapped cacheable",
    ),
    0x95: ("NS_IMAGE_RETURNED", "App or IronSide DFU reset handler returned"),
    0x96: (
        "AUTHOP_REVOKE",
        "an AUTHOP generation the image configuration revokes is still valid after its write",
    ),
    0x97: ("IKG_SEED_BLOCK", "KMU slots of the unselected IKG SEED entry cannot be blocked"),
}

_STAGE_ERRORS = {
    STAGE_IROT: IROT_ERRORS,
    STAGE_UROT: UROT_ERRORS,
}


def decode(word: int) -> dict[str, Any]:
    """Split a BOOTFAIL word into its fields and name each one.

    Returns a dict with "valid" False for 0 (no stage has halted since the
    mailbox was last cleared) or a word without the BOOTFAIL magic, such as an
    ADAC protocol word.
    """
    result: dict[str, Any] = {"status_word": word, "valid": False}
    if word == 0 or (word >> 28) != MAGIC:
        return result

    stage = (word >> 27) & 0x1
    error = word & 0xFF

    result.update(
        valid=True,
        stage=STAGE_NAMES[stage],
        error=error,
        error_name=None,
        error_text=None,
    )

    if error != 0:
        result["error_name"], result["error_text"] = _error_names(stage, error)
    return result


def _error_names(stage: int, error: int) -> tuple[str, str]:
    if error in FAULTS:
        return FAULTS[error]
    errors = _STAGE_ERRORS[stage]
    return errors.get(error, (f"UNKNOWN_0x{error:02X}", f"unknown error 0x{error:02x}"))
