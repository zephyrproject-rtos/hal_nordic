# TWIM with TWIS TX-TX {#twim_twis_txtx}

The sample demonstrates a functionality of nrfx_twim and nrfx_twis drivers, when TWI transfer is performed in the TX-TX mode.

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
The main purpose of this sample is to emulate a situation, in which you write data to a particular register of the slave (usual controller -> sensor communication using two wire interface).
The sample works as a simple simulation in which you can communicate with a drone by defining X, Y, Z forces and sending some additional data.
* @p m_drone_reg is used as an internal register map of the drone.
  In the first write operation (TX1), define a register number to which data from the second write operation (TX2) will be saved.
* @p twim_handler() is executed with relevant log messages.
* @p twis_handler() is used to prepare data for sending and updating particular registers of @p m_drone_reg.

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
- "Starting nrfx_twim_twis txtx example."
- "..............................."
- "DRONE REGISTERS:    DEC  |  HEX"
- "drone_idx:          1    |  0x1"
- "drone_mass:         10   |  0xA"
- "x_force:            0    |  0x0"
- "y_force:            0    |  0x0"
- "z_force:            0    |  0x0"
- "x_acceleration:     0    |  0x0"
- "y_acceleration:     0    |  0x0"
- "z_acceleration:     0    |  0x0"
- "additional_data[0]: 0    |  0x0"
- "additional_data[1]: 0    |  0x0"
- "additional_data[2]: 0    |  0x0"
- "additional_data[3]: 0    |  0x0"
- "..............................."
- "MASTER EVENTS:"
- "--> Master event: done - transfer completed"
- "--> Master event: done - transfer completed"
- "--> Master event: done - transfer completed"
- "--> Master event: done - transfer completed"
- "TXTX transfers finished."
- "..............................."
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
```

[//]: #
[Building and running]: <../../../README.md#building-and-running>
