# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Security counter layout of the IronSide TE UROT and DFU images.

The counter is EPOCH << 20 | MAJOR << 16 | MINOR << 8 | PATCH. The security
epoch is the most significant field, so a release at a higher epoch is accepted
over every release at a lower one, whatever their versions. Within one epoch the
version orders the releases.
"""

ROLES = ("urot", "dfu")

# (name, width in bits), most significant first.
_FIELDS = (("EPOCH", 12), ("MAJOR", 4), ("MINOR", 8), ("PATCH", 8))


def pack(epoch: int, version: str) -> int:
    """Return the counter of @version at @epoch.

    @version is MAJOR.MINOR.PATCH. Raises ValueError for a field that does not
    fit its width.
    """
    try:
        values = (epoch, *(int(field) for field in version.split(".")))
    except ValueError:
        raise ValueError(f"version {version!r} is not MAJOR.MINOR.PATCH") from None
    if len(values) != len(_FIELDS):
        raise ValueError(f"version {version!r} is not MAJOR.MINOR.PATCH")
    counter = 0
    for (name, width), value in zip(_FIELDS, values, strict=True):
        if not 0 <= value < 1 << width:
            raise ValueError(
                f"epoch {epoch}, version {version}: {name} {value} does not fit the "
                f"security counter, which holds {name} 0 to {(1 << width) - 1}"
            )
        counter = counter << width | value
    return counter


def unpack(counter: int) -> tuple[int, str]:
    """Return the (epoch, MAJOR.MINOR.PATCH version) that packs to @counter."""
    epoch = counter >> 20 & 0xFFF
    return epoch, f"{counter >> 16 & 0xF}.{counter >> 8 & 0xFF}.{counter & 0xFF}"


def display(counter: int) -> str:
    """Format @counter as its epoch and version."""
    epoch, version = unpack(counter)
    return f"epoch {epoch}, version {version}"


if __name__ == "__main__":
    assert pack(0, "0.4.0") == 0x00000400
    assert pack(1, "0.4.1") == 0x00100401
    assert unpack(0x00100501) == (1, "0.5.1")
    assert pack(4095, "15.255.255") == 0xFFFFFFFF
    for epoch, version in ((4096, "0.0.0"), (0, "16.0.0"), (0, "0.256.0"), (0, "0.0.256"),
                           (-1, "0.0.0"), (0, "0.0"), (0, "0.0.x")):
        try:
            pack(epoch, version)
        except ValueError:
            continue
        raise AssertionError(f"pack({epoch}, {version!r}) was accepted")
    assert display(0x00100401) == "epoch 1, version 0.4.1"
    print("component_counter: self-test OK")
