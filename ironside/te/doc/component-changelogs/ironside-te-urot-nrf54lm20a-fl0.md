# Changelog

Change history for ironside-te-urot on nrf54lm20a.

## 0.4.0 - 2026-10-22

This release is not compatible with earlier releases.
Make sure to install IROT and UROT together from this release.

### Added

- The PSA key slot storage size in the base configuration, which sets how many PSA key slots IronSide TE offers and where non-secure RAM starts.
- The `ram_ns` field of the boot report, which is the non-secure RAM window in effect.

### Updated

- The condition that starts the decommission sequence, which is now only the exact value that the ADAC LCS Change command commits.
- The application and IronSide DFU image verification, which uses ECDSA P-384 over SHA-384 instead of Ed25519.
- The ADAC authentication, which uses ECDSA P-384 (ADAC cryptosystem `0x0A`) instead of Ed25519 (`0x05`).
- The attestation signer ID of the application and IronSide DFU components, which is now the SHA-256 of the component's verification key digest.
- The handling of a service call that does not return, which UROT now leaves in flight instead of resetting the device with a watchdog.
  Every later call is refused with `IRONSIDE_ERROR_CALL_IN_FLIGHT` until a reset.
- [Breaking change] The UROT RAM layout, which is not compatible with earlier IROT releases.
- The number of PSA key slots on a device with the default storage size (now 8, down from 12).
  Non-secure RAM still starts at `0x20008000`.
- [Breaking change] The App and IronSide DFU rollback counters, which are now stored in the UROT storage (instead of being stored in KMU slots).
  Make sure to reprovision a device provisioned by an earlier release, starting from the Empty LCS.
- The replacement and deletion of UROT storage records, which now overwrite the old record, so `psa_destroy_key()` leaves no ciphertext of the key on the device.

### Removed

- The UROT BOOTFAIL codes `0x86` (`APP_COUNTER_LOCK`) and `0x87` (`DFU_COUNTER_LOCK`).

### Fixed

- An issue where a secure call that waits for CRACEN hung when the non-secure `AIRCR.PRIGROUP` was 6 or 7.
  The CRACEN interrupt priority raised for each call now accounts for the caller's group priority.

## 0.3.0 - 2026-10-05

### Added

- Added the Nordic vendor ADAC command `BOOT_REPORT_GET` (`0x8002`).
  It returns the boot report of the last boot that reached its boot decision, in the same layout the boot-report service uses.
  It requires a completed ADAC authentication in the same session and answers `UNAUTHORIZED` otherwise.
  No permission bit is needed.
- The boot report carries the non-secure partition layout IronSide resolved from UICR at boot: the address and size of the App, DFUBANK, APPSTORAGE and DFUIMAGE partitions.
  The report grows from 132 to 164 bytes, both from the boot-report service and from ADAC `BOOT_REPORT_GET`.
- Added the batched platform register services MEM_READ (handle `0x40000506`) and MEM_WRITE (handle `0x40000507`).
  One call takes up to 16 entries.
  Every entry is checked before anything is written, and a write changes only the bits under the entry's mask.
  A call with more entries or with an unaligned address returns `INVALID_ARGUMENT`, and an address or value bit outside the allow-list returns `NOT_PERMITTED`.
  The allow-list covers `OSCILLATORS.PLL.FREQ`/`CURRENTFREQ` (read only), `XOSC32KI.BYPASS`, `XOSC32KI.INTCAP`, `XOSC32M.CONFIG.INTCAP`, `REGULATORS.VREGMAIN.DCDCEN`, and `PIN_CNF` of every GPIO port, where only the `CTRLSEL` field can be written.
- The image configuration can enable the `REGULATORS` power-fail comparator, setting its `POF`, `THRESHOLD` and `EVENTDISABLE` fields.
  UROT merges them into the live register early in its boot and preserves every other bit, so the register's internal fields stay out of reach of a configuration.
  Releases ship with the comparator off.
  A threshold above the board's supply asserts `POFWARN` permanently, which blocks writes to non-volatile memory.
- The non-secure PSA Crypto service now accepts `PSA_ALG_CMAC` (full length only; a truncated CMAC is still refused) and `PSA_ALG_ECB_NO_PADDING` (encrypt and decrypt).
  These are the algorithms the Bluetooth LE host needs.
- Added the system reset service (handle `0x40000601`).
  It applies the nRF54L errata [63] workaround and then resets the device, and the next boot reports the reset reason as SREQ.
  UROT's own soft resets, including the reset after a fault, take the same path.

### Updated

