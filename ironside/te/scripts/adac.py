#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Run IronSide ADAC commands over the J-Link CTRL-AP mailbox.

ADAC provides discovery, authentication, lock debug, the lifecycle change to
Decommissioned, authenticated erase-all, boot-mode selection and the boot report
through UROT.

Commands normally enter ADAC mode and return the device to normal boot. Run
enter, use --command-only for each command, then run exit to keep one session
open.
"""

import argparse
import json
import os
import struct
import sys
import time
from pathlib import Path

import pylink
from lib import boot_report, ecdsa

# pylink addresses registers by index within the bank selected through DP SELECT.
# Full AP offset 0xNN maps to bank (off>>4)&0xF and index (off>>2)&0x3.
DP_CTRL_STAT = 0x1  # CTRL/STAT (DPBANKSEL == 0)
DP_SELECT = 0x2  # SELECT

# DP CTRL/STAT power request / acknowledge bits.
CSYSPWRUPREQ = 1 << 30
CSYSPWRUPACK = 1 << 31
CDBGPWRUPREQ = 1 << 28
CDBGPWRUPACK = 1 << 29
PWRUP_REQ = CSYSPWRUPREQ | CDBGPWRUPREQ
PWRUP_ACK = CSYSPWRUPACK | CDBGPWRUPACK

# CTRL-AP index and debugger-side register offsets.
CTRL_AP_IDX = 2
CTRL_AP_RESET = 0x00  # level: 1 = assert/hold, 0 = deassert/release
CTRL_AP_ERASEALL = 0x04  # unconditional secure erase (when unprotected)
CTRL_AP_ERASEALLSTATUS = 0x08  # 0 Ready, 1 ReadyToReset, 2 Busy, 3 Error
CTRL_AP_ERASEPROTECT_STATUS = 0x0C  # bit 0 = PALL (1 = erase protection engaged)
CTRL_AP_ERASEPROTECT_DISABLE = 0x10  # key-matched ERASEALL (debugger-side half)
CTRL_AP_TXDATA = 0x20  # debugger -> CPU
CTRL_AP_TXSTATUS = 0x24  # 1 = word pending (not yet read by the CPU)
CTRL_AP_RXDATA = 0x28  # CPU -> debugger
CTRL_AP_RXSTATUS = 0x2C  # 1 = word pending (CPU wrote, host not yet read)
CTRL_AP_IDR = 0xFC  # AP identification register

# CTRL-AP ERASEALLSTATUS finite-state values (shared by the erase paths).
ERASEALLSTATUS_NAMES = {0: "Ready", 1: "ReadyToReset", 2: "Busy", 3: "Error"}

# CTRL-AP RESET register values (level: written to assert, 0 to release).
RESET_NONE = 0x0
RESET_SOFT = 0x1  # soft reset -> RESETREAS.CTRLAPSOFT (normal boot; keeps the SWD link)
RESET_HARD = 0x2  # hard reset -> RESETREAS.CTRLAPHARD (the ADAC-entry request)
RESET_PIN = 0x4  # pin reset  -> RESETREAS.CTRLAPPIN (normal boot; the ADAC exit)

# ADAC entry-handshake words (mirror UROT's own handshake words).  All 0xADAC-prefixed.
ADAC_SDA_ID_UROT = 0x02
ADAC_MAGIC_READY = 0xADAC1B07  # UROT -> host: ready
ADAC_MAGIC_ENTER = 0xADAC0000 | ADAC_SDA_ID_UROT  # host -> UROT: enter this SDA
ADAC_MAGIC_ACK = 0xADAC000C  # UROT -> host: handshake acknowledge

EXIT_VIA_DEFAULT = "pin"
SESSION_COMMANDS = ("enter", "exit")
# Commands that operate purely on CTRL-AP registers, without the ADAC (mailbox)
# entry handshake and without a resident UROT.
CTRLAP_ONLY = ("recover",)


def dp_select(jlink, apsel, apbanksel=0):
    """Program the DP SELECT register: which AP and which AP register bank."""
    jlink.coresight_write(DP_SELECT, ((apsel & 0xFF) << 24) | ((apbanksel & 0xF) << 4), ap=False)


def ap_read(jlink, apsel, offset):
    """Read an AP register addressed by its full byte offset."""
    dp_select(jlink, apsel, (offset >> 4) & 0xF)
    return jlink.coresight_read((offset >> 2) & 0x3, ap=True)


def ap_write(jlink, apsel, offset, value):
    """Write an AP register addressed by its full byte offset."""
    dp_select(jlink, apsel, (offset >> 4) & 0xF)
    jlink.coresight_write((offset >> 2) & 0x3, value, ap=True)


def power_up(jlink, timeout_s=1.0):
    """Request and wait for debug + system power via DP CTRL/STAT."""
    dp_select(jlink, apsel=0, apbanksel=0)  # DPBANKSEL == 0 -> index 1 = CTRL/STAT
    jlink.coresight_write(DP_CTRL_STAT, PWRUP_REQ, ap=False)

    deadline = time.monotonic() + timeout_s
    while True:
        status = jlink.coresight_read(DP_CTRL_STAT, ap=False)
        if (status & PWRUP_ACK) == PWRUP_ACK:
            return status
        if time.monotonic() > deadline:
            raise TimeoutError(
                "Timed out waiting for CDBGPWRUPACK/CSYSPWRUPACK (CTRL/STAT = 0x%08X)" % status
            )
        time.sleep(0.001)


# --- ADAC command IDs / status codes (mirror UROT's own ctrlap header) ---
CMD_DISCOVERY = 0x0001
CMD_AUTH_START = 0x0002
CMD_AUTH_RESPONSE = 0x0003
CMD_LOCK_DEBUG = 0x0006
CMD_LCS_CHANGE = 0x0100
CMD_ERASE_ALL = 0x8000
CMD_BOOTMODE_SET = 0x8001
CMD_BOOT_REPORT_GET = 0x8002

STATUS = {
    0x0000: "SUCCESS",
    0x0001: "FAILURE",
    0x0002: "NEED_MORE_DATA",
    0x0003: "UNSUPPORTED",
    0x0004: "UNAUTHORIZED",
    0x0005: "INVALID_PARAMETERS",
    0x7FFF: "INVALID_COMMAND",
}
ST_SUCCESS = 0x0000
ST_FAILURE = 0x0001
ST_NEED_MORE_DATA = 0x0002

# --- ADAC constants (mirror UROT's own adac header) ---
CRYPTO_ECDSA_P384_SHA384 = 0x0A  # ADAC 1.1
TLV_PSA_LIFECYCLE = 0x0008
TLV_EFFECTIVE_PERMISSIONS = 0x000C
TLV_CRYPTOSYSTEMS = 0x0102
FMT_TOKEN_ADAC = 0x0200
FMT_CERT_ADAC = 0x0201
ROLE_ROOT = 0x1
ROLE_LEAF = 0x3
USAGE_STANDARD = 0x1
PERM_BIT_ERASE_ALL = 64

# BOOTMODE_SET uses bits [2:0]. UROT rejects nonzero reserved bits.
BOOTMODE = {"app": 0, "dfu": 1}

# UROT reports Open as PSA Assembly and Test.
PSA_LIFECYCLE = {
    "assembly": 0x1000,  # Assembly and Test (incl. the Open sub-state)
    "prot-provisioning": 0x2000,
    "secured": 0x3000,
    "non-prot-debug": 0x4000,
    "decommissioned": 0x6000,
}
PSA_LIFECYCLE_NAME = {v: k for k, v in PSA_LIFECYCLE.items()}

# LCS_CHANGE accepts only Decommissioned. Other transitions use provisioning,
# authenticated debug, or erase.
LCS_CHANGE_TARGET = "decommissioned"

# LCS_CHANGE requires authentication but has no assigned permission bit. Requesting
# all permissions would persist a non-secure debug grant across reset.
PERM_BITS_LCS_CHANGE = ()

# BOOT_REPORT_GET requires authentication but has no assigned permission bit either.
PERM_BITS_BOOT_REPORT = ()

# Covers reboot through IROT and UROT plus their signature verification.
MB_TIMEOUT_S = 5.0


def _words(payload: bytes):
    """Split a 4-byte-aligned payload into little-endian 32-bit words."""
    assert len(payload) % 4 == 0, "payload must be 32-bit aligned"
    return struct.unpack("<%dI" % (len(payload) // 4), payload)


def perm_vector(bits=None, all_ones=True) -> bytes:
    """Build a 16-byte (128-bit) permission bitmap."""
    if all_ones and not bits:
        return b"\xff" * 16
    v = bytearray(16)
    for b in bits or ():
        v[b // 8] |= 1 << (b % 8)
    return bytes(v)


def parse_tlv_seq(data: bytes) -> dict:
    """Parse a TLV sequence into {type_id: value_bytes} (first wins)."""
    out, off = {}, 0
    while off + 8 <= len(data):
        type_id = data[off + 2] | (data[off + 3] << 8)
        length = struct.unpack_from("<I", data, off + 4)[0]
        padded = (length + 3) & ~3
        if off + 8 + padded > len(data):
            break
        out.setdefault(type_id, data[off + 8 : off + 8 + length])
        off += 8 + padded
    return out


class AdacSession:
    """The ADAC command protocol over the CTRL-AP mailbox."""

    def __init__(self, jlink):
        self.jlink = jlink
        self.ready_word = None  # set by enter(): UROT's readiness word (0xADAC1B07)

    # --- mailbox word transport (status bit: nonzero == data pending) ---
    def _wait(self, status_reg, want_pending):
        deadline = time.monotonic() + MB_TIMEOUT_S
        while True:
            pending = ap_read(self.jlink, CTRL_AP_IDX, status_reg) != 0
            if pending == want_pending:
                return
            if time.monotonic() > deadline:
                raise TimeoutError(
                    "mailbox status 0x%02X stuck (want pending=%s)" % (status_reg, want_pending)
                )
            time.sleep(0.001)

    def _tx_word(self, word):
        # Wait for the host->device slot to drain (device read the last word).
        self._wait(CTRL_AP_TXSTATUS, want_pending=False)
        ap_write(self.jlink, CTRL_AP_IDX, CTRL_AP_TXDATA, word & 0xFFFFFFFF)

    def _rx_word(self):
        self._wait(CTRL_AP_RXSTATUS, want_pending=True)
        return ap_read(self.jlink, CTRL_AP_IDX, CTRL_AP_RXDATA)

    def enter(self, wait_ms, ready_timeout_ms=None):
        """Reset into ADAC, wait for readiness, and complete the SDA handshake."""
        # Flush retained device-to-host data before waiting for this boot.
        while ap_read(self.jlink, CTRL_AP_IDX, CTRL_AP_RXSTATUS) != 0:
            ap_read(self.jlink, CTRL_AP_IDX, CTRL_AP_RXDATA)

        # 1. HardReset: RESET is a level, so pulse assert -> release to boot.
        ap_write(self.jlink, CTRL_AP_IDX, CTRL_AP_RESET, RESET_HARD)
        ap_write(self.jlink, CTRL_AP_IDX, CTRL_AP_RESET, RESET_NONE)

        # 2. Wait for the readiness word.
        ready_s = (
            max(wait_ms, MB_TIMEOUT_S * 1000) if ready_timeout_ms is None else ready_timeout_ms
        ) / 1000.0
        deadline = time.monotonic() + ready_s
        while ap_read(self.jlink, CTRL_AP_IDX, CTRL_AP_RXSTATUS) == 0:
            if time.monotonic() > deadline:
                raise TimeoutError(
                    "no ADAC readiness word after HardReset (device not in ADAC entry?)"
                )
            time.sleep(0.01)
        ready = ap_read(self.jlink, CTRL_AP_IDX, CTRL_AP_RXDATA)
        # Store this for human output without adding a separate JSON object.
        self.ready_word = ready
        if ready != ADAC_MAGIC_READY:
            raise RuntimeError(
                "unexpected ADAC readiness word 0x%08X (want 0x%08X)" % (ready, ADAC_MAGIC_READY)
            )

        # 3. Request entry for the UROT SDA (host -> UROT).
        self._tx_word(ADAC_MAGIC_ENTER)

        # 4. Confirm the handshake acknowledge (UROT -> host).
        ack = self._rx_word()
        if ack != ADAC_MAGIC_ACK:
            raise RuntimeError(
                "device did not acknowledge ADAC entry "
                "(got 0x%08X, want 0x%08X)" % (ack, ADAC_MAGIC_ACK)
            )

    def command(self, cmd, payload=b""):
        """Send a request and return (status, data_bytes)."""
        if len(payload) % 4 != 0:
            payload += b"\x00" * (4 - len(payload) % 4)
        self._tx_word(cmd << 16)  # word0: _reserved(0) | command<<16
        self._tx_word(len(payload))  # word1: data_count (bytes)
        for w in _words(payload):
            self._tx_word(w)

        status = (self._rx_word() >> 16) & 0xFFFF
        data_count = self._rx_word()
        data = b"".join(struct.pack("<I", self._rx_word()) for _ in range(data_count // 4))
        return status, data[:data_count]

    # --- high-level commands ---
    def discovery(self):
        status, data = self.command(CMD_DISCOVERY)
        return status, parse_tlv_seq(data)

    def auth_start(self):
        status, data = self.command(CMD_AUTH_START)
        # adac_auth_challenge_v1_0: version(2) + reserved(2) + challenge(32).
        return status, data[4:36]

    def auth_response(self, fmt, fragment):
        payload = struct.pack("<HH", fmt, 0) + fragment
        return self.command(CMD_AUTH_RESPONSE, payload)

    def lcs_change(self, psa_value):
        # Request data = a TLV sequence containing psa_lifecycle.
        value = struct.pack("<H", psa_value)
        tlv = struct.pack("<BBHI", 0, 0, TLV_PSA_LIFECYCLE, len(value)) + value
        tlv += b"\x00" * ((-len(tlv)) % 4)
        return self.command(CMD_LCS_CHANGE, tlv)

    def erase_all(self, key):
        return self.command(CMD_ERASE_ALL, struct.pack("<I", key))

    def lock_debug(self):
        return self.command(CMD_LOCK_DEBUG)

    def bootmode_set(self, word):
        # Keep the raw word path for reserved-bit validation tests.
        return self.command(CMD_BOOTMODE_SET, struct.pack("<I", word))

    def boot_report_get(self, payload=b""):
        # A non-empty payload is only for rejection tests.
        return self.command(CMD_BOOT_REPORT_GET, payload)


# --- ECDSA P-384 cert/token builders (wire layout per UROT's own adac header) ---
CURVE = ecdsa.P384
_EXT_HASH = b"\x00" * 48  # extensions_bytes == 0 -> all-zero SHA-384 hash, no ext data


def build_certificate(
    signer_priv,
    subject_pub,
    perm_mask,
    role=ROLE_LEAF,
    lifecycle=0,
    soc_class=0,
    soc_id=b"\x00" * 16,
):
    """Certificate (v1.1) for @subject_pub, signed by @signer_priv over header||key||hash."""
    header = struct.pack(
        "<BB BBBB HHH I I 16s 16s".replace(" ", ""),
        1,
        1,  # format_version 1.1
        CRYPTO_ECDSA_P384_SHA384,  # signature_type
        CRYPTO_ECDSA_P384_SHA384,  # key_type
        role,
        USAGE_STANDARD,  # role, usage
        0,
        lifecycle,
        0,  # policies_mask, lifecycle, oem_constraint
        0,  # extensions_bytes
        soc_class,  # soc_class
        soc_id,  # soc_id[16]
        perm_mask,  # permissions_mask[16]
    )
    assert len(header) == 52, len(header)
    signed = header + subject_pub + _EXT_HASH
    return signed + CURVE.sign(signer_priv, signed)  # 52 + 96 + 48 + 96 = 292


def build_root_certificate(authop_priv, perm_mask):
    """The self-signed root certificate that carries the AUTHOP key.

    The device holds only SHA-256 of each AUTHOP key generation, so the key reaches
    it here and must hash to one the device holds.
    """
    return build_certificate(authop_priv, raw_pub(authop_priv), perm_mask, role=ROLE_ROOT)


def build_token(leaf_priv, challenge, requested):
    """Debug token (v1.0), signed by the leaf key over header||hash||challenge."""
    header = struct.pack("<BBBBI16s", 1, 0, CRYPTO_ECDSA_P384_SHA384, 0, 0, requested)
    assert len(header) == 24, len(header)
    signed = header + _EXT_HASH + challenge
    return header + _EXT_HASH + CURVE.sign(leaf_priv, signed)  # 24 + 48 + 96 = 168


def raw_pub(priv):
    """The certificate form of a P-384 key: X || Y, without the 0x04 prefix."""
    return ecdsa.public_bytes(priv)[1:]


def new_leaf_key():
    """A fresh P-384 key for one authentication."""
    return CURVE.load_private_key(CURVE.generate_private_pem())


def authenticate(sess, authop_priv, perm_bits=None):
    """Run authentication and return its status without printing."""
    mask = perm_vector(perm_bits, all_ones=perm_bits is None)
    info = {}

    # Firmware that receives a certificate larger than its packet buffer drops
    # the request without a response, so refuse before sending one.
    status, tlvs = sess.discovery()
    offered = tlvs.get(TLV_CRYPTOSYSTEMS, b"")
    if status == ST_SUCCESS and CRYPTO_ECDSA_P384_SHA384 not in offered:
        sys.exit(
            "adac: the device offers ADAC cryptosystem %s, not 0x%02X (ECDSA P-384). "
            "Use the adac.py of the release whose firmware the device runs."
            % (", ".join("0x%02X" % c for c in offered) or "none", CRYPTO_ECDSA_P384_SHA384)
        )

    status, challenge = sess.auth_start()
    info["challenge"] = challenge.hex()
    if status != ST_SUCCESS:
        info.update(stage="auth_start", status_code=status, status=STATUS.get(status, hex(status)))
        return False, info

    leaf = new_leaf_key()
    for stage, cert in (
        ("root", build_root_certificate(authop_priv, mask)),
        ("cert", build_certificate(authop_priv, raw_pub(leaf), mask)),
    ):
        status, _ = sess.auth_response(FMT_CERT_ADAC, cert)
        if status != ST_NEED_MORE_DATA:
            info.update(stage=stage, status_code=status, status=STATUS.get(status, hex(status)))
            return False, info

    token = build_token(leaf, challenge, mask)
    status, data = sess.auth_response(FMT_TOKEN_ADAC, token)
    if status != ST_SUCCESS:
        info.update(stage="token", status_code=status, status=STATUS.get(status, hex(status)))
        return False, info

    eff = parse_tlv_seq(data).get(TLV_EFFECTIVE_PERMISSIONS, b"")
    info.update(stage="done", status_code=status, status="SUCCESS", effective_permissions=eff.hex())
    return True, info


def _emit(args, result, *human_lines):
    """Print either the JSON result (machine mode) or the human lines."""
    if getattr(args, "json", False):
        print(json.dumps(result))
    else:
        for line in human_lines:
            print(line)


def _auth_fail_line(info):
    return "auth failed at %s -> %s" % (info.get("stage"), info.get("status"))


# --- CLI ---
def open_jlink(args):
    # JLINK_DLL overrides pylink's library search for CI and custom installs.
    dll = os.environ.get("JLINK_DLL")
    lib = pylink.Library(dllpath=dll) if dll else None
    jlink = pylink.JLink(lib=lib)
    jlink.open(serial_no=args.serial)
    jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
    jlink.set_speed(args.speed)
    jlink.coresight_configure()
    power_up(jlink)
    return jlink


def load_authop(args):
    """Load the AUTHOP private key from a PEM or keyset."""
    gen = getattr(args, "authop_gen", 0)
    if args.authop_key is not None:
        pem = Path(args.authop_key).read_bytes()
    else:
        sys.path.insert(0, str(Path(__file__).resolve().parent))
        import iron_keychain as kc

        key_id = kc.key_id_for("authopkey", gen)
        try:
            source = kc.KeySource.parse(args.keyset or kc.sole_keyset_spec())
            pem = source.get_bytes(key_id)
        except kc.KeySourceError as exc:
            sys.exit(f"adac: {exc}")
        if pem is None:
            sys.exit(
                f"adac: no {key_id} in {source.describe()}. Use the keyset "
                "that provisioned this device."
            )
    try:
        return CURVE.load_private_key(pem, label="AUTHOP key")
    except (ValueError, TypeError) as exc:
        sys.exit(f"adac: {exc}")


def cmd_discovery(sess, args):
    status, tlvs = sess.discovery()
    # Keep raw TLVs in JSON and decode lifecycle for convenience.
    result = {
        "command": "discovery",
        "status": STATUS.get(status, hex(status)),
        "status_code": status,
        "tlvs": {"0x%04X" % t: v.hex() for t, v in tlvs.items()},
    }
    if TLV_PSA_LIFECYCLE in tlvs:
        result["psa_lifecycle"] = struct.unpack("<H", tlvs[TLV_PSA_LIFECYCLE])[0]

    human = ["discovery -> %s" % result["status"]]
    for tid, val in sorted(tlvs.items()):
        if tid == TLV_PSA_LIFECYCLE:
            lc = struct.unpack("<H", val)[0]
            human.append("  psa_lifecycle: 0x%04X (%s)" % (lc, PSA_LIFECYCLE_NAME.get(lc, "?")))
        else:
            human.append("  0x%04X: %s" % (tid, val.hex()))
    _emit(args, result, *human)
    return 0 if status == ST_SUCCESS else 1


def cmd_auth(sess, args):
    ok, info = authenticate(sess, load_authop(args), perm_bits=args.perm_bits)
    result = {"command": "auth", "ok": ok, **info}
    if ok:
        human = [
            "challenge: %s" % info["challenge"],
            "authenticated, effective_permissions: %s" % info["effective_permissions"],
        ]
    else:
        human = ["challenge: %s" % info["challenge"], _auth_fail_line(info)]
    _emit(args, result, *human)
    return 0 if ok else 1


def cmd_lcs_change(sess, args):
    result = {"command": "lcs-change", "state": args.state}

    bits = args.perm_bits if args.perm_bits is not None else list(PERM_BITS_LCS_CHANGE)
    ok, info = authenticate(sess, load_authop(args), perm_bits=bits)
    result["auth"] = info
    if not ok:
        _emit(args, result, _auth_fail_line(info))
        return 1

    status, data = sess.lcs_change(PSA_LIFECYCLE[args.state])
    result["status"] = STATUS.get(status, hex(status))
    result["status_code"] = status
    human = ["lcs-change %s -> %s" % (args.state, result["status"])]
    if status == ST_SUCCESS and data:
        tlvs = parse_tlv_seq(data)
        if TLV_PSA_LIFECYCLE in tlvs:
            lc = struct.unpack("<H", tlvs[TLV_PSA_LIFECYCLE])[0]
            result["psa_lifecycle"] = lc
            human.append("  new lifecycle: 0x%04X (%s)" % (lc, PSA_LIFECYCLE_NAME.get(lc, "?")))
    _emit(args, result, *human)
    return 0 if status == ST_SUCCESS else 1


def cmd_erase_all(sess, args):
    # Report the effective debugger key, including its default.
    debugger_key = args.debugger_key if args.debugger_key is not None else args.key
    result = {
        "command": "erase-all",
        "key": args.key,
        "debugger_key": debugger_key,
        "executed": False,
    }

    # Authentication grants the erase permission unless test options override it.
    if args.skip_auth:
        result["authenticated"] = False
    else:
        bits = args.perm_bits if args.perm_bits is not None else [PERM_BIT_ERASE_ALL]
        ok, info = authenticate(sess, load_authop(args), perm_bits=bits)
        result["auth"] = info
        if not ok:
            _emit(args, result, _auth_fail_line(info))
            return 1

    # UROT arms ERASEPROTECT.DISABLE. Erase starts only after the debugger writes
    # the same key.
    status, _ = sess.erase_all(args.key)
    result.update(status=STATUS.get(status, hex(status)), status_code=status)
    if status != ST_SUCCESS:
        _emit(args, result, "erase-all (key 0x%08X) -> %s" % (args.key, result["status"]))
        return 1

    # A matching debugger key erases flash, SRAM, and UICR.
    ap_write(sess.jlink, CTRL_AP_IDX, CTRL_AP_ERASEPROTECT_DISABLE, debugger_key)

    # Wait through Ready and Busy for ReadyToReset or Error.
    names = {0: "Ready", 1: "ReadyToReset", 2: "Busy", 3: "Error"}
    st = 0
    for _ in range(50):
        st = ap_read(sess.jlink, CTRL_AP_IDX, CTRL_AP_ERASEALLSTATUS)
        if st in (1, 3):  # terminal: ReadyToReset or Error
            break
        time.sleep(0.1)
    result.update(executed=True, eraseall_status=st, eraseall_status_name=names.get(st, "?"))

    if st != 1:  # only ReadyToReset is real success; Ready/Busy/Error are all failures
        # The CTRL-AP erase failure path requires a pin reset before retrying.
        ap_write(sess.jlink, CTRL_AP_IDX, CTRL_AP_RESET, 0x4)  # PinReset
        # A pin reset drops SWD. Reconfigure before the next AP access.
        try:
            ap_write(sess.jlink, CTRL_AP_IDX, CTRL_AP_RESET, 0x0)  # NoReset (release)
        except pylink.errors.JLinkException:
            time.sleep(0.3)
            sess.jlink.coresight_configure()
            power_up(sess.jlink)
            ap_write(sess.jlink, CTRL_AP_IDX, CTRL_AP_RESET, 0x0)
        time.sleep(0.3)
        sess.jlink.coresight_configure()
        power_up(sess.jlink)
        _emit(
            args,
            result,
            "wrote key 0x%08X to CTRL-AP ERASEPROTECT.DISABLE (0x%02X)"
            % (debugger_key, CTRL_AP_ERASEPROTECT_DISABLE),
            "  ERASEALLSTATUS = %d (%s)" % (st, names.get(st, "?")),
            "erase-all failed (expected ReadyToReset, got %s)" % names.get(st, "?"),
            "issued CTRL-AP pin reset",
        )
        return 1
    # ReadyToReset -> finalize
    ap_write(sess.jlink, CTRL_AP_IDX, CTRL_AP_RESET, 0x2)  # HardReset
    ap_write(sess.jlink, CTRL_AP_IDX, CTRL_AP_RESET, 0x0)  # NoReset (release)
    _emit(
        args,
        result,
        "wrote key 0x%08X to CTRL-AP ERASEPROTECT.DISABLE (0x%02X)"
        % (args.key, CTRL_AP_ERASEPROTECT_DISABLE),
        "  ERASEALLSTATUS = %d (%s)" % (st, names.get(st, "?")),
        "erase-all complete, device returned to the erased state",
    )
    return 0


def cmd_recover(sess, args):
    """Unauthenticated CTRL-AP ERASEALL: erase an unprotected device to Empty.

    The debugger-side equivalent of `nrfutil device recover`: it drives the
    CTRL-AP ERASEALL register directly, needs no AUTHOP key and no resident UROT,
    and works with the application access port closed (CTRL-AP stays reachable).
    It erases only a device whose ERASEPROTECT is not engaged; a locked device
    needs the authenticated erase-all or the IROT backdoor.
    """
    j = sess.jlink
    protect = ap_read(j, CTRL_AP_IDX, CTRL_AP_ERASEPROTECT_STATUS)
    result = {
        "command": "recover",
        "eraseprotect": protect,
        "protected": bool(protect & 1),
        "executed": False,
    }

    # Trigger the unconditional erase (flash + UICR + SRAM).
    ap_write(j, CTRL_AP_IDX, CTRL_AP_ERASEALL, 1)

    # Wait through Ready and Busy for a terminal ReadyToReset or Error.
    st = 0
    for _ in range(50):
        st = ap_read(j, CTRL_AP_IDX, CTRL_AP_ERASEALLSTATUS)
        if st in (1, 3):  # terminal: ReadyToReset or Error
            break
        time.sleep(0.1)
    result.update(
        executed=True, eraseall_status=st, eraseall_status_name=ERASEALLSTATUS_NAMES.get(st, "?")
    )

    lines = [
        "ERASEPROTECT.STATUS = 0x%08X (PALL=%d)" % (protect, protect & 1),
        "  ERASEALLSTATUS = %d (%s)" % (st, ERASEALLSTATUS_NAMES.get(st, "?")),
    ]

    if st != 1:  # only ReadyToReset means the erase actually ran
        # The CTRL-AP erase failure path requires a pin reset before retrying. A
        # pin reset drops SWD, so reconfigure the link before the next AP access.
        ap_write(j, CTRL_AP_IDX, CTRL_AP_RESET, RESET_PIN)
        try:
            ap_write(j, CTRL_AP_IDX, CTRL_AP_RESET, RESET_NONE)
        except pylink.errors.JLinkException:
            time.sleep(0.3)
            j.coresight_configure()
            power_up(j)
            ap_write(j, CTRL_AP_IDX, CTRL_AP_RESET, RESET_NONE)
        if protect & 1:
            lines.append(
                "recover cannot erase a protected device (ERASEPROTECT engaged); "
                "use erase-all or the IROT backdoor"
            )
        lines.append(
            "recover failed (expected ReadyToReset, got %s)"
            % ERASEALLSTATUS_NAMES.get(st, "?")
        )
        lines.append("issued CTRL-AP pin reset")
        _emit(args, result, *lines)
        return 1

    # ReadyToReset -> commit with a hard reset into a normal boot.
    ap_write(j, CTRL_AP_IDX, CTRL_AP_RESET, RESET_HARD)
    ap_write(j, CTRL_AP_IDX, CTRL_AP_RESET, RESET_NONE)
    lines.append("recover complete, device erased to Empty")
    _emit(args, result, *lines)
    return 0


def cmd_lock_debug(sess, args):
    status, _ = sess.lock_debug()
    result = {
        "command": "lock-debug",
        "status": STATUS.get(status, hex(status)),
        "status_code": status,
    }
    _emit(args, result, "lock-debug -> %s" % result["status"])
    return 0 if status == ST_SUCCESS else 1


def cmd_bootmode(sess, args):
    """Set the boot mode used after leaving ADAC."""
    word = args.raw if args.raw is not None else BOOTMODE[args.mode]
    status, _ = sess.bootmode_set(word)
    result = {
        "command": "boot-mode",
        "mode": args.mode,
        "word": word,
        "status": STATUS.get(status, hex(status)),
        "status_code": status,
    }
    _emit(
        args,
        result,
        "boot-mode %s (word 0x%08X) -> %s" % (args.mode or "raw", word, result["status"]),
    )
    return 0 if status == ST_SUCCESS else 1


def cmd_boot_report(sess, args):
    """Authenticate, then read the report of the last boot that completed its boot decision."""
    result = {"command": "boot-report"}
    if args.skip_auth:
        result["authenticated"] = False
    else:
        bits = args.perm_bits if args.perm_bits is not None else list(PERM_BITS_BOOT_REPORT)
        ok, info = authenticate(sess, load_authop(args), perm_bits=bits)
        result["auth"] = info
        if not ok:
            _emit(args, result, _auth_fail_line(info))
            return 1

    status, data = sess.boot_report_get(args.raw_payload)
    result.update(status=STATUS.get(status, hex(status)), status_code=status)
    if status != ST_SUCCESS:
        hint = " (no boot has stored a report since it last halted or lost power)"
        _emit(
            args,
            result,
            f"boot-report -> {result['status']}{hint if status == ST_FAILURE else ''}",
        )
        return 1
    result["raw"] = data.hex()
    try:
        report = boot_report.parse(data)
    except ValueError as exc:
        result["error"] = str(exc)
        _emit(args, result, f"boot-report: {exc}")
        return 1
    result["report"] = boot_report.to_dict(report)
    _emit(args, result, boot_report.format_human(result["report"]))
    return 0


def cmd_enter(sess, args):
    """Report CTRL-AP identity and mailbox registers after ADAC entry."""
    j = sess.jlink
    idr = ap_read(j, CTRL_AP_IDX, CTRL_AP_IDR)
    regs = {
        name: ap_read(j, CTRL_AP_IDX, off)
        for name, off in (
            ("TXDATA", CTRL_AP_TXDATA),
            ("TXSTATUS", CTRL_AP_TXSTATUS),
            ("RXDATA", CTRL_AP_RXDATA),
            ("RXSTATUS", CTRL_AP_RXSTATUS),
        )
    }
    result = {
        "command": "enter",
        "ready_word": sess.ready_word,
        "mode": "ADAC",  # enter() raises unless the handshake completed
        "idr": idr,
        "mailbox": regs,
    }
    _emit(
        args,
        result,
        "entered ADAC mode (SDA 0x%02X, readiness 0x%08X)" % (ADAC_SDA_ID_UROT, sess.ready_word),
        "CTRL-AP[%d] IDR = 0x%08X" % (CTRL_AP_IDX, idr),
        "CTRL-AP mailbox registers:",
        *["  %-8s = 0x%08X" % (n, v) for n, v in regs.items()],
    )
    return 0


def leave_adac(sess, via=EXIT_VIA_DEFAULT):
    """Reset out of ADAC mode into a normal boot."""
    reset_val = RESET_PIN if via == "pin" else RESET_SOFT
    ap_write(sess.jlink, CTRL_AP_IDX, CTRL_AP_RESET, reset_val)  # reset (level)
    if reset_val == RESET_PIN:
        # Pin reset drops SWD, so the release write may need reconfiguration.
        try:
            ap_write(sess.jlink, CTRL_AP_IDX, CTRL_AP_RESET, RESET_NONE)  # release -> boot
        except pylink.errors.JLinkException:
            time.sleep(0.3)
            sess.jlink.coresight_configure()
            power_up(sess.jlink)
            ap_write(sess.jlink, CTRL_AP_IDX, CTRL_AP_RESET, RESET_NONE)
    else:
        ap_write(sess.jlink, CTRL_AP_IDX, CTRL_AP_RESET, RESET_NONE)  # release -> boot


def cmd_exit(sess, args):
    """Leave ADAC through a pin or soft reset."""
    if args.via == "soft":
        print("warning: a CTRL-AP soft reset may not fully reset the device "
              "(nRF54L errata [63]); prefer --via pin", file=sys.stderr)
    leave_adac(sess, args.via)
    result = {"command": "exit", "via": args.via}
    _emit(args, result, "exited ADAC mode (%s reset -> normal boot)" % args.via)
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    connection = ap.add_argument_group("device connection")
    connection.add_argument(
        "--serial-number", dest="serial", type=int, default=None, help="J-Link serial number"
    )
    # Older spellings, kept so existing scripts keep working.
    connection.add_argument("-s", "--serial", dest="serial", type=int, help=argparse.SUPPRESS)
    connection.add_argument(
        "--speed", type=int, default=4000, help="SWD speed in kHz (default: %(default)s)"
    )
    connection.add_argument(
        "--wait-ms",
        type=int,
        default=500,
        help="delay after ADAC entry reset (default: %(default)s)",
    )
    connection.add_argument(
        "--ready-timeout-ms",
        type=int,
        default=None,
        help=f"ADAC readiness timeout (default: at least {MB_TIMEOUT_S:g} seconds)",
    )
    connection.add_argument(
        "--command-only",
        action="store_true",
        help="run without entry or exit resets (requires ADAC mode)",
    )
    output = ap.add_argument_group("output")
    output.add_argument("--json", action="store_true", help="print command results as JSON")
    sub = ap.add_subparsers(dest="cmd", required=True, title="commands", metavar="COMMAND")

    def add_authop_key(parser):
        parser.add_argument(
            "--keyset",
            dest="keyset",
            metavar="SPEC",
            help="AUTHOP key source as keyring:NAME or dir:PATH "
            "(default: only keyset in the OS keyring)",
        )
        parser.add_argument(
            "--authop-gen",
            type=int,
            default=0,
            choices=(0, 1, 2),
            help="AUTHOP key generation (default: 0)",
        )
        parser.add_argument(
            "--authop-key", metavar="PEM", help="AUTHOP ECDSA P-384 private key PEM instead of --keyset"
        )

    def add_perm_bits(parser):
        parser.add_argument(
            "--perm-bits", type=lambda s: [int(x, 0) for x in s.split(",")], help=argparse.SUPPRESS
        )

    sub.add_parser(
        "enter",
        help="enter ADAC mode and show mailbox registers without exiting",
    )
    p_exit = sub.add_parser("exit", help="leave ADAC mode and boot normally")
    p_exit.add_argument(
        "--via",
        choices=("soft", "pin"),
        default="pin",
        help="reset kind used to leave ADAC mode (default: pin reset). A soft reset is "
        "exposed to nRF54L errata [63] and may leave part of the device un-reset; use it "
        "only to exercise that reset path",
    )
    sub.add_parser("discovery", help="query target properties")
    sub.add_parser("lock-debug", help="remove a non-secure debug grant")
    sub.add_parser(
        "recover",
        help="unauthenticated CTRL-AP ERASEALL: erase an unprotected device to "
        "Empty (no key, no resident UROT; works with the app AP closed)",
    )
    p_auth = sub.add_parser("auth", help="authenticate with the device")
    add_authop_key(p_auth)
    add_perm_bits(p_auth)
    p_lcs = sub.add_parser(
        "lcs-change",
        help="authenticate and move to Decommissioned, destroying device assets",
    )
    p_lcs.add_argument("state", choices=(LCS_CHANGE_TARGET,))
    add_authop_key(p_lcs)
    add_perm_bits(p_lcs)
    p_boot = sub.add_parser(
        "boot-mode",
        help="set the next boot mode (automatic exit reset unless --command-only)",
    )
    p_boot.add_argument(
        "mode",
        nargs="?",
        choices=sorted(BOOTMODE),
        help="app = boot the application, dfu = boot IronSide DFU",
    )
    p_boot.add_argument("--raw", type=lambda x: int(x, 0), default=None, help=argparse.SUPPRESS)
    p_report = sub.add_parser(
        "boot-report",
        help="authenticate and read the report of the last completed boot (--json for every field)",
    )
    p_report.add_argument("--raw-payload", type=bytes.fromhex, default=b"", help=argparse.SUPPRESS)
    p_report.add_argument("--skip-auth", action="store_true", help=argparse.SUPPRESS)
    add_authop_key(p_report)
    add_perm_bits(p_report)
    p_erase = sub.add_parser("erase-all", help="authenticate and erase all device data")
    p_erase.add_argument(
        "--key",
        type=lambda x: int(x, 0),
        default=0xDEADBEEF,
        help="nonzero 32-bit erase key written by UROT and CTRL-AP",
    )
    p_erase.add_argument(
        "--debugger-key", type=lambda x: int(x, 0), default=None, help=argparse.SUPPRESS
    )
    p_erase.add_argument("--skip-auth", action="store_true", help=argparse.SUPPRESS)
    add_authop_key(p_erase)
    add_perm_bits(p_erase)

    args = ap.parse_args()
    if args.cmd == "boot-mode" and args.mode is None and args.raw is None:
        ap.error("boot-mode requires MODE")
    if args.cmd == "boot-mode" and args.mode is not None and args.raw is not None:
        ap.error("boot-mode MODE cannot be combined with --raw")
    handlers = {
        "enter": cmd_enter,
        "exit": cmd_exit,
        "discovery": cmd_discovery,
        "lock-debug": cmd_lock_debug,
        "auth": cmd_auth,
        "lcs-change": cmd_lcs_change,
        "erase-all": cmd_erase_all,
        "recover": cmd_recover,
        "boot-mode": cmd_bootmode,
        "boot-report": cmd_boot_report,
    }

    # recover drives CTRL-AP registers directly - no ADAC entry handshake, and it
    # owns its own commit reset, so it neither enters nor leaves ADAC mode.
    automatic = (
        args.cmd not in SESSION_COMMANDS
        and args.cmd not in CTRLAP_ONLY
        and not args.command_only
    )

    jlink = open_jlink(args)
    try:
        sess = AdacSession(jlink)
        if automatic or args.cmd == "enter":
            sess.enter(args.wait_ms, args.ready_timeout_ms)
        try:
            return handlers[args.cmd](sess, args)
        finally:
            # Preserve the command result if the exit reset fails.
            if automatic:
                try:
                    leave_adac(sess)
                except pylink.errors.JLinkException as exc:
                    print("adac: could not leave ADAC mode: %s" % exc, file=sys.stderr)
                    print("Reset the device to boot its firmware.", file=sys.stderr)
    finally:
        jlink.close()


if __name__ == "__main__":
    sys.exit(main())
