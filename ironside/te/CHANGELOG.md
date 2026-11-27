# Changelog

Change history for ironside-integration-kit.

## Prototype compatibility disclaimer

IronSide TE prototype releases (with major version 0, such as 0.4.0) do not have compatibility promises.
You must re-install IronSide TE to use the latest prototype release.

See "Reinstalling IronSide TE" in the user guide for re-installation of IronSide TE.

## 2026.10.1 - 2026-10-22

This IronSide Integration Kit release is the fourth prototype release and is intended to be used with IronSide TE 0.4.0 for all components for all supported SoCs.

### Highlights

Notable new features are:

- ECDSA P-384 keys for the application, IronSide DFU and ADAC, and ECDSA P-384 with ML-DSA-65 for UROT
- Configurable PSA key slot storage size
- IKG SEED check before provisioning, which is the new default
- New name: IronSide Integration Kit, formerly IronSDK
- New nRF54L15 layout, with a 19 KB IROT

### Added

- The application key space report of `ironside_bundle.py show` and `check`, which shows whether the application keys fit in the space a device has for them.
- The `--key-slot-storage-size-bytes` option of `ironside_bundle.py update-config` and the base-profile key `key-slot-storage-size-bytes`, which set the PSA key slot storage size.
- The `ironside_bundle.py export-config --app-header` command, which writes the bundle configuration header that an application is built against.
- The `--ram` option of `read_partitions.py`, which reads the RAM layout a device booted with from the boot report.
- The `ram_ns` field of `struct ironside_boot_report`, which is the non-secure RAM window in effect.

### Updated

- The keyset key types, which are ECDSA P-384 for the application, IronSide DFU and AUTHOP keys, and ECDSA P-384 with ML-DSA-65 for UROT.
- The `sign.py` signatures, which use the new key types and are deterministic.
  `-k` takes an ECDSA P-384 key for the application and IronSide DFU.
- The `adac.py` authentication, which uses ECDSA P-384 (ADAC cryptosystem `0x0A`) and takes an ECDSA P-384 key with `--authop-key`.
- [Breaking change] The default `ikg-seed` provisioning configuration, which is now `check` instead of `nordic`.
  Make sure to set `--ikg-seed` with `ironside_bundle.py update-config` for a bundle that provisions new devices.
- The IKG SEED checks during provisioning, which stop before anything is written to a device that is seeded differently from the bundle configuration.
- The maximum number of application keys in a provisioning batch (now 48, down from 50).
- The product name, which is IronSide Integration Kit instead of IronSDK.
  The release archive is `ironside-integration-kit_<version>.zip`, unpacking to a directory of the same name, and its `manifest.json` names `ironside-integration-kit`.
- The names in the sample library and the build, which follow the new product name.
  The `ironsdk_*()` functions and the `ironsdk_*.h` headers are `integration_kit_*()` and `integration_kit_*.h`, the console markers `IRONSDK_TEST_*` are `INTEGRATION_KIT_TEST_*`, the `IRONSDK_*` environment and CMake variables are `IRONSIDE_INTEGRATION_KIT_*`, and the Zephyr module is `ironside-integration-kit`, so its directory variable is `ZEPHYR_IRONSIDE_INTEGRATION_KIT_MODULE_DIR`.
- The nRF54L15 layout, which places UROT at `0x00005000` with a 120 KB slot.
- The handling of a service call that does not return, which IronSide TE now leaves in flight instead of resetting the device with a watchdog.
  See "Calls that the application keeps from returning" in the user guide.
- [Breaking change] The PSA Crypto wrappers, which are compiled in `client/src/psa_crypto.c` instead of being inline.
  Make sure to add every source in `client/src/` to the application build.
- The `ironside_client/psa/crypto_extra.h` header, which no longer includes `crypto_values.h`.
- [Breaking change] The `ironside_client/layout.h` header, which requires the bundle configuration header and derives the partition addresses, the non-secure RAM start and the PSA key slot count from it.
  Make sure to build the application for the bundle the device is installed from.
