"""
Copyright (c) 2025 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0
"""

from __future__ import annotations

import argparse
import enum
import json
import re
import struct
import warnings
from collections.abc import Iterator, Sequence
from dataclasses import dataclass
from enum import Enum, Flag
from functools import cached_property, lru_cache, partial
from itertools import groupby
from pathlib import Path
from textwrap import dedent, indent
from typing import Any

from intelhex import IntelHex

from .. import CommandError, SubCommand, SubParsers

try:
    from tabulate import SEPARATING_LINE, tabulate
except ImportError:
    warnings.warn(
        "'tabulate' is not installed, table pretty printing is unavailable",
        stacklevel=0,
    )

    SEPARATING_LINE = ""

    def tabulate(table: list, **kwargs: Any) -> str:
        return "\n".join(" ".join(row) for row in table)


class FileFormat(enum.Enum):
    # .bin file
    BIN = enum.auto()
    # .hex file
    HEX = enum.auto()


class OffsetFormat(enum.Enum):
    # "0x3000 0x10"
    # "0x3000 20entries"
    LITERAL = enum.auto()
    # "/path/to/tlv_data.bin 0x800"
    MCUBOOT_TLV = enum.auto()


@dataclass
class InputFile:
    format: FileFormat
    path: Path
    offsets: list[tuple[int, int]]


class InputFileAction(argparse.Action):
    """Custom Action for handling PERIPHCONF file inputs."""

    def __init__(
        self,
        option_strings,
        dest,
        input_format,
        offset_format,
        **kwargs,
    ) -> None:
        self._input_format = input_format
        self._offset_format = offset_format
        if self._offset_format == OffsetFormat.MCUBOOT_TLV:
            nargs = 3
        else:
            nargs = "+"
        super().__init__(option_strings, dest, nargs=nargs, **kwargs)

    @classmethod
    def with_format(cls, input_format: str, offset_format: str):
        return partial(cls, input_format=input_format, offset_format=offset_format)

    def __call__(
        self,
        parser,
        namespace,
        values,
        option_string=None,
        **kwargs,
    ) -> None:
        file_name = Path(values[0])
        offsets = []

        match self._offset_format:
            case OffsetFormat.LITERAL:
                # Since nargs="+" we don't get exact validation from before
                if len(values) % 2 == 0:
                    raise argparse.ArgumentError(self, "(offset, size) must be given in pairs")

                for i in range(1, len(values), 2):
                    offset = int(values[i], 0)
                    size_str = values[i + 1]
                    if size_str.endswith("entries"):
                        size_no_suffix_str = size_str.removesuffix("entries")
                        size = int(size_no_suffix_str, 0) * 8
                    else:
                        size = int(size_str, 0)
                    offsets.append((offset, size))

            case OffsetFormat.MCUBOOT_TLV:
                desc_bin = Path(values[1])
                desc_start_offset = int(values[2], 0)
                try:
                    desc_data = desc_bin.read_bytes()
                except FileNotFoundError as err:
                    raise argparse.ArgumentError(self, str(err)) from err

                for offset, count in struct.iter_unpack("<II", desc_data):
                    offsets.append((offset + desc_start_offset, count * 8))

            case other:
                raise NotImplementedError(f"No implementation exists for offset format {other}")

        dest_attr = getattr(namespace, self.dest, None)
        if dest_attr is None:
            dest_attr = []
            setattr(namespace, self.dest, dest_attr)

        dest_attr.append(InputFile(self._input_format, file_name, offsets))


