# UARTE double-buffered {#uarte_rx_double_buffered}

The sample demonstrates a functionality of the nrfx_uarte driver utilizing double-buffering feature.

## Requirements

The sample supports the following development kits:

| **Board**           | **Support** |
|---------------------|:-----------:|
| nrf52dk_nrf52832    |     No      |
| nrf52833dk_nrf52833 |     Yes     |
| nrf52840dk_nrf52840 |     Yes     |
| nrf5340dk_nrf5340   |     Yes     |
| nrf9160dk_nrf9160   |     Yes     |

## Overview

Application initializes the nrfx_uarte driver, and using UARTE instance sends a specified message stored in @p m_tx_buffer.
The same instance of UARTE is configured to receive the message.
To perform this operation, different buffers are used, and data is transferred as follows:
* @p m_tx_buffer -> @p m_rx_buffers

To show the double-buffering feature the data is written to different buffers in the @p m_rx_buffers structure.
The @p uarte_handler() is executed with relevant log messages.

> For more information, see **UARTE driver - nrfx documentation**.

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
- "Starting nrfx_uarte RX double-buffered example."
- "Content of TX buffer: Nordic Semiconductor nRF"
- "................................"
- "RX buffer1 - addr: (.*) | content:"
- "RX buffer2 - addr: (.*) | content:"
- "RX buffer3 - addr: (.*) | content:"
- "................................"
- "--> TX done"
- "--> Bytes transfered: 25"
- "................................"
- "RX buffer1 - addr: (.*) | content: Nordic"
- "RX buffer2 - addr: (.*) | content:  nRF"
- "RX buffer3 - addr: (.*) | content:  Secmiconductor"
- "................................"
```
> (.*) stands for memory address

[//]: #
[Building and running]: <../../../README.md#building-and-running>
