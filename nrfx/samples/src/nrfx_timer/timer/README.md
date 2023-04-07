# TIMER {#timer_basic}

The sample demonstrates a basic functionality of the nrfx_timer driver in the Timer mode.

## Requirements

The sample supports the following development kits:


| **Board**           | **Support** |
|---------------------|:-----------:|
| nrf52dk_nrf52832    |     Yes     |
| nrf52833dk_nrf52833 |     Yes     |
| nrf52840dk_nrf52840 |     Yes     |
| nrf5340dk_nrf5340   |     Yes     |
| nrf9160dk_nrf9160   |     Yes     |

## Overview

Application initializes the nrfx_timer driver.
The @p timer_handler() is executed after a specified time.

> For more information, see **TIMER driver - nrfx documentation**.

## Wiring

To run this sample, no special configuration is needed.
User should monitor the output from the board to check if it is as expected.

## Building and running

To run this sample, build it for the appropriate board and then flash it as per instructions in [Building and running](@ref building_and_running) section.

## Sample output

You should see the following output:

```
    - "Starting nrfx_timer basic example"
    - "Time to wait: 5000 ms"
    - "Timer status: enabled"
    - "Timer finished. Context passed to the handler: >Some context<"
```

[//]: #
[Building and running]: <../../../README.md#building-and-running>
