# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Host-script Python version gate."""

from __future__ import annotations

import sys

MIN_PYTHON = (3, 10)


def require_python() -> None:
    if sys.version_info >= MIN_PYTHON:
        return
    major, minor = MIN_PYTHON
    got = f"{sys.version_info.major}.{sys.version_info.minor}"
    sys.exit(
        f"Python {major}.{minor}+ is required; got {got} ({sys.executable}). "
        "On macOS, /usr/bin/python3 is 3.9 - install Python 3.10 or newer "
        "(for example `brew install python@3.12`) and invoke that interpreter."
    )
