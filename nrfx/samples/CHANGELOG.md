# Changelog
All notable changes to this project are documented in this file.

## [3.8.0] - 2024-10-17
### Changed
- Aligned Kconfig definitions to DPPIC multi-instance support in the GPPI and SAADC driver samples.

## [3.7.0] - 2024-09-27
### Fixed
- Fixed missing pinctrl definitions in existing samples for the following drivers: SPIM, TWIM, TWIS.

## [3.4.0] - 2023-03-08
### Changed
- Aligned all samples to Zephyr hardware model v2.

## [3.3.0] - 2023-12-21
### Changed
- Applied nrfx 3.2 changes related to modified GPIOTE driver API to existing samples for the following drivers: GPPI, SAADC.

## [3.2.0] - 2023-10-02
### Changed
- Applied nrfx 3.2 changes to existing samples for the following drivers: GPIOTE, SAADC.
- Replaced routine used for initializing an interrupt handler for an IRQ. Now IRQ_CONNECT is used instead of IRQ_DIRECT_CONNECT.

## [3.0.0] - 2023-04-21
### Changed
- Applied nrfx 3.0 changes to existing samples for the following drivers: GPPI, SAADC, SPIM, TIMER, TWIM, TWIS.
- Removed support for the nRF5340 in the maximum performance SAADC sample.

## [2.11.0] - 2023-04-07
### Added
- Added `pinctrl` definitions to align with pin handling changes introduced in the Zephyr RTOS. Pins utilized by samples are still defined through symbols from the `nrfx_examples.h` file.

### Changed
- Refactored code responsible for enabling interrupts in the Zephyr RTOS environment. Now the dedicated `NRFX_{PERIPH}_INST_HANDLER_GET()` macro is used.

## [2.10.0] - 2022-10-25
### Added
- Added this CHANGELOG.md file.
- Added README.md file with introduction and configuration.
- Added README.md file with description for each sample.
- Added documentation generated using Doxygen tool.
- Added documentation generated using Breathe and Sphinx tools.
- Added sample for the EGU driver.
- Added sample for the RNG driver.
- Added below samples for the TIMER driver:
    - counter
    - timer
- Added below samples for the TEMP driver:
    - blocking
    - non-blocking
- Added below samples for the PWM driver:
    - common mode
    - grouped mode
- Added below samples for the GPPI driver:
    - one-to-one
    - fork
- Added below samples for the SPIM driver:
    - blocking
    - non-blocking
- Added below samples for the SPIM and SPIS drivers:
    - non-blocking
    - advanced non-blocking
- Added below samples for the TWIM and TWIS drivers:
    - tx-rx blocking
    - tx-rx non-blocking
    - tx-rx
    - tx-tx
- Added below samples for the UARTE driver:
    - rx double-buffered
    - tx-rx non-blocking
- Added below samples for the SAADC driver:
    - simple blocking
    - simple non-blocking
    - advanced blocking
    - advanced non-blocking with internal timer
    - maximum performance
