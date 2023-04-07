# SPIM with SPIS non-blocking {#spim_spis_non_blocking}

The sample demonstrates a basic functionality of nrfx_spim and nrfx_spis drivers operating in the non-blocking mode.

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

Application initializes nrfx_spim and nrfx_spis drivers and starts operating in the non-blocking mode.
@p MSG_TO_SEND_MASTER message from @p m_tx_buffer_master is transmitted to @p m_rx_buffer_slave.
Simultaneously, @p MSG_TO_SEND_SLAVE from @p m_tx_buffer_slave is transmitted to @p m_rx_buffer_master.
When the transactions are completed both @p spim_handler() and @p spis_handler() are executed.
The received messages are then read from @p m_rx_buffer_master and @p m_rx_buffer_slave.

> For more information, see **SPIM driver** and **SPIS driver** - **nrfx documentation**.

## Wiring

To run the sample correctly, connect pins as follows:
* `LOOPBACK_PIN_1A` with `LOOPBACK_PIN_1B`,
* `LOOPBACK_PIN_2A` with `LOOPBACK_PIN_2B`,
* `LOOPBACK_PIN_3A` with `LOOPBACK_PIN_3B`,
* `LOOPBACK_PIN_4A` with `LOOPBACK_PIN_4B`.

> Refer to pin definitions in `common/nrfx_example.h`.

You should monitor the output from the board to check if it is as expected.

## Building and running

To run this sample, build it for the appropriate board and then flash it as per instructions in [Building and running](@ref building_and_running) section.

## Sample output

You should see the following output:

```
- "Starting nrfx_spim basic non-blocking SPIM with SPIS example."
- "SPIM finished. Context passed to the handler: >Some context<"
- "SPIM rx buffer: Nordic"
- "SPIS finished. Context passed to the handler: >Some context<"
- "SPIS rx buffer: Semiconductor"
```

[//]: #
[Building and running]: <../../../README.md#building-and-running>
