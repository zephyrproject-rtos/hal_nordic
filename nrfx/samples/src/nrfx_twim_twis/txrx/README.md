# TWIM with TWIS TX-RX {#twim_twis_txrx}

The sample demonstrates a functionality of nrfx_twim and nrfx_twis drivers, when TWI transfer is performed in the TX-RX mode.

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

Application initializes nrfx_twim and nrfx_twis drivers.
The main purpose of this sample is to emulate a situation, in which you read data from particular registers of the slave (usual sensor -> controller communication using two wire interface).
The sample works as a simple simulation in which you can communicate with a drone by reading the values from its individual registers.
* @p m_drone_reg is used as an internal register map of the drone.
  In the first operation, which is the write operation (TX), define a register number from which the data will be read in the read operation (RX).
* @p twim_handler() is executed with relevant log messages.
* @p twis_handler() is used to prepare data for receiving and sending.
* @p register_read() function is used to write data from particular registers to buffer in master device and print content of that buffer.

> For more information, see **TWIM driver** and **TWIS driver** - **nrfx documentation**.
## Wiring

To run the sample correctly, connect pins as follows:
* `LOOPBACK_PIN_1A` with `LOOPBACK_PIN_1B`
* `LOOPBACK_PIN_2A` with `LOOPBACK_PIN_2B`

> Refer to pin definitions in `common/nrfx_example.h`.

You should monitor the output from the board to check if it is as expected.

## Building and running

To run this sample, build it for the appropriate board and then flash it as per instructions in [Building and running](@ref building_and_running) section.

## Sample output

You should see the following output:

```
- "Starting nrfx_twim_twis txrx example."
- "................................"
- "DRONE REGISTERS:    DEC  |  HEX"
- "drone_idx:          1    |  0x1"
- "drone_mass:         10   |  0xA"
- "x_force:            50   |  0x32"
- "y_force:            100  |  0x64"
- "z_force:            150  |  0x96"
- "x_acceleration:     5    |  0x5"
- "y_acceleration:     10   |  0xA"
- "z_acceleration:     15   |  0xF"
- "additional_data[0]: 239  |  0xEF"
- "additional_data[1]: 190  |  0xBE"
- "additional_data[2]: 173  |  0xAD"
- "additional_data[3]: 222  |  0xDE"
- "................................"
- "Reading value of DRONE_REG_FORCE_X register:"
- "--> Master event: done - transfer completed"
- "ctrl buff: 0x32"
- "..."
- "Reading value of DRONE_REG_ACC_Y register:"
- "--> Master event: done - transfer completed"
- "ctrl buff: 0xA"
- "..."
- "Reading values of DRONE_REG_DATA registers:"
- "--> Master event: done - transfer completed"
- "ctrl buff: 0xDEADBEEF"
- "..."
```

[//]: #
[Building and running]: <../../../README.md#building-and-running>
