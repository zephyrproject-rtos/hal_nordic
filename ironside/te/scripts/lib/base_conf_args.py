# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Resolve base-configuration arguments into a partition layout.

The flags are declared in the configuration-field resource and built from it by
lib/config_fields.py. What stays here is the part the resource cannot express:
how the three sizes combine into the layout the write-once UICR words carry.

The bundle scope offers the partition layout, the PSA key slot storage size
and the IKG SEED selection. Key material comes from
--keyset, and BOOTCONF, the watchdog USERCNF entries and the FORMATVERSION words
belong to the IronSide images that read them. Options that produce a
non-bootable or bench-only configuration are in the generator scope only.
"""

import argparse

from . import config_fields as cf
from . import uicr

PROFILE = "base"

BUNDLE_TITLE = cf.profile(PROFILE).title
GENERATOR_TITLE = cf.profile(PROFILE).generator_title

# IROT panics on first boot if the PARTITIONS words remain erased.
REFERENCE_PARTITIONS_KB = uicr.REFERENCE_PARTITIONS_KB


def add_bundle_arguments(ap, group_title=BUNDLE_TITLE):
    """Add the base-configuration parameters a deployment bundle carries."""
    return cf.add_arguments(ap, PROFILE, group_title, scope=cf.BUNDLE)


def add_arguments(ap, group_title=GENERATOR_TITLE):
    """Add every base-configuration parameter, generator-only ones included."""
    return cf.add_arguments(ap, PROFILE, group_title, scope=cf.GENERATOR)


def partitions_given(args) -> bool:
    """Whether the caller requested a partition layout.

    IROT images contain the reference layout. Base configuration must write these
    write-once words only when a deployment requests a different layout.
    """
    if args.no_partitions:
        return False
    return any(getattr(args, dest) is not None for _, dest in _partition_flags())


def partitions_kb(args):
    """The partition sizes to write, or None to leave them erased."""
    if args.no_partitions:
        return None
    flags = _partition_flags()
    sizes = tuple(getattr(args, dest) for _, dest in flags)
    if all(size is None for size in sizes):
        return tuple(REFERENCE_PARTITIONS_KB)
    if any(size is None for size in sizes):
        missing = [
            f"--{name}" for (name, _), size in zip(flags, sizes, strict=True) if size is None
        ]
        raise uicr.UicrError(
            f"set all three partition sizes or use --no-partitions (missing: {', '.join(missing)})"
        )
    return sizes


def _partition_flags() -> tuple:
    """(flag name, argparse destination) per partition, in PARTITION_NAMES order."""
    declared = {f.partition: f for f in cf.profile(PROFILE).fields if f.partition}
    return tuple((declared[name].name, declared[name].dest) for name in uicr.PARTITION_NAMES)


def ikg_seed_select(spec: str) -> str:
    """An --ikg-seed-select value: the IKG SEED entry the base configuration
    selects, or auto to leave the selection to provisioning."""
    entry = spec.strip().lower()
    choices = (*uicr.IKG_SEED_SELECTIONS, uicr.IKG_SEED_SELECT_AUTO)
    if entry not in choices:
        raise argparse.ArgumentTypeError(
            f"IKG SEED selection must be one of {', '.join(choices)}, got {spec!r}"
        )
    return entry
