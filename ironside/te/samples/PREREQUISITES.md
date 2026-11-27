# Sample build prerequisites

Building sample firmware from source needs a bare-metal toolchain and two upstream libraries.
The IronSide Integration Kit client library in `../client/` and the host scripts in `../scripts/` do not depend on these tools and libraries.
Only the sample build depends on them.

If you only want to try a sample, flash the prebuilt `hello_world_app_nRF54LM20-DK.hex` file in the IronSide Integration Kit root directory, as described in [IronSide Integration Kit quick start](../doc/quick-start.md), and skip this guide.

## Tools

To build the samples, you need the following tools:

- **CMake** >= 3.20 and **Ninja**
- A bare-metal **`arm-none-eabi-gcc`** toolchain, verified with versions 9-2020-q2 and 14.3.rel1

To install the Arm GNU toolchain on x86_64 Linux, run the following commands:

```sh
wget https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz
tar xf arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz
export PATH=$PWD/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin:$PATH
```

For other hosts, see the [Arm GNU toolchain downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).
Instead of adding the toolchain to `PATH`, you can set `IRONSIDE_INTEGRATION_KIT_GCC` to the full path of the compiler.

A bare-metal **RISC-V GNU Compiler Collection (GCC)** toolchain is optional.
Only a sample that runs code on a VPR core, the FLPR, needs it.
Every other sample builds without it, so install it only if you build such a sample.
If you configure such a sample without a RISC-V GCC, the configuration fails with an error message.

You can use any bare-metal toolchain that supports RV32E.
The verified toolchains are `riscv64-unknown-elf-gcc` 10.2.0 and `riscv64-zephyr-elf-gcc` 14.3.0.
The [Zephyr SDK](https://github.com/zephyrproject-rtos/sdk-ng/releases) ships a prebuilt `riscv64-zephyr-elf` toolchain for every host, and the nRF Connect SDK toolchain installs it for you.

The build searches `PATH` for the following compiler names, in this order:

```text
riscv64-zephyr-elf-gcc  riscv64-unknown-elf-gcc  riscv32-unknown-elf-gcc  riscv-none-elf-gcc
```

Instead of adding the toolchain to `PATH`, you can set `IRONSIDE_INTEGRATION_KIT_RISCV_GCC` to the full path of the compiler.

From GCC 12, the control and status register (CSR) instructions that such a sample needs are spelled `rv32emc_zicsr`.
In earlier versions, they are part of the base instruction set architecture (ISA).
The build tries both and uses the one that your compiler accepts.

## nrfx and CMSIS-Core

The sample build links against **nrfx** and **CMSIS-Core**.
The nrfx library provides the drivers, the hardware abstraction layer (HAL), and the MDK startup files.
IronSide Integration Kit does not redistribute these libraries, so you must fetch the pinned versions yourself.

To fetch nrfx and CMSIS-Core, complete the following steps:

1. Open `deps/VERSIONS.txt` in this directory and fetch the exact tags that it lists.
2. Clone each library at its pinned tag:

   ```sh
   # Example - use the tag/fetch lines from deps/VERSIONS.txt for your release
   git clone --depth 1 --branch <pinned-tag> https://github.com/nrfconnect/nrfx deps/nrfx
   git clone --depth 1 --branch <pinned-tag> https://github.com/ARM-software/CMSIS_6 deps/CMSIS_6
   ```

   The `--depth 1` option clones only the tagged commit, without history.
   Alternatively, download the matching `zip =` archives from `deps/VERSIONS.txt` and unpack them under `deps/<name>`.

If you already have nrfx and CMSIS-Core checkouts elsewhere, you can point the build at them instead of fetching new copies.

> **Caution:** Do not use the nrfx copy from an nRF Connect SDK west workspace.
> That copy omits the MDK startup files that the samples link against.

To use your own checkouts, set the following environment variables:

```sh
export IRONSIDE_INTEGRATION_KIT_NRFX=/path/to/nrfx
export IRONSIDE_INTEGRATION_KIT_CMSIS=/path/to/CMSIS_6/CMSIS/Core/Include
```

## Verify and build

To verify the setup, complete the following steps from the IronSide Integration Kit root directory, which is the parent of this directory:

1. Build the `hello_world` sample for the IronSide bundle that the DK is provisioned from:

   ```sh
   python scripts/build.py hello_world --soc nrf54lm20b --bundle ironside-te.iron.zip
   ```

2. Flash the built image:

   ```sh
   python scripts/flash.py samples/build/hello_world/hello_world.hex
   ```

3. View the UART output as described in [Viewing the UART output](README.md#viewing-the-uart-output).

For the full list of samples and the test markers, see [IronSide Integration Kit samples](README.md).
