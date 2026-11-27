# Limiting debugger access

To restrict debugger access, transition the device to a lifecycle state that limits it.
For the access that each lifecycle state allows, see [Lifecycle states](architecture.md#lifecycle-states).
Before you limit debugger access, read [Backing up the keyset](#backing-up-the-keyset).

This page contains the following sections:

* [Backing up the keyset](#backing-up-the-keyset)
* [Protecting secure assets](#protecting-secure-assets)
* [Protecting all assets](#protecting-all-assets)
* [Removing protection](#removing-protection)
* [Using authenticated debug](#using-authenticated-debug)
* [Revoking an AUTHOP key](#revoking-an-authop-key)

## Backing up the keyset

> **Caution:** If you lose the keyset, the steps on this page can brick the device.

Back up the keyset before you continue.
The following command exports the keyset from the OS keyring.
The export exposes secret values, so do it only for development keysets.

```sh
# This will store secrets in plaintext
scripts/iron_keychain.py export --out-dir super-safe-backup-dir dev
```

## Protecting secure assets

The following command moves the device to the PRoT Provisioning state.
It removes access to the secure and AUX access ports, but keeps the non-secure access port open.
It also disables the unauthenticated ERASEALL operation.

You can use this state to allow development of non-secure firmware.

```text
# Only do this if you have control of the keyset installed on the device
scripts/ironside_protect.py ironside-te.iron.zip --keep-nonsecure-ap-open
>   ...
> verified: prot
```

## Protecting all assets

The following command moves the device to the Secured state.
It removes access to all access ports and disables the unauthenticated ERASEALL operation.

You can use this state to mimic a deployed device.

```text
scripts/ironside_protect.py ironside-te.iron.zip
> IronSide TE protect  (open -> secured)
>   ...
> verified: psa_lifecycle 0x3000 (secured)
```

## Removing protection

To remove all protection, reinstall IronSide TE.
This is an authenticated operation that triggers an ERASEALL operation over ADAC.
For details, see [Erasing a protected device](#erasing-a-protected-device).

```sh
scripts/ironside_install.py ironside-te.iron.zip --force --keyset keyring:dev
```

After the reinstallation, the device is back in the **Open** LCS.

## Using authenticated debug

The following sections describe the ADAC operations that the `scripts/adac.py` tool performs.
For the ADAC protocol and the commands that IronSide TE supports, see [Authenticated debug](architecture.md#authenticated-debug).
All `adac.py` commands take the `-s SERIAL` option to select the debug probe and the `--json` option for machine-readable output.

### Reading the lifecycle state

`discovery` works in every lifecycle state and needs no key.
It confirms that IronSide TE is installed and reports the lifecycle state of the device, even on a device whose access ports are closed.

```sh
scripts/adac.py discovery
```

### Debugging a protected device

On a device whose non-secure access port is closed with the `authenticated` access port closure method, an authenticated session reopens that port.
The grant opens only that port, so the application can be debugged as in the PRoT Provisioning state while the secure and AUX access ports stay closed.
It survives ordinary resets and ends with `lock-debug` or with a power-on reset.

```sh
scripts/adac.py auth --keyset keyring:dev
scripts/adac.py lock-debug
```

This is the path for debugging a production-configured device during development, and for servicing a returned device without erasing it.

### Erasing a protected device

`erase-all` authenticates and then runs ERASEALL on a device whose erase protection is engaged.
It returns the device to an erased state from which IronSide TE can be installed again.
`ironside_install.py --force` performs this command when it reinstalls a protected device.
For details, see [Removing protection](#removing-protection).

```sh
scripts/adac.py erase-all --keyset keyring:dev
```

For a device that is not protected, `recover` performs the unauthenticated CTRL-AP ERASEALL and needs neither a key nor an installed IronSide TE:

```sh
scripts/adac.py recover
```

### Selecting the next boot target

`boot-mode` needs no key and selects the non-secure image that IronSide TE boots after the reset that leaves ADAC.
It is the debugger-side way to enter the DFU image for a recovery update.
For details, see [Entering DFU through ADAC](architecture.md#entering-dfu-through-adac).

```sh
scripts/adac.py boot-mode dfu
```

### Decommissioning a device

`lcs-change decommissioned` authenticates and moves the device to the terminal Decommissioned state.
On every following boot, IronSide TE then closes the debugger paths, revokes the keys that verify the application, the IronSide DFU image and ADAC authentication, and erases the non-volatile memory above its own firmware and the non-secure RAM.
ADAC keeps answering afterwards: `discovery` reports Decommissioned, and authentication fails because its keys are revoked.
Without an ADAC request, the device halts and the CTRL-AP mailbox holds the boot failure code `DECOMMISSIONED` (STAGE 1, `0x85`), see [Boot failure codes](secure-services.md#boot-failure-codes).
The first boot after `lcs-change` can take up to about 30 seconds before ADAC answers or the code appears, because it erases up to about 2 MB of non-volatile memory.
Use it to retire a device at end of life so that no assets remain on it.

```sh
scripts/adac.py lcs-change decommissioned --keyset keyring:dev
```

## Revoking an AUTHOP key

To remove debugger access for an authenticated operation (AUTHOP) key generation on devices in the field, revoke the generation with an update of the updatable root of trust (UROT) over DFU.
The update revokes the generation it names and every older one.
UROT applies the revocation on the first boot after the update, before it can enter ADAC mode.
It also withdraws any non-secure debug grant that an earlier ADAC authentication made, whichever key generation authenticated it.
The reset that completes the update therefore ends every open debug session, and from that reset on, ADAC refuses every revoked generation.

> **Caution:** IronSide TE does not refuse an AUTHOP revocation setting.
> Revoking generation 2 revokes every AUTHOP key on each device that receives the update, and the revocation cannot be undone.
> In the PRoT Provisioning and Secured states, such a device cannot be debug-unlocked, erased with `erase-all`, or decommissioned, because it cannot authenticate over ADAC.

To revoke an AUTHOP key generation, complete the following steps:

1. Update the UROT image configuration to revoke the target generation, and re-sign the UROT image:

   ```sh
   scripts/ironside_bundle.py update-config ironside-te.iron.zip --authop-revoke-gen 0 --resign --keyset keyring:dev
   ```

2. Verify that the image configuration contains the revocation setting:

   ```text
   scripts/ironside_bundle.py show ironside-te.iron.zip | grep authop-revoke
   > authop-revoke-gen:         0
   ```

3. Deploy the update:

   ```sh
   scripts/ironside_update.py ironside-te.iron.zip --component urot --keyset keyring:dev
   ```

4. Check the result by authenticating over ADAC with each generation.
   Authentication with the revoked generation fails, and authentication with a higher generation succeeds:

   ```text
   scripts/adac.py auth --keyset keyring:dev --authop-gen 0
   > challenge: f2dda9bc33ec9ee2db1102fd497b1444e8b3d6bc50e0bdc2ee01c879bc69ac02
   > auth failed at root -> FAILURE

   scripts/adac.py auth --keyset keyring:dev --authop-gen 1
   > challenge: 2c30ff1f1cd7c19196f0b3be656dff4dc808d20b52cee418ec22b640e3801ab2
   > authenticated, effective_permissions: ffffffffffffffffffffffffffffffff
   ```

> **Caution:** The revocation setting must be included in every later UROT release.
> A device that is offline applies the setting from the latest release.
> If the revocation setting was deployed in an earlier release and the latest no longer carries it, a device will miss the setting.
> When you replace the UROT component of the bundle, leave out `--drop-config` to carry the setting over to the new UROT.
