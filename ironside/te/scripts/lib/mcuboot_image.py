# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Build and sign MCUboot images for IronSide secure boot.

UROT verifies an ECDSA P-384 signature over the image header, body, and
protected TLV area of the App and DFU images; IROT verifies the UROT image with an
ECDSA P-384 and an ML-DSA-65 signature (hybrid.py). This module implements the
required subset of the MCUboot image format without depending on imgtool.

Image layout:

  image_header (32 B, little-endian, == struct image_header in image.h;
                ih_hdr_size is 32)
  image body:  [ gap up to ENTRY_OFFSET ]   (0xff when pad_header, else the
                                             gap already in the input, which
                                             may carry linked code or data)
               [ vector table at ENTRY_OFFSET, then the firmware ]
  protected TLV area   (info magic 0x6908): SEC_CNT(0x50) [, IROT_IMAGE_ID(0x17E0)]
                                            [, IRONSIDE_CONF(0x17E1, image config)]
  unprotected TLV area (info magic 0x6907): SHA256(0x10, hash of the signed region)
                                            ECDSA_SIG(0x17E2, 96-byte r || s)
                                            ECDSA_PUBKEY(0x17E4, 97-byte point)
                                            [, ENC_KW(0x31), added by encryption]

IronSide TE boots an image from its vector table at ENTRY_OFFSET, not at
ih_hdr_size, so the gap after the header is body: signed, and encrypted with
the rest of the body. The signature covers the header, body, and protected TLVs. The root of trust
does not read the SHA256 TLV; MCUmgr clients do, for the image hash. KEYHASH,
SIG_PURE, and swap trailer data are omitted because IronSide does not use them.

