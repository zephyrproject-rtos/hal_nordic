# Changelog

Change history for ironside-te-irot on nrf54lm20b.

## 0.4.0 - 2026-10-22

This release is not compatible with earlier releases.
Make sure to install IROT and UROT together from this release.

### Updated

- The UROT image verification, which requires both an ECDSA P-384 and an ML-DSA-65 signature and no longer accepts Ed25519.
- [Breaking change] The IROT RAM layout, which is not compatible with earlier UROT releases.

## 0.3.0 - 2026-10-05

### Updated

- When IROT halts, it publishes a BOOTFAIL word in the CTRL-AP mailbox that names the stage and the halt reason.
  The earlier panic identifiers are replaced and not preserved.
  A CPU fault now halts with its own error code.
- Built on nRF Connect SDK v3.4.1 (previously v3.4.0), which brings Oberon PSA Crypto 2.1.0.
- IROT no longer disables the voltage glitch detector, which stays at its hardware reset state (enabled) for the whole boot chain and the application.
  A detector trigger resets the device.
- IROT no longer opens the CPU debug access port.
  Debug access is configured by UROT, so a debugger cannot attach to a device that halts in IROT.
  Read the halt reason from the CTRL-AP mailbox, and recover the device with CTRL-AP ERASEALL.

## 0.2.0 - 2026-09-11

### Updated

- Moved the DECOMISSIONED field from UICR.IRONSIDE.UROT to UICR.IRONSIDE.IROT.
  This is a backwards incompatible change and a full IronSide TE reinstall is needed.

## 0.1.0 - 2026-08-31

### Added

- Initial release