def add_parser(subparsers: SubParsers) -> argparse.ArgumentParser:
    parser = subparsers.add_parser(
        "periphconf-check",
        allow_abbrev=False,
        description=(
            "Decode and validate a PERIPHCONF blob. "
            "Can be used to detect errors in the PERIPHCONF that would prevent the "
            "device from booting. "
            "Multiple input files can be given, and will be concatenated in the order they were "
            "provided and validated as a combined list of entries."
        ),
    )

    LITERAL_OFFSET_DESC = (
        "( Format: BLOB_FILE [[OFFSET SIZE] ...] ). "
        "Path to a PERIPHCONF blob in {0} format. Can be given multiple times. "
        "One or more (offset, size) pairs can optionally be given for each file "
        "to instruct the decoder to read PERIPHCONF blobs from parts of the file rather "
        "than interpreting the entire file as a PERIPHCONF blob. "
        "The offset and size parameters are given as decimal or 0x-prefixed hex. "
        "The size can be given the suffix 'entries' to specify number of entries instead "
        "of byte size. Example: '/path/to/blob 0x3000 0x20 0x8000 32entries'."
    )
    parser.add_argument(
        "--in-periphconf-hex",
        dest="in_periphconf_blobs",
        action=InputFileAction.with_format(FileFormat.HEX, OffsetFormat.LITERAL),
        help=LITERAL_OFFSET_DESC.format("Intel HEX"),
    )
    parser.add_argument(
        "--in-periphconf-bin",
        dest="in_periphconf_blobs",
        action=InputFileAction.with_format(FileFormat.BIN, OffsetFormat.LITERAL),
        help=LITERAL_OFFSET_DESC.format("binary"),
    )

    MCUBOOT_TLV_OFFSET_DESC = (
        "Path to a signed MCUboot image in {0} format, "
        "along with metadata describing where to find PERIPHCONF blobs in the image file. "
        "The metadata consists of a pair (TLV data bin file, start offset). "
        "The TLV data bin file should contain a packed sequence of "
        "(4B little-endian offset, 4B little-endian entry count) pairs describing where in the "
        "file the PERIPHCONF blobs are located, relative to the start offset. "
        "The start offset is a decimal or 0x-prefixed hex value which serves as the base offset."
    )
    parser.add_argument(
        "--in-periphconf-hex-mcuboot-tlv",
        dest="in_periphconf_blobs",
        metavar=("IMAGE", "TLV_DATA_FILE", "START_OFFSET"),
        action=InputFileAction.with_format(FileFormat.HEX, OffsetFormat.MCUBOOT_TLV),
        help=MCUBOOT_TLV_OFFSET_DESC.format("Intel HEX"),
    )
    parser.add_argument(
        "--in-periphconf-bin-mcuboot-tlv",
        dest="in_periphconf_blobs",
        metavar=("IMAGE", "TLV_DATA_FILE", "START_OFFSET"),
        action=InputFileAction.with_format(FileFormat.BIN, OffsetFormat.MCUBOOT_TLV),
        help=MCUBOOT_TLV_OFFSET_DESC.format("binary"),
    )
    parser.add_argument(
        "--in-periphconf-registers-json",
        type=argparse.FileType("r", encoding="utf-8"),
        required=True,
        help=(
            "Path to a JSON file describing the peripheral registers managed through PERIPHCONF. "
            "The JSON file is distributed with each release of IronSide SE. "
            "Ensure that you are using the JSON file corresponding to the IronSide SE version you "
            "are targeting."
        ),
    )
    parser.add_argument(
        "--mode",
        choices=["full", "errors"],
        default="full",
        help=(
            "Command mode: 'full': validate and print full PERIPHCONF blob, "
            "'errors': only print entries from the PERIPHCONF that have validation errors "
            "(the command outputs nothing if this option is provided and there are no errors)."
        ),
    )
    parser.add_argument(
        "--style",
        choices=["regs", "raw"],
        default="regs",
        help=(
            "Selects whether PERIPHCONF entries are displayed "
            "using register and field names or as raw hexadecimal values."
        ),
    )
    return parser


def cmd_handler(args: argparse.Namespace) -> None:
    register_info = json.load(args.in_periphconf_registers_json)
    combined_entries, sources = load_input_blobs(args.in_periphconf_blobs, register_info)

    validate_status = validate_periphconf(combined_entries)
    if args.mode == "errors":
        if not validate_status.is_error():
            # No errors; we are done.
            return
        error_style = TableErrorStyle.ONLY_ERRORS
    elif validate_status == ValidationStatus.SUCCESS:
        error_style = TableErrorStyle.HIDE_ERROR_COLUMNS
    else:
        error_style = TableErrorStyle.SHOW_ERROR_COLUMNS

    table_str = render_periphconf_table(
        combined_entries,
        sources,
        reg_style=TableRegStyle(args.style),
        error_style=error_style,
    )
    files_str = render_input_files(sources)
    if validate_status != ValidationStatus.SUCCESS:
        status_str = render_validation_status(validate_status)
    else:
        status_str = None

    if table_str:
        print()
        print(indent(table_str, "  "))

    if files_str:
        print()
        print("Files:")
        print(indent(files_str, "  "))

    if table_str:
        print()
        print("Register description:")
        print("  See datasheet")

    if status_str:
        print()
        print("Error description:", end="\n\n")
        print(indent(status_str, "  "))

    if validate_status.is_fatal_error():
        raise CommandError(
            f"{'Combined ' if len(args.in_periphconf_blobs) > 1 else ''}PERIPHCONF entries "
            "have errors that will prevent the device from booting correctly.\n"
        )


PERIPHCONF_CHECK_COMMAND = SubCommand(
    add_parser=add_parser,
    cmd_handler=cmd_handler,
    cmd_handler_unpack_args=False,
)


@dataclass
class Source:
    path: Path
    offset: tuple[int, int]
    entry_range: tuple[int, int] = tuple()