Every image header carries IMAGE_F_ENCRYPTED_AES256, because every update is
encrypted and the flag is part of the signed region. A signed image whose body
is still plaintext is what imgtool --clear produces, and installs as it is.
"""

import hashlib
import struct
from pathlib import Path

from intelhex import IntelHex, IntelHexError

# uicr owns the memory-map address used to separate firmware from UICR data.
try:
    from lib import component_counter, ecdsa  # noqa: I001  (a try/except is not a sortable block)
    from lib import uicr as _uicr
except ImportError:  # run directly: this file's own dir is sys.path[0]
    import component_counter
    import ecdsa
    import uicr as _uicr

# --- format constants (mirror IROT's own image format header) ---------------
IMAGE_MAGIC = 0x96F3B83D
IMAGE_HEADER_SIZE = 32
TLV_INFO_MAGIC = 0x6907  # unprotected TLV area
TLV_PROT_INFO_MAGIC = 0x6908  # protected (signed) TLV area
TLV_INFO_SIZE = 4  # sizeof(struct image_tlv_info)
TLV_HDR_SIZE = 4  # sizeof(struct image_tlv)

IMAGE_F_ENCRYPTED_AES128 = 0x04
IMAGE_F_ENCRYPTED_AES256 = 0x08

TLV_SEC_CNT = 0x50  # protected: anti-rollback counter (u32)
TLV_IROT_IMAGE_ID = 0x17E0  # protected: image role identifier (u8)
TLV_IRONSIDE_CONF = 0x17E1  # protected: struct ironside_conf_img (image config)
TLV_SHA256 = 0x10  # unprotected: SHA-256 of header + plaintext body + protected TLVs
TLV_ECDSA_SIG = 0x17E2  # unprotected: r || s
TLV_ECDSA_PUBKEY = 0x17E4  # unprotected: 0x04 || X || Y
TLV_ENC_KW = 0x31  # unprotected: AES key wrap of the image body key
# The Ed25519 signature TLV of an App or DFU image signed by an earlier tool. No
# IronSide TE that verifies with ECDSA reads it; check() names it.
TLV_RETIRED_ED25519 = 0x24
ERASED_VAL = 0xFF  # imgtool default header-pad fill

# --- image roles -------------------------------------------------------------
# Signed image role used to route a shared update candidate to its slot.
# gen_img_meta.py and sign.py share this mapping.
IMAGE_ID = {"urot": 0, "app": 1, "dfu": 2}

# The curve App and DFU images are signed with.
CURVE = ecdsa.P384
ROLES = tuple(IMAGE_ID)

# Whether raw firmware needs a header gap prepended. Zephyr images reserve the
# gap through CONFIG_ROM_START_OFFSET. The bare-metal App does not.
PAD_HEADER_DEFAULT = {"urot": False, "app": True, "dfu": False}

# IronSide TE boots an image from its vector table this far into its slot. Raw
# firmware without a gap gets one of this size prepended.
ENTRY_OFFSET = 0x800

# The initial stack pointer and the reset vector, which the signed body must hold
# at ENTRY_OFFSET for IronSide TE to boot the image.
ENTRY_MIN_LEN = 8


class SignError(Exception):
    pass


# Released image.hex files may also contain UICR data far above the firmware.
# Split at the UICR base to avoid treating the sparse address gap as image data.
AUX_BASE = _uicr.UICR_HW_BASE


def _split_hex(path):
    """Split firmware below AUX_BASE from UICR data at or above it."""
    try:
        src = IntelHex(str(path))
    except IntelHexError as exc:
        raise SignError(f"{path}: not a readable Intel hex ({exc})") from exc
    image, aux = IntelHex(), IntelHex()
    for start, end in src.segments():
        if start < AUX_BASE <= end:
            raise SignError(
                f"{path}: a segment spans the UICR base "
                f"0x{AUX_BASE:08X}, so the firmware and the UICR words "
                f"cannot be told apart"
            )
        target = aux if start >= AUX_BASE else image
        target.frombytes(src.tobinarray(start=start, size=end - start), offset=start)
    image.padding = 0xFF
    return image, aux


def _read_hex(path):
    """Return (contiguous image bytes, base_addr); any gaps fill with 0xff."""
    image, _aux = _split_hex(path)
    if not len(image):
        raise SignError(f"{path}: no firmware below UICR base 0x{AUX_BASE:08X}")
    return bytes(image.tobinarray()), image.minaddr()


def _write_hex(path, data, base, aux=None):
    ih = IntelHex()
    ih.frombytes(bytes(data), offset=base)
    if aux is not None and len(aux):
        ih.merge(aux, overlap="error")
    ih.tofile(str(path), "hex")


def read_image(path):
    """Load a .hex (returns base addr) or .bin (base addr None).

    The FIRMWARE only -- see _split_hex(). Use read_aux() for the UICR words a
    released image.hex carries beside it.
    """
    if str(path).lower().endswith(".hex"):
        return _read_hex(path)
    return Path(path).read_bytes(), None


def read_aux(path):
    """Return UICR data from a .hex, or an empty IntelHex for other inputs."""
    if not str(path).lower().endswith(".hex"):
        return IntelHex()
    return _split_hex(path)[1]


def write_image(path, data, base_addr, aux=None):
    """Write image data and retain auxiliary UICR data in .hex output."""
    if str(path).lower().endswith(".hex"):
        if base_addr is None:
            raise SignError("a .hex output needs a base address (input was .bin)")
        _write_hex(path, data, base_addr, aux)
    else:
        Path(path).write_bytes(data)


# --- version + TLV helpers ---------------------------------------------------
VERSION_FIELDS = ("MAJOR", "MINOR", "PATCH", "TWEAK")

# Counter 1 avoids producing a downgrade for normally provisioned boards.
# An explicit version 0.0.0.0 or counter 0 is still accepted.
VERSION_DEFAULT = "0.0.0.1"


def parse_version(s):
    """Parse MAJOR.MINOR.PATCH[.TWEAK] into four one-byte fields."""
    fields = str(s).split(".")
    if not len(VERSION_FIELDS) - 1 <= len(fields) <= len(VERSION_FIELDS):
        raise SignError(
            f"version {s!r}: need 3 or 4 dot-separated fields, "
            f"{'.'.join(VERSION_FIELDS[:-1])}[.{VERSION_FIELDS[-1]}] "
            f"(got {len(fields)})"
        )
    fields += ["0"] * (len(VERSION_FIELDS) - len(fields))
    out = []
    for name, field in zip(VERSION_FIELDS, fields, strict=True):
        # isascii() as well as isdigit(): the latter alone accepts non-ASCII
        # digit characters that int() then rejects with a bare ValueError.
        if not (field.isascii() and field.isdigit()):
            raise SignError(f"version {s!r}: {name} {field!r} is not a decimal number")
        value = int(field)
        if value > 0xFF:
            raise SignError(f"version {s!r}: {name} is {value}, must fit in a byte (0-255)")
        out.append(value)
    return tuple(out)


def _version_fields(version):
    """Return and validate the four fields of a string or parsed version."""
    if isinstance(version, str):
        return parse_version(version)
    fields = tuple(version)
    if len(fields) != len(VERSION_FIELDS):
        raise SignError(
            f"version {fields!r}: need exactly 4 fields, "
            f"{'.'.join(VERSION_FIELDS)} (got {len(fields)})"
        )
    for name, value in zip(VERSION_FIELDS, fields, strict=True):
        if not isinstance(value, int) or isinstance(value, bool):
            raise SignError(f"version {fields!r}: {name} {value!r} is not an integer")
        if not 0 <= value <= 0xFF:
            raise SignError(f"version {fields!r}: {name} is {value}, must fit in a byte (0-255)")
    return fields


def security_counter_from_version(version):
    """Pack four version bytes as MAJOR<<24 | MINOR<<16 | PATCH<<8 | TWEAK."""
    major, minor, patch, tweak = _version_fields(version)
    return (major << 24) | (minor << 16) | (patch << 8) | tweak


def version_display(version):
    """Format a version as MAJOR.MINOR.PATCH+TWEAK."""
    major, minor, patch, tweak = _version_fields(version)
    return f"{major}.{minor}.{patch}+{tweak}"


def version_for_security_counter(counter):
    """Return the four-byte version that packs to a security counter."""
    if not 0 <= counter <= 0xFFFFFFFF:
        raise SignError(f"security counter {counter} is not a uint32")
    return (
        f"{(counter >> 24) & 0xFF}.{(counter >> 16) & 0xFF}."
        f"{(counter >> 8) & 0xFF}.{counter & 0xFF}"
    )


def counter_display(counter, role):
    """Format the security counter of a @role image as what it packs."""
    if role in component_counter.ROLES:
        return component_counter.display(counter)
    return f"version {version_display(version_for_security_counter(counter))}"


def security_counter_of(path):
    """Read the security counter from an image's protected TLV area."""
    _header_size, _version, _body, tlvs = read_unsigned(read_image(path)[0])
    return int.from_bytes(tlvs[TLV_SEC_CNT], "little")


