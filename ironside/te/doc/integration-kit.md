# IronSide Integration Kit versioning

This page describes the IronSide Integration Kit version and the IronSide TE feature level that an IronSide Integration Kit release supports.

This page contains the following sections:

* [IronSide Integration Kit version](#ironside-integration-kit-version)
* [Supported feature level](#supported-feature-level)

## IronSide Integration Kit version

The IronSide Integration Kit version identifies the IronSide Integration Kit release that you install and update.
IronSide Integration Kit releases are versioned by date, in the format `YYYY.MM.counter`.
`MM` is the month and `counter` is a release counter, both with two digits, so that a listing of release names sorts by date.
`counter` starts from `00` in every `YYYY.MM` and increases by one with each release in that month, as in the following examples:

* `ironside-integration-kit_2026.09.00`: The first release in September 2026.
* `ironside-integration-kit_2027.12.12`: The thirteenth release in December 2027.

IronSide Integration Kit releases up to and including `ironside-integration-kit_2026.10.1` have a counter with one digit.

## Supported feature level

The supported feature level of an IronSide Integration Kit release is the highest feature level that the release supports.
The client library and the host-side tools of the release support the features added in that feature level and in every earlier feature level.
The IronSide Integration Kit records the supported feature level as `supported_feature_level` in its `manifest.json` file.
It also records the value as `IRONSIDE_CLIENT_SUPPORTED_FEATURE_LEVEL` in `client/include/ironside_client/feature_level.h`.
For the meaning of the feature level, see [Feature level](artifacts-and-bundles.md#feature-level).