def load_input_blobs(periphconf_blobs: Sequence[InputFile], register_info: dict) -> tuple:
    sources = []
    combined_entries = []

    for blob_file in periphconf_blobs:
        try:
            match blob_file.format:
                case FileFormat.HEX:
                    ihex = IntelHex()
                    with blob_file.path.open("r", encoding="utf-8") as fp:
                        ihex.loadhex(fp)

                    if len(ihex.segments()) > 1:
                        raise CommandError(
                            f"Invalid PERIPHCONF HEX file {blob_file.name}: "
                            "expected a file containing a single contiguous data segment."
                        )
                    raw = ihex.tobinstr()
                case FileFormat.BIN:
                    raw = blob_file.path.read_bytes()
        except FileNotFoundError as err:
            raise CommandError(f"Failed to open input file: {str(err)}") from err

        if not blob_file.offsets:
            offsets = [(0, len(raw))]
        else:
            offsets = blob_file.offsets

        for offset, size in offsets:
            source = Source(path=blob_file.path, offset=(offset, size))
            entries = load_periphconf(register_info, raw[offset : offset + size], source)

            range_start = len(combined_entries)
            range_end = range_start + len(entries)
            source.entry_range = (range_start, range_end)

            combined_entries.extend(entries)
            sources.append(source)

    return combined_entries, sources


@enum.unique
class ValidationStatus(Flag):
    """Status returned on validation of a PERIPHCONF entry.
    SUCCESS indicates no errors, any other value indicates an error.

    Multiple error statuses can be set simultaneously.
    """

    SUCCESS = 0

    CONFLICTING_VALUES_NON_FATAL = enum.auto()

    CONFLICTING_VALUES_FATAL = enum.auto()

    UNIMPLEMENTED_REGISTER = enum.auto()
    UNRECOGNIZED_REGISTER = enum.auto()

    SPU_PERM_DMASEC_NOT_APPLICABLE = enum.auto()
    SPU_PERM_OWNERID_NOT_APPLICABLE = enum.auto()
    SPU_PERM_SECATTR_NOT_APPLICABLE = enum.auto()
    SPU_REGISTER_LOCKED = enum.auto()

    MEMCONF_POWER_REGION_NOT_PRESENT = enum.auto()

    def is_error(self) -> bool:
        """Check if any error status is set."""
        return self != ValidationStatus.SUCCESS

    def is_fatal_error(self) -> bool:
        """Check if any fatal error status (an error that prevents normal boot) is set."""
        fatal = self & ~ValidationStatus.CONFLICTING_VALUES_NON_FATAL
        return fatal.is_error()

    # Flag.__iter__() is only available from 3.11
    def __iter__(self) -> Iterator[ValidationStatus]:
        for member in ValidationStatus.__members__.values():
            # Note that SUCCESS is skipped, otherwise it is always included
            if member in self and member != ValidationStatus.SUCCESS:
                yield member


class RegType(Enum):
    """Represents the types of peripheral registers that can be configured through PERIPHCONF."""

    GPIO_PIN_CNF = enum.auto()
    IPCMAP_CHANNEL_SINK = enum.auto()
    IPCMAP_CHANNEL_SOURCE = enum.auto()
    IRQMAP_IRQ_SINK = enum.auto()
    MEMCONF_POWER_CONTROL = enum.auto()
    MEMCONF_POWER_RET = enum.auto()
    MEMCONF_POWER_RET2 = enum.auto()
    MRAMC_CONFIGNVR_PAGE = enum.auto()
    L2CACHE_ENABLE = enum.auto()
    PPIB_PUBLISH_RECEIVE = enum.auto()
    PPIB_SUBSCRIBE_SEND = enum.auto()
    SPU_PERIPH_PERM = enum.auto()
    SPU_FEATURE_BELLS_PROCESSOR_EVENTS = enum.auto()
    SPU_FEATURE_BELLS_PROCESSOR_INTERRUPT = enum.auto()
    SPU_FEATURE_BELLS_PROCESSOR_TASKS = enum.auto()
    SPU_FEATURE_DPPIC_CH = enum.auto()
    SPU_FEATURE_DPPIC_CHG = enum.auto()
    SPU_FEATURE_GPIO_PIN = enum.auto()
    SPU_FEATURE_GPIOTE_CH = enum.auto()
    SPU_FEATURE_GPIOTE_INTERRUPT = enum.auto()
    SPU_FEATURE_GRTC_CC = enum.auto()
    SPU_FEATURE_GRTC_CLK = enum.auto()
    SPU_FEATURE_GRTC_INTERRUPT = enum.auto()
    SPU_FEATURE_GRTC_PWMCONFIG = enum.auto()
    SPU_FEATURE_GRTC_SYSCOUNTER = enum.auto()
    SPU_FEATURE_IPCT_CH = enum.auto()
    SPU_FEATURE_IPCT_INTERRUPT = enum.auto()

    def has_lock_bit(self) -> bool:
        return self.is_spu_register() or self == RegType.MRAMC_CONFIGNVR_PAGE

    def is_spu_register(self) -> bool:
        return (
            RegType.SPU_PERIPH_PERM.value <= self.value <= RegType.SPU_FEATURE_IPCT_INTERRUPT.value
        )


