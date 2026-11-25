# IronSide Integration Kit

**IronSide Integration Kit** is the standalone kit for integrating **IronSide Trusted Execution** (IronSide TE) into an application.
It does not depend on a specific real-time operating system (RTOS) or SDK.
It contains host-side tools, OS-agnostic C libraries, and application firmware samples.

**IronSide TE** is a set of prebuilt firmware images that provides a secure-by-default security solution for applications.

## IronSide Integration Kit contents

The IronSide Integration Kit root directory organizes these tools, libraries, and samples as follows:

| Path | Contents |
| --- | --- |
| `doc/` | Quick start, user guide, production programming guide, and their diagrams |
| `client/` | C library for calling IronSide TE secure services from the application |
| `scripts/` | Host tools for building, signing, provisioning, and device firmware updates |
| `scripts/internal/` | Lower-level tooling that the tools in `scripts/` run. This tooling is not part of the supported tooling, and the documented flows never call it directly. |
| `samples/` | Bare-metal application firmware samples and their build system |
| `hello_world_app_nRF54LM20-DK.hex`, `hello_world_app_nRF54LV10-DK.hex`, `hello_world_app_nRF54L15-DK.hex` | Prebuilt `hello_world` sample, one per development kit (DK) |

## Getting started

To bring up a new device and run the prebuilt `hello_world` sample on it, follow the steps in the [IronSide Integration Kit quick start](doc/quick-start.md).

For the reference documentation on IronSide Integration Kit and IronSide TE, see the [IronSide Trusted Execution User Guide](doc/README.md).
