# IronSide TE releases

This page lists the IronSide TE features, fixed issues, and component releases by feature level and release date.
For the meaning of the feature level, see [Feature level](artifacts-and-bundles.md#feature-level).

This page contains the following sections:

- [Features by feature level](#features-by-feature-level)
- [Fixed issues](#fixed-issues)
- [Release history](#release-history)
  - [Feature level 0](#feature-level-0)
    - [Features and changes](#fl0-features)
    - [Releases](#fl0-releases)

## Features by feature level

This section lists the IronSide TE features, the feature level that adds each feature on each SoC, and the first IronSide Integration Kit release that supports it.
A feature is available from the feature level in its **From feature level** column and in every higher feature level, unless stated otherwise.
The **From IronSide Integration Kit** column gives the first IronSide Integration Kit release that supports the feature level.

<table>
<tr><th>Feature</th><th>From feature level</th><th>SoC</th><th>From IronSide Integration Kit</th></tr>
<tr><td><a id="ite-f-1"></a>ITE-F-1: Secure boot of the application firmware, with signature verification and rollback protection</td><td>0</td><td>All SoCs</td><td>2026.08.2</td></tr>
<tr><td><a id="ite-f-2"></a>ITE-F-2: Memory and peripheral isolation of the secure and non-secure domains</td><td>0</td><td>All SoCs</td><td>2026.08.2</td></tr>
<tr><td><a id="ite-f-3"></a>ITE-F-3: Firmware update of the application firmware and IronSide TE through the DFU image over serial SMP</td><td>0</td><td>All SoCs</td><td>2026.08.2</td></tr>
<tr><td><a id="ite-f-4"></a>ITE-F-4: Base, image and provisioning configuration</td><td>0</td><td>All SoCs</td><td>2026.08.2</td></tr>
<tr><td><a id="ite-f-5"></a>ITE-F-5: PSA Crypto secure service</td><td>0</td><td>All SoCs</td><td>2026.08.2</td></tr>
<tr><td><a id="ite-f-6"></a>ITE-F-6: PSA Attestation secure service</td><td>0</td><td>All SoCs</td><td>2026.08.2</td></tr>
<tr><td><a id="ite-f-7"></a>ITE-F-7: Platform secure service, including the reset service and batched register access</td><td>0</td><td>All SoCs</td><td>2026.08.2</td></tr>
<tr><td><a id="ite-f-8"></a>ITE-F-8: DFU boot selection</td><td>0</td><td>All SoCs</td><td>2026.08.2</td></tr>
<tr><td><a id="ite-f-9"></a>ITE-F-9: Boot report, including the partition layout</td><td>0</td><td>All SoCs</td><td>2026.08.2</td></tr>
<tr><td rowspan="4"><a id="ite-f-10"></a>ITE-F-10: Executable RAM region</td><td>0</td><td>nRF54LM20A</td><td>2026.08.2</td></tr>
<tr><td>0</td><td>nRF54LM20B</td><td>2026.08.2</td></tr>
<tr><td>0</td><td>nRF54LV10A</td><td>2026.08.2</td></tr>
<tr><td>Not supported</td><td>Other SoCs</td><td>Not supported</td></tr>
<tr><td><a id="ite-f-11"></a>ITE-F-11: Secure service calls from non-secure interrupt context</td><td>0</td><td>All SoCs</td><td>2026.08.2</td></tr>
<tr><td><a id="ite-f-12"></a>ITE-F-12: Fault handling and boot failure codes</td><td>0</td><td>All SoCs</td><td>2026.08.2</td></tr>
<tr><td><a id="ite-f-13"></a>ITE-F-13: Debugger access protection</td><td>0</td><td>All SoCs</td><td>2026.08.2</td></tr>
<tr><td><a id="ite-f-14"></a>ITE-F-14: Authenticated debug (ADAC), including the ADAC boot report</td><td>0</td><td>All SoCs</td><td>2026.08.2</td></tr>
<tr><td><a id="ite-f-15"></a>ITE-F-15: Application keys provisioned in the factory</td><td>0</td><td>All SoCs</td><td>2026.08.2</td></tr>
<tr><td><a id="ite-f-16"></a>ITE-F-16: Production programming</td><td>0</td><td>All SoCs</td><td>2026.08.2</td></tr>
</table>

## Fixed issues

This section lists the fixed issues, the components and SoCs that each issue is fixed on, the release dates of the fixes, and the affected versions.
`affects A to B` gives the inclusive range of affected versions.
`A` is the version that introduced the issue, and `B` is the last version before the fix.
`affects A` means that only version `A` is affected.
N/A means the issue does not affect the component on that SoC.

None yet.

## Release history

This section lists the component releases of each feature level by release date, with the features and changes of each feature level.
A release date with releases in more than one feature level is listed under each feature level.

### Feature level 0

The first IronSide Integration Kit release that supports feature level 0 is 2026.08.2.

<a id="fl0-features"></a>

#### Features and changes

- `ironside-te-irot`
  - All SoCs
    - Initial release
    - Moved the DECOMISSIONED field from UICR.IRONSIDE.UROT to UICR.IRONSIDE.IROT.
      This is a backwards incompatible change and a full IronSide TE reinstall is needed.
    - When IROT halts, it publishes a BOOTFAIL word in the CTRL-AP mailbox that names the stage and the halt reason.
      The earlier panic identifiers are replaced and not preserved.
      A CPU fault now halts with its own error code.
    - Built on nRF Connect SDK v3.4.1 (previously v3.4.0), which brings Oberon PSA Crypto 2.1.0.
    - IROT no longer disables the voltage glitch detector, which stays at its hardware reset state (enabled) for the whole boot chain and the application.
      A detector trigger resets the device.
    - IROT no longer opens the CPU debug access port.
      Debug access is configured by UROT, so a debugger cannot attach to a device that halts in IROT.
      Read the halt reason from the CTRL-AP mailbox, and recover the device with CTRL-AP ERASEALL.
    - The UROT image verification, which requires both an ECDSA P-384 and an ML-DSA-65 signature and no longer accepts Ed25519.
    - [Breaking change] The IROT RAM layout, which is not compatible with earlier UROT releases.
  - nRF54L15
    - [Breaking change] The IROT partition, which grew from 16 KB to 19 KB, so this IROT boots only a UROT built for the same layout.
- `ironside-te-urot`
  - All SoCs
    - Initial release
    - Added the version sub-claim (RFC 9783 section 4.4.1, key 4) to every software component in the PSA attestation token, taken from the image slot's own header version and rendered fixed-width as `NNN.NNN.NNN+NNN`, which grows the token from 478/494/510 to 529/545/561 bytes for a 32/48/64-byte challenge.
    - Added support for decommissioning via the ADAC LCS_CHANGE command.
    - Added support for a second CRACEN IKG SEED: Nordic SEED occupies KMU slots 250-252 and User SEED 253-255, User SEED overriding Nordic SEED once it is enrolled, and UROT resolves which entry is active during its boot sequence.
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
    - The PSA key slot storage size in the base configuration, which sets how many PSA key slots IronSide TE offers and where non-secure RAM starts.
    - The `ram_ns` field of the boot report, which is the non-secure RAM window in effect.
    - Moved the DECOMISSIONED field from UICR.IRONSIDE.UROT to UICR.IRONSIDE.IROT.
      This is a backwards incompatible change and a full IronSide TE reinstall is needed.
    - UROT now refuses to boot unless the six IKG SEED slots are in one of the three states the specification accepts -- Nordic SEED locked with User SEED revoked, both locked, or Nordic SEED revoked with User SEED locked -- otherwise halting with 0xFA1702xx over the CTRL-AP mailbox, whose detail byte reports (Nordic SEED << 4) | User SEED, so a device provisioned before User SEED existed reads (locked, empty) = 0x10 and must have User SEED settled in a provisioning lifecycle before this UROT is installed on it.
    - The non-secure PSA key policy now withholds both IKG SEED slot spans, where User SEED's slots 253-255 were previously reachable from non-secure code during a provisioning lifecycle.
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
    - Removed CTRLAPSOFT as a DFU-entry reset-reason trigger.
    - Removed the single-range platform read service (handle `0x40000500`) and the GPIO pin CTRLSEL service (handle `0x40000505`).
      They are replaced by MEM_READ and by a masked MEM_WRITE of `PIN_CNF.CTRLSEL`.
    - The UROT BOOTFAIL codes `0x86` (`APP_COUNTER_LOCK`) and `0x87` (`DFU_COUNTER_LOCK`).
  - nRF54L15
    - [Breaking change] The UROT slot, which now starts at `0x00005000` and is 120 KB, so this UROT boots only behind an IROT built for the same layout.
    - UROT no longer applies the nRF54L15 errata [32] and [40] workarounds.
      IronSide TE applies only the errata the user guide lists (31, 37 and 63), and the application and IronSide DFU are responsible for any other errata.
- `ironside-te-dfu-serial`
  - All SoCs
    - Initial release
    - The console boot report lists the address and size of each non-secure partition.
    - Raised the required IronSide TE UROT version to 0.2.0.
      The serial DFU image is otherwise unchanged, but UROT 0.2.0 is a backwards incompatible release needing a full IronSide TE reinstall, so this image is not released for a device running UROT 0.1.0.
    - The DFU image reads the update destinations, and the slot it runs from, from the IronSide boot report at runtime instead of using build-time addresses, so it follows the partition layout provisioned on the device.
      It requires an IronSide that reports the layout; without one it serves no uploads.
    - The IronSide DFU RAM start, which moved up by 42 kB so that IronSide DFU runs under every PSA key slot storage size.
- `ironside-te-prov-secure`
  - All SoCs
    - Initial release
    - The `check-ikg-seed` operation, which checks the IKG SEED slots without writing them.
      `resources/interface.cddl` describes its parameters and results.
    - The provisioning ABI in `resources/interface.cddl` moved to version 3 and no longer accepts version 2 batches, which must be rebuilt: `enroll-ikg-seed` now takes an entry index (0 for Nordic SEED in KMU slots 250-252, 1 for User SEED in 253-255), and `revoke-ikg-seed` joined the vocabulary to permanently retire an entry that holds no seed, which is how a batch settles on the other one.
    - `export-iak-pubkey` and `import-fw-kek` now derive from whichever CRACEN IKG SEED entry the device has selected instead of always from Nordic SEED, so a batch that enrolls User SEED must do so before either operation runs, which the supplied batch builders guarantee.
    - The provisioning ABI in `resources/interface.cddl` moved to version 4 and no longer accepts a version 3 batch, so every batch encoded against version 3 has to be rebuilt: `import-kmu-key` is now spelled `import-key`, because the operation imports any key the packed `psa_key_attributes_t` describes and was never specific to the KMU.
    - A batch may now carry up to 64 operations instead of 16, which is what makes room for an application's own KMU keys alongside the rollback counters and the OTA KEKs.
      A batch still has to fit the command buffer, which is the smaller cap on the parts that have 8 kB of it.
    - `import-key` now also imports AES-128 and AES-192 keys, ChaCha20 keys (including for ChaCha20-Poly1305), HMAC keys for SHA-256 and SHA-384, secp384r1 keys, and ECC key pairs.
      The provisioning image previously did not include these key types, so an install that provisioned one failed with `PSA_ERROR_NOT_SUPPORTED` partway through.
    - [Breaking change] The provisioning ABI in `resources/interface.cddl`, which moved to version 7 and no longer accepts a version 6 batch.
      Make sure to rebuild every batch encoded against version 6.
    - [Breaking change] The provisioning firmware entry point, vector table, and stack top, which all moved above the largest PSA key slot storage size: the entry point is `0x20012800` (was `0x20008000`), the vector table `0x20013000` (was `0x20008800`), and the stack top moves up by the same 42 kB.
      Make sure that tools that start the provisioning firmware, such as J-Link scripts, use the new addresses.
    - [Breaking change] The `provision-counter` operation for the App and IronSide DFU roles, which writes the counter to the UROT storage.
      Make sure that `init-urot-storage` comes earlier in the batch, or the operation reports `PSA_ERROR_BAD_STATE`.
- `ironside-te-prov-nonsecure`
  - All SoCs
    - Initial release
    - The provisioning ABI in `resources/interface.cddl` moved to version 3 and no longer accepts a version 2 batch, so every batch encoded against version 2 has to be rebuilt; version 3 carries the two CRACEN IKG SEED entries (`enroll-ikg-seed` now names an entry, and `revoke-ikg-seed` is new), both secure-only operations this non-secure image reports as unsupported.
    - The provisioning ABI in `resources/interface.cddl` moved to version 4 and no longer accepts a version 3 batch, so every batch encoded against version 3 has to be rebuilt: `import-kmu-key` is now spelled `import-key`, because the operation imports any key the packed `psa_key_attributes_t` describes and was never specific to the KMU.
    - A batch may now carry up to 64 operations instead of 16, which is what makes room for an application's own KMU keys alongside the rollback counters and the OTA KEKs.
      A batch still has to fit the command buffer, which is the smaller cap on the parts that have 8 kB of it.
    - [Breaking change] The provisioning ABI in `resources/interface.cddl`, which moved to version 7 and no longer accepts a version 6 batch, and this image reports `UNSUPPORTED` for the new `check-ikg-seed` operation.
      Make sure to rebuild every batch encoded against version 6.
    - [Breaking change] The provisioning firmware entry point, vector table, and stack top, which all moved above the largest PSA key slot storage size: the entry point is `0x20012800` (was `0x20008000`), the vector table `0x20013000` (was `0x20008800`), and the stack top moves up by the same 42 kB.
      Make sure that tools that start the provisioning firmware, such as J-Link scripts, use the new addresses.

<a id="fl0-releases"></a>

#### Releases

<table>
<tr><th>Release date</th><th colspan="2">Released</th></tr>
<tr><td rowspan="25"><a id="fl0-2026-10-22"></a>2026-10-22</td><th colspan="2"><code>ironside-te-irot</code></th></tr>
<tr><td>nRF54L15</td><td><a href="component-changelogs/ironside-te-irot-nrf54l15-fl0.md">0.4.0</a></td></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-irot-nrf54lm20a-fl0.md">0.4.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-irot-nrf54lm20b-fl0.md">0.4.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-irot-nrf54lv10a-fl0.md">0.4.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-urot</code></th></tr>
<tr><td>nRF54L15</td><td><a href="component-changelogs/ironside-te-urot-nrf54l15-fl0.md">0.4.0</a></td></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-urot-nrf54lm20a-fl0.md">0.4.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-urot-nrf54lm20b-fl0.md">0.4.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-urot-nrf54lv10a-fl0.md">0.4.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-dfu-serial</code></th></tr>
<tr><td>nRF54L15</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54l15-fl0.md">0.4.0</a></td></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54lm20a-fl0.md">0.4.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54lm20b-fl0.md">0.4.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54lv10a-fl0.md">0.4.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-prov-secure</code></th></tr>
<tr><td>nRF54L15</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54l15-fl0.md">0.4.0</a></td></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54lm20a-fl0.md">0.4.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54lm20b-fl0.md">0.4.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54lv10a-fl0.md">0.4.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-prov-nonsecure</code></th></tr>
<tr><td>nRF54L15</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54l15-fl0.md">0.4.0</a></td></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54lm20a-fl0.md">0.4.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54lm20b-fl0.md">0.4.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54lv10a-fl0.md">0.4.0</a></td></tr>
<tr><td rowspan="25"><a id="fl0-2026-10-05"></a>2026-10-05</td><th colspan="2"><code>ironside-te-irot</code></th></tr>
<tr><td>nRF54L15</td><td><a href="component-changelogs/ironside-te-irot-nrf54l15-fl0.md">0.3.0</a></td></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-irot-nrf54lm20a-fl0.md">0.3.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-irot-nrf54lm20b-fl0.md">0.3.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-irot-nrf54lv10a-fl0.md">0.3.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-urot</code></th></tr>
<tr><td>nRF54L15</td><td><a href="component-changelogs/ironside-te-urot-nrf54l15-fl0.md">0.3.0</a></td></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-urot-nrf54lm20a-fl0.md">0.3.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-urot-nrf54lm20b-fl0.md">0.3.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-urot-nrf54lv10a-fl0.md">0.3.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-dfu-serial</code></th></tr>
<tr><td>nRF54L15</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54l15-fl0.md">0.3.0</a></td></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54lm20a-fl0.md">0.3.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54lm20b-fl0.md">0.3.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54lv10a-fl0.md">0.3.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-prov-secure</code></th></tr>
<tr><td>nRF54L15</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54l15-fl0.md">0.3.0</a></td></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54lm20a-fl0.md">0.3.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54lm20b-fl0.md">0.3.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54lv10a-fl0.md">0.3.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-prov-nonsecure</code></th></tr>
<tr><td>nRF54L15</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54l15-fl0.md">0.3.0</a></td></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54lm20a-fl0.md">0.3.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54lm20b-fl0.md">0.3.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54lv10a-fl0.md">0.3.0</a></td></tr>
<tr><td rowspan="25"><a id="fl0-2026-09-11"></a>2026-09-11</td><th colspan="2"><code>ironside-te-irot</code></th></tr>
<tr><td>nRF54L15</td><td><a href="component-changelogs/ironside-te-irot-nrf54l15-fl0.md">0.2.0</a></td></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-irot-nrf54lm20a-fl0.md">0.2.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-irot-nrf54lm20b-fl0.md">0.2.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-irot-nrf54lv10a-fl0.md">0.2.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-urot</code></th></tr>
<tr><td>nRF54L15</td><td><a href="component-changelogs/ironside-te-urot-nrf54l15-fl0.md">0.2.0</a></td></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-urot-nrf54lm20a-fl0.md">0.2.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-urot-nrf54lm20b-fl0.md">0.2.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-urot-nrf54lv10a-fl0.md">0.2.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-dfu-serial</code></th></tr>
<tr><td>nRF54L15</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54l15-fl0.md">0.2.0</a></td></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54lm20a-fl0.md">0.2.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54lm20b-fl0.md">0.2.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54lv10a-fl0.md">0.2.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-prov-secure</code></th></tr>
<tr><td>nRF54L15</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54l15-fl0.md">0.2.0</a></td></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54lm20a-fl0.md">0.2.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54lm20b-fl0.md">0.2.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54lv10a-fl0.md">0.2.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-prov-nonsecure</code></th></tr>
<tr><td>nRF54L15</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54l15-fl0.md">0.2.0</a></td></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54lm20a-fl0.md">0.2.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54lm20b-fl0.md">0.2.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54lv10a-fl0.md">0.2.0</a></td></tr>
<tr><td rowspan="20"><a id="fl0-2026-08-31"></a>2026-08-31</td><th colspan="2"><code>ironside-te-irot</code></th></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-irot-nrf54lm20a-fl0.md">0.1.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-irot-nrf54lm20b-fl0.md">0.1.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-irot-nrf54lv10a-fl0.md">0.1.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-urot</code></th></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-urot-nrf54lm20a-fl0.md">0.1.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-urot-nrf54lm20b-fl0.md">0.1.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-urot-nrf54lv10a-fl0.md">0.1.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-dfu-serial</code></th></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54lm20a-fl0.md">0.1.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54lm20b-fl0.md">0.1.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54lv10a-fl0.md">0.1.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-prov-secure</code></th></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54lm20a-fl0.md">0.1.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54lm20b-fl0.md">0.1.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54lv10a-fl0.md">0.1.0</a></td></tr>
<tr><th colspan="2"><code>ironside-te-prov-nonsecure</code></th></tr>
<tr><td>nRF54LM20A</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54lm20a-fl0.md">0.1.0</a></td></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54lm20b-fl0.md">0.1.0</a></td></tr>
<tr><td>nRF54LV10A</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54lv10a-fl0.md">0.1.0</a></td></tr>
<tr><td rowspan="2"><a id="fl0-2026-08-26"></a>2026-08-26</td><th colspan="2"><code>ironside-te-dfu-serial</code></th></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-dfu-serial-nrf54lm20b-fl0.md">0.0.2</a></td></tr>
<tr><td rowspan="8"><a id="fl0-2026-08-23"></a>2026-08-23<br>Features: <a href="#fl0-features">Features and changes</a></td><th colspan="2"><code>ironside-te-irot</code></th></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-irot-nrf54lm20b-fl0.md">0.0.2</a></td></tr>
<tr><th colspan="2"><code>ironside-te-urot</code></th></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-urot-nrf54lm20b-fl0.md">0.0.2</a></td></tr>
<tr><th colspan="2"><code>ironside-te-prov-secure</code></th></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-prov-secure-nrf54lm20b-fl0.md">0.0.2</a></td></tr>
<tr><th colspan="2"><code>ironside-te-prov-nonsecure</code></th></tr>
<tr><td>nRF54LM20B</td><td><a href="component-changelogs/ironside-te-prov-nonsecure-nrf54lm20b-fl0.md">0.0.2</a></td></tr>
</table>
