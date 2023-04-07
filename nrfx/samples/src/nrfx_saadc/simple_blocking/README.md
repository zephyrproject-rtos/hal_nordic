# SAADC simple blocking {#saadc_simple_blocking}

The sample demonstrates a simple functionality of the nrfx_saadc driver operating in the blocking mode.

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

Application initializes the nrfx_saadc driver and starts operating in the blocking mode.
Program works as a simple state machine and starts in state @p m_current_state == STATE_SINGLE_CONFIG.
In this state the following configurations apply:
- STATE_SINGLE_CONFIG - SAADC driver is configured to work with only one channel ( @p m_single_channel ) in the blocking mode.
  Afterward, @p m_current_state changes to STATE_SINGLE_SAMPLING.
- STATE_SINGLE_SAMPLING - sampling on a single channel ( @p m_single_channel ) is performed specified number of times ( @p SAMPLING_ITERATIONS ), and then @p m_current_state changes to STATE_MULTIPLE_CONFIG.
- STATE_MULTIPLE_CONFIG - SAADC driver is configured to work with multiple channels ( @p m_multiple_channels ) in the blocking mode.
  Afterward, @p m_current_state changes to STATE_MULTIPLE_SAMPLING.
- STATE_MULTIPLE_SAMPLING - sampling on multiple channels ( @p m_multiple_channels ) is performed specified number of times ( @p SAMPLING_ITERATIONS ).

Before every sampling a calibration in the blocking manner is performed.
Sampling is then invoked by the @p nrfx_saadc_mode_trigger() function.

In the sample there are GPIOTE tasks configured to toggle the specified ( @p m_out_pins ) loopback pins.
Those tasks are triggered between successive samplings to verify the functionality of the SAADC on the nonconstant analog signal.

> For more information, see **SAADC driver - nrfx documentation**.

## Wiring

To run the sample correctly, connect pins as follows:
* `LOOPBACK_PIN_1A` with `LOOPBACK_PIN_1B`,
* `LOOPBACK_PIN_2A` with `LOOPBACK_PIN_2B`,
* `LOOPBACK_PIN_3A` with `LOOPBACK_PIN_3B`.

## Building and running

To run this sample, build it for the appropriate board and then flash it as per instructions in [Building and running](@ref building_and_running) section.

## Sample output

You should see the following output:
```
- "Starting nrfx_saadc simple blocking example."
- "Single channel SAADC test."
- "Calibration in the blocking manner finished successfully."
- "Sampling 1 / 8"
- "[CHANNEL 0] Sampled value == ([0-9]{3})"
- "Calibration in the blocking manner finished successfully."
- "Sampling 2 / 8"
- "[CHANNEL 0] Sampled value == -?[0-9]"
- "Calibration in the blocking manner finished successfully."
- "Sampling 3 / 8"
- "[CHANNEL 0] Sampled value == ([0-9]{3})"
- "Calibration in the blocking manner finished successfully."
- "Sampling 4 / 8"
- "[CHANNEL 0] Sampled value == -?[0-9]"
- "Calibration in the blocking manner finished successfully."
- "Sampling 5 / 8"
- "[CHANNEL 0] Sampled value == ([0-9]{3})"
- "Calibration in the blocking manner finished successfully."
- "Sampling 6 / 8"
- "[CHANNEL 0] Sampled value == -?[0-9]"
- "Calibration in the blocking manner finished successfully."
- "Sampling 7 / 8"
- "[CHANNEL 0] Sampled value == ([0-9]{3})"
- "Calibration in the blocking manner finished successfully."
- "Sampling 8 / 8"
- "[CHANNEL 0] Sampled value == -?[0-9]"
- "Multiple channels SAADC test."
- "Calibration in the blocking manner finished successfully."
- "Sampling 1 / 8"
- "[CHANNEL 0] Sampled value == ([0-9]{3})"
- "[CHANNEL 1] Sampled value == ([0-9]{3})"
- "[CHANNEL 2] Sampled value == ([0-9]{3})"
- "Calibration in the blocking manner finished successfully."
- "Sampling 2 / 8"
- "[CHANNEL 0] Sampled value == -?[0-9]"
- "[CHANNEL 1] Sampled value == -?[0-9]"
- "[CHANNEL 2] Sampled value == -?[0-9]"
- "Calibration in the blocking manner finished successfully."
- "Sampling 3 / 8"
- "[CHANNEL 0] Sampled value == ([0-9]{3})"
- "[CHANNEL 1] Sampled value == ([0-9]{3})"
- "[CHANNEL 2] Sampled value == ([0-9]{3})"
- "Calibration in the blocking manner finished successfully."
- "Sampling 4 / 8"
- "[CHANNEL 0] Sampled value == -?[0-9]"
- "[CHANNEL 1] Sampled value == -?[0-9]"
- "[CHANNEL 2] Sampled value == -?[0-9]"
- "Calibration in the blocking manner finished successfully."
- "Sampling 5 / 8"
- "[CHANNEL 0] Sampled value == ([0-9]{3})"
- "[CHANNEL 1] Sampled value == ([0-9]{3})"
- "[CHANNEL 2] Sampled value == ([0-9]{3})"
- "Calibration in the blocking manner finished successfully."
- "Sampling 6 / 8"
- "[CHANNEL 0] Sampled value == -?[0-9]"
- "[CHANNEL 1] Sampled value == -?[0-9]"
- "[CHANNEL 2] Sampled value == -?[0-9]"
- "Calibration in the blocking manner finished successfully."
- "Sampling 7 / 8"
- "[CHANNEL 0] Sampled value == ([0-9]{3})"
- "[CHANNEL 1] Sampled value == ([0-9]{3})"
- "[CHANNEL 2] Sampled value == ([0-9]{3})"
- "Calibration in the blocking manner finished successfully."
- "Sampling 8 / 8"
- "[CHANNEL 0] Sampled value == -?[0-9]"
- "[CHANNEL 1] Sampled value == -?[0-9]"
- "[CHANNEL 2] Sampled value == -?[0-9]"
```
[//]: #
[Building and running]: <../../../README.md#building-and-running>