def signed_region_sha256(path):
    """SHA-256 over an image's signed region: header + body + protected TLVs.

    The reference value for that image's measurement in a PSA attestation
    token: IronSide measures a software component over exactly these bytes, so
    this is what a verifier compares the token's measurement-value against.
    It is the signed region, so it excludes the signature itself and anything
    else past the protected TLV area.
    """
    region, _signature = split_signed(read_image(path)[0])
    return hashlib.sha256(region).hexdigest()


def _tlv(tlv_type, value):
    return struct.pack("<HH", tlv_type, len(value)) + value


def _tlv_area(magic, tlvs):
    return struct.pack("<HH", magic, TLV_INFO_SIZE + len(tlvs)) + tlvs


def hash_tlv(region):
    """Return the SHA256 TLV for signed region @region, the first unprotected TLV."""
    return _tlv(TLV_SHA256, hashlib.sha256(region).digest())


def load_signing_key_from_pem(pem_bytes, label="key"):
    """Load the ECDSA private key an App or DFU image is signed with."""
    try:
        return CURVE.load_private_key(pem_bytes, label)
    except (ValueError, TypeError) as exc:
        raise SignError(str(exc)) from exc


def load_signing_key(path):
    return load_signing_key_from_pem(Path(path).read_bytes(), label=str(path))


def parse_version_header(data):
    """Read and validate the four-field version from an image header."""
    if len(data) < IMAGE_HEADER_SIZE:
        raise SignError("file is smaller than an image header")
    if struct.unpack("<I", data[:4])[0] != IMAGE_MAGIC:
        raise SignError("not an MCUboot image")
    return _version_fields(struct.unpack("<BBHI", data[20:28]))


# --- the unsigned image: firmware + metadata, no signature -------------------
# The SHA256 TLV every signed image carries in its unprotected area.
HASH_TLV_SIZE = TLV_HDR_SIZE + hashlib.sha256().digest_size

# Space required for the unprotected TLV header, the hash TLV, the signature TLV and
# the public-key TLV of an App or DFU image.
SIGNATURE_AREA_SIZE = (
    TLV_INFO_SIZE + HASH_TLV_SIZE + 2 * TLV_HDR_SIZE + CURVE.sig_len + CURVE.pubkey_len
)

# Canonical order keeps rebuilt images byte-identical to newly built images.
PROTECTED_TLV_ORDER = (TLV_SEC_CNT, TLV_IROT_IMAGE_ID, TLV_IRONSIDE_CONF)


def _protected_area(tlvs):
    """Build protected TLVs in canonical order."""
    unknown = set(tlvs) - set(PROTECTED_TLV_ORDER)
    if unknown:
        raise SignError(
            "unknown protected TLV type(s) " + ", ".join(f"0x{t:02X}" for t in sorted(unknown))
        )
    body = b"".join(_tlv(t, tlvs[t]) for t in PROTECTED_TLV_ORDER if t in tlvs)
    return _tlv_area(TLV_PROT_INFO_MAGIC, body)


