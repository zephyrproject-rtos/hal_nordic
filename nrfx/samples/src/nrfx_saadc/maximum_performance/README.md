# SAADC maxmimum performance {#saadc_maximum_performance}

The sample demonstrates an advanced functionality of the nrfx_saadc driver operating at its peak performance.

## Requirements

The sample supports the following development kits:

| **Board**           | **Support** |
|---------------------|:-----------:|
| nrf52dk_nrf52832    |      No     |
| nrf52833dk_nrf52833 |     Yes     |
| nrf52840dk_nrf52840 |     Yes     |
| nrf5340dk_nrf5340   |     No      |
| nrf9160dk_nrf9160   |     Yes     |

## Overview

Application initializes the nrfx_saadc driver and starts operating in the non-blocking mode.
Sampling is performed at the highest supported frequency.
In the sample @p m_single_channel is configured, and the SAADC driver is set to the advanced mode.
To achieve the maximum performance, do the following:
- Provide an external timer in order to perform sampling at @p MAX_SAADC_SAMPLE_FREQUENCY.
  You can do this by setting up endpoints of the channel @p m_gppi_channels [ @p gppi_channels_purpose_t::SAADC_SAMPLING ] to trigger the SAADC sample task ( @p nrf_saadc_task_t::NRF_SAADC_TASK_SAMPLE ) on the TIMER COMPARE event.
- Provide hardware start-on-end.
  You can do this by setting up endpoints of the channel @p m_gppi_channels [ @p gppi_channels_purpose_t::SAADC_START_ON_END ] to trigger SAADC task start ( @p nrf_saadc_task_t::NRF_SAADC_TASK_START ) on the SAADC event end ( @p nrf_saadc_event_t::NRF_SAADC_EVENT_END ).

@p nrfx_saadc_offset_calibrate triggers calibration in a non-blocking manner.
Then, sampling is initiated at @p NRFX_SAADC_EVT_CALIBRATEDONE event in @p saadc_handler() by calling @p nrfx_saadc_mode_trigger() function.
Consecutive sample tasks are triggered by the external timer at the sample rate specified in @p SAADC_SAMPLE_FREQUENCY symbol.

In the example there is GPPI channel configured to test the functionality of SAADC.
The endpoints are setup up in a way that connects the @p NRF_SAADC_EVENT_RESULTDONE event with the GPIOTE task toggling the @p OUT_GPIO_PIN pin.

> For more information, see **SAADC driver - nrfx documentation**.

## Wiring

To run the sample correctly, connect pins as follows:
* `LOOPBACK_PIN_1A` with `LOOPBACK_PIN_1B`.

## Building and running

To run this sample, build it for the appropriate board and then flash it as per instructions in [Building and running](@ref building_and_running) section.
## Sample output

You should see the following output:

```
- "Starting nrfx_saadc maximum performance example."
- "SAADC event: CALIBRATEDONE"
- "SAADC event: READY"
- "SAADC event: BUF_REQ"
- "SAADC event: DONE"
- "Sample buffer address == 0x([0-9a-z]{8})"
- ".Sample 0. value == -?[0-9]"
- ".Sample 1. value == ([0-9]{3})"
- "SAADC event: BUF_REQ"
- "SAADC event: DONE"
- "Sample buffer address == 0x([0-9a-z]{8})"
- ".Sample 0. value == -?[0-9]"
- ".Sample 1. value == ([0-9]{3})"
- "SAADC event: BUF_REQ"
- "SAADC event: DONE"
- ".Sample 0. value == -?[0-9]"
- ".Sample 1. value == ([0-9]{3})"
```
[//]: #
[Building and running]: <../../../README.md#building-and-running>