- The provisioning firmware start addresses in the factory J-Link scripts and the production programming guide, which are now entry point `0x20012800` and vector table `0x20013000`.
- The `uart.py` port selection, which takes `--serial-number` and no longer asks which port to use.
  It refuses to choose between several probes.

### Removed

- [Breaking change] The `IRONSIDE_RESET_REASON_*` macros.
  Make sure to test the boot report's `reset_reason` field with the MDK `RESET_RESETREAS_*_Msk` masks instead.
- The `--port-by-path` option of `app_update.py` and `ironside_update.py`.
  `--port` takes the port name as given and no longer accepts a glob.

### Fixed

- An issue where `ironside_download.py` and `ironside_bundle.py init` hung when the OS keyring was locked and the session had no display, for example over SSH.
  They now fall back to `~/.netrc` or a browser sign-in.
- An issue where `app_update.py`, `ironside_update.py` and `smp_boot_report.py` could open the console port instead of the DFU SMP port on Windows and time out.
  The scripts now take the ports from the probe's own virtual serial port numbering.
- An issue where `smp_boot_report.py` did not accept `--serial-number`, which its error message suggests when several boards are attached.
- An issue where `smp_boot_report.py` timed out with "no SMP response" on an incomplete or corrupted response.
  It now reports the incomplete or corrupted response.

## 2026.10.0 - 2026-10-05

This IronSDK release is the third prototype release and is intended to be used with IronSide TE 0.3.0 for all components for all supported SoCs.

IronSide TE 0.3.0, and all other prototype releases (with major version 0) do not have compatiblity promises and it is therefore required to re-install IronSide TE.

See the user guide for re-installation of IronSide TE.

Notable new features are:

- ADAC boot report
- Reset service (must be used instead of NVIC_SystemReset)
- Secure services callable from non-secure interrupt context
- Provisioning of application keys with provisioning image

This release includes security fixes.

### Added

- `adac.py boot-report` authenticates over ADAC and prints the boot report of the last completed boot, using the new ADAC vendor command `BOOT_REPORT_GET`.
- `add_application_key.py` stores one of an application's own KMU keys in a key set.
  `ironside_bundle.py update-keys` provisions whatever application keys the key set holds.
- `ironside_bundle.py update-keys` provisions an application's own keys into its KMU slots, taking them from the key set `--keyset` names, alongside the verification keys and the OTA KEKs.
- Batched platform register services: `ironside_platform_mem_read()`, `ironside_platform_mem_write()`, and the single-register helpers `ironside_platform_reg_read()` / `ironside_platform_reg_write()`.
  One call takes at most 16 entries, and a write changes only the bits under its entry's mask.
- `--board-lfxo CAP_PF[,bypass]`, `--board-hfxo CAP_PF` and `--board-vreg dcdc|ldo` on `ironside_bundle.py update-config`, and the matching `board-lfxo`, `board-hfxo` and `board-vreg` keys in the image-configuration profile.
  They set the crystal load capacitances and the main regulator mode IronSide programs at boot, which no longer default to the DK values in the firmware.
  IronSide refuses to boot without them.
  `sign.py` fills any unset value with the DK value (LFXO 17 pF, HFXO 15 pF, DC/DC) and prints a warning.
- `struct ironside_boot_report` gains `app`, `dfubank`, `appstorage` and `dfuimage`, each a `struct ironside_boot_region` holding the address and size of that partition in the layout provisioned on the device.
  The struct grows to 164 bytes.
  Against an IronSide that does not report the layout, `ironside_boot_report_get()` returns a `report_length` shorter than the struct, which the caller treats as an error.
  `adac.py boot-report` prints the layout.
- New script `read_boot_error.py` decodes the BOOTFAIL word IronSide TE now publishes in the CTRL-AP mailbox when a stage halts: the stage and the halt reason.
  `--json` reports each field by value and by name.
- `--pof CODE[,noevent]` and `--no-pof` on `ironside_bundle.py update-config`, and the matching `pof` and `no-pof` keys in the image-configuration profile.
  `CODE` is the raw `REGULATORS.POFCON.THRESHOLD` code 0 to 15, whose voltage is specific to the part.