def _metadata_tlvs(*, security_counter, image_id=None, conf_img=None):
    """Build image metadata TLVs. The security counter is always present."""
    tlvs = {TLV_SEC_CNT: struct.pack("<I", security_counter)}
    if image_id is not None:
        tlvs[TLV_IROT_IMAGE_ID] = struct.pack("<B", image_id & 0xFF)
    if conf_img is not None:
        tlvs[TLV_IRONSIDE_CONF] = bytes(conf_img)
    return tlvs


def _check_counter(security_counter):
    if not 0 <= security_counter <= 0xFFFFFFFF:
        raise SignError(f"security counter {security_counter} must fit in a uint32 (0..4294967295)")
    return security_counter


def _pack_header(*, protect_tlv_size, img_size, version):
    """Pack struct image_header with zero load address and the AES-256 encryption flag."""
    major, minor, patch, tweak = _version_fields(version)
    header = struct.pack(
        "<IIHHII",
        IMAGE_MAGIC,
        0,
        IMAGE_HEADER_SIZE,
        protect_tlv_size,
        img_size,
        IMAGE_F_ENCRYPTED_AES256,
    )
    header += struct.pack("<BBHI", major, minor, patch, tweak)
    header += struct.pack("<I", 0)
    assert len(header) == IMAGE_HEADER_SIZE
    return header


def build_unsigned(
    image,
    *,
    slot_size=0,
    version=VERSION_DEFAULT,
    security_counter=None,
    pad_header=False,
    image_id=None,
    conf_img=None,
    signature_area_size=SIGNATURE_AREA_SIZE,
):
    """Build firmware and metadata without a signature.

    The security counter defaults to the packed version. Returns the unsigned
    bytes and the slot base for HEX input, or None for binary input.
    @signature_area_size is the space the signature scheme will append.
    """
    version = _version_fields(version)
    if security_counter is None:
        security_counter = security_counter_from_version(version)
    _check_counter(security_counter)

    data, base = read_image(image)

    if pad_header:
        payload = bytes([ERASED_VAL]) * ENTRY_OFFSET + data
        if base is not None:
            base -= ENTRY_OFFSET
    else:
        # The header overwrites only its own 32 bytes. The rest of the gap is
        # body, so an image may link code or data there.
        if any(b != 0 for b in data[:IMAGE_HEADER_SIZE]):
            raise SignError(
                f"no --pad-header but the image does not start with {IMAGE_HEADER_SIZE} zero "
                "bytes for the header"
            )
        payload = bytearray(data)

    if len(payload) < ENTRY_OFFSET + ENTRY_MIN_LEN:
        raise SignError(
            f"image ends before its vector table at 0x{ENTRY_OFFSET:x}. Pass --pad-header for "
            "firmware that does not reserve the gap."
        )
    img_size = len(payload) - IMAGE_HEADER_SIZE

    prot_area = _protected_area(
        _metadata_tlvs(security_counter=security_counter, image_id=image_id, conf_img=conf_img)
    )
    header = _pack_header(protect_tlv_size=len(prot_area), img_size=img_size, version=version)
    out = header + bytes(payload)[IMAGE_HEADER_SIZE:] + prot_area

    # Reserve signature space even though this function does not write it.
    if slot_size and len(out) + signature_area_size > slot_size:
        raise SignError(
            f"image + metadata + signature "
            f"(0x{len(out) + signature_area_size:x}) exceeds slot "
            f"0x{slot_size:x}"
        )
    return out, base


def read_unsigned(data):
    """Return header size, version, body, and protected TLVs from an unsigned image."""
    if len(data) < IMAGE_HEADER_SIZE:
        raise SignError("file is smaller than an image header")
    magic, _load, header_size, protect_tlv_size, img_size, _flags = struct.unpack(
        "<IIHHII", data[:20]
    )
    if magic != IMAGE_MAGIC:
        raise SignError(
            f"not an MCUboot image (magic 0x{magic:08X}). Expected firmware with metadata."
        )
    major, minor, patch, tweak = struct.unpack("<BBHI", data[20:28])
    if patch > 0xFF or tweak > 0xFF:
        raise SignError(
            f"image header version {major}.{minor}.{patch}+{tweak} has a field outside a byte"
        )
    end = header_size + img_size
    if protect_tlv_size < TLV_INFO_SIZE or end + protect_tlv_size > len(data):
        raise SignError(
            "image header describes a protected TLV area that is not "
            "present. Metadata is missing or truncated."
        )
    if len(data) > end + protect_tlv_size:
        raise SignError(
            "image already carries an unprotected TLV area (it is signed). Refusing to re-sign it."
        )

    area = data[end : end + protect_tlv_size]
    area_magic, area_len = struct.unpack("<HH", area[:TLV_INFO_SIZE])
    if area_magic != TLV_PROT_INFO_MAGIC or area_len != protect_tlv_size:
        raise SignError("protected TLV area magic/length mismatch")
    tlvs, off = {}, TLV_INFO_SIZE
    while off + TLV_HDR_SIZE <= len(area):
        t, ln = struct.unpack("<HH", area[off : off + TLV_HDR_SIZE])
        off += TLV_HDR_SIZE
        if off + ln > len(area):
            raise SignError(f"protected TLV 0x{t:02X} runs past the area")
        tlvs[t] = bytes(area[off : off + ln])
        off += ln
    if TLV_SEC_CNT not in tlvs:
        raise SignError("protected area carries no security counter")
    return header_size, (major, minor, patch, tweak), bytes(data[header_size:end]), tlvs