@dataclass
class ConfEntry:
    index: int
    regptr: int
    value: int
    info: dict | None
    source: Source
    status: ValidationStatus = ValidationStatus.SUCCESS

    @property
    def name(self) -> str:
        if self.info is None:
            return f"0x{self.regptr:09_x} (unrecognized)"
        return self.info["name"]

    @property
    def mask(self) -> int:
        if self.info is None:
            # Mask unknown, assume 0xFFFF_FFFF.
            return 0xFFFF_FFFF
        return self.info["mask"]

    @property
    def masked_value(self) -> int:
        return self.value & self.mask

    @property
    def masked_default_value(self) -> int:
        if self.info is None:
            raise ValueError()
        return self.info["default"] & self.info["mask"]

    @cached_property
    def reg_type_props(self) -> Any | None:
        if self.info is None:
            return None

        data = None
        reg_type = None

        if self.name.startswith("SPU"):
            if data := self._parse_name("PERIPH{0}.PERM"):
                reg_type = RegType.SPU_PERIPH_PERM
            elif data := self._parse_name("FEATURE.BELLS.PROCESSOR{0}.TASKS{0}"):
                reg_type = RegType.SPU_FEATURE_BELLS_PROCESSOR_TASKS
            elif data := self._parse_name("FEATURE.BELLS.PROCESSOR{0}.EVENTS{0}"):
                reg_type = RegType.SPU_FEATURE_BELLS_PROCESSOR_EVENTS
            elif data := self._parse_name("FEATURE.BELLS.PROCESSOR{0}.INTERRUPT{0}"):
                reg_type = RegType.SPU_FEATURE_BELLS_PROCESSOR_INTERRUPT
            elif data := self._parse_name("FEATURE.DPPIC.CH{0}"):
                reg_type = RegType.SPU_FEATURE_DPPIC_CH
            elif data := self._parse_name("FEATURE.DPPIC.CHG{0}"):
                reg_type = RegType.SPU_FEATURE_DPPIC_CHG
            elif data := self._parse_name("FEATURE.GPIO{0}.PIN{0}"):
                reg_type = RegType.SPU_FEATURE_GPIO_PIN
            elif data := self._parse_name("FEATURE.GPIOTE{0}.CH{0}"):
                reg_type = RegType.SPU_FEATURE_GPIOTE_CH
            elif data := self._parse_name("FEATURE.GPIOTE{0}.INTERRUPT{0}"):
                reg_type = RegType.SPU_FEATURE_GPIOTE_INTERRUPT
            elif data := self._parse_name("FEATURE.GRTC.CC{0}"):
                reg_type = RegType.SPU_FEATURE_GRTC_CC
            elif data := self._parse_name("FEATURE.GRTC.PWMCONFIG"):
                reg_type = RegType.SPU_FEATURE_GRTC_PWMCONFIG
            elif data := self._parse_name("FEATURE.GRTC.CLK"):
                reg_type = RegType.SPU_FEATURE_GRTC_CLK
            elif data := self._parse_name("FEATURE.GRTC.SYSCOUNTER"):
                reg_type = RegType.SPU_FEATURE_GRTC_SYSCOUNTER
            elif data := self._parse_name("FEATURE.GRTC.INTERRUPT{0}"):
                reg_type = RegType.SPU_FEATURE_GRTC_INTERRUPT
            elif data := self._parse_name("FEATURE.IPCT.CH{0}"):
                reg_type = RegType.SPU_FEATURE_IPCT_CH
            elif data := self._parse_name("FEATURE.IPCT.INTERRUPT{0}"):
                reg_type = RegType.SPU_FEATURE_IPCT_INTERRUPT
        elif self.name.startswith("IPCMAP"):
            if data := self._parse_name("CHANNEL{0}.SINK"):
                reg_type = RegType.IPCMAP_CHANNEL_SINK
            elif data := self._parse_name("CHANNEL{0}.SOURCE"):
                reg_type = RegType.IPCMAP_CHANNEL_SOURCE
        elif self.name.startswith("IRQMAP"):
            if data := self._parse_name("IRQ{0}.SINK"):
                reg_type = RegType.IRQMAP_IRQ_SINK
        elif self.name.startswith("MEMCONF"):
            if data := self._parse_name("POWER{0}.CONTROL"):
                reg_type = RegType.MEMCONF_POWER_CONTROL
            elif data := self._parse_name("POWER{0}.RET"):
                reg_type = RegType.MEMCONF_POWER_RET
            elif data := self._parse_name("POWER{0}.RET2"):
                reg_type = RegType.MEMCONF_POWER_RET2
        elif self.name.startswith("MRAMC") and (data := self._parse_name("CONFIGNVR.PAGE{0}")):
            reg_type = RegType.MRAMC_CONFIGNVR_PAGE
        elif self.name.startswith("L2CACHE"):
            if data := self._parse_name("ENABLE"):
                reg_type = RegType.L2CACHE_ENABLE
        elif self.name.startswith("PPIB"):
            if data := self._parse_name("SUBSCRIBE_SEND{0}"):
                reg_type = RegType.PPIB_SUBSCRIBE_SEND
            elif data := self._parse_name("PUBLISH_RECEIVE{0}"):
                reg_type = RegType.PPIB_PUBLISH_RECEIVE
        elif self.name.startswith("P") and (data := self._parse_name("PIN_CNF{0}")):
            reg_type = RegType.GPIO_PIN_CNF

        if data is None or reg_type is None:
            raise NotImplementedError(f"Failed to parse register name {self.name}")

        return (reg_type, data.periph, *data.array_indices)

    def _parse_name(self, path_str: str) -> PathData | None:
        """Parse a peripheral register path into parts.
        The path_str uses its own syntax for convenience, see _make_path_pattern for details.
        """
        path_pattern = _make_path_pattern(path_str)
        if match := re.fullmatch(path_pattern, self.name):
            groups = match.groups()
            return PathData(groups[0], [int(s) for s in groups[1:]])
        return None

    def conf_field_equals_default(self, field_name: str) -> bool:
        """Returns true if the value in the entry for the field equals the default value."""
        return self.get_conf_field(field_name) == self.get_default_field(field_name)

    def get_conf_field(self, field_name: str) -> int:
        """Get the value set in the entry for the field."""
        return self._get_field(field_name, self.value)

    def get_default_field(self, field_name: str) -> int:
        """Get the default value of the field."""
        if self.info is None:
            raise ValueError()
        return self._get_field(field_name, self.info["default"])

    def _get_field(self, field_name: str, reg_value: int) -> int:
        if self.info is None:
            raise ValueError()

        field_mask = self._field_info_by_name[field_name]["mask"]
        lsb_pos = (field_mask & -field_mask).bit_length() - 1
        field_value = (reg_value & field_mask) >> lsb_pos
        return field_value

    @property
    def field_desc(self) -> str:
        if self.info is None:
            return f"0x{self.value:09_x} (unrecognized)"

        field_desc_parts = []
        for field in self.info["fields"]:
            # Skip printing of read-only fields
            if field["mask"] & ~self.mask == field["mask"]:
                continue
            field_content = self.get_conf_field(field["name"])
            field_desc_parts.append(f"{field['name']}={field_content}")
        return ", ".join(field_desc_parts)

    @property
    def _field_info_by_name(self) -> dict[str, dict]:
        assert self.info is not None
        return {f["name"]: f for f in self.info["fields"]}

    def __lt__(self, other: object) -> bool:
        if not isinstance(other, ConfEntry):
            raise NotImplementedError(f"ConfEntry can not be compared with {type(other)}")
        return (self.regptr, self.value) < (other.regptr, other.value)


