# Changelog

Change history for ironside-te-prov-nonsecure on nrf54lm20b.

## 0.4.0 - 2026-10-22

### Updated

- [Breaking change] The provisioning ABI in `resources/interface.cddl`, which moved to version 7 and no longer accepts a version 6 batch, and this image reports `UNSUPPORTED` for the new `check-ikg-seed` operation.
  Make sure to rebuild every batch encoded against version 6.
- [Breaking change] The provisioning firmware entry point, vector table, and stack top, which all moved above the largest PSA key slot storage size: the entry point is `0x20012800` (was `0x20008000`), the vector table `0x20013000` (was `0x20008800`), and the stack top moves up by the same 42 kB.
  Make sure that tools that start the provisioning firmware, such as J-Link scripts, use the new addresses.

## 0.3.0 - 2026-10-05

### Updated

- The provisioning ABI in `resources/interface.cddl` moved to version 4 and no longer accepts a version 3 batch, so every batch encoded against version 3 has to be rebuilt: `import-kmu-key` is now spelled `import-key`, because the operation imports any key the packed `psa_key_attributes_t` describes and was never specific to the KMU.
- A batch may now carry up to 64 operations instead of 16, which is what makes room for an application's own KMU keys alongside the rollback counters and the OTA KEKs.
  A batch still has to fit the command buffer, which is the smaller cap on the parts that have 8 kB of it.

## 0.2.0 - 2026-09-11

### Updated

- The provisioning ABI in `resources/interface.cddl` moved to version 3 and no longer accepts a version 2 batch, so every batch encoded against version 2 has to be rebuilt; version 3 carries the two CRACEN IKG SEED entries (`enroll-ikg-seed` now names an entry, and `revoke-ikg-seed` is new), both secure-only operations this non-secure image reports as unsupported.

## 0.1.0 - 2026-08-31

### Added

- Initial release
