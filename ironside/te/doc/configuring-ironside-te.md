# Configuring IronSide TE

You can modify the behavior of IronSide TE with the available configuration options.
A bundle carries the following types of configuration:

* Base configuration: You set these options once, and you cannot update them afterwards.
* Image configuration: You can update these options by updating the UROT image.
* Provisioning configuration: These options decide how the programming artifacts of the bundle provision a device.
  They apply when a device is provisioned rather than after it.

You configure verification keys with `update-keys`.
They are not part of the configuration profiles described below, and `update-keys` is the only one of these commands that needs a key source.
Apply the profiles first, because `update-keys` builds the programming artifacts of the bundle for the configuration it finds.

This page contains the following sections:

* [Configuration profiles](#configuration-profiles)
* [Base configuration](#base-configuration)
* [Image configuration](#image-configuration)
* [Provisioning configuration](#provisioning-configuration)

## Configuration profiles

Use `export-config` to write the configuration from a bundle to JSON files, and `import-config` to apply those files to another bundle, for example a newly downloaded release.
There are three file kinds: `ironside-te-config.base.json`, `ironside-te-config.image.json`, and `ironside-te-config.provisioning.json`.
JSON field names are the same as the `update-config` flags without the leading `--`.
The following example exports the configuration from one bundle, imports it into another, and then adds the keys and signs the components:

```sh
scripts/ironside_bundle.py export-config ironside-te.iron.zip \
--base ironside-te-config.base.json --image ironside-te-config.image.json \
--provisioning ironside-te-config.provisioning.json

scripts/ironside_bundle.py import-config ironside-te.iron.zip \
--base ironside-te-config.base.json --image ironside-te-config.image.json \
--provisioning ironside-te-config.provisioning.json
scripts/ironside_bundle.py update-keys ironside-te.iron.zip --keyset keyring:dev
scripts/ironside_bundle.py sign-component ironside-te.iron.zip --all --keyset keyring:dev
```

The base configuration is written when the bundle is installed on the device.
For the first installation, see [Installing IronSide TE](installing-and-updating-firmware.md#installing-ironside-te).
To change the base configuration on a provisioned device, see [Reinstalling IronSide TE](installing-and-updating-firmware.md#reinstalling-ironside-te).

The image configuration is carried in the UROT image.
Apply it with a full installation or reinstallation, or by [updating IronSide TE](installing-and-updating-firmware.md#updating-ironside-te) on an existing device.

The provisioning configuration is carried by the bundle itself and is applied when a device is provisioned.

The initial rollback counters are also provisioning-time values, but they are not configuration options.
Each counter is set from the counter of the image it ships with, so an older image cannot be installed later.

To inspect the configuration carried by a bundle, use `show`.
To also see the rollback counter floors and the rest of the provisioning configuration, use `show -v`.

You can also pass flags directly to `update-config`.
The following sections list them.

## Base configuration

The base configuration options are located in the write-once UICR area.
You must set these options when you install IronSide TE on an empty device, as described in [Installing IronSide TE](installing-and-updating-firmware.md#installing-ironside-te).
The IronSide TE UROT releases include a default configuration that you can use as it is if you do not need any changes.
The `--ikg-seed-select` option defaults to `auto`.
With `auto`, the provisioning firmware sets the IKG SEED selection to User SEED when User SEED is Locked, and to Nordic SEED otherwise.
The provisioning firmware writes the IKG SEED selection to the base configuration, and the device derives its keys from that entry from then on.
For more information, see [User SEED provisioning](#user-seed-provisioning).

To customize the base configuration in a firmware bundle before installation, use `import-config --base` or `update-config`.
The following options are available:

```sh
--slot1-size-kb KB        SLOT1 partition size in kB (multiple of 4)
--storage-size-kb KB      STORAGE partition size in kB (multiple of 4, 0 disables)
--urotstorage-size-kb KB  UROTSTORAGE partition size in kB (multiple of 4, minimum 4)
--dfuimage-size-kb KB     DFUIMAGE partition size in kB (multiple of 4, 0 disables)
--key-slot-storage-size-bytes BYTES  Size of the Secure RAM used to store volatile keys (minimum 1536).
--ikg-seed-select ENTRY   IKG SEED entry the device derives its identity from: nordic, user or auto (default: auto)
```

The base configuration contains the keys used for firmware verification and authenticated operations (AUTHOP).
When you provision keys into the bundle, you choose which set of keys to include with `update-keys`.
The full set of keys in the specified key source is embedded into the configuration, and any role or generation that the source does not hold is revoked in the same write.
To revoke a key role, point `--keyset` at a key source that omits it.

A key source must hold either all three AUTHOP key generations or none.
The `update-keys` command refuses a key source that holds only one or two generations.

For example, to provision a bundle with all the keys in the OS keyring keyset named `dev`, run the following commands:

```sh
scripts/ironside_bundle.py update-keys ironside-te.iron.zip --keyset keyring:dev
scripts/ironside_bundle.py sign-component ironside-te.iron.zip --all --keyset keyring:dev
```

To provision a bundle where the authenticated operation keys are revoked instead of included, use a key source that contains no AUTHOP key, for example an exported directory:

```sh
scripts/ironside_bundle.py update-keys ironside-te.iron.zip --keyset dir:keys_no_authop
scripts/ironside_bundle.py sign-component ironside-te.iron.zip --all --keyset dir:keys_no_authop
```

The partition layout flags of `update-config` partition the non-volatile memory.
You can modify four partition sizes, and a size that you do not give keeps its current value, as in the following example:

```sh
scripts/ironside_bundle.py update-config ironside-te.iron.zip \
--slot1-size-kb 124 \
--storage-size-kb 80 \
--urotstorage-size-kb 4 \
--dfuimage-size-kb 64
```

The official IronSide DFU image is linked for a 32 kB DFUIMAGE, so the 64 kB DFUIMAGE in this example needs a DFU image linked for that size.
`ironside_bundle.py check` and the installation tools refuse a bundle whose DFU image does not start at the DFUIMAGE base.

You can also store the same fields in `ironside-te-config.base.json` and apply them with `import-config --base`.

To inspect the resulting partition layout, run the following command:

```sh
scripts/read_partitions.py base_conf_repartitioned.hex --soc nrf54lm20b
```

```text
IronSide TE partition layout
  source: artifact base_conf_repartitioned.hex
  SoC: nrf54lm20b, NVM 0x001FD000 (2036 kB usable)
  IROT format version: not in artifact

  0x00000000 +--------------------------+    140 kB  reserved     fixed    irot + IronSide TE image slots
  0x00023000 +--------------------------+   1624 kB  slot0        derived  implicit remainder, up to slot1
  0x001B9000 +--------------------------+    124 kB  slot1        uicr     SIZE4KB=31
  0x001D8000 +--------------------------+     80 kB  appstorage   uicr     SIZE4KB=20
  0x001EC000 +--------------------------+      4 kB  urotstorage  uicr     SIZE4KB=1
  0x001ED000 +--------------------------+     64 kB  dfuimage     uicr     SIZE4KB=16
  0x001FD000 +--------------------------+            end of NVM

  UICR PARTITIONS at 0x00FFD2B0: slot1=31, appstorage=20, urotstorage=1, dfuimage=16 (4 kB blocks)

IronSide TE RAM layout
  key slot storage size: 1536 B, the base slots (KEYSLOTSTORAGESIZE not in artifact)

  0x20000000 +--------------------------+     32 kB  reserved    8 key slots
  0x20008000 +--------------------------+    479 kB  APP-RAM
  0x2007FC00 +--------------------------+    736 B   VPRCTX-RAM  not linked
  0x2007FEE0 +--------------------------+    288 B   reserved
  0x20080000 +--------------------------+            end of RAM
```

The output also shows the RAM layout, described in [RAM layout](#ram-layout).

To produce a machine-readable layout, add the `--json` argument.
To read the partition layout from a device instead of from the build artifact, pass the serial number of the device.
To read from several artifacts, pass them all, and the later ones take precedence.

The following sections give further guidelines on how to partition the non-volatile memory.
They also describe how the base configuration sets the RAM layout.

### Partitioning the non-volatile memory

The IronSide TE partition layout specifies the sizes and locations of the DFU image and SLOT1 area, and determines the memory permissions for the current boot.
IronSide TE defines the following partitions for non-volatile memory:

* SLOT0: The application firmware.
* SLOT1: The update candidate partition, which stages updates of the UROT, DFU, and application firmware.
* STORAGE: The storage area of the application and DFU firmware.
* UROTSTORAGE: The secure storage of IronSide TE, which holds the persistent keys and the application and DFU rollback counters.
* DFUIMAGE: The DFU firmware.

You can resize these partitions, depending on the application design, and you can omit STORAGE and DFUIMAGE.

IronSide TE places the partitions from the end of the non-volatile memory down, in the order DFUIMAGE, UROTSTORAGE, STORAGE, and SLOT1.
SLOT0 takes the space between the reserved area and SLOT1.
The following figure shows the fixed addresses, and for each configurable partition the base configuration option that sets its size and its default size.

![Partitions from low to high address: Reserved at 0x00000000, 140 kB, fixed; SLOT0 at 0x00023000, the rest, more than 0; SLOT1, set by --slot1-size-kb, default 128, a multiple of 4 and at least 4; STORAGE, set by --storage-size-kb, default 64, a multiple of 4, where 0 omits it; UROTSTORAGE, set by --urotstorage-size-kb, default 4, a multiple of 4 and at least 4; DFUIMAGE, set by --dfuimage-size-kb, default 32, a multiple of 4, where 0 omits it, ending at the NVM end.](images/ug-nvm-addresses.svg)

The end of the non-volatile memory depends on the part, as the following table shows.

| **Part** | **NVM end** |
| --- | --- |
| nRF54LM20A/B | 0x001FD000 (2036 kB) |
| nRF54LV10A | 0x000FD000 (1012 kB) |
| nRF54L15 | 0x0017D000 (1524 kB) |

For the layout of one configuration, with its addresses and sizes, see the `read_partitions.py` output in [Base configuration](#base-configuration).

Memory permissions can vary with the lifecycle state (LCS) of the device.
The partition layout diagrams in the following sections show a permission in brackets where the partition gains extra access in the Open and PRoT Provisioning states, described in [Lifecycle states](architecture.md#lifecycle-states).
Outside those states, the permission without brackets applies.
For the full permission table for every partition, boot mode, and state, see [Memory isolation](architecture.md#memory-isolation).

The following sections describe two alternative partitioning schemes, defined by the method used to update the application firmware.
They also describe how to size the UROTSTORAGE partition.

#### Single-bank application update partition layout (default)

In the single-bank application update partition layout, the DFU image updates the application firmware in place.
This leaves more space for the application code, because the SLOT1 area only needs to fit update candidates for the smaller UROT and DFU images.
The following figure shows this layout.

![Partitions from low to high address with their permissions: Reserved; SLOT0, RX (App) | RW (DFU), in brackets RWX (App) | RW (DFU); SLOT1, RW; STORAGE, RW; UROTSTORAGE, no access; DFUIMAGE, R (App) | RX (DFU), in brackets RW (App) | RWX (DFU). A note explains that bracketed permissions apply in Open and PRoT Provisioning, and that perm (M) is the permission for boot mode M.](images/single-bank-layout.svg)

To partition the memory according to this layout, complete the following steps.
Set all sizes to a multiple of 4 kB.

1. Set `--slot1-size-kb`, the SLOT1 partition size, to the larger of the UROT partition size and the DFUIMAGE partition size.
   Typically, the UROT partition size is the larger of the two.
2. Set `--storage-size-kb`, the STORAGE partition size, according to the storage needs of the application and DFU images.
3. Set `--urotstorage-size-kb`, the UROTSTORAGE partition size, to fit the persistent keys of the application.
   The minimum is 4 kB, and IronSide TE does not boot with a smaller UROTSTORAGE partition.
   For details, see [Sizing the UROTSTORAGE partition](#sizing-the-urotstorage-partition).
4. Set `--dfuimage-size-kb`, the DFUIMAGE partition size, to fit the DFU image implementation:

   * If you use an official IronSide DFU image, use 32 kB, which is the size the image is linked for.
   * If you use a custom DFU image, reserve enough space to fit that implementation.

The SLOT0 partition automatically takes up any space that remains after the other partitions are allocated.

If you use an official IronSide DFU image, these steps reduce to deciding the sizes of the STORAGE and UROTSTORAGE partitions, because all other sizes are constant.

#### Dual-bank application update partition layout

In the dual-bank application update layout, UROT can update the application firmware from the SLOT1 area.
In this layout, the application firmware must support downloading firmware update candidates and copying them to the update candidate slot.
SLOT1 must be large enough to fit an application update candidate.

The DFU image is optional and can be disabled in this layout, but it is recommended to keep a standalone DFU image as the recovery path.
The following figure shows the layout with the DFU image omitted, that is, with the DFUIMAGE partition size set to 0.

![Partitions from low to high address with their permissions: Reserved; SLOT0, RX, in brackets RWX; SLOT1, RW, equal in size to SLOT0; STORAGE, RW; UROTSTORAGE, no access. There is no DFUIMAGE partition. A note explains that bracketed permissions apply in Open and PRoT Provisioning.](images/dual-bank-layout.svg)

To partition the memory according to this layout, complete the following steps.
Set all sizes to a multiple of 4 kB.

1. Set `--storage-size-kb`, the STORAGE partition size, according to the storage needs of the application.
2. Set `--urotstorage-size-kb`, the UROTSTORAGE partition size, to fit the persistent keys of the application.
   The minimum is 4 kB, and IronSide TE does not boot with a smaller UROTSTORAGE partition.
   For details, see [Sizing the UROTSTORAGE partition](#sizing-the-urotstorage-partition).
3. Set `--dfuimage-size-kb`, the DFUIMAGE partition size, to fit the DFU image implementation, or set it to 0 to disable the DFU image feature.
4. Set `--slot1-size-kb`, the SLOT1 partition size, to the remaining space divided by two.
   Calculate it as follows: SLOT1 size = (total space - reserved size - STORAGE size - UROTSTORAGE size - DFUIMAGE size) / 2, where the total space is the full non-volatile memory size.

The SLOT0 partition takes up the space that remains after the other partitions are allocated, and becomes equal to the SLOT1 size.

#### Sizing the UROTSTORAGE partition

UROTSTORAGE holds the application's persistent keys.
`ironside_bundle.py show` reports the space the provisioned application keys take and the space available to them, and `show -v` adds the space each key takes.
`ironside_bundle.py check` fails a bundle whose application keys do not fit.

> **Note:** Size UROTSTORAGE for both the keys provisioned with the bundle and the persistent keys the application creates at run time.
> The bundle tool counts only the provisioned keys.

To calculate the size by hand, use the following equations.
For a UROTSTORAGE size of S kB, the space available to application keys is:

```text
space = 944 * S - 2000 bytes
```

A bundle provisioned with `update-keys --without-ota-kek` leaves 384 bytes more, `944 * S - 1616` bytes.

Each key takes its key material size M plus 81 bytes, rounded up to a multiple of 16 bytes:

```text
key space = 16 * ceil((M + 81) / 16) bytes
```

M is the size of the key material as imported: the key size in bytes for an AES, ChaCha20 or HMAC key and for an ECC key pair, and 1 + 2 * (key size in bytes) for an uncompressed ECC public key.

| **Key** | **Key material** | **Space taken** |
| --- | --- | --- |
| AES-128 | 16 bytes | 112 bytes |
| AES-256, ECC secp256r1 key pair | 32 bytes | 128 bytes |
| ECC secp256r1 public key | 65 bytes | 160 bytes |
| ECC secp384r1 public key | 97 bytes | 192 bytes |

The keys fit when the sum of the space they take is at most the available space.
For example, a 4 kB UROTSTORAGE has 1776 bytes for application keys, which fit 13 AES-256 keys, and an 8 kB UROTSTORAGE has 5552 bytes, which fit 43.

The equations have the following caveats:

* A destroyed `IRONSIDE_CLIENT_KEY_PERSISTENCE_REVOKABLE` key keeps 64 bytes for the lifetime of the device.
* With keys of different sizes, a few bytes of the available space can remain unusable, so leave a margin.
* A key holds at most 145 bytes of key material.

### RAM layout

IronSide TE divides the RAM into the following areas, from low to high address:

* Reserved: The secure RAM of IronSide TE, which neither the Application nor the DFU image can access.
  It starts at the beginning of RAM, and its size depends on the base configuration.
* APP-RAM: The non-secure RAM of the Application and the DFU image.
  It starts at the end of the secure RAM.
* APP-XRAM: The executable RAM region, which the Application can define at the end of APP-RAM at runtime.
  For details, see [Executing code from RAM](secure-services.md#executing-code-from-ram).
* VPRCTX-RAM and Reserved: On the nRF54LM20A/B and the nRF54LV10A, the last 1 kB of physical RAM, above the App RAM window.
  The non-secure image must not link this area.
  VPRCTX-RAM is non-secure so that the FLPR can save and restore its context there when it hibernates (the MDK's `VPR00_VPRSAVEADDR`).
  The last 288 bytes are reserved.

The following figure shows the RAM layout of the nRF54LM20A/B and the nRF54LV10A, with the Open and PRoT Provisioning permissions in brackets.

![RAM from low to high address with its permissions: Reserved; APP-RAM, RW, in brackets RWX; APP-XRAM, RX; VPRCTX-RAM, RW, in brackets RWX; Reserved. A note explains that bracketed permissions apply in Open and PRoT Provisioning, that APP-XRAM exists only once it is defined at runtime, and that the base configuration sets the size of Reserved.](images/ug-ram-layout.svg)

On the nRF54L15, APP-RAM reaches the end of RAM, and there is no APP-XRAM.
All non-secure RAM is readable, writable, and executable in every lifecycle state.
For the full permission table, see [Memory isolation](architecture.md#memory-isolation).
The MPU feature of the application core is unaffected and can still do the execution permission isolation.
The FLPR core does not have this ability.

The order of the RAM areas is fixed, and the size of the secure RAM sets where APP-RAM starts.
The key slot storage size of the base configuration, `--key-slot-storage-size-bytes`, sets the size of the secure RAM:

```text
secure RAM size in bytes = 31232 + key slot storage size in bytes
```

The default key slot storage size of 1536 bytes gives 32 kB of secure RAM, and the largest size, 42496 bytes, gives 72 kB.
Each byte added to the secure RAM is removed from APP-RAM.
APP-RAM starts at the RAM base address plus the secure RAM size, so build the application for the bundle, as described in [Building application firmware for a bundle](installing-and-updating-firmware.md#building-application-firmware-for-a-bundle).
For how to choose the key slot storage size, see [Sizing the secure volatile key storage](#sizing-the-secure-volatile-key-storage).

The following figure shows where each RAM area starts and what sets its size.
The RAM split granularity of each part is listed in [Sizing the secure volatile key storage](#sizing-the-secure-volatile-key-storage).

![RAM from low to high address: Reserved at 0x20000000, 31232 bytes plus the key slot storage size, which --key-slot-storage-size-bytes sets to 1536 plus a multiple of the RAM split granule, at most 42496; APP-RAM, starting at 0x20008000 by default and at 0x20012000 at the latest, the rest up to APP-XRAM or VPRCTX-RAM; APP-XRAM, the size argument of ironside_devmgmt_xram_region_define(), a non-zero multiple of 4 kB smaller than APP-RAM, only once defined at runtime; VPRCTX-RAM at the RAM end minus 1 kB, 736 bytes, not linked; Reserved at the RAM end minus 288 bytes, 288 bytes. The last three are not on the nRF54L15.](images/ug-ram-addresses.svg)

The end of RAM depends on the part, as the following table shows.

| **Part** | **RAM end** |
| --- | --- |
| nRF54LM20A/B | 0x20080000 |
| nRF54LV10A | 0x20030000 |
| nRF54L15 | 0x20040000 |

APP-RAM starts at 0x20008000 with the default key slot storage size and at 0x20012000 with the largest.
An application that must run with every key slot storage size can place its RAM from 0x20012000.

The following sections describe how to inspect the RAM layout and how to size the secure volatile key storage.

#### Inspecting the RAM layout

To inspect the RAM layout, use one of the following commands:

* `scripts/read_partitions.py`, which prints the RAM layout of a device or of build artifacts below the partition layout, as described in [Base configuration](#base-configuration).
  The key slot storage size is in the UROT image and the partition sizes in the IROT image, so pass the artifacts of both.
* `scripts/read_partitions.py --ram --serial-number <serial number>`, which prints the RAM layout that the device used in its last completed boot.
  It reads the boot report over ADAC, so it needs the AUTHOP key of the keyset, given with `--keyset`.
  It works in every lifecycle state in which the device answers ADAC.
* `scripts/ironside_bundle.py show`, which prints the RAM layout that the base configuration of a bundle gives.

For an example of the output, see [Base configuration](#base-configuration).

#### Sizing the secure volatile key storage

IronSide TE holds 8 volatile key slots in its default 32 kB of secure RAM.
To hold more key slots, increase the key slot storage size with the base configuration option `--key-slot-storage-size-bytes BYTES`.
The secure RAM grows by the bytes above 1536, and IronSide TE uses them for more key slots, as described in [RAM layout](#ram-layout).
The size must be between 1536 and 42496 bytes.
The bytes above 1536 must be a multiple of the granularity of the part, as the following table lists.

| **Part** | **Granularity** |
| --- | --- |
| nRF54LM20 | 32 bytes |
| nRF54LV10A | 32 bytes |
| nRF54L15 | 4096 bytes |

Each key slot takes 192 bytes, and IronSide TE provides as many key slots as fit in the configured size.

For example, `--key-slot-storage-size-bytes 3840` gives 20 slots on the nRF54LM20, but is not valid on a part with a 4096-byte granularity.
`--key-slot-storage-size-bytes 5632` gives 29 slots on every part.
The `show` command prints the size and the slot count it gives.

The size affects the non-secure RAM base, so build the application for the bundle, as described in [Building application firmware for a bundle](installing-and-updating-firmware.md#building-application-firmware-for-a-bundle).

## Image configuration

The image configuration options are part of the UROT image metadata.
You can change these options before signing the UROT image, and update them in the field through a firmware update of UROT.
The IronSide TE UROT releases include a default configuration that you can use as it is if you do not need any changes.

To customize the image configuration in a firmware bundle, use `import-config --image` or `update-config`.
If the UROT image of the bundle is already signed, pass `--resign --keyset <spec>` to `import-config` or `update-config` when you change the image configuration.
The following options are available:

```sh
--dfu-trigger-gpio P<PORT>.<PIN>[,LEVEL][,PULL]
DFU-entry GPIO trigger, e.g. 'P1.8,low' or 'P1.8,low,disabled'
--no-dfu-trigger-gpio clear the GPIO trigger
--dfu-trigger-resetreas R [R ...]
DFU-entry reset-reason trigger, space- or comma-separated:
['dog0', 'dog1', 'lockup', 'resetpin', 'none']
--dfu-trigger-fault   boot IronSide DFU on the boot after a non-secure
			fault, so it can export the fault record
--no-dfu-trigger-fault
			disable the fault trigger
--wdtstart-crv-ms MS  non-secure watchdog timeout in milliseconds (0 for none)
--pof CODE[,noevent]  power-fail comparator threshold, as the raw POFCON
			THRESHOLD code 0 to 15 for this part; add noevent to
			suppress the POFWARN event
--no-pof              clear the power-fail comparator
--board-lfxo CAP_PF[,bypass]
			LFXO crystal load capacitance in picofarads; add
			bypass when an external clock drives LFXO
--board-hfxo PF       HFXO crystal load capacitance in picofarads
--board-vreg MODE     main regulator mode: dcdc or ldo
--authop-revoke-gen GEN
			revoke AUTHOP generation GEN (0, 1 or 2) and every
			older one on each device the UROT update reaches, or
			none
```

A UROT update can revoke AUTHOP key generations on devices in the field when its image configuration includes an AUTHOP revocation setting.
At every boot, UROT reads the setting and revokes the named generation and every older one.
Therefore, a setting of generation 2 revokes every AUTHOP key on the device.
For details, see [Revoking an AUTHOP key](debugger-access.md#revoking-an-authop-key).

### Enabling automatic DFU mode triggers

When the DFU image is present, IronSide TE can boot into DFU mode.
For the full list of mandatory and configurable triggers, see [DFU entry conditions](architecture.md#dfu-entry-conditions).

You can enable additional triggers for entering DFU mode in the image configuration.
The optional triggers improve the fallback capabilities of the system, which increases the chance of recovering from a system error through DFU mode.

The `--dfu-trigger-gpio` option configures a GPIO pin that is sampled at boot and causes DFU mode entry if it is active.
This is useful for triggering DFU mode externally.
The pin is used for this purpose only during the UROT boot procedure, and the application can use it for other purposes.

The `--dfu-trigger-resetreas` option enables DFU mode entry when the system starts with certain reset reasons present.
You can use it as an alternative external method for triggering DFU mode, with the RESETPIN reset reason.
You can also use it to detect and recover from issues in the application firmware, with the watchdog or lockup reset reasons.

The `--dfu-trigger-fault` option enables DFU mode entry on the boot after the application or DFU image faults and leaves a fault record.
The DFU image can then read the fault record from the boot report and act on it.
When this trigger is enabled, the application does not receive the fault record because the boot that reports it starts the DFU image.
The trigger also fires after a fault in the DFU image, so a DFU image that faults on every boot keeps the device in DFU mode.

### Enabling automatic watchdog timer start

IronSide TE can also start the non-secure watchdog timer before it boots the Application or DFU image.
On the initial boot, the non-secure watchdog timer starts before the secure watchdog timer stops.
This ensures that a watchdog timer is always active during the boot process, which makes the system more robust against firmware issues.

To start the watchdog timer automatically, pass the `--wdtstart-crv-ms` option to `update-config` with the initial timeout in milliseconds.
For information about when IronSide TE feeds a watchdog timer that keeps running through a soft reset, see [Non-secure watchdog](architecture.md#non-secure-watchdog).

### Board crystals and regulator

IronSide TE programs the LFXO and HFXO crystal load capacitances and the main regulator mode.
The values depend on the board, so the image configuration must carry all three: `--board-lfxo`, `--board-hfxo`, and `--board-vreg`.
Without them, IronSide TE does not boot the Application or DFU image, and `read_boot_error.py` reports the error `BOARD_UNSET`.
For details, see [Boot failure codes](secure-services.md#boot-failure-codes).

> **Caution:** Set the values of your own board with `update-config` before signing.
> An invalid configuration can make the device unbootable.

The released UROT leaves all three values unset.
The first signing fills any unset value with the nRF54L DK value (17 pF LFXO, 15 pF HFXO, `dcdc`) and prints a warning.
From then on, the bundle carries those values, and later signings do not warn again.

## Provisioning configuration

The provisioning configuration decides how the programming artifacts that a bundle carries provision a device.
It is applied while a device is provisioned, so you cannot change it in the field afterwards.

A release ships the provisioning configuration with its default values in `provisioning.json`, inside the package of the secure provisioning component.
To customize it, use `import-config --provisioning` or `update-config`.
Both commands write `provisioning.user.json` next to the released file, and `provisioning.user.json` overrides it.
You do not need a key source or re-signing, so you can set the configuration on a release bundle before you add any keys to it.

Set the provisioning configuration before you run `update-keys`.
`update-keys` builds the programming artifacts of the bundle for the configuration it finds, so if you change the configuration afterwards, those artifacts remain built for the previous values.
In this case, `update-config` reports it, and `check` reports a failure until you run `update-keys` again.
The following options are available:

```sh
--ap-closure METHOD           access port closure method: authenticated or hardware-fixed
--provision-user-seed CHOICE  whether provisioning enrolls User SEED: no or yes
```

### Access port closure method

Both methods close the non-secure access port, and both disable unauthenticated ERASEALL.
They differ in whether the closure can be lifted:

* `authenticated` engages only the authenticated-debug lock.
  Non-secure debug is blocked by default, and a debugger that holds the matching key can reopen it through an authenticated debug session.
* `hardware-fixed` also writes the write-once `UICR.APPROTECT` registers, so the hardware relocks the non-secure access port on every boot.
  Authenticated debug cannot reopen it, and the choice cannot be reversed on that device.

The configured method applies to every programming artifact that `build-production-hex` writes for the bundle.
To see which method a bundle configures, use `show`.
To confirm that the programming artifacts of the bundle agree with the method, use `check`.
`check` verifies this without a key source, so a party that holds no keys can review a bundle.
`build-production-hex` does not need a key source either, because it programs artifacts that `update-keys` already built.

### User SEED provisioning

The device derives the Identity Attestation Key (IAK), Master Key Encryption Key (MKEK), and Master External Storage Key (MEXT) from one of two CRACEN IKG SEED entries: Nordic SEED or User SEED.
The base configuration option `--ikg-seed-select` selects the entry that the device derives from.
The `--provision-user-seed` option decides whether provisioning enrolls User SEED, and it takes the following values:

* `no` writes no SEED slot and uses the entry the device already has.
  This is the default.
  Provisioning an unseeded device stops before anything is written, with a message to configure the bundle with `update-config --provision-user-seed yes`.
* `yes` enrolls User SEED and leaves Nordic SEED as the device has it, so `--ikg-seed-select` must be `user` or `auto`.
  This option is for a device that has no pre-programmed Nordic SEED.
  This applies to all currently produced devices.

Provisioning never writes an entry that it does not enroll, so an unprogrammed entry stays unprogrammed.
`yes` is permanent once a device is provisioned.
Provisioning with `yes` stops before any SEED slot is written on a device whose User SEED entry cannot be enrolled.
With `--ikg-seed-select auto`, the provisioning firmware sets the IKG SEED selection to User SEED when User SEED is Locked, and to Nordic SEED otherwise.
The entry that is not selected can stay unprogrammed.
Set `nordic` or `user` before you build production programming artifacts.
With `auto`, the state of each device decides which entry it derives its identity from.
`build-production-hex` and the installation tools refuse a bundle that sets `--provision-user-seed yes` and selects `nordic`.
The selection is in the write-once base configuration, so you can change it only by erasing the device and installing again.
