# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Validate provisioning-configuration values against the provisioning ABI.

The flags are declared in the configuration-field resource and built from it by
lib/config_fields.py, which reaches the two value parsers below through the
parser hooks the resource names.
"""

import argparse

from . import config_fields as cf
from . import prov_batches as pb

PROFILE = "provisioning"

SECTION_TITLE = cf.profile(PROFILE).title


def add_arguments(ap, group_title=SECTION_TITLE):
    """Add the provisioning-configuration flags to @ap."""
    return cf.add_arguments(ap, PROFILE, group_title)


def closure(spec: str) -> str:
    """An --ap-closure value, as the ABI spells it."""
    try:
        return pb.closure_arg(spec)
    except pb.BatchError as exc:
        raise argparse.ArgumentTypeError(str(exc)) from None


def user_seed(spec: str) -> str:
    """A --provision-user-seed value, as the SEED mode it names."""
    try:
        return pb.user_seed_arg(spec)
    except pb.BatchError as exc:
        raise argparse.ArgumentTypeError(str(exc)) from None


def given(args) -> bool:
    """Whether any provisioning-configuration field was named."""
    return cf.given(args, PROFILE)


def described(closure_mode: str, seed: str | None = None) -> str:
    """One line naming the configured fields, for a status message.

    Takes either closure spelling, for the same reason closure_arg() does - a
    caller holding a profile value and one holding an ABI mode both want this
    line. @seed is a SEED mode, and optional, for a caller reporting the
    closure alone.
    """
    carried = {pb.CONFIG_AP_CLOSURE: pb.closure_method(pb.closure_arg(closure_mode))}
    if seed is not None:
        carried[pb.CONFIG_PROVISION_USER_SEED] = pb.user_seed_choice(seed)
    return ", ".join(f"{label} {value}" for label, value in cf.display(PROFILE, carried))
