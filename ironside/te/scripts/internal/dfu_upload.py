#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Upload an encrypted image to IronSide DFU over SMP serial.

Uses the stock MCUmgr image-management ABI (SMP group 1) through
`nrfutil mcu-manager serial image-upload`. The artifact on disk is an MCUboot
encrypted image from `dfu_encrypt.py` (`.ota.bin` suffix).

Before the upload, the wrapped body key from the image's ENC_KW TLV is sent with
the IronSide ENC_KEY command (SMP group 0x17E0). IronSide DFU unwraps it and decrypts
each chunk as it arrives, and refuses an upload that no ENC_KEY command preceded.

Image selectors (the `image` field on the wire):

    candidate -> image 0 (staging slot; the root of trust routes by image id)
    app       -> image 1 (in-place write to the App partition)

UROT and DFU updates use the candidate destination. The root of trust reads
IMAGE_TLV_ROT_IMAGE_ID to route a staged image to UROT, App, or DFU. Image 0 is
what a client sends when it names no image, so such a client stages a candidate.

The upload uses the SMP buffer size that the device reports as the MTU. A
device that does not report it gets nrfutil's default MTU.
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import mcuboot_encrypt
from lib.smp_serial import (
    BOOTLOADER_NAME_IRONSIDE,
    DFU_ID_ENC_KEY,
    GROUP_DFU,
    GROUP_OS,
    OS_ID_BOOTLOADER_INFO,
    OS_ID_MCUMGR_PARAMS,
    SmpError,
    SmpSerial,
)

NRFUTIL = os.environ.get("IRONSIDE_INTEGRATION_KIT_NRFUTIL", "nrfutil")

# Must match the IronSide DFU devicetree UART rate.
SMP_BAUD_DEFAULT = 1000000

TARGETS = {"app": 1, "candidate": 0}


def serial_port_arg(port: str, baud: int) -> str:
    return f"port={port},baud_rate={baud}"


# MCUmgr result codes the ENC_KEY command answers with.
ENC_KEY_ERRORS = {
    3: "the request is malformed",
    8: "this IronSide DFU does not accept a body key for this target",
    11: "no provisioned OTA KEK unwraps the body key. Encrypt with a KEK this device holds",
}


def read_enc_kw(path: Path) -> bytes:
    """Return the wrapped body key of encrypted image @path, or exit when there is none."""
    try:
        wrapped = mcuboot_encrypt.enc_kw_of(path.read_bytes())
    except mcuboot_encrypt.EncryptError as exc:
        sys.exit(f"error: {path}: {exc}")
    if wrapped is None:
        sys.exit(f"error: {path} is not encrypted. Run dfu_encrypt.py first.")
    return wrapped


def prepare_device(port: str, baud: int, image_num: int, enc_kw: bytes) -> int | None:
    """Check that the device runs IronSide DFU, send the wrapped body key, and
    return the device's SMP buffer size.

    The buffer size is None when the device does not report it.
    """
    try:
        with SmpSerial.open(port, baud) as smp:
            check_ironside(smp)
            buf_size = read_smp_buf_size(smp)
            send_enc_key(smp, image_num, enc_kw)
    except SmpError as exc:
        sys.exit(f"error: SMP on {port}: {exc}")
    return buf_size


def check_ironside(smp: SmpSerial) -> None:
    rsp = smp.read_map(GROUP_OS, OS_ID_BOOTLOADER_INFO)
    name = rsp.get("bootloader")
    if name != BOOTLOADER_NAME_IRONSIDE:
        found = f"bootloader {name!r}" if name else f"no bootloader name ({rsp!r})"
        sys.exit(f"error: the device is not IronSide DFU: it reports {found}")


def read_smp_buf_size(smp: SmpSerial) -> int | None:
    try:
        rsp = smp.read_map(GROUP_OS, OS_ID_MCUMGR_PARAMS)
    except SmpError as exc:
        print(f"warning: cannot read the SMP buffer size ({exc}); using the default MTU")
        return None
    buf_size = rsp.get("buf_size")
    return buf_size if isinstance(buf_size, int) and buf_size > 0 else None


def send_enc_key(smp: SmpSerial, image_num: int, enc_kw: bytes) -> None:
    rsp = smp.write_map(GROUP_DFU, DFU_ID_ENC_KEY, {"image": image_num, "enc_kw": enc_kw})
    rc = rsp.get("rc", 0)
    if rc:
        reason = ENC_KEY_ERRORS.get(rc, f"MCUmgr result code {rc}")
        sys.exit(f"error: the device refused the body key: {reason}")
    print(f"body key unwrapped with OTA KEK generation {rsp.get('gen')}")


def ensure_mcu_manager() -> None:
    proc = subprocess.run(
        [NRFUTIL, "mcu-manager", "serial", "--help"],
        capture_output=True,
        text=True,
    )
    if proc.returncode != 0:
        sys.exit("error: nrfutil mcu-manager is not installed. Run:\n  nrfutil install mcu-manager")


def main():
    ap = argparse.ArgumentParser(
        description="Upload an encrypted IronSide firmware update over serial."
    )
    ap.add_argument(
        "image",
        help=f"encrypted update from dfu_encrypt.py ({mcuboot_encrypt.OTA_ARTIFACT_SUFFIX})",
    )
    ap.add_argument("--port", required=True, help="serial port, for example /dev/ttyACM0")
    ap.add_argument(
        "--target",
        default="app",
        choices=TARGETS,
        help="upload destination (app writes in place, candidate stages an update)",
    )
    ap.add_argument(
        "--baud",
        type=int,
        default=SMP_BAUD_DEFAULT,
        help=f"serial baud rate (default: {SMP_BAUD_DEFAULT})",
    )
    ap.add_argument("--reset", action="store_true", help="reset the device after upload")
    args = ap.parse_args()

    image = Path(args.image)
    if not image.is_file():
        sys.exit(f"error: no such file: {image}")

    enc_kw = read_enc_kw(image)
    ensure_mcu_manager()

    serial = serial_port_arg(args.port, args.baud)
    image_num = TARGETS[args.target]
    upload = [
        NRFUTIL,
        "mcu-manager",
        "serial",
        "image-upload",
        "--firmware",
        str(image),
        "--serial-port",
        serial,
        "--image-number",
        str(image_num),
    ]
    mtu = prepare_device(args.port, args.baud, image_num, enc_kw)
    if mtu is not None:
        upload += ["--mtu", str(mtu)]

    print(f"uploading {image.name} to {args.target} (image {image_num}, MTU {mtu or 'default'})")
    if subprocess.run(upload).returncode != 0:
        sys.exit("error: the upload failed; nrfutil printed the reason above")
    print("upload complete")

    if args.reset:
        print("resetting device")
        subprocess.run(
            [NRFUTIL, "mcu-manager", "serial", "reset", "--serial-port", serial],
            check=True,
        )


if __name__ == "__main__":
    main()
