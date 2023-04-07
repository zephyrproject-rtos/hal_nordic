# RNG {#rng_basic_desc}

The sample demonstrates a basic functionality of the nrfx_rng driver.

## Requirements

The sample supports the following development kits:

| **Board**           | **Support** |
|---------------------|:-----------:|
| nrf52dk_nrf52832    |     Yes     |
| nrf52833dk_nrf52833 |     Yes     |
| nrf52840dk_nrf52840 |     Yes     |
| nrf5340dk_nrf5340   |     Yes     |
| nrf9160dk_nrf9160   |     No      |
## Overview

Application initializes the nrfx_rng driver.
The @p rng_handler() is executed with a relevant log message until a number of generated numbers is equal to the value specified by the user.

> For more information, see **RNG driver - nrfx documentation**.

## Wiring

To run this sample, no special configuration is needed.
You should monitor the output from the board to check if it is as expected.

## Building and running

To run this sample, build it for the appropriate board and then flash it as per instructions in [Building and running](@ref building_and_running) section.

## Testing

You should see the following output:

```
    - "Starting nrfx_rng example"
    - "Generated value: (.*)"
```
> `(.*)` stands for the generated random number [0..255].

[//]: #
[Building and running]: <../../README.md#building-and-running>
