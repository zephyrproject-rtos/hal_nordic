# TEMP blocking {#temp_blocking}

The sample demonstrates a functionality of the nrfx_temp driver operating in the blocking mode.
## Requirements

The sample supports the following development kits:

| **Board**           | **Support** |
|---------------------|:-----------:|
| nrf52dk_nrf52832    |     Yes     |
| nrf52833dk_nrf52833 |     Yes     |
| nrf52840dk_nrf52840 |     Yes     |
| nrf5340dk_nrf5340   |     Yes     |
| nrf9160dk_nrf9160   |      No     |
## Overview

Application initializes the nrfx_temp driver and starts operating in the blocking mode.
Temperature is measured with relevant messages logged until a number of measurements performed is equal to the value specified by the user.

> For more information, see **TEMP driver - nrfx documentation**.

## Wiring
To run this sample, no special configuration is needed.
You should monitor the output from the board to check if it is as expected.

## Building and running

To run this sample, build it for the appropriate board and then flash it as per instructions in [Building and running](@ref building_and_running) section.

## Sample output

You should see the following output:

```
- "Starting nrfx_temp blocking example."
- "Measured temperature: <temp_value> [C]"
- "Measured temperature: <temp_value> [C]"
- "Measured temperature: <temp_value> [C]"
- "Measured temperature: <temp_value> [C]"
- "Measured temperature: <temp_value> [C]"
```
> In the output, `temp_value` should be displayed as a temperature value in Celsius rounded to two decimal places.

[//]: #
[Building and running]: <../../../README.md#building-and-running>
