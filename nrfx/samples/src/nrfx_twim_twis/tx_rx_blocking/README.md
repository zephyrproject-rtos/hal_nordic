# TWIM with TWIS blocking {#twim_twis_tx_rx_blocking}

The sample demonstrates a functionality of nrfx_twim and nrfx_twis drivers operating in the blocking mode.

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

Application initializes nrfx_twim and nrfx_twis drivers and sends a specified message ( @p MSG_TO_SEND ) from master to slave and back using TWIM and TWIS peripherals.
To perform this operation, different buffers are used and data is transferred as follows:
@p m_tx_buffer_master -> @p m_rx_buffer_slave -> @p m_rx_buffer_master

> For more information, see **TWIM driver** and **TWIS driver** - **nrfx documentation**.

## Wiring

To run the sample correctly, connect pins as follows:
* `LOOPBACK_PIN_1A` with `LOOPBACK_PIN_1B`.
* `LOOPBACK_PIN_2A` with `LOOPBACK_PIN_2B`

> Refer to pin definitions in `common/nrfx_example.h`.

You should monitor the output from the board to check if it is as expected.

## Building and running

To run this sample, build it for the appropriate board and then flash it as per instructions in [Building and running](@ref building_and_running) section.

## Sample output

You should see the following output:

```
- "Starting nrfx_twim_twis blocking example."
- "Message to send: Nordic Semiconductor"
- "Content of slave RX buffer:  "
- "Content of master RX buffer: "
- "m_tx_buffer_master -> m_rx_buffer_slave -> m_rx_buffer_master"
- "Content of slave RX buffer:  Nordic Semiconductor"
- "Content of master RX buffer: Nordic Semiconductor"
```

[//]: #
[Building and running]: <../../../README.md#building-and-running>
