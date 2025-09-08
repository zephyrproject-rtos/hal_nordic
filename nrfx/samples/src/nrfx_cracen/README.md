# CRACEN {#cracen_basic_desc}

The sample demonstrates a basic functionality of the nrfx_cracen driver.

## Requirements

The sample supports the following development kits:

| **Board**           | **Support** |
|---------------------|:-----------:|
| nrf52dk_nrf52832    |     No      |
| nrf52833dk_nrf52833 |     No      |
| nrf52840dk_nrf52840 |     No      |
| nrf5340dk_nrf5340   |     No      |
| nrf54l15dk_nrf54l15 |     Yes     |
| nrf9160dk_nrf9160   |     No      |

## Overview

Application initializes nrfx_cracen driver and fills a buffer with random data.

> For more information, see **CRACEN driver - nrfx documentation**.

## Wiring

To run this sample, no special configuration is needed.
You should monitor the output from the board to check if it is as expected.

## Building and running

To run this sample, build it for the appropriate board and then flash it as per instructions in [Building and running](@ref building_and_running) section.

## Testing

You should see the following output:

```
    - "Starting nrfx_cracen example"
    - "Generated value: (.*)"
```
> `(.*)` stands for the generated random number [0..255].

[//]: #
[Building and running]: <../../README.md#building-and-running>