@lru_cache
def _make_path_pattern(path_str: str) -> str:
    r"""Create a regex that matches a peripheral register name as defined by the path_str.
    Escapes '.'-characters, replaces instances of {0} with a pattern matching an array index
    like [123], and adds a pattern matching a peripheral name at the start.

    For example:
      "PERIPH{0}.PERM"
    becomes:
      r"^([^\.]+)\.PERIPH\[([0-9]+)\]\.PERM"
    """
    path_pattern = path_str.format(r"\[([0-9]+)\]")
    path_pattern = path_pattern.replace(".", r"\.")
    path_pattern = r"^([^\.]+)\." + path_pattern + "$"
    return path_pattern


@dataclass
class PathData:
    periph: str
    array_indices: list[int]


REGPTR_MASK = 0xFFFF_FFFC


def load_periphconf(register_info: dict, periphconf_raw: bytes, source: Source) -> list[ConfEntry]:
    """Parse/load PERIPHCONF from raw bytes"""
    blob = []

    for i in range(0, len(periphconf_raw), 8):
        regptr = int.from_bytes(periphconf_raw[i : i + 4], "little") & REGPTR_MASK
        # This indicates end of table
        if regptr == REGPTR_MASK:
            break

        value = int.from_bytes(periphconf_raw[i + 4 : i + 8], "little")
        info = register_info.get(str(regptr))
        blob.append(ConfEntry(index=i // 8, regptr=regptr, value=value, info=info, source=source))

    return blob


def validate_periphconf(
    conf_list: list[ConfEntry],
) -> ValidationStatus:
    """Validate the entries in the list of PERIPHCONF entries.

    Updates the validation status on each of the entries in the conf list.
    """
    status_combined = ValidationStatus.SUCCESS

    sorted_confs = sorted(conf_list, key=lambda c: c.regptr)
    confs_by_regptr = groupby(sorted_confs, key=lambda c: c.regptr)

    for _, regptr_confs in confs_by_regptr:
        # To be able to iterate over it more than once
        regptr_conf_list = list(regptr_confs)

        status_combined |= check_if_conflicting_values(regptr_conf_list)

        for conf in regptr_conf_list:
            status_combined |= check_if_unrecognized_register(conf)
            status_combined |= check_if_unimplemented_register(conf)
            status_combined |= check_if_invalid_register_config(conf)

    return status_combined


def check_if_conflicting_values(
    regptr_confs: list[ConfEntry],
) -> ValidationStatus:
    """Check if the entries, which all target the same register, have values that conflict.
    A conflict is reported whenever the values are different, because the PERIPHCONF is designed
    so that none of the exposed registers require multiple entries.
    The function separates the conflicts into fatal and non-fatal, where fatal conflicts are those
    that lead to a boot error.
    """

    if len(regptr_confs) < 2:
        # Need at least two confs to have a conflict
        return ValidationStatus.SUCCESS

    conf_by_masked_value: dict[int, list[ConfEntry]] = {}
    for conf in regptr_confs:
        conf_by_masked_value.setdefault(conf.masked_value, []).append(conf)

    if len(conf_by_masked_value) < 2:
        # All masked values are the same, so no conflict
        return ValidationStatus.SUCCESS

    # We assume that all entries have the same regtype
    if reg_type_props := regptr_confs[0].reg_type_props:
        # A fatal conflict occurs if two or more registers has lock bit = Locked
        # This implies that one or more of the other bits are different, otherwise we would
        # not get here.
        reg_type = reg_type_props[0]
        if reg_type.has_lock_bit():
            all_lock_bit_values = []
            for confs in conf_by_masked_value.values():
                # We ignore the reg_type when getting the field since it is the same in all
                # lockable registers.
                all_lock_bit_values.append(bool(confs[0].get_conf_field("LOCK")))
            if any(all_lock_bit_values):
                # At least one entry is locked, so it is expected to fail at runtime.
                # It could be possible to have it succeed if only the last entry sets the lock
                # bit, but we ignore that here as this is almost certainly an unintentional config.
                status = ValidationStatus.CONFLICTING_VALUES_FATAL
            else:
                status = ValidationStatus.CONFLICTING_VALUES_NON_FATAL
        else:
            status = ValidationStatus.CONFLICTING_VALUES_NON_FATAL
    else:
        # We don't know which register this is, only that it has a conflict
        status = ValidationStatus.CONFLICTING_VALUES_NON_FATAL

    for conf in regptr_confs:
        conf.status |= status

    return status


def check_if_unrecognized_register(conf: ConfEntry) -> ValidationStatus:
    """Check if the entry points to an unsupported/invalid register."""

    if conf.info is not None:
        return ValidationStatus.SUCCESS

    # We assume that we have already loaded the info for the supported registers,
    # so if the entry has no info, then it is not supported.
    status = ValidationStatus.UNRECOGNIZED_REGISTER
    conf.status |= status

    return status


def check_if_unimplemented_register(conf: ConfEntry) -> ValidationStatus:
    """Check if the entry points to a register that is valid but not implemented in the hardware.
    Such registers typically read as zero regardless of the value written, which causes a readback
    error when the PERIPHCONF is applied.
    """

    match conf.reg_type_props:
        case (RegType.IPCMAP_CHANNEL_SINK, *_) | (RegType.IPCMAP_CHANNEL_SOURCE, *_) | None:
            # All IPCMAP registers are implemented.
            # If type is None then the register is not recognized, so we don't report an error
            # here but through the other checks.
            return ValidationStatus.SUCCESS

        case (RegType.PPIB_PUBLISH_RECEIVE, *_) | (RegType.PPIB_SUBSCRIBE_SEND, *_):
            # These don't completely follow the rule below, so we exempt them here for now.
            return ValidationStatus.SUCCESS

        case _:
            # None was handled above.
            assert conf.info is not None

            # For the registers managed through PERIPHCONF, a default value of 0
            # indicates that the register is not implemented.
            is_implemented = conf.info["default"] != 0

    if is_implemented:
        return ValidationStatus.SUCCESS

    status = ValidationStatus.UNIMPLEMENTED_REGISTER
    conf.status |= status

    return status


def check_if_invalid_register_config(conf: ConfEntry) -> ValidationStatus:
    """Does register specific checks to see if the entry value is invalid for that register."""
    status_combined = ValidationStatus.SUCCESS

    match conf.reg_type_props:
        case (RegType.SPU_PERIPH_PERM, *_):
            fixed_dmasec = conf.get_default_field("DMA") in (
                SpuPermDma.NO_DMA,
                SpuPermDma.NO_SEPARATE_ATTRIBUTE,
            )
            if fixed_dmasec and not conf.conf_field_equals_default("DMASEC"):
                status = ValidationStatus.SPU_PERM_DMASEC_NOT_APPLICABLE
                conf.status |= status
                status_combined |= status

            fixed_owner = not conf.get_default_field("OWNERPROG")
            if fixed_owner and not conf.conf_field_equals_default("OWNERID"):
                status = ValidationStatus.SPU_PERM_OWNERID_NOT_APPLICABLE
                conf.status |= status
                status_combined |= status

            fixed_secattr = (
                conf.get_default_field("SECUREMAPPING") != SpuPermSecuremapping.USER_SELECTABLE
            )
            if fixed_secattr and not conf.conf_field_equals_default("SECATTR"):
                status = ValidationStatus.SPU_PERM_SECATTR_NOT_APPLICABLE
                conf.status |= status
                status_combined |= status

            is_locked = bool(conf.get_default_field("LOCK"))
            if is_locked and conf.masked_value != conf.masked_default_value:
                status = ValidationStatus.SPU_REGISTER_LOCKED
                conf.status |= status
                status_combined |= status

        case (
            (RegType.SPU_FEATURE_BELLS_PROCESSOR_TASKS, *_)
            | (RegType.SPU_FEATURE_BELLS_PROCESSOR_EVENTS, *_)
            | (RegType.SPU_FEATURE_BELLS_PROCESSOR_INTERRUPT, *_)
            | (RegType.SPU_FEATURE_DPPIC_CH, *_)
            | (RegType.SPU_FEATURE_DPPIC_CHG, *_)
            | (RegType.SPU_FEATURE_GPIO_PIN, *_)
            | (RegType.SPU_FEATURE_GPIOTE_CH, *_)
            | (RegType.SPU_FEATURE_GPIOTE_INTERRUPT, *_)
            | (RegType.SPU_FEATURE_GRTC_CC, *_)
            | (RegType.SPU_FEATURE_GRTC_CLK, *_)
            | (RegType.SPU_FEATURE_GRTC_SYSCOUNTER, *_)
            | (RegType.SPU_FEATURE_GRTC_INTERRUPT, *_)
            | (RegType.SPU_FEATURE_IPCT_CH, *_)
            | (RegType.SPU_FEATURE_IPCT_INTERRUPT, *_)
        ):
            is_locked = bool(conf.get_default_field("LOCK"))
            if is_locked and conf.masked_value != conf.masked_default_value:
                status = ValidationStatus.SPU_REGISTER_LOCKED
                conf.status |= status
                status_combined |= status

        case (
            (RegType.MEMCONF_POWER_CONTROL, *_)
            | (RegType.MEMCONF_POWER_RET, *_)
            | (RegType.MEMCONF_POWER_RET2, *_)
        ):
            # For these registers the default state should have all user programmable bits set to 1.
            # Therefore, user programmable bits that are zero in the default value can be considered
            # not implemented/programmable.
            conf_bad_bits = (conf.masked_value ^ conf.masked_default_value) & conf.mask
            if conf_bad_bits:
                status = ValidationStatus.MEMCONF_POWER_REGION_NOT_PRESENT
                conf.status |= status
                status_combined |= status

    return status_combined


class SpuPermSecuremapping(int, Enum):
    """Enum values used in the SPU PERIPH[n].PERM SECUREMAPPING field"""

    NONSECURE = 0
    SECURE = 1
    USER_SELECTABLE = 2
    SPLIT = 3


class SpuPermDma(int, Enum):
    """Enum values used in the SPU PERIPH[n].PERM DMA field"""

    NO_DMA = 0
    NO_SEPARATE_ATTRIBUTE = 1
    SEPARATE_ATTRIBUTE = 2


def render_input_files(sources: list[Source]) -> str:
    """Render list of input files and number of entries in each."""
    lines = []

    sources_by_path = [(p, list(s)) for p, s in groupby(sources, lambda s: s.path)]

    for i, (path, src) in enumerate(sources_by_path):
        lines.append(f"{i}: {path}")
        for source in src:
            offset, size = source.offset
            start, end = source.entry_range
            count = end - start
            if count > 0:
                lines.append(
                    f"  offset [0x{offset:_x}, 0x{size:_x}] "
                    f"-> index [{start} - {end - 1}] ({count} entries)"
                )
            else:
                lines.append("  No entries")
        if i < len(sources_by_path) - 1:
            lines.append("")

    table_str = "\n".join(lines)

    return table_str


class TableRegStyle(enum.Enum):
    REGS = "regs"
    RAW = "raw"


class TableErrorStyle(enum.Enum):
    SHOW_ERROR_COLUMNS = enum.auto()
    HIDE_ERROR_COLUMNS = enum.auto()
    ONLY_ERRORS = enum.auto()


def render_periphconf_table(
    entries: list[ConfEntry],
    sources: list[Source],
    reg_style: TableRegStyle,
    error_style: TableErrorStyle,
) -> str:
    """Render validated PERIPHCONF entries in table form."""
    table = []
    for source in sources:
        start, end = source.entry_range
        for conf in entries[start:end]:
            if error_style == TableErrorStyle.ONLY_ERRORS and not conf.status.is_error():
                continue

            if len(sources) > 1:
                # Left padding to 3 spaces makes it look nice for reasonable numbers
                index = f"{conf.index + start:<3} | {conf.index:<3}"
            else:
                index = str(conf.index)

            if reg_style == TableRegStyle.REGS:
                reg_name = conf.name
                field_desc = conf.field_desc
            else:
                reg_name = fmt_addr(conf.regptr)
                field_desc = fmt_addr(conf.value)

            if conf.status != ValidationStatus.SUCCESS:
                error_char = "X"
                error_desc = ", ".join(str(s.name) for s in conf.status)
            else:
                error_char = ""
                error_desc = ""

            match error_style:
                case TableErrorStyle.HIDE_ERROR_COLUMNS:
                    table.append([index, reg_name, field_desc])
                case TableErrorStyle.ONLY_ERRORS:
                    table.append([index, reg_name, field_desc, error_desc])
                case _:
                    table.append([error_char, index, reg_name, field_desc, error_desc])

        table.append(SEPARATING_LINE)

    match error_style:
        case TableErrorStyle.HIDE_ERROR_COLUMNS:
            headers = ["Index", "Register", "Fields"]
        case TableErrorStyle.ONLY_ERRORS:
            headers = ["Index", "Register", "Fields", "Error"]
        case _:
            headers = ["E", "Index", "Register", "Fields", "Error"]

    table_str = tabulate(
        table,
        headers=headers,
        tablefmt="simple",
    )

    return table_str


def render_validation_status(status: ValidationStatus) -> str:
    """Render descriptions of each status flag in the combined status."""
    lines = []

    for present_status in status:
        lines.append(f"{present_status.name}:")
        status_description = VALIDATION_STATUS_DESCRIPTIONS[present_status]
        lines.append(indent(status_description, "  "))
        lines.append("")

    return "\n".join(lines)


def fmt_addr(addr: int) -> str:
    """Format an address in a readable way."""
    return f"0x{addr:09_x}"


VALIDATION_STATUS_DESCRIPTIONS = {
    # This is here for completeness
    ValidationStatus.SUCCESS: "No errors",
    ValidationStatus.CONFLICTING_VALUES_NON_FATAL: dedent(
        """\
        Two or more PERIPHCONF entries target the same register but have different values.
        This is likely caused by conflicting configurations in the device tree or source code.
        The targeted register is not locked, therefore the latest entry in the table will
        take precedence and overwrite previous entries."""
    ),
    ValidationStatus.CONFLICTING_VALUES_FATAL: dedent(
        """\
        Two or more PERIPHCONF entries target the same register but have different values.
        This is likely caused by conflicting configurations in the device tree or source code.
        The targeted register is locked, therefore the second entry will cause a read-back
        error, preventing the device from booting normally."""
    ),
    ValidationStatus.UNIMPLEMENTED_REGISTER: dedent(
        """\
        The PERIPHCONF entry targets a register that is not implemented in the hardware.
        This typically means that the hardware feature configured by the entry, e.g. GPIO pin or
        DPPI channel, does not exist. The entry will cause a read-back error, preventing the
        device from booting normally."""
    ),
    ValidationStatus.UNRECOGNIZED_REGISTER: dedent(
        """\
        The PERIPHCONF entry register address is not recognized. Therefore the entry will cause
        a permission error, preventing the device from booting normally."""
    ),
    ValidationStatus.SPU_PERM_DMASEC_NOT_APPLICABLE: dedent(
        """\
        The PERIPHCONF entry targets a SPU PERIPH[n].PERM register that does not accept
        the DMA security (DMASEC) setting set by the entry. This is either because the peripheral
        governed by this PERM register does not have DMA, or that the DMA security has a fixed
        value. This entry will cause a read-back error, preventing the device from booting
        normally."""
    ),
    ValidationStatus.SPU_PERM_OWNERID_NOT_APPLICABLE: dedent(
        """\
        The PERIPHCONF entry targets a SPU PERIPH[n].PERM register that does not accept
        the Owner ID (OWNERID) setting set by the entry. This is because the peripheral
        governed by this PERM register does not have programmable ownership.
        This entry will cause a read-back error, preventing the device from booting normally."""
    ),
    ValidationStatus.SPU_PERM_SECATTR_NOT_APPLICABLE: dedent(
        """\
        The PERIPHCONF entry targets a SPU PERIPH[n].PERM register that does not accept
        the security (SECATTR) setting set by the entry. This is because the peripheral
        governed by this PERM register does not have programmable security.
        This entry will cause a read-back error, preventing the device from booting normally."""
    ),
    ValidationStatus.SPU_REGISTER_LOCKED: dedent(
        """\
        The PERIPHCONF entry targets a SPU register that is by default locked in the hardware,
        and contains a different value than the default value. This entry will cause a read-back
        error, preventing the device from booting normally."""
    ),
    ValidationStatus.MEMCONF_POWER_REGION_NOT_PRESENT: dedent(
        """\
        The PERIPHCONF entry targets a MEMCONF POWER register, and has enabled a memory region
        index that is not present in the hardware. This entry will cause a read-back
        error, preventing the device from booting normally."""
    ),
}
