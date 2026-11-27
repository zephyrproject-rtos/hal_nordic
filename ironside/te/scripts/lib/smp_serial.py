# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""MCUmgr SMP client for the serial transport of IronSide DFU.

A request or response is one SMP packet: an 8-byte header and a CBOR payload.
On the wire the packet is length-prefixed, followed by a CRC16, base64-encoded
and split into lines of at most 127 bytes. The first line starts with
SERIAL_START and each further line with SERIAL_CONT.
"""

from __future__ import annotations

import base64
import struct
import time

OP_READ = 0
OP_WRITE = 2

GROUP_OS = 0
OS_ID_ECHO = 0
OS_ID_MCUMGR_PARAMS = 6
OS_ID_BOOTLOADER_INFO = 8

# The bootloader info name IronSide DFU answers with, where MCUboot answers "MCUboot".
BOOTLOADER_NAME_IRONSIDE = "IronSide"

GROUP_DFU = 0x17E0  # IronSide DFU group, DFU_MGMT_GROUP_ID
DFU_ID_BOOT_REPORT = 1
DFU_ID_ENC_KEY = 2

SERIAL_START = b"\x06\x09"
SERIAL_CONT = b"\x04\x14"
# A line is the 2-byte marker, the base64 text and a newline. 124 is the
# largest multiple of 4 that fits, so every line holds whole base64 quanta.
FRAME_MAX = 127
FRAG_B64_MAX = 124

HEADER = struct.Struct(">BBHHBB")
B64_ALPHABET = frozenset(b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/")

PORT_SETTLE = 0.2
RESPONSE_TIMEOUT = 5.0
TXN_RETRIES = 3
TXN_RETRY_DELAY = 0.5


class SmpError(RuntimeError):
    """An SMP transaction failed."""


class SmpTimeout(SmpError):
    """No part of a response arrived."""


class SmpIncomplete(SmpError):
    """A response started but fewer bytes arrived than it declared."""


class SmpCorrupt(SmpError):
    """A complete response failed its CRC or did not decode."""


class SmpSerial:
    """SMP transactions over a pyserial-like stream.

    The stream needs write(), flush(), readline() and reset_input_buffer().
    readline() returns b"" when its read timeout expires.
    """

    def __init__(
        self,
        stream,
        *,
        timeout: float = RESPONSE_TIMEOUT,
        retries: int = TXN_RETRIES,
        retry_delay: float = TXN_RETRY_DELAY,
    ):
        self.stream = stream
        self.timeout = timeout
        self.retries = retries
        self.retry_delay = retry_delay
        self.seq = 0

    @classmethod
    def open(cls, port: str, baud: int, **kwargs) -> SmpSerial:
        """Open `port` exclusively and discard what the device sent before."""
        try:
            import serial  # pyserial
        except ImportError:
            raise SmpError("pyserial is required (pip install pyserial)") from None
        try:
            stream = serial.Serial(port, baud, timeout=0.5, exclusive=True)
        except OSError as exc:
            raise SmpError(f"cannot open {port} exclusively: {exc}") from None
        time.sleep(PORT_SETTLE)
        stream.reset_input_buffer()
        return cls(stream, **kwargs)

    def close(self) -> None:
        self.stream.close()

    def __enter__(self) -> SmpSerial:
        return self

    def __exit__(self, *exc) -> None:
        self.close()

    def request(self, op: int, group: int, cmd_id: int, payload: bytes) -> bytes:
        """Send one request and return the CBOR payload of its response.

        A lost or damaged response is retried. The last failure is raised.
        """
        self.seq = (self.seq + 1) & 0xFF
        frame = encode_frame(HEADER.pack(op, 0, len(payload), group, self.seq, cmd_id) + payload)
        expected = (op + 1, group, self.seq, cmd_id)
        last_err: SmpError | None = None
        for attempt in range(self.retries):
            if attempt:
                time.sleep(self.retry_delay * attempt)
                self.stream.reset_input_buffer()
            self.stream.write(frame)
            self.stream.flush()
            try:
                return self._read_response(expected)
            except SmpError as exc:
                last_err = exc
        raise last_err

    def read_map(self, group: int, cmd_id: int) -> dict:
        """Run a read command with an empty request map and decode the response map."""
        return cbor_decode_map(self.request(OP_READ, group, cmd_id, b"\xa0"))

    def write_map(self, group: int, cmd_id: int, fields: dict) -> dict:
        """Run a write command with request map @fields and decode the response map."""
        return cbor_decode_map(self.request(OP_WRITE, group, cmd_id, cbor_encode_map(fields)))

    def echo(self, text: str) -> str:
        """Run the OS group echo command and return the echoed text."""
        rsp = self.write_map(GROUP_OS, OS_ID_ECHO, {"d": text})
        if "r" not in rsp:
            raise SmpError(f"echo response has no text: {rsp!r}")
        return rsp["r"]

    def _read_response(self, expected: tuple) -> bytes:
        """Return the payload of the response matching `expected`.

        Responses to other requests, such as a late reply to an earlier attempt,
        are skipped.
        """
        b64 = b""
        started = False
        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            line = self.stream.readline()
            if line.startswith(SERIAL_START):
                b64, started = line[2:].strip(), True
            elif line.startswith(SERIAL_CONT) and started:
                b64 += line[2:].strip()
            else:
                continue
            packet = decode_frame(b64)
            if packet is None:
                continue
            b64, started = b"", False
            op, _, length, group, seq, cmd_id = HEADER.unpack_from(packet)
            if (op, group, seq, cmd_id) != expected:
                continue
            if length != len(packet) - HEADER.size:
                raise SmpCorrupt(
                    f"SMP header declares {length} payload bytes, packet carries "
                    f"{len(packet) - HEADER.size}"
                )
            return packet[HEADER.size :]
        if not started:
            raise SmpTimeout("no SMP response")
        got, total = frame_progress(b64)
        raise SmpIncomplete(
            f"incomplete SMP response: {got} of {total} bytes arrived. Bytes from the "
            "device were lost on the way to the host; a UART without hardware flow "
            "control can drop them"
        )


def encode_frame(packet: bytes) -> bytes:
    """Return the serial lines that carry `packet`."""
    body = struct.pack(">H", len(packet) + 2) + packet + struct.pack(">H", crc16(packet))
    b64 = base64.b64encode(body)
    lines = []
    for i in range(0, len(b64), FRAG_B64_MAX):
        marker = SERIAL_START if i == 0 else SERIAL_CONT
        lines.append(marker + b64[i : i + FRAG_B64_MAX] + b"\n")
    return b"".join(lines)


def decode_frame(b64: bytes) -> bytes | None:
    """Return the packet in accumulated base64 text, or None while more is due.

    Raises SmpCorrupt for a complete frame that fails its checks.
    """
    if len(b64) % 4:
        return None
    try:
        body = base64.b64decode(b64, validate=True)
    except ValueError:
        raise SmpCorrupt("SMP response is not valid base64") from None
    if len(body) < 2:
        return None
    total = struct.unpack_from(">H", body)[0]
    if len(body) - 2 < total:
        return None
    if total < HEADER.size + 2 or len(body) - 2 > total:
        raise SmpCorrupt(f"SMP frame length {total} does not match {len(body) - 2} bytes")
    packet, crc = body[2:total], struct.unpack_from(">H", body, total)[0]
    if crc16(packet) != crc:
        raise SmpCorrupt("SMP response failed its CRC check")
    return packet


def frame_progress(b64: bytes) -> tuple[int, int]:
    """Return (bytes received, bytes declared) for a partial frame.

    A lost newline merges two lines and leaves a line marker inside the text,
    so only base64 alphabet characters are counted.
    """
    text = bytes(c for c in b64 if c in B64_ALPHABET)
    body = base64.b64decode(text[: len(text) - len(text) % 4])
    total = struct.unpack_from(">H", body)[0] if len(body) >= 2 else 0
    return max(len(body) - 2, 0), total


def crc16(data: bytes) -> int:
    """CRC-16/XMODEM, the checksum of the SMP serial transport."""
    crc = 0
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def cbor_encode_map(fields: dict) -> bytes:
    """Encode a CBOR map of text keys to text, byte string or unsigned integer values."""
    if len(fields) >= 24:
        raise ValueError("too many fields for an SMP request map")
    return bytes([0xA0 | len(fields)]) + b"".join(
        _cbor_item(key) + _cbor_item(value) for key, value in fields.items()
    )


def _cbor_item(value: str | bytes | int) -> bytes:
    if isinstance(value, str):
        data = value.encode()
        return _cbor_encode_head(3, len(data)) + data
    if isinstance(value, bytes):
        return _cbor_encode_head(2, len(value)) + value
    if isinstance(value, int) and not isinstance(value, bool) and value >= 0:
        return _cbor_encode_head(0, value)
    raise ValueError(f"cannot encode {value!r} in an SMP request")


def _cbor_encode_head(major: int, arg: int) -> bytes:
    if arg < 24:
        return bytes([(major << 5) | arg])
    for ai, fmt in ((24, ">B"), (25, ">H"), (26, ">I")):
        if arg < 1 << (8 * struct.calcsize(fmt)):
            return bytes([(major << 5) | ai]) + struct.pack(fmt, arg)
    raise ValueError("value too large for an SMP packet")


def cbor_decode_map(buf: bytes) -> dict:
    """Decode a buffer that holds exactly one CBOR map."""
    try:
        value, pos = _cbor_decode(buf, 0)
    except (IndexError, struct.error, UnicodeDecodeError) as exc:
        raise SmpCorrupt(f"truncated or invalid CBOR: {exc}") from None
    if pos != len(buf):
        raise SmpCorrupt("trailing bytes after the CBOR map")
    if not isinstance(value, dict):
        raise SmpCorrupt("expected a CBOR map")
    return value


def _cbor_decode(buf: bytes, pos: int):
    """Decode one CBOR item at `pos`; return the value and the next position."""
    major, val, pos = _cbor_head(buf, pos)
    if major == 0:
        return val, pos
    if major == 1:
        return -1 - val, pos
    if major in (2, 3):
        if val is None or pos + val > len(buf):
            raise SmpCorrupt("CBOR string runs past the end of the buffer")
        out = buf[pos : pos + val]
        return (out.decode() if major == 3 else out), pos + val
    if major == 4:
        items = []
        while _more(buf, pos, val, len(items)):
            item, pos = _cbor_decode(buf, pos)
            items.append(item)
        return items, pos + (val is None)
    if major == 5:
        out = {}
        while _more(buf, pos, val, len(out)):
            key, pos = _cbor_decode(buf, pos)
            out[key], pos = _cbor_decode(buf, pos)
        return out, pos + (val is None)
    raise SmpCorrupt(f"unsupported CBOR major type {major}")


def _more(buf: bytes, pos: int, count: int | None, done: int) -> bool:
    """Whether a container holds another item; an indefinite one ends at 0xff."""
    if count is None:
        return buf[pos] != 0xFF
    return done < count


def _cbor_head(buf: bytes, pos: int) -> tuple[int, int | None, int]:
    ib = buf[pos]
    pos += 1
    major, ai = ib >> 5, ib & 0x1F
    if ai < 24:
        return major, ai, pos
    if ai == 31:
        return major, None, pos
    width = {24: 1, 25: 2, 26: 4, 27: 8}.get(ai)
    if width is None:
        raise SmpCorrupt("unsupported CBOR additional information")
    if pos + width > len(buf):
        raise SmpCorrupt("CBOR item runs past the end of the buffer")
    return major, int.from_bytes(buf[pos : pos + width], "big"), pos + width
