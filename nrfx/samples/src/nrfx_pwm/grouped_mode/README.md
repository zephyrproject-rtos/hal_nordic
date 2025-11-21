@page pwm_grouped_desc PWM grouped mode

# PWM grouped mode

The sample demonstrates a basic functionality of the nrfx_pwm driver for a sequence loaded in the group mode.

## Requirements

The sample supports the following development kits:

| **Board**           | **Support** |  **Number of LEDs expected to blink** |
|---------------------|:-----------:|:-------------------------------------:|
| nrf52dk_nrf52832    |     Yes     |                  4                    |
| nrf52833dk_nrf52833 |     Yes     |                  4                    |
| nrf52840dk_nrf52840 |     Yes     |                  4                    |
| nrf5340dk_nrf5340   |     Yes     |                  4                    |
| nrf54l15dk_nrf54l15 |     Yes     |                  2                    |
| nrf9160dk_nrf9160   |     Yes     |                  4                    |

## Overview

Application initializes the nrfx_pwm driver.
It starts two-sequence playback on LEDs and replays this sequence @p NUM_OF_LOOPS times.
The @p pwm_handler() is executed with a relevant log message after every loop.
Additionally, it changes SEQ1 each time it is called.

> For more information, see **PWM driver - nrfx documentation**.

## Wiring

To run this sample, no special configuration is needed.
You should monitor the output from the board to check if it is as expected.

## Building and running

To run this sample, build it for the appropriate board and then flash it as per instructions in [Building and running](@ref building_and_running) section.

## Sample output

You should see the following output:

```
- Starting nrfx_pwm example for sequences loaded in grouped mode.
- Loops: 1 / 6
- SEQ1 changed to sequence number 2 from the seq array
- Loops: 2 / 6
- SEQ1 changed to sequence number 1 from the seq array
- Loops: 3 / 6
- SEQ1 changed to sequence number 2 from the seq array
- Loops: 4 / 6
- SEQ1 changed to sequence number 1 from the seq array
- Loops: 5 / 6
- SEQ1 changed to sequence number 2 from the seq array
- Loops: 6 / 6
- SEQ1 changed to sequence number 1 from the seq array
- PWM finished
```
Two or four LEDs on the development kit should blink in the following sequence:

@cond DOXYGEN_EXCLUDE
![pwm_grouped_gif](../../../doc/images/pwm_grouped_desc.gif "PWM grouped example")
@endcond

@image html pwm_grouped_desc.gif "PWM grouped example"

[//]: #
[Building and running]: <../../../README.md#building-and-running>