def rebuild(data, *, version=None, security_counter=None, conf_img=None, image_id=None):
    """Replace metadata in an unsigned image.

    A version change also derives a new security counter unless one is supplied.
    """
    header_size, current_version, body, tlvs = read_unsigned(data)
    # An image built with the gap as header padding keeps it, now as body.
    body = bytes(data[IMAGE_HEADER_SIZE : header_size + len(body)])

    if version is not None:
        version = _version_fields(version)
        if security_counter is None:
            security_counter = security_counter_from_version(version)
    else:
        version = current_version

    if security_counter is not None:
        tlvs[TLV_SEC_CNT] = struct.pack("<I", _check_counter(security_counter))
    if image_id is not None:
        tlvs[TLV_IROT_IMAGE_ID] = struct.pack("<B", image_id & 0xFF)
    if conf_img is not None:
        tlvs[TLV_IRONSIDE_CONF] = bytes(conf_img)

    prot_area = _protected_area(tlvs)
    header = _pack_header(protect_tlv_size=len(prot_area), img_size=len(body), version=version)
    return header + body + prot_area


def sign_unsigned(data, key, *, slot_size=0, pad=False):
    """Sign an unsigned image with ECDSA and append the signature and key TLVs."""
    read_unsigned(data)  # reject a non-image, or an already-signed one
    region = bytes(data)
    tlvs = (
        hash_tlv(region)
        + _tlv(TLV_ECDSA_SIG, CURVE.sign(key, region))
        + _tlv(TLV_ECDSA_PUBKEY, ecdsa.public_bytes(key))
    )
    out = region + _tlv_area(TLV_INFO_MAGIC, tlvs)
    assert len(out) == len(region) + SIGNATURE_AREA_SIZE

    if slot_size and len(out) > slot_size:
        raise SignError(f"signed image (0x{len(out):x}) exceeds slot 0x{slot_size:x}")
    if pad and slot_size:
        out += bytes([ERASED_VAL]) * (slot_size - len(out))
    return out


# --- the signer --------------------------------------------------------------
def find_protected_tlv(image, tlv_type):
    """Read a protected TLV from an image, or return None if absent."""
    data, _ = read_image(image)
    if len(data) < IMAGE_HEADER_SIZE:
        raise SignError("file is smaller than an image header")
    magic, _load, header_size, protect_tlv_size, img_size, _flags = struct.unpack(
        "<IIHHII", data[:20]
    )
    if magic != IMAGE_MAGIC:
        raise SignError(f"not an MCUboot image (magic 0x{magic:08X})")
    if protect_tlv_size < TLV_INFO_SIZE:
        return None  # no protected area at all
    start = header_size + img_size
    area = data[start : start + protect_tlv_size]
    area_magic, area_len = struct.unpack("<HH", area[:TLV_INFO_SIZE])
    if area_magic != TLV_PROT_INFO_MAGIC or area_len != protect_tlv_size:
        raise SignError("protected TLV area magic/length mismatch")
    off = TLV_INFO_SIZE
    while off + TLV_HDR_SIZE <= len(area):
        t, ln = struct.unpack("<HH", area[off : off + TLV_HDR_SIZE])
        off += TLV_HDR_SIZE
        if t == tlv_type:
            return bytes(area[off : off + ln])
        off += ln
    return None


