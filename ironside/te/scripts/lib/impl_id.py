# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""The PSA attestation implementation ID encoding, host side.

UROT composes this value itself when it mints a token, from registers and from
IROT's reported version, so nothing here is on the path that puts it on a device.
This module is the reference implementation of the same encoding, for working out
what a given part and IROT version should report: an endorsement reference value,
a bench check against a token, or a test. It must agree byte for byte with the
encoder IronSide runs on the device.

RFC 9783 defines the claim (PSA label 2396) as exactly 32 bytes identifying "the
hardware assembly of the immutable PSA RoT", and adds "Note that this identifies
the PSA RoT implementation, not a particular instance". IronSide's immutable PSA
RoT is the part plus IROT, so both are inputs:

  * the part, from FICR.INFO: PART, VARIANT (which the MDK documents as "Part
    Variant, Hardware version and Production configuration"), CONFIGID (whose
    HWID field is the "Identification number for the HW"), RAM and RRAM.
  * IROT, as its version. IROT carries no image header and is never signed, so
    its version reaches the token through IronSide's boot report, and reaches this
    module from the version the IROT component package declares.

FICR.INFO.DEVICEID and FICR.INFO.UUID are not inputs. Either would make the value
identify one die rather than an implementation, which the sentence above rules
out. The die is named by the token's device-uuid claim instead.

Encoding, 32 bytes, every field big-endian so a hex dump reads left to right:

    off  len  field
      0    1  scheme version, 1
      1    4  FICR.INFO.PART
      5    4  FICR.INFO.VARIANT, which reads as four ASCII characters
      9    4  FICR.INFO.CONFIGID
     13    2  FICR.INFO.RAM, kB
     15    2  FICR.INFO.RRAM, kB
     17    4  IROT version, one byte each of major, minor, patch, tweak
     21   11  reserved for future use, written zero

The whole value is transparent. RFC 9783 requires it to be opaque to the
verifier, not opaque in itself, and leaves the format to the manufacturer, so a
bench can read the part and the IROT version straight out of a token.

Bytes 21..31 are reserved for future use - a writer sets them to zero and a
reader must ignore what it finds there. That is what leaves room to name
something further without moving what is already here, and without a reader
written against this layout rejecting the result, since the fields before the
tail keep their meaning whatever the tail holds. Defining a field there is still
release-visible, because the 32 bytes an endorsement is published against
change; what the reserve buys is that everything ahead of it stays readable.

