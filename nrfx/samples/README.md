## Introduction and configuration

This repository contains sample applications showing the usage examples of the [nrfx drivers] within [Zephyr RTOS].

## Installation

1. Set up Zephyr by following [Getting started] guide.

> Always activate the virtual environment with previously installed `west` and set up `Zephyr`.

## Requirements

Samples support the following development kits:

| **Hardware platforms**   | **Board name**            | **Build target**                |
| ------------------------ | ------------------------- | ------------------------------- |
| nR52 DK                  | nrf52dk_nrf52832          | `nrf52dk_nrf52832`              |
| nRF52833 DK              | nrf52833dk_nrf52833       | `nrf52833dk_nrf52833`           |
| nRF52840 DK              | nrf52840dk_nrf52840       | `nrf52840dk_nrf52840`           |
| nRF5340 DK               | nrf5340dk_nrf5340         | `nrf5340dk_nrf5340_cpuapp`      |
| nRF5340 DK               | nrf5340dk_nrf5340         | `nrf5340dk_nrf5340_cpunet`      |
| nRF54L15 DK              | nrf54l15dk_nrf54l15       | `nrf54l15dk_nrf54l15_cpuapp`    |
| nRF9160 DK               | nrf9160dk_nrf9160         | `nrf9160dk_nrf9160`             |

## Wiring

Samples use a unified system of pins assignment.
Refer to their definitnions in `common/nrfx_example.h`.
By default, pins are connected through different boards as follows:

| **Board name**       | **1A** | **1B**   | **2A**  | **2B** | **3A** | **3B** | **4A** | **4B** |
|--------------------- | ------ | -------- | ------- | ------ | ------ | ------ | ------ | ------ |
| nrf52dk_nrf52832     | P0.03  | P0.31    | P0.04   | P0.30  | P0.28  | P0.29  | P0.26  | P0.27  |
| nrf52833dk_nrf52833  | P0.03  | P0.31    | P0.04   | P0.30  | P0.28  | P0.29  | P0.26  | P0.27  |
| nrf52840dk_nrf52840  | P0.03  | P0.31    | P0.04   | P0.30  | P0.28  | P0.29  | P0.26  | P0.27  |
| nrf5340dk_nrf5340    | P0.04  | P0.26    | P0.05   | P0.25  | P0.06  | P0.07  | P1.02  | P1.03  |
| nrf54l15dk_nrf54l15  | P1.13  | P1.12    | P1.11   | P1.10  | P1.09  | P1.08  | P2.06  | P1.04  |
| nrf9160dk_nrf9160    | P0.14  | P0.19    | P0.15   | P0.18  | P0.16  | P0.17  | P0.30  | P0.31  |

> If a specific sample is using some loopback pins, their corresponding pins should be connected together.
> For example, `LOOPBACK_PIN_1A` should be connected with `LOOPBACK_PIN_1B` and `LOOPBACK_PIN_2A` with `LOOPBACK_PIN_2B`.

@anchor building_and_running
## Building and running

1. Change the directory in `zephyrproject-rtos` repository to one of the samples:
```
cd modules/hal/nordic/nrfx/samples/src/nrfx_timer/timer/
```
2. Build the sample with the `west` on one of the supported boards.
```
west build -b <BOARD>
```
3. Flash the application on the connected target.
```
west flash
```
4. Configure a serial monitor to recognize the port connection with the following parameters:
    - 115200 baud rate,
    - 8 data bits,
    - no parity bit,
    - 1 stop bit,
    - RTS/CTS flow control.
## Directories

```
.
├── common          # Various boards and examples common files
├── doc             # Project documentation files
└── src             # Examples base
    ├── nrfx_cracen # Examples related to CRACEN peripheral
    ├── nrfx_egu    # Examples related to EGU peripheral
    ├── nrfx_gppi   # Examples related to GPPI helper
    ├── nrfx_pwm    # Examples related to PWM peripheral
    ├── nrfx_rng    # Examples related to RNG peripheral
    .
    .
    .
```

@cond DOXYGEN_EXCLUDE
## Generating documentation

Documentation for the samples is available in the `doc/html` folder.
For more details refer to [Documentation].
@endcond

[//]: #
[nrfx drivers]: https://infocenter.nordicsemi.com/topic/struct_drivers/struct/nrfx_latest.html
[Zephyr RTOS]: https://docs.zephyrproject.org/latest/index.html
[Getting started]: https://docs.zephyrproject.org/latest/develop/getting_started/index.html
[Documentation]: doc/README.md