def split_unprotected_tlvs(data):
    """Return the signed region and the unprotected TLVs, keyed by TLV type.

    A type that appears more than once maps to its first value, the one the
    firmware's TLV lookup finds.
    """
    if len(data) < IMAGE_HEADER_SIZE:
        raise SignError("file is smaller than an image header")
    magic, _load, header_size, protect_tlv_size, img_size, _flags = struct.unpack(
        "<IIHHII", data[:20]
    )
    if magic != IMAGE_MAGIC:
        raise SignError(f"not an MCUboot image (magic 0x{magic:08X})")
    end = header_size + img_size + protect_tlv_size
    if end > len(data):
        raise SignError("image header describes more than the file holds")
    region, area = bytes(data[:end]), bytes(data[end:])
    tlvs = {}
    if len(area) < TLV_INFO_SIZE:
        return region, tlvs
    area_magic, area_len = struct.unpack("<HH", area[:TLV_INFO_SIZE])
    if area_magic != TLV_INFO_MAGIC:
        raise SignError("unprotected TLV area magic mismatch")
    if area_len > len(area):
        raise SignError("unprotected TLV area runs past the file")
    off = TLV_INFO_SIZE
    while off + TLV_HDR_SIZE <= area_len:
        tlv_type, length = struct.unpack("<HH", area[off : off + TLV_HDR_SIZE])
        off += TLV_HDR_SIZE
        tlvs.setdefault(tlv_type, bytes(area[off : off + length]))
        off += length
    return region, tlvs


def append_unprotected_tlv(data, tlv_type, value):
    """Return signed image bytes @data with one TLV added to its unprotected area."""
    region, _tlvs = split_unprotected_tlvs(data)
    area = bytes(data[len(region) :])
    if len(area) < TLV_INFO_SIZE:
        raise SignError("image has no unprotected TLV area. Sign it first.")
    _magic, area_len = struct.unpack("<HH", area[:TLV_INFO_SIZE])
    if area_len != len(area):
        raise SignError("unprotected TLV area does not end the file")
    return region + _tlv_area(TLV_INFO_MAGIC, area[TLV_INFO_SIZE:] + _tlv(tlv_type, value))


def split_signed(data):
    """Return the signed region and the ECDSA signature, if present.

    Every IronSide TE image is signed with ECDSA, the UROT image as one leg of its
    hybrid signature, so a signature here means the image is signed.
    """
    region, tlvs = split_unprotected_tlvs(data)
    return region, tlvs.get(TLV_ECDSA_SIG)


def has_retired_signature(data):
    """Whether image bytes @data carry only the Ed25519 signature of an earlier tool."""
    try:
        _region, tlvs = split_unprotected_tlvs(data)
    except SignError:
        return False
    return TLV_RETIRED_ED25519 in tlvs and TLV_ECDSA_SIG not in tlvs


def verify_signed(image):
    """Check an image's ECDSA signature against the public key it carries.

    Returns the key digest a device stores for that key when the signature verifies,
    or None. It establishes that the image matches the key it carries, not trust in
    the key: a device trusts the key only if its digest is one the device holds.
    """
    data, _ = read_image(image)
    region, tlvs = split_unprotected_tlvs(data)
    signature = tlvs.get(TLV_ECDSA_SIG)
    public = tlvs.get(TLV_ECDSA_PUBKEY)
    if signature is None or public is None:
        return None
    for name, value, length in (
        ("signature", signature, CURVE.sig_len),
        ("public key", public, CURVE.pubkey_len),
    ):
        if len(value) != length:
            raise SignError(f"ECDSA {name} TLV is {len(value)} bytes, expected {length}")
    if not CURVE.verify(public, region, signature):
        return None
    return CURVE.key_digest(public)


def has_metadata(image):
    """Return whether an image starts with MCUboot metadata."""
    data, _ = read_image(image)
    if len(data) < IMAGE_HEADER_SIZE:
        return False
    return struct.unpack("<I", data[:4])[0] == IMAGE_MAGIC


def unsigned_image(
    image,
    *,
    slot_size,
    version=None,
    security_counter=None,
    pad_header=False,
    image_id=None,
    conf_img=None,
    signature_area_size=SIGNATURE_AREA_SIZE,
):
    """Build metadata if needed and apply overrides, without signing.

    Existing metadata is retained unless replaced. A raw image receives default
    metadata. Returns the unsigned bytes and the HEX slot base, or None for
    binary input.
    """
    if has_metadata(image):
        data, base = read_image(image)
        unsigned = rebuild(
            data,
            version=version,
            security_counter=security_counter,
            conf_img=conf_img,
            image_id=image_id,
        )
        return unsigned, base
    return build_unsigned(
        image,
        slot_size=slot_size,
        version=VERSION_DEFAULT if version is None else version,
        security_counter=security_counter,
        pad_header=pad_header,
        image_id=image_id,
        conf_img=conf_img,
        signature_area_size=signature_area_size,
    )


def sign_image(image, key, *, pad=False, **unsigned_args):
    """Build metadata if needed, apply overrides, and sign an image with ECDSA P-384.

    Takes unsigned_image()'s arguments. Returns the signed bytes and the HEX slot
    base, or None for binary input.
    """
    unsigned, base = unsigned_image(image, **unsigned_args)
    return sign_unsigned(unsigned, key, slot_size=unsigned_args["slot_size"], pad=pad), base


