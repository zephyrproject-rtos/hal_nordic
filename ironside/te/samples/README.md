<!-- SPDX-License-Identifier: LicenseRef-Nordic-5-Clause -->

# IronSide Integration Kit samples

The IronSide Integration Kit samples are bare-metal, non-secure application firmware samples for IronSide TE.
They show you how to use IronSide TE from a plain nrfx application that does not require Zephyr.
Each sample runs after the updatable root of trust (UROT) completes its TrustZone setup and hands over control.
Each sample also checks its own results, so that it can run in the test suite.

## Sample directory contents

| Path | Contents |
| --- | --- |
| `<name>/` | One subdirectory per sample (`main.c`, optional `test.toml` and `sample.cmake`) |
| `lib/` | Linker scripts, nrfx glue, UART helper, fault handler, test markers |
| `cmake/` | Bare-metal Arm GCC toolchain file and the IronSide TE Client glue (`ironside_client_add()`) |
| `deps/` | nrfx and CMSIS-Core checkouts that you fetch yourself, as described in [Sample build prerequisites](PREREQUISITES.md) |
| `CMakeLists.txt` | Plain-CMake build recipe (selected with `-DSAMPLE=<name>`) |
| [PREREQUISITES.md](PREREQUISITES.md) | Toolchain, nrfx, and CMSIS setup for building from source |

## Building and running

Before you run a sample, flash the secure root of trust, which consists of the immutable root of trust (IROT) and UROT, and provision the device.
For the end-to-end procedure, see [IronSide Integration Kit quick start](../doc/quick-start.md).
You can run the prebuilt `hello_world` sample without building it, or build any sample from source.

`main()` must not return.

### Running the prebuilt `hello_world` sample

You do not need a sample build environment to run the prebuilt `hello_world` sample.
The IronSide Integration Kit root directory contains a prebuilt HEX file for each DK.

The following commands use the image for the nRF54LM20 DK, `hello_world_app_nRF54LM20-DK.hex`.
For the nRF54LV10 DK or the nRF54L15 DK, use `hello_world_app_nRF54LV10-DK.hex` with `--soc nrf54lv10a`, or `hello_world_app_nRF54L15-DK.hex` with `--soc nrf54l15`.

To run the prebuilt `hello_world` sample, complete the following steps:

1. Sign the prebuilt image with your keyset:

   ```sh
   python scripts/sign.py hello_world_app_nRF54LM20-DK.hex -o hello_world_app.signed.hex --keyset keyring:my-keyset --soc nrf54lm20b
   ```

2. Flash the signed image:

   ```sh
   python scripts/flash.py hello_world_app.signed.hex
   ```