- The LFXO load capacitance and bypass setting, the HFXO load capacitance, and the main regulator mode (DC/DC or LDO) are now read from three BOARD words in the image configuration.
  Previously they were fixed at the DK values (LFXO 17 pF, HFXO 15 pF, DC/DC).
  All three words are mandatory: if any is unset, UROT halts with `BOARD_UNSET` and neither the application nor IronSide DFU boots.
  Capacitances are set in whole picofarads.
  When the IronSide Integration Kit signs UROT, it fills any unset BOARD word with the DK value and prints a warning.
- When UROT halts, it publishes a BOOTFAIL word in the CTRL-AP mailbox that names the stage and the halt reason.
  It replaces the `0xFA17` status word, whose reason and detail values are not preserved.
- The voltage glitch detector is disabled before the Application or DFU image starts.
  UROT enables it only while it handles a call from the non-secure image.
- Built on nRF Connect SDK v3.4.1 (previously v3.4.0), which brings Oberon PSA Crypto 2.1.0.
- Secure services can now be called from non-secure handler mode as well as thread mode.
  Previously only thread mode was supported, and a call from an ISR could overflow the secure stack and fault.
  For the duration of each call, the CRACEN interrupt is raised one priority level above the caller, so that crypto calls made from a high-priority ISR can complete.
- A secure call made while another call is in flight, including one from an ISR that preempts it, now returns `-249` (`IRONSIDE_ERROR_CALL_IN_FLIGHT`) instead of `PSA_ERROR_BAD_STATE`.
- The NVM write service now masks non-secure interrupts for one 16-byte RRAM write line at a time instead of one 256-byte chunk, so the worst-case non-secure interrupt latency during a write is one line's write time.
  Large writes take somewhat longer.
- The voltage glitch detector is no longer disabled at boot and stays at its hardware reset state (enabled) when the application runs.
  `GLITCHDET` is secure, so the application cannot change it.
  A detector trigger resets the device.
- `SYSRESETREQ` is now secure-only (`AIRCR.SYSRESETREQS` is set), so `NVIC_SystemReset()` or a direct AIRCR write from a non-secure image no longer resets the device.
  A non-secure image has to reset through the system reset service.
  IronSide DFU must come from the same release as this UROT, because an older IronSide DFU cannot reset itself after an upload.

### Removed

- Removed the single-range platform read service (handle `0x40000500`) and the GPIO pin CTRLSEL service (handle `0x40000505`).
  They are replaced by MEM_READ and by a masked MEM_WRITE of `PIN_CNF.CTRLSEL`.

### Fixed

- Fixed the System OFF service, which reset the device with reset reason SREQ instead of entering System OFF.

## 0.2.0 - 2026-09-11

### Added

- Added the version sub-claim (RFC 9783 section 4.4.1, key 4) to every software component in the PSA attestation token, taken from the image slot's own header version and rendered fixed-width as `NNN.NNN.NNN+NNN`, which grows the token from 478/494/510 to 529/545/561 bytes for a 32/48/64-byte challenge.
- Added support for decommissioning via the ADAC LCS_CHANGE command.
- Added support for a second CRACEN IKG SEED: Nordic SEED occupies KMU slots 250-252 and User SEED 253-255, User SEED overriding Nordic SEED once it is enrolled, and UROT resolves which entry is active during its boot sequence.

### Updated

- Moved the DECOMISSIONED field from UICR.IRONSIDE.UROT to UICR.IRONSIDE.IROT.
  This is a backwards incompatible change and a full IronSide TE reinstall is needed.
- UROT now refuses to boot unless the six IKG SEED slots are in one of the three states the specification accepts -- Nordic SEED locked with User SEED revoked, both locked, or Nordic SEED revoked with User SEED locked -- otherwise halting with 0xFA1702xx over the CTRL-AP mailbox, whose detail byte reports (Nordic SEED << 4) | User SEED, so a device provisioned before User SEED existed reads (locked, empty) = 0x10 and must have User SEED settled in a provisioning lifecycle before this UROT is installed on it.
- The non-secure PSA key policy now withholds both IKG SEED slot spans, where User SEED's slots 253-255 were previously reachable from non-secure code during a provisioning lifecycle.

### Removed

- Removed CTRLAPSOFT as a DFU-entry reset-reason trigger.

### Fixed

- Fixed the PSA attestation token's software-component measurements, which hashed a whole partition slot -- including the tail an update does not erase, so two devices running identical firmware could report different values: each is now SHA-256 over the image's signed region (header, body and protected TLVs), the same bytes IronSide verifies before booting the image, with a slot holding no parseable image measuring as the hash of zero bytes.
- Fixed the PSA attestation token's profile-definition claim, which named a URI RFC 9783 section 4.5.2.1 does not permit, to `tag:psacertified.org,2023:psa#ironside-----`, padded to the previous claim length so the token size is unchanged.

## 0.1.0 - 2026-08-31

### Added

- Initial release
