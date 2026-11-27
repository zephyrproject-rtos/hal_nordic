# Changelog

Change history for ironside-te-dfu-serial on nrf54lv10a.

## 0.4.0 - 2026-10-22

### Updated

- The IronSide DFU RAM start, which moved up by 42 kB so that IronSide DFU runs under every PSA key slot storage size.

### Fixed

- An issue where SMP responses longer than one serial frame, such as the boot report, lost bytes on the DK's J-Link virtual serial port.
  The SMP UART now uses hardware flow control.

## 0.3.0 - 2026-10-05

### Added

- The console boot report lists the address and size of each non-secure partition.

### Updated

- The DFU image reads the update destinations, and the slot it runs from, from the IronSide boot report at runtime instead of using build-time addresses, so it follows the partition layout provisioned on the device.
  It requires an IronSide that reports the layout; without one it serves no uploads.

### Fixed

- The DFU image feeds the non-secure watchdog that IronSide TE starts from the image configuration's watchdog setting.
  Before, the watchdog reset the device during an update whenever the timeout was shorter than the update, so a device with a short timeout could not be updated.
  The image feeds it four times per timeout while its SMP server waits for requests, so a request that never completes still lets the watchdog reset the device.
- A secure call the DFU image makes can be preempted by another of its threads without faulting.
  The image stores the exception return state per thread and serializes its secure calls with a mutex.

## 0.2.0 - 2026-09-11

### Updated

- Raised the required IronSide TE UROT version to 0.2.0.
  The serial DFU image is otherwise unchanged, but UROT 0.2.0 is a backwards incompatible release needing a full IronSide TE reinstall, so this image is not released for a device running UROT 0.1.0.

## 0.1.0 - 2026-08-31

### Added

- Initial release
