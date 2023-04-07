# EGU {#egu_basic_desc}

The sample demonstrates a basic functionality of the nrfx_egu driver.
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

Application initializes the nrfx_egu driver instance and triggers three subsequent EGU channels.
@p egu_handler() is then executed once for each triggered channel with a relevant log message.

> For more information, see **EGU driver - nrfx documentation**.

## Wiring

To run this sample, no special configuration is needed.
You should monitor the output from the board to check if it is as expected.

## Building and running

To run this sample, build it for the appropriate board and then flash it as per instructions in [Building and running](@ref building_and_running) section.

## Sample output

You should see the following output:

```
- "Starting nrfx_egu example"
- "EGU handler triggered on channel: 0. Context passed to the handler: >Some context<"
- "EGU handler triggered on channel: 1. Context passed to the handler: >Some context<"
- "EGU handler triggered on channel: 2. Context passed to the handler: >Some context<"
```

[//]: #
[Building and running]: <../../README.md#building-and-running>