3. View the UART output as described in [Viewing the UART output](#viewing-the-uart-output).

For details, see [Installing application firmware](../doc/installing-and-updating-firmware.md#installing-application-firmware) in the user guide.

### Building a sample from source

To build and run a sample from source, complete the following steps:

1. Install the [sample build prerequisites](PREREQUISITES.md).

2. Build the sample, where `<name>` is the directory name of the sample, `<soc>` is the SoC of your DK, and `<bundle>` is the IronSide bundle that the DK is provisioned from:

   ```sh
   python scripts/build.py <name> --soc <soc> --bundle <bundle>
   ```

   The sample is built for the configuration of the bundle.
   For details, see [Building application firmware for a bundle](../doc/installing-and-updating-firmware.md#building-application-firmware-for-a-bundle).

3. Sign the built image with your keyset, using the same `<soc>`:

   ```sh
   python scripts/sign.py samples/build/<name>/<name>.hex \
       -o samples/build/<name>/<name>.signed.hex --keyset keyring:my-keyset --soc <soc>
   ```

4. Flash the signed image:

   ```sh
   python scripts/flash.py samples/build/<name>/<name>.signed.hex
   ```

5. View the UART output as described in [Viewing the UART output](#viewing-the-uart-output).

Each of the host scripts takes `--help`.

### Viewing the UART output

The samples print their output to the UART.
You can use any serial terminal emulator to see the output, for example the [Serial Terminal app](https://docs.nordicsemi.com/bundle/swtools_docs/page/app/pc-nrfconnect-serial-terminal/index.html) in [nRF Connect for Desktop](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Connect-for-desktop).
Use the second virtual serial port (port 1) of the DK with the following serial port settings:

| Baud rate | Data bits | Stop bits | Parity | Hardware flow control |
|-----------|-----------|-----------|--------|-----------------------|
| 115200    | 8         | 1         | None   | None                  |

For more information, see [Identifying serial ports](https://docs.nordicsemi.com/r/bundle/nrf-connect-for-desktop/page/app/pc-nrfconnect-serial-terminal/selecting_serial_port.html/identifying-serial-ports).

## Samples

| Sample | Description |
| --- | --- |
| `hello_world` | The bare-metal build, flash, and UART flow, followed by blinking the DK LEDs, without IronSide TE services |
| `app_version` | Reading the image version and the IROT-to-UROT boot report, including the fault report |
| `dfu_enter` | Requesting a boot into IronSide DFU on the next boot, and then resetting into it |
| `platform_cpu_freq` | Reading the CPU frequency from a register of OSCILLATORS, a peripheral that IronSide TE keeps secure, and the result of a refused request |
| `vpr_launch` | Running code on the VPR (FLPR) core and handing it a pin with a secure register write to `GPIO.PIN_CNF[n].CTRLSEL`, and how the pin behaves without that write |
| `psa_sha256` | Hashing through the Platform Security Architecture (PSA) Crypto service of UROT, the canonical PSA demo, and stopping on a recorded fault |
| `psa_hash_sha2_sha3` | The wider SHA-2 and SHA-3 digests |
| `psa_sha1` | SHA-1 as a plain hash, and why HMAC and ECDSA over it are refused |
| `psa_random` | Random bytes from the CRACEN deterministic random bit generator |
| `psa_aead` | AES-256-GCM authenticated encryption |
| `psa_aead_chachapoly` | ChaCha20-Poly1305 and AES-256-CCM |
| `psa_aead_ccm` | Multi-part (chunked) AES-256-CCM with a generated key and nonce |
| `psa_cipher_cbc` | AES-256-CBC |
| `psa_ecdh` | Elliptic Curve Diffie-Hellman (ECDH) key agreement with a raw shared secret |
| `psa_ecdh_kdf` | ECDH chained onto a key derivation function, which derives a session key without the shared secret leaving UROT |
| `psa_hkdf` | HKDF-SHA-256 in one step and as extract then expand, against RFC 5869 |
| `psa_pbkdf2` | PBKDF2 from a password key, with HMAC-SHA-256 and with AES-CMAC-PRF-128 |
| `psa_pake_jpake` | EC-JPAKE between two parties, both run in the one App |
| `psa_pake_spake2p` | SPAKE2+ (the Matter variant) between a prover and a verifier, from a passcode |
| `psa_copy_key` | Copying a key with narrower usage, or into a persistent key |
| `psa_x25519` | X25519 key agreement on Curve25519, against the RFC 7748 known-answer vector |
| `psa_import_ecc`, `psa_import_hmac` | Importing an elliptic curve cryptography (ECC) key or a hash-based message authentication code (HMAC) key, and using it |
| `psa_ecdsa_p384` | ECDSA on secp384r1 |
| `psa_ecdsa_deterministic` | Deterministic ECDSA (RFC 6979) on secp256r1, against the RFC 6979 known-answer signature |
| `psa_ed25519` | Ed25519 (PureEdDSA) signing, against the RFC 8032 known-answer vector |
| `psa_persistent_key` | A persistent key, used by ID across resets |
| `psa_data_at_rest_key` | A persistent key for data at rest, with an AES-GCM tamper check |
| `psa_key_rotation` | Rotating a persistent key in place by destroying it and regenerating it at the same ID |
| `psa_fw_decrypt` | A persistent key-encryption key used for firmware decryption |
| `psa_initial_attestation` | A signed PSA Initial Attestation token (RFC 9783) |
| `psa_mext` | Deriving a per-device key for encryption at rest |

## Writing a sample

To add a sample, create a `samples/<name>/` directory that contains a `main.c` file.
Every sample is built as a non-secure application image that is linked at the application slot.
The console uses UARTE20, which is virtual serial port 1 of the DK, at 115200 8N1.

To make a sample a test, complete the following steps:

1. Add a `test.toml` file with the following content to the sample directory:

   ```toml
   description = "one line describing what this verifies"
   timeout = 5
   ```

2. Print the following markers from `lib/integration_kit_test.h` on the console:

   - `INTEGRATION_KIT_TEST_SUCCESS <name>` when all checks pass
   - `INTEGRATION_KIT_TEST_FINISHED <name>` as the last line

   In both markers, `<name>` must match the directory name of the sample.
   This prevents a stale image from being counted as a pass.
   It also prevents leftover console output from that image from cutting short the capture of the next test.
