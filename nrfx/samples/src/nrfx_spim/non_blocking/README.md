# SPIM basic non-blocking {#spim_basic_non_blocking}

The sample demonstrates a basic functionality of the nrfx_spim driver operating in the non-blocking mode.

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

Application initializes the nrfx_spim driver and starts operating in the non-blocking mode.
A specified message ( @p MSG_TO_SEND ) from @p m_tx_buffer is transmitted.
When the transfer finishes @p spim_handler(), the received message is read from @p m_rx_buffer.

> For more information, see **SPIM driver - nrfx documentation**.

## Wiring

To run the sample correctly, connect pins as follows:
* `LOOPBACK_PIN_1A` with `LOOPBACK_PIN_1B`.

> Refer to pin definitions in `common/nrfx_example.h`.

You should monitor the output from the board to check if it is as expected.

## Building and running

To run this sample, build it for the appropriate board and then flash it as per instructions in [Building and running](@ref building_and_running) section.
## Sample output

You should see the following output:

```
- "Starting nrfx_spim basic non-blocking example."
- "SPIM finished. Context passed to the handler: >Some context<"
- "Message received: Nordic Semiconductor"
```

[//]: #
[Building and running]: <../../../README.md#building-and-running>