Changing any field changes the value on every device and invalidates any
endorsement published against it. That is what the scheme version is for - a
later layout gets a new version number rather than reinterpreting this one. It
is one byte because the claim is 32 bytes in total, and the fields below need
them more than a wider tag would.
"""

from __future__ import annotations

import os
import re
import struct
import subprocess
from dataclasses import dataclass

NRFUTIL = os.environ.get("IRONSIDE_INTEGRATION_KIT_NRFUTIL", "nrfutil")

# The encoding this module emits, in the claim's first byte. Bump it with any
# layout change.
SCHEME_VERSION = 1

IMPL_ID_LEN = 32  # RFC 9783 fixes the size, so this is not a maximum
# Start of the reserved-for-future-use tail: written zero, ignored on read.
RESERVED_OFF = 21

# FICR.INFO on nRF54L: 12 words, non-secure and factory-fixed.
FICR_INFO_ADDR = 0x00FFC300
FICR_INFO_WORDS = 12
FICR_INFO_LEN = FICR_INFO_WORDS * 4

_UNPROGRAMMED_WORD = 0xFFFFFFFF
_HEX_LINE = re.compile(r"^\s*0x([0-9A-Fa-f]{8}):\s+((?:[0-9A-Fa-f]{8}\s*)+)")


class ImplIdError(RuntimeError):
    """The implementation ID could not be computed from these inputs."""


@dataclass(frozen=True)
class HwDescriptor:
    """The part-identifying inputs to the implementation ID.

    Every field is a property of the part and its silicon revision, so two dice of
    the same part produce the same descriptor. Nothing per-die is admitted.
    """

    part: int
    variant: int
    configid: int
    ram_kb: int
    rram_kb: int

    @classmethod
    def from_ficr_info(cls, blob: bytes) -> HwDescriptor:
        """Parse FICR_INFO_LEN bytes read from FICR_INFO_ADDR.

        Refuses a part whose PART, VARIANT or CONFIGID reads erased. Such a part
        does not identify its own implementation, and deriving from erased words
        would give every one of them the same value.
        """
        if len(blob) != FICR_INFO_LEN:
            raise ImplIdError(f"FICR.INFO is {len(blob)} bytes, expected {FICR_INFO_LEN}")
        words = struct.unpack(f"<{FICR_INFO_WORDS}I", blob)
        # words[1:3] is DEVICEID and words[3:7] is UUID. Both are per-die and are
        # deliberately not read. words[9] is PACKAGE, which is not an input.
        configid, part, variant, ram_kb, rram_kb = (
            words[0],
            words[7],
            words[8],
            words[10],
            words[11],
        )
        for name, value in (("CONFIGID", configid), ("PART", part), ("VARIANT", variant)):
            if value == _UNPROGRAMMED_WORD:
                raise ImplIdError(
                    f"FICR.INFO.{name} reads erased (0x{_UNPROGRAMMED_WORD:08X}), so this "
                    f"part does not identify its own implementation. Refusing to compute "
                    f"a PSA implementation ID for it"
                )
        for name, value in (("RAM", ram_kb), ("RRAM", rram_kb)):
            if not 0 < value <= 0xFFFF:
                raise ImplIdError(
                    f"FICR.INFO.{name} reads {value} kB, which does not fit the "
                    f"encoding's 16-bit field"
                )
        return cls(
            part=part,
            variant=variant,
            configid=configid,
            ram_kb=ram_kb,
            rram_kb=rram_kb,
        )

    def describe(self) -> str:
        return (
            f"PART 0x{self.part:08X} VARIANT {_as_text(self.variant)} "
            f"CONFIGID 0x{self.configid:08X} RAM {self.ram_kb} kB RRAM {self.rram_kb} kB"
        )


def _as_text(word: int) -> str:
    """Spell a FICR word as its ASCII reading when it has one, otherwise as hex."""
    raw = struct.pack(">I", word)
    if all(0x20 <= b < 0x7F for b in raw):
        return f'"{raw.decode("ascii")}" (0x{word:08X})'
    return f"0x{word:08X}"


def parse_version(text: str) -> tuple:
    """Parse an IROT version into the four bytes the encoding carries.

    A component version is three fields and IROT reports a zero tweak, so a
    fourth field may be given only as 0. An `@soc` suffix, as a release manifest
    spells it, is accepted and ignored.
    """
    fields = text.split("@")[0].strip().split(".")
    if len(fields) not in (3, 4):
        raise ImplIdError(f"IROT version {text!r} is not MAJOR.MINOR.PATCH")
    try:
        values = [int(f, 10) for f in fields]
    except ValueError:
        raise ImplIdError(f"IROT version {text!r} has a non-numeric field") from None
    if len(values) == 3:
        values.append(0)
    if values[3] != 0:
        raise ImplIdError(f"IROT version {text!r} has tweak {values[3]}, but IROT always reports 0")
    for name, value in zip(("major", "minor", "patch"), values, strict=False):
        if not 0 <= value <= 0xFF:
            raise ImplIdError(f"IROT version {text!r}: {name} {value} does not fit one byte")
    return tuple(values)


def encode(hw: HwDescriptor, irot_version) -> bytes:
    """Return the 32-byte implementation ID for this part and IROT version.

    irot_version is the four bytes parse_version() returns, or a string it parses.
    """
    if isinstance(irot_version, str):
        irot_version = parse_version(irot_version)
    if len(irot_version) != 4 or any(not 0 <= v <= 0xFF for v in irot_version):
        raise ImplIdError(f"IROT version {irot_version!r} is not four byte-sized fields")
    value = struct.pack(
        ">BIIIHH", SCHEME_VERSION, hw.part, hw.variant, hw.configid, hw.ram_kb, hw.rram_kb
    ) + bytes(irot_version)
    if len(value) != RESERVED_OFF:
        raise ImplIdError(f"encoded {len(value)} bytes before the reserved tail")
    # A writer zeroes the reserved bytes so a later field has a defined value
    # meaning absent. A reader must not require it. See describe().
    return value + bytes(IMPL_ID_LEN - RESERVED_OFF)


def describe(impl_id: bytes) -> str:
    """Decode an implementation ID.

    Only values in this encoding decode. A scheme version this module does not
    know means the 32 bytes came from a certification scheme or a later
    encoding, and are reported as opaque rather than parsed as this layout.

    A written reserved tail is reported and not refused. Those bytes are
    reserved for future use, so a value carrying a field this module does not
    know still decodes as scheme version 1 for every field ahead of them.
    """
    if len(impl_id) != IMPL_ID_LEN:
        return f"{impl_id.hex()} ({len(impl_id)} bytes, expected {IMPL_ID_LEN})"
    if impl_id[0] != SCHEME_VERSION:
        return f"{impl_id.hex()} (opaque, not scheme version {SCHEME_VERSION})"
    part, variant, configid, ram_kb, rram_kb = struct.unpack(">IIIHH", impl_id[1:17])
    major, minor, patch, tweak = impl_id[17:21]
    reserved = impl_id[RESERVED_OFF:]
    tail = "" if reserved == bytes(len(reserved)) else f", reserved tail {reserved.hex()}"
    return (
        f"scheme version {SCHEME_VERSION}, PART 0x{part:08X}, "
        f"VARIANT {_as_text(variant)}, CONFIGID 0x{configid:08X}, RAM {ram_kb} kB, "
        f"RRAM {rram_kb} kB, IROT {major}.{minor}.{patch}+{tweak}{tail}"
    )


def read_ficr_info(serial_number=None) -> HwDescriptor:
    """Read FICR.INFO off an attached device and parse it.

    FICR is factory-fixed and survives ERASEALL, so it reads at any lifecycle
    state where the access port is open.
    """
    # --direct skips the check against the memory ranges nrfutil defines for
    # programming, which do not cover FICR. --width 32 keeps the dump in the
    # 32-bit words parse_read_output reads, rather than leaving the width to be
    # inferred from the address alignment.
    cmd = [
        NRFUTIL,
        "device",
        "read",
        "--address",
        hex(FICR_INFO_ADDR),
        "--bytes",
        str(FICR_INFO_LEN),
        "--width",
        "32",
        "--direct",
    ]
    if serial_number is not None:
        cmd += ["--serial-number", str(serial_number)]
    try:
        out = subprocess.run(cmd, capture_output=True, text=True, check=True).stdout
    except FileNotFoundError:
        raise ImplIdError(f"{NRFUTIL} not found on PATH") from None
    except subprocess.CalledProcessError as exc:
        raise ImplIdError(
            f"{' '.join(cmd)} failed:\n{(exc.stderr or exc.stdout).strip()}"
        ) from None
    return HwDescriptor.from_ficr_info(parse_read_output(out, FICR_INFO_ADDR, FICR_INFO_LEN))


def parse_read_output(text: str, addr: int, length: int) -> bytes:
    """Turn `nrfutil device read` output into the bytes of one address range.

    The dump is little-endian words with an address per line. A short or
    misaddressed answer is an error rather than a zero-filled gap, because a
    partial FICR read would produce a stable wrong value.
    """
    seen: dict[int, int] = {}
    for line in text.splitlines():
        m = _HEX_LINE.match(line)
        if m is None:
            continue
        base = int(m.group(1), 16)
        for i, word in enumerate(m.group(2).split()):
            offset = base + i * 4
            for j, byte in enumerate(int(word, 16).to_bytes(4, "little")):
                seen[offset + j] = byte
    missing = [a for a in range(addr, addr + length) if a not in seen]
    if missing:
        raise ImplIdError(
            f"the device read covered {len(seen)} bytes but not 0x{missing[0]:08X}. "
            f"Expected {length} bytes from 0x{addr:08X}"
        )
    return bytes(seen[a] for a in range(addr, addr + length))