- The IronSDK now ships the production programming guide and the J-Link command files it uses, in `factory/`.
- The PSA Crypto service accepts `PSA_ALG_CMAC` (full length only) and `PSA_ALG_ECB_NO_PADDING` from the application, as the Bluetooth LE host requires.
  The client headers now define `PSA_ALG_ECB_NO_PADDING`.
- `ironside_platform_system_reset()` in `client/include/ironside_client/platform.h` resets the device through IronSide, which applies the nRF54L errata [63] workaround first.
  The reset reason on the next boot is SREQ.
  A Zephyr image that enables `CONFIG_IRONSIDE_CLIENT` gets it through `sys_reboot()` with no source change.

### Updated

- The hidden `--kms` alias for `--keyset` is gone from every tool.
  A script that still passes `--kms` fails with `unrecognized arguments` rather than reading a key source nobody meant.
- Editing the provisioning configuration needs no key source, but it has to happen before `update-keys`.
  After a later `update-config`, the tool names the members whose files are stale and `check` fails until `update-keys` runs again.
  `check` reviews a bundle without key access.
  `ironside_install.py` builds the install batch itself, so its key set now also supplies the OTA KEKs.
- `ironside_platform_mem_read()` now takes an array of secure addresses and returns one `uint32_t` per element (the retired single-range `{addr, len}` form used to handle `0x40000500`).
- The IronSDK user guide states that every firmware image in a bundle is an Intel HEX file and that a DFU image must start at the DFUIMAGE base, and its bundle listings show a 32 kB DFUIMAGE instead of 96 kB.
- The user guide's Isolation section now covers memory as well as peripherals: which parts of non-volatile memory and RAM a non-secure image can read, write and execute, how that depends on the booted image and the lifecycle state, the rules for NVM writes, secure-service buffers and DMA, and where an application reads the provisioned partition layout.
- `IRONSIDE_CLIENT_APPLICATION_KDK_ID` is renamed `IRONSIDE_CLIENT_MEXT_ID`, with no alias for the old name.
  The key ID is unchanged.
  The `psa_application_kdk` sample is renamed `psa_mext`.
- Secure services may be called from any non-secure context, thread or interrupt, one call at a time.
  The contract was previously thread context only.
  A second, concurrent call - including one made from an ISR that preempts a call already in flight - is refused with `IRONSIDE_ERROR_CALL_IN_FLIGHT` (-249, defined in the new header `ironside_client/error.h`) rather than producing an undefined result.
  IronSide previously returned `PSA_ERROR_BAD_STATE`.
- The recommended override for `ironside_client_lock_acquire`/`ironside_client_lock_release` is a blocking mutex that both hooks bypass in interrupt and pre-kernel context, where no lock that waits can be taken.
  A thread caller that takes the mutex is never the call that gets refused; an interrupt caller must handle `IRONSIDE_ERROR_CALL_IN_FLIGHT` by retrying later or deferring the work to a thread.
  An application whose interrupt-context caller can not tolerate a refusal may mask interrupts for the duration of every call instead, at the cost of that call's full duration in interrupt latency.
- IronSide makes `SYSRESETREQ` secure-only, so `NVIC_SystemReset()` or a direct AIRCR write from a non-secure image no longer resets the device.
  A bare-metal application must call `ironside_platform_system_reset()` instead.
  IronSide TE and IronSide DFU must be installed from the same release: an older IronSide DFU cannot reset itself after an upload under this IronSide TE.

### Removed

- `ironside_platform_gpio_pin_ctrlsel_set()` and the `IRONSIDE_PLATFORM_GPIO_CTRLSEL_*` macros.
  Route pads through a `GPIO.PIN_CNF` read-modify-write using the batched write service instead.
- The `reset-config` and `reset-keys` subcommands of `ironside_bundle.py`, and its `-k`/`--key PEM` option.
  A script that uses them now fails with an argument error.

### Fixed

- `ironside_bundle.py check`, `build-production-hex` and `ironside_install.py` refuse a bundle whose DFU image does not start at the DFUIMAGE base the bundle's partition layout gives.
  They previously installed it, and the DFU image did not start on any DFUIMAGE size it was not linked for.
  `update-config` and `import-config` warn about the mismatch.
