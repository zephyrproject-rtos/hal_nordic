# Changelog

Change history for ironside-te-prov-secure on nrf54lv10a.

## 0.4.0 - 2026-10-22

### Added

- The `check-ikg-seed` operation, which checks the IKG SEED slots without writing them.
  `resources/interface.cddl` describes its parameters and results.

### Updated

- [Breaking change] The provisioning ABI in `resources/interface.cddl`, which moved to version 7 and no longer accepts a version 6 batch.
  Make sure to rebuild every batch encoded against version 6.
- [Breaking change] The provisioning firmware entry point, vector table, and stack top, which all moved above the largest PSA key slot storage size: the entry point is `0x20012800` (was `0x20008000`), the vector table `0x20013000` (was `0x20008800`), and the stack top moves up by the same 42 kB.
  Make sure that tools that start the provisioning firmware, such as J-Link scripts, use the new addresses.
- [Breaking change] The `provision-counter` operation for the App and IronSide DFU roles, which writes the counter to the UROT storage.
  Make sure that `init-urot-storage` comes earlier in the batch, or the operation reports `PSA_ERROR_BAD_STATE`.

## 0.3.0 - 2026-10-05

### Updated

- The provisioning ABI in `resources/interface.cddl` moved to version 4 and no longer accepts a version 3 batch, so every batch encoded against version 3 has to be rebuilt: `import-kmu-key` is now spelled `import-key`, because the operation imports any key the packed `psa_key_attributes_t` describes and was never specific to the KMU.
- A batch may now carry up to 64 operations instead of 16, which is what makes room for an application's own KMU keys alongside the rollback counters and the OTA KEKs.
  A batch still has to fit the command buffer, which is the smaller cap on the parts that have 8 kB of it.
- `import-key` now also imports AES-128 and AES-192 keys, ChaCha20 keys (including for ChaCha20-Poly1305), HMAC keys for SHA-256 and SHA-384, secp384r1 keys, and ECC key pairs.
  The provisioning image previously did not include these key types, so an install that provisioned one failed with `PSA_ERROR_NOT_SUPPORTED` partway through.

## 0.2.0 - 2026-09-11

### Updated

- The provisioning ABI in `resources/interface.cddl` moved to version 3 and no longer accepts version 2 batches, which must be rebuilt: `enroll-ikg-seed` now takes an entry index (0 for Nordic SEED in KMU slots 250-252, 1 for User SEED in 253-255), and `revoke-ikg-seed` joined the vocabulary to permanently retire an entry that holds no seed, which is how a batch settles on the other one.
- `export-iak-pubkey` and `import-fw-kek` now derive from whichever CRACEN IKG SEED entry the device has selected instead of always from Nordic SEED, so a batch that enrolls User SEED must do so before either operation runs, which the supplied batch builders guarantee.

## 0.1.0 - 2026-08-31

### Added

- Initial release
