# TIMER counter {#timer_counter}

The sample demonstrates a basic functionality of the nrfx_timer driver in Timer and Counter modes.

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
The @p timer_handler() is executed in the following situations:
* Regularly after a specified time (@p TIME_TO_WAIT_MS).
* When the counter value is equal to @p COUNTER_MAX_VAL.

> For more information, see **TIMER driver - nrfx documentation**.

## Wiring

To run this sample, no special configuration is needed.
You should monitor the output from the board to check if it is as expected.

## Building and running

To run this sample, build it for the appropriate board and then flash it as per instructions in [Building and running](@ref building_and_running) section.

## Sample output

You should see the following output:

```
- "Starting nrfx_timer basic counter example."
- "Time between timer ticks: 1000 ms"
- "Timer status: enabled"
- "Counter status: enabled"
- "Timer: 1"
- "Timer: 2"
- "Timer: 3"
- "Counter: 1 / 3"
- "Timer: 1"
- "Timer: 2"
- "Timer: 3"
- "Counter: 2 / 3"
- "Timer: 1"
- "Timer: 2"
- "Timer: 3"
- "Counter finished"
- "Counter: 3 / 3"
- "Timer status: disabled"
- "Counter status: disabled"
```

[//]: #
[Building and running]: <../../../README.md#building-and-running>