- The samples' nrfx dependency now points at `github.com/nordicsemi/nrfx`, tag v4.2.1.
  Its earlier pin no longer resolved, so a fresh checkout of the sample dependencies failed.

## 2026.09.0 - 2026-09-11

This IronSDK release is the second prototype release and is intended to be used with IronSide TE 0.2.0.

IronSide TE 0.2.0, and all other prototype releases (with major version 0) do not have compatiblity promises and it is therefore required to re-install IronSide TE.

See the user guide for re-installation of IronSide TE.

Notable new features are:

- 54L15 support
- CTRLSEL service
- Sample demonstrating VPR support

### Added

- A PSA attestation token now carries the RFC 9783 boot-seed claim (`PSA_INITIAL_ATTEST_CLAIM_BOOT_SEED`, label 268), 32 public bytes that are the same in every token of a boot session and different across a reset, which grows `PSA_INITIAL_ATTEST_MAX_TOKEN_SIZE` to 620 bytes.
- Added support for the nRF54L15, which the SDK tools and samples now build for alongside the nRF54LM20A, nRF54LM20B and nRF54LV10A.
- `ironside_platform_gpio_pin_ctrlsel_set()` in `client/include/ironside_client/platform.h`: a request that selects which module drives a pad (`GPIO.PIN_CNF[n].CTRLSEL` -- the GPIO block, the VPR or GRTC) and reports the applied selection back, which an application cannot do itself because that field is admitted and read only from a secure transaction.
- PSA Crypto is now published as the standard PSA Crypto header set -- `crypto_types.h`, `crypto_values.h`, `crypto_struct.h`, `crypto_sizes.h` and `crypto_extra.h` (the `IRONSIDE_CLIENT_*` extensions) beside the `crypto.h` that already existed, all under `client/include/ironside_client/psa/` -- so code written against another implementation ports file for file, and including `crypto.h` alone still gives the whole API.
- `PSA_KEY_TYPE_DERIVE` is now declared; key-derivation secrets were already supported, but the constant was missing, so an application could only name that key type by writing its raw numeric value.

### Updated

- Each software component in a PSA attestation token now reports its version (software-components sub-claim key 4), which grows `PSA_INITIAL_ATTEST_MAX_TOKEN_SIZE` in `client/include/ironside_client/psa/initial_attestation.h`, so an application that sizes its token buffer with it needs a rebuild.
- The provisioning member of a prepared bundle is now container version 3, its configuration map having gained the IKG SEED entry, so a bundle prepared by an earlier tool reads as unreadable -- run `ironside_bundle.py update-keys` again.
- Added the nrfutil mcu-manager subcommand to the SDK tooling requirements.

### Removed

- Removed every byte offset into a PSA attestation token from `client/include/ironside_client/psa/initial_attestation.h`, so an application must now decode the token with a CBOR parser and look claims up by their `PSA_INITIAL_ATTEST_CLAIM_*` label -- against the token format the new `client/psa_attest_token.cddl` describes, as the `psa_initial_attestation` sample now does with the small standalone decoder in `samples/psa_initial_attestation/attest_cbor.c`.
- Removed the ctrlapsoft value from --dfu-trigger-resetreas, following its removal as a DFU-entry trigger in IronSide TE.

### Fixed

- Fixed the PSA attestation token's implementation-id claim (RFC 9783 label 2396), which reported a provisioned UICR field and so read as 32 bytes of 0xff on any device nobody supplied a value for: UROT now composes it from the part (FICR.INFO) and the IROT version, in the layout `scripts/lib/impl_id.py` encodes, so `update-config --psa-implementation-id`, the `psa-implementation-id` bundle-profile field and `scripts/lib/uicr.py`'s `IMPLEMENTATIONID_OFF`/`IMPLEMENTATIONID_LEN` are gone.
- Fixed the software-component measurements in a PSA attestation token, which covered a whole firmware partition rather than the image in it: each is now SHA-256 over the image's signed region, which `scripts/lib/mcuboot_image.py`'s `signed_region_sha256()` computes from a released image, so a verifier can hold the claim against a reference value.

## 2026.08.4 - 2026-08-31

### Added

- Initial release
