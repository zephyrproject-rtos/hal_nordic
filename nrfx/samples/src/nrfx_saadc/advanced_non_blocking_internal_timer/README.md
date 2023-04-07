# SAADC advanced non-blocking internal timer {#saadc_advanced_non_blocking_internal_timer}

The sample demonstrates an advanced functionality of the nrfx_saadc driver operating in the non-blocking continuous sampling mode with internal timer.

## Requirements

The sample supports the following development kits:

| **Board**           | **Support** |
|---------------------|:-----------:|
| nrf52dk_nrf52832    |      No     |
| nrf52833dk_nrf52833 |     Yes     |
| nrf52840dk_nrf52840 |     Yes     |
| nrf5340dk_nrf5340   |     Yes     |
| nrf9160dk_nrf9160   |     Yes     |

## Overview

Application initializes the nrfx_saadc driver and starts operating in the non-blocking mode.
In the sample the @p m_single_channel is configured, and the SAADC driver is set to the advanced mode with the @p nrfx_saadc_adv_config_t::start_on_end option enabled.
With that flag, the SAADC driver automatically triggers the @p NRF_SAADC_TASK_START task on the @p NRF_SAADC_EVENT_END event.

@p nrfx_saadc_offset_calibrate triggers calibration in a non-blocking manner.
Upon receiving the @p NRFX_SAADC_EVT_CALIBRATEDONE event in @p saadc_handler(), sampling is initiated by calling @p nrfx_saadc_mode_trigger() function.
Consecutive sample tasks are triggered by the internal timer at the sample rate specified in @p SAADC_SAMPLE_FREQUENCY symbol.

The example features GPPI channel configured to test the functionality of SAADC.
The endpoints are set up in a way that connects the @p NRF_SAADC_EVENT_RESULTDONE event with the GPIOTE task toggling the @p OUT_GPIO_PIN pin.

Internal timer can only be used in the non-blocking mode with only a single input channel enabled.

> For more information, see **SAADC driver - nrfx documentation**.

## Wiring

To run the sample correctly, connect pins as follows:
* `LOOPBACK_PIN_1A` with `LOOPBACK_PIN_1B`.

## Building and running

To run this sample, build it for the appropriate board and then flash it as per instructions in [Building and running](@ref building_and_running) section.

## Sample output

You should see the following output:

```
- "Starting nrfx_saadc advanced non-blocking sampling with internal timer example."
- "SAADC event: CALIBRATEDONE"
- "SAADC event: READY"
- "SAADC event: BUF_REQ"
- "SAADC event: DONE"
- "Sample buffer address == 0x([0-9a-z]{8})"
- ".Sample 0. value == -?[0-9]"
- ".Sample 1. value == ([0-9]{3})"
- ".Sample 2. value == -?[0-9]"
- "SAADC event: BUF_REQ"
- "SAADC event: DONE"
- "Sample buffer address == 0x([0-9a-z]{8})"
- ".Sample 0. value == ([0-9]{3})"
- ".Sample 1. value == -?[0-9]"
- ".Sample 2. value == ([0-9]{3})"
- "SAADC event: BUF_REQ"
- "SAADC event: DONE"
- "Sample buffer address == 0x([0-9a-z]{8})"
- ".Sample 0. value == -?[0-9]"
- ".Sample 1. value == ([0-9]{3})"
- ".Sample 2. value == -?[0-9]"
- "SAADC event: BUF_REQ"
- "SAADC event: DONE"
- "Sample buffer address == 0x([0-9a-z]{8})"
- ".Sample 0. value == ([0-9]{3})"
- ".Sample 1. value == -?[0-9]"
- ".Sample 2. value == ([0-9]{3})"
- "SAADC event: FINISHED"
```

[//]: #
[Building and running]: <../../../README.md#building-and-running>