def _self_test() -> None:
    """Check version, counter, metadata, and signing behavior."""
    import tempfile  # self-test only; the signing path needs no temp file

    # Counter 0 is a downgrade on a normally provisioned board.
    assert VERSION_DEFAULT == "0.0.0.1", VERSION_DEFAULT
    assert security_counter_from_version(VERSION_DEFAULT) == 1

    # The 4-byte version read as one uint32 (Zephyr's APPVERSION packing).
    for version, counter in (
        ("0.0.0.0", 0x00000000),
        ("0.0.0.1", 0x00000001),
        ("1.2.3.4", 0x01020304),
        ("8.1.0.0", 0x08010000),
        ("0.0.1.0", 0x00000100),
        ("255.255.255.255", 0xFFFFFFFF),
    ):
        assert security_counter_from_version(version) == counter, version
        assert security_counter_from_version(parse_version(version)) == counter

    assert parse_version("1.2.3.4") == (1, 2, 3, 4)
    assert parse_version(VERSION_DEFAULT) == (0, 0, 0, 1)

    # A three-field version implies TWEAK 0.
    for short, full in (
        ("0.0.1", "0.0.1.0"),
        ("1.2.3", "1.2.3.0"),
        ("255.255.255", "255.255.255.0"),
    ):
        assert parse_version(short) == parse_version(full), short
        assert security_counter_from_version(short) == security_counter_from_version(full), short
    assert security_counter_from_version("0.0.1") == 0x00000100

    # The command line spells TWEAK after a dot, a header reader after a '+'.
    assert version_display("1.2.3.4") == "1.2.3+4"
    assert version_display((8, 1, 0, 0)) == "8.1.0+0"

    for bad in (
        "8.1",  # 2 fields
        "1.2.3.4.5",  # 5 fields
        "1.2.3+4",  # imgtool/Zephyr's '+' spelling
        "1.2.300.0",  # field over a byte
        "1.2.-3.4",  # negative
        "1.2..4",  # empty field
        "1.2.3.0x4",  # not decimal
        "1.2.3.\u0664",  # non-ASCII digit int() would also reject
        "",
    ):
        try:
            parse_version(bad)
        except SignError:
            continue
        raise AssertionError(f"version {bad!r} must be rejected")

    # Parsed tuples receive the same validation as strings.
    for bad_fields in (
        (1, 2, 3, 999),  # field over a byte
        (1, 2, 3, -1),  # negative (would reach struct.pack)
        (1, 2, 3),  # too few
        (1, 2, 3, 4, 5),  # too many
        (1, 2, 3, "4"),  # not an integer
        (1, 2, 3, True),
    ):  # bool is an int subclass; not a version
        try:
            security_counter_from_version(bad_fields)
        except SignError:
            continue
        raise AssertionError(f"version fields {bad_fields!r} must be rejected")

    # End to end: the derived counter reaches the protected TLV, the header
    # carries the four bytes, and the signature covers header+body+protected area.
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / "fixture.bin"
        src.write_bytes(bytes(0x800) + bytes(range(256)))
        key = CURVE.load_private_key(CURVE.generate_private_pem())
        signed, base = sign_image(
            src, key, slot_size=0, version="1.2.3.4", image_id=1
        )
        assert base is None
        out = Path(tmp) / "signed.bin"
        write_image(out, signed, base)
        assert find_protected_tlv(out, TLV_SEC_CNT) == struct.pack("<I", 0x01020304)
        region, tlvs = split_unprotected_tlvs(signed)
        assert tlvs[TLV_SHA256] == hashlib.sha256(region).digest()
        assert struct.unpack("<BBHI", signed[20:28]) == (1, 2, 3, 4)

        # An explicit counter overrides the derivation and nothing else changes.
        explicit, _ = sign_image(
            src,
            key,
            slot_size=0,
            version="1.2.3.4",
            security_counter=1,
            image_id=1,
        )
        assert explicit != signed
        assert explicit[20:28] == signed[20:28]
        for bad_counter in (-1, 0x100000000):
            try:
                sign_image(src, key, slot_size=0, security_counter=bad_counter)
            except SignError:
                continue
            raise AssertionError(f"security counter {bad_counter} must be rejected")

    # Split and one-step signing must produce identical images.
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / "fixture.bin"
        src.write_bytes(bytes(0x800) + bytes(range(256)))
        key = CURVE.load_private_key(CURVE.generate_private_pem())
        conf_a, conf_b = bytes(range(40)), bytes(range(40, 80))
        build = dict(slot_size=0, version="0.0.1", image_id=0)

        unsigned, _ = build_unsigned(src, conf_img=conf_a, **build)
        # An unsigned image contains only the signed region.
        assert len(unsigned) == 0x800 + 256 + len(
            _protected_area(_metadata_tlvs(security_counter=0x100, image_id=0, conf_img=conf_a))
        )
        header_size, version, body, tlvs = read_unsigned(unsigned)
        assert (header_size, version) == (IMAGE_HEADER_SIZE, (0, 0, 1, 0))
        assert body == bytes(ENTRY_OFFSET - IMAGE_HEADER_SIZE) + bytes(range(256))  # gap is body
        assert tlvs[TLV_SEC_CNT] == struct.pack("<I", 0x100)  # derived from 0.0.1
        assert tlvs[TLV_IRONSIDE_CONF] == conf_a

        # An image whose header covered the gap rebuilds with the gap as body.
        legacy = bytearray(unsigned)
        struct.pack_into("<H", legacy, 8, ENTRY_OFFSET)
        struct.pack_into("<I", legacy, 12, 256)
        assert rebuild(bytes(legacy)) == unsigned

        # The body must reach the vector table at the entry offset.
        short = Path(tmp) / "short.bin"
        short.write_bytes(bytes(ENTRY_OFFSET))
        try:
            build_unsigned(short, **build)
        except SignError:
            pass
        else:
            raise AssertionError("an image without a vector table at the entry must be refused")

        # sign_image() over a raw image == build_unsigned() then sign_unsigned().
        one_step, _ = sign_image(src, key, conf_img=conf_a, **build)
        assert one_step == sign_unsigned(unsigned, key)

        # Signing a pre-built image retains its metadata.
        pre = Path(tmp) / "image.bin"
        write_image(pre, unsigned, None)
        assert has_metadata(pre) and not has_metadata(src)
        assert sign_image(pre, key, **build)[0] == one_step

        # An override rebuilds the signed region in canonical order.
        assert (
            rebuild(unsigned, conf_img=conf_b) == build_unsigned(src, conf_img=conf_b, **build)[0]
        )
        assert (
            sign_image(pre, key, conf_img=conf_b, **{**build, "version": None})[0]
            == sign_image(src, key, conf_img=conf_b, **build)[0]
        )

        # A version override derives a new counter unless one is supplied.
        assert read_unsigned(rebuild(unsigned, version="1.2.3"))[3][TLV_SEC_CNT] == struct.pack(
            "<I", 0x01020300
        )
        assert read_unsigned(rebuild(unsigned, version="1.2.3", security_counter=7))[3][
            TLV_SEC_CNT
        ] == struct.pack("<I", 7)

        # Signed and malformed images are rejected.
        signed_path = Path(tmp) / "signed.bin"
        write_image(signed_path, one_step, None)
        for bad in (one_step, bytes(64), bytes([0xFF]) * 128):
            try:
                read_unsigned(bad)
            except SignError:
                continue
            raise AssertionError("read_unsigned must reject a signed/non-image input")

        # Slot validation includes space for the signature.
        exact = len(unsigned) + SIGNATURE_AREA_SIZE
        build_unsigned(src, conf_img=conf_a, **{**build, "slot_size": exact})
        try:
            build_unsigned(src, conf_img=conf_a, **{**build, "slot_size": exact - 1})
        except SignError:
            pass
        else:
            raise AssertionError("build_unsigned must reserve the signature's room")

        # Unknown protected TLVs are not silently dropped.
        try:
            _protected_area({TLV_SEC_CNT: b"\x00" * 4, 0x99: b""})
        except SignError:
            pass
        else:
            raise AssertionError("an unknown protected TLV type must be refused")

        # UICR data is excluded from the image and retained through signing.
        hex_in = Path(tmp) / "component.hex"
        uicr_word = AUX_BASE + 0x2B0
        composed = IntelHex()
        composed.frombytes(unsigned, offset=0x4000)
        composed.frombytes(bytes([0x00, 0x00, 0x01, 0x00]), offset=uicr_word)
        composed.tofile(str(hex_in), "hex")

        data, base_addr = read_image(hex_in)
        assert base_addr == 0x4000
        assert data == unsigned  # the UICR words are not in the image
        aux = read_aux(hex_in)
        assert aux.minaddr() == uicr_word and len(aux) == 4

        hex_out = Path(tmp) / "component.signed.hex"
        write_image(hex_out, sign_unsigned(unsigned, key), base_addr, aux=aux)
        back = IntelHex(str(hex_out))
        assert back.tobinarray(start=uicr_word, size=4).tobytes() == bytes([0x00, 0x00, 0x01, 0x00])
        assert read_image(hex_out)[0] == sign_unsigned(unsigned, key)

    print("mcuboot_image self-test: OK")


if __name__ == "__main__":
    _self_test()
