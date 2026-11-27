# IronSide TE artifacts

The IronSide TE release artifacts contain everything you need to integrate IronSide TE into your project and device.

This page contains the following sections:

* [Versioning](#versioning)
* [Component archives](#component-archives)
* [IronSide bundle](#ironside-bundle)

## Versioning

The following table describes the version identifiers, using the component archive `ironside-te-urot_fl8_nrf54lm20b-0.3.0.zip` as an example.

| Identifier | Identifies | Format | Example |
| --- | --- | --- | --- |
| Feature level | The IronSide TE feature set of the component release | Integer | `8` |
| Firmware version | The semantic version of the component release on one SoC | `MAJOR.MINOR.PATCH@soc` | `0.3.0@nrf54lm20b` |

For the version format of IronSide Integration Kit releases, see [IronSide Integration Kit version](integration-kit.md#ironside-integration-kit-version).

### Feature level

Every component release has a feature level.
The feature level describes the set of IronSide TE features that are available in the component release.

Within an [IronSide bundle](#ironside-bundle), each IronSide TE component archive records its feature level in its `manifest.json` file.
A component archive released before feature levels existed records none, and its release has feature level 0.
The firmware does not report its feature level at runtime.

An increase in feature level from one component release to the next does not mean that the component has new features.
To see whether a component release adds features, check its changelog and its firmware version.

For the features, the fixed issues, and the component releases by feature level and release date, see [IronSide TE releases](ironside-te-releases.md).
For the feature level that an IronSide Integration Kit release supports, see [Supported feature level](integration-kit.md#supported-feature-level).

### Firmware versions

Each firmware component has its own version on each SoC.
The IronSide Integration Kit tools read and check these versions.
The firmware version identifies the exact firmware in a bug report and the release that fixes a problem.

The version uses semantic versioning in the format `MAJOR.MINOR.PATCH`, where the fields have the following meanings:

* `MAJOR`: Increases when the component changes an existing interface.
* `MINOR`: Increases when the component adds features without changing an existing interface.
* `PATCH`: Increases when the component changes without adding features or changing an interface, for example a bug fix.

When checking the firmware version, pay attention to the following rules:

* The firmware version belongs to one SoC.
  Within an IronSide bundle, the `manifest.json` file of a component archive records the version together with its SoC, for example `0.3.0@nrf54lm20b`.
  The `CHANGELOG.md` file in the archive lists the changes for that SoC only.
* A component can be at different versions on different SoCs, for example `ironside-te-urot` at `0.3.0` for nrf54lm20b and at `0.3.2` for nrf54lv10a.
  The two versions are unrelated, so compare versions only for the same component on the same SoC.
* A component can depend on a minimum version of another component on the same SoC.
  For example, `ironside-te-urot` `0.3.0@nrf54lm20b` requires `ironside-te-irot` `0.2.0@nrf54lm20b` or newer.
  The manifest lists the dependencies, as described in [Manifest file format](#manifest-file-format).
  The IronSide Integration Kit tools refuse a bundle that does not meet them.

## Component archives

The IronSide TE firmware consists of several components.
These components are distributed in **component archives**, which are versioned separately.
The components use the following identifiers:

* `ironside-te-irot`: IronSide TE IROT firmware.
* `ironside-te-urot`: IronSide TE UROT firmware.
* `ironside-te-dfu-serial`: IronSide TE serial DFU firmware.
* `ironside-te-dfu-ble`: IronSide TE Bluetooth LE DFU firmware.
* `ironside-te-prov-secure`: Secure mode provisioning firmware.
* `ironside-te-prov-nonsecure`: Non-secure mode provisioning firmware.
* `ironside-te-app`: Application firmware.
  Nordic does not release this component.
  For how to create it, see [Adding application firmware](SPEC-production-programming.md#32-adding-application-firmware) in the IronSide TE production programming guide.

`ironside-te-dfu-serial` and `ironside-te-dfu-ble` both fill the `dfu` role.
A bundle carries one or the other, never both.

A component archive is a ZIP file that follows a common directory structure.
Its file name carries the component identifier, the feature level, the SoC, and the version.
The file name of an application component archive, or of a component archive released before feature levels existed, has no feature level.
Outside the `release` channel, the file name also carries the channel name, the build date, and the git reference.
The `manifest.json` file inside the archive records the channel and the git reference on every channel.
A component archive has the following structure:

* `<component identifier>[_fl<feature level>]_<soc name>-<semver>[_<channel>_<timestamp>_<ref>].zip`: The file name, for example `ironside-te-urot_fl1_nrf54lm20b-0.1.0.zip`.

  * `README.md`: A short description of the archive contents, in Markdown format.
  * `CHANGELOG.md`: A changelog that describes the history of changes to the component, in Markdown format.
  * `manifest.json`: A top-level metadata file that describes the archive contents, in JSON format.
  * `image.hex`: Firmware data in HEX format, built for `<soc name>`.
  * `resources/`: A directory that contains miscellaneous files associated with the component.

    * `sbom.txt`: A software bill of materials file that lists the software material used by the component.
    * Any component-specific resource files.

The following sections describe what the standard files contain for each component archive.

### The `ironside-te-irot` archive

* `image.hex`: Contains the IROT firmware itself, the `UICR.BOOTCONF` value used to protect the IROT, and `UICR.USERCNF` entries defined by IronSide TE.
* `resources/`: No component-specific files.

### The `ironside-te-urot` archive

* `image.hex`: Contains the UROT firmware and image metadata, without a signature.
  You must sign it before programming, as described in [Installing IronSide TE](installing-and-updating-firmware.md#installing-ironside-te).
* `resources/`: No component-specific files.

### The `ironside-te-dfu-serial` archive

* `image.hex`: Contains the IronSide TE serial DFU firmware and image metadata, without a signature.
  You must sign it before programming, as described in [Installing IronSide TE](installing-and-updating-firmware.md#installing-ironside-te).
* `resources/`: No component-specific files.

### The `ironside-te-dfu-ble` archive

* `image.hex`: Contains the IronSide TE Bluetooth LE DFU firmware and image metadata, without a signature.
  You must sign it before programming, as described in [Installing IronSide TE](installing-and-updating-firmware.md#installing-ironside-te).
* `resources/`: No component-specific files.

### The `ironside-te-prov-secure` archive

* `image.hex`: Contains the IronSide TE secure mode provisioning firmware.
* `resources/`:

  * `interface.cddl`: A CDDL document that describes the CBOR format used by the provisioning firmware.

### The `ironside-te-prov-nonsecure` archive

* `image.hex`: Contains the IronSide TE non-secure mode provisioning firmware.
* `resources/`:

  * `interface.cddl`: A CDDL document that describes the CBOR format used by the provisioning firmware.

### Manifest file format

Each component archive contains a manifest file with additional information about its firmware artifacts.
Every manifest file contains the following fields:

* `name`: The component name.
* `channel`: The channel that the artifact is released on: `release`, `nightly`, or `dev`.
* `ref`: The git reference that the artifact was built from.
* `timestamp`: An ISO 8601 timestamp of the build time of the artifact.
  This field is not present for the `release` channel.
* `soc`: The name of the SoC that this firmware artifact targets.
* `feature_level`: The IronSide TE feature level of the component release, as an integer.
  This field is not present in application component archives or in component archives released before feature levels existed.
* `version`: The ABI that this firmware artifact provides.
* `release_date`: The date that the version was released, as `YYYY-MM-DD`.
  This field is present on the `release` channel only, and not in component archives released before release dates existed.
* `deps`: An object that maps each component identifier that this firmware artifact depends on to the minimum ABI it requires, for example `{"ironside-te-irot": "0.2.0@nrf54lm20b"}`.
  The same SoC at that version or newer satisfies a dependency.
  An empty object means no dependencies.

## IronSide bundle

Several of the IronSide Integration Kit tools expect a ZIP archive file in a specific format known as the **IronSide bundle**.
An IronSide bundle holds the component archives defined by IronSide TE.
You can extend it to contain application component archives, application-specific keys, static data, and configurations.
The bundle file is a simple ZIP archive that contains one or more component archives.
For example, a bundle file with the latest versions of each component for the nRF54LM20B SoC can contain the following files:

* `ironside-te_nrf54lm20b.iron.zip`: The bundle ZIP file.

  * `ironside-te-irot_fl1_nrf54lm20b-0.0.1.zip`
  * `ironside-te-urot_fl1_nrf54lm20b-0.0.1.zip`
  * `ironside-te-dfu-serial_fl1_nrf54lm20b-0.0.1.zip`
  * `ironside-te-prov-secure_fl1_nrf54lm20b-0.0.1.zip`
  * `ironside-te-prov-nonsecure_fl1_nrf54lm20b-0.0.1.zip`

Each member component archive must match the structure used for the component release artifacts.
Every firmware image that a component archive carries is an Intel HEX file, and the tools read the image addresses from it.
A DFU image must start at the DFUIMAGE base given by the partition layout of the bundle, which is the end of the non-volatile memory minus the DFUIMAGE size.
The member file names must start with the component identifier, for example `ironside-te-urot*`.
The tools use this identifier to locate each component.
For how to prepare a bundle and build the production programming artifacts from it, see [Preparing an IronSide bundle](SPEC-production-programming.md#31-preparing-an-ironside-bundle) in the IronSide TE production programming guide.
