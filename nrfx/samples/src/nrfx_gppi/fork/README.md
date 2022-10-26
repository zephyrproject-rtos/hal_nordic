# GPPI fork example overview {#gppi_basic_fork_desc}

The sample demonstrates a basic fork functionality of the nrfx_gppi helper.
## Requirements

The sample supports the following development kits:

| **Board**           | **Support** | **Driver** |
|---------------------|:-----------:|:----------:|
| nrf52dk_nrf52832    |     Yes     |     PPI    |
| nrf52833dk_nrf52833 |     Yes     |     PPI    |
| nrf52840dk_nrf52840 |     Yes     |     PPI    |
| nrf5340dk_nrf5340   |     Yes     |    DPPI    |
| nrf9160dk_nrf9160   |     Yes     |    DPPI    |

## Overview

Application initializes nrfx_gpiote, nrfx_timer drivers and the nrfx_gppi helper in a way that sets up the TIMER COMPARE event to be forwarded via PPI/DPPI to GPIOTE and to toggle a pin.
Fork mechanism in PPI/DPPI is also set up to simultaneously toggle a second pin in GPIOTE.

> For more information, see **GPPI helper - nrfx documentation**.

## Wiring

To run this sample, no special configuration is needed.
You should monitor the output from the board to check if it is as expected.

## Building and running

To run this sample, build it for the appropriate board and then flash it as per instructions in [Building and running](@ref building_and_running) section.

## Sample output

You should see the following output:

```
- "Starting nrfx_gppi basic fork example."
- "GPIOTE status: initialized"
- "Time to wait: 1000 ms"
- "Timer status: enabled"
- "Timer finished. Context passed to the handler: >Some context<"
- "GPIOTE output pin .primary.: <LED1_PIN> is low"
- "GPIOTE output pin .fork.: <LED2_PIN> is high"
- "Timer finished. Context passed to the handler: >Some context<"
- "GPIOTE output pin .primary.: <LED1_PIN> is high"
- "GPIOTE output pin .fork.: <LED2_PIN> is low"
```

> In the output, instead of `<LED1_PIN>` and `<LED2_PIN>`, you should see the corresponding pin numbers defined in `common/nrfx_example.h`.
