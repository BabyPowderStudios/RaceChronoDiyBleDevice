#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass, field
from decimal import Decimal
from pathlib import Path
from typing import Iterable


MESSAGE_RE = re.compile(r"^BO_\s+(\d+)\s+(\S+)\s*:\s*(\d+)\s+(\S+)")
SIGNAL_RE = re.compile(
    r'^\s*SG_\s+(\w+)(?:\s+(M|m\d+))?\s*:\s*'
    r'(\d+)\|(\d+)@([01])([+-])\s+'
    r'\(([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?),'
    r'([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)\)\s+'
    r'\[([^\|\]]+)\|([^\]]+)\]\s+"([^"]*)"\s+(\S+)'
)
MESSAGE_COMMENT_RE = re.compile(r'^CM_\s+BO_\s+(\d+)\s+"(.*)";$')
SIGNAL_COMMENT_RE = re.compile(r'^CM_\s+SG_\s+(\d+)\s+(\w+)\s+"(.*)";$')
VALUE_TABLE_RE = re.compile(r'^VAL_\s+(\d+)\s+(\w+)\s+(.*);$')
VERSION_RE = re.compile(r'^VERSION\s+"([^"]*)"')
GLOBAL_COMMENT_RE = re.compile(r'^CM_\s+"(.*)";$')
VALUE_PAIR_RE = re.compile(r'(-?\d+)\s+"([^"]*)"')


@dataclass
class Signal:
    name: str
    multiplexer: str | None
    start_bit: int
    bit_length: int
    is_little_endian: bool
    is_signed: bool
    factor: Decimal
    offset: Decimal
    minimum: str
    maximum: str
    unit: str
    receiver: str
    comment: str = ""
    values: list[tuple[str, str]] = field(default_factory=list)


@dataclass
class Message:
    can_id: int
    name: str
    length: int
    sender: str
    comment: str = ""
    signals: list[Signal] = field(default_factory=list)


@dataclass
class DbcDocument:
    source_path: Path
    display_source_path: str = ""
    version: str = ""
    global_comments: list[str] = field(default_factory=list)
    messages: list[Message] = field(default_factory=list)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Generate RaceChrono formula markdown files from all DBC files in a directory."
        )
    )
    parser.add_argument(
        "input",
        nargs="?",
        default="dbc",
        help="DBC file or directory to scan. Defaults to ./dbc",
    )
    parser.add_argument(
        "output",
        nargs="?",
        default="can_db",
        help="Output directory for generated markdown files. Defaults to ./can_db",
    )
    return parser.parse_args()


def format_decimal(value: Decimal) -> str:
    text = format(value, "f")
    if "." in text:
        text = text.rstrip("0").rstrip(".")
    if text == "-0":
        text = "0"
    return text or "0"


def escape_markdown_cell(value: str) -> str:
    return value.replace("|", "\\|").replace("\n", " ").strip()


def parse_value_table(payload: str) -> list[tuple[str, str]]:
    return VALUE_PAIR_RE.findall(payload)


def parse_dbc(path: Path) -> DbcDocument:
    document = DbcDocument(source_path=path)
    messages_by_id: dict[int, Message] = {}

    with path.open("r", encoding="utf-8", errors="replace") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line:
                continue

            version_match = VERSION_RE.match(line)
            if version_match:
                document.version = version_match.group(1)
                continue

            global_comment_match = GLOBAL_COMMENT_RE.match(line)
            if global_comment_match:
                document.global_comments.append(global_comment_match.group(1))
                continue

            message_match = MESSAGE_RE.match(line)
            if message_match:
                message = Message(
                    can_id=int(message_match.group(1)),
                    name=message_match.group(2),
                    length=int(message_match.group(3)),
                    sender=message_match.group(4),
                )
                document.messages.append(message)
                messages_by_id[message.can_id] = message
                continue

            signal_match = SIGNAL_RE.match(line)
            if signal_match and document.messages:
                message = document.messages[-1]
                signal = Signal(
                    name=signal_match.group(1),
                    multiplexer=signal_match.group(2),
                    start_bit=int(signal_match.group(3)),
                    bit_length=int(signal_match.group(4)),
                    is_little_endian=signal_match.group(5) == "1",
                    is_signed=signal_match.group(6) == "-",
                    factor=Decimal(signal_match.group(7)),
                    offset=Decimal(signal_match.group(8)),
                    minimum=signal_match.group(9).strip(),
                    maximum=signal_match.group(10).strip(),
                    unit=signal_match.group(11).strip(),
                    receiver=signal_match.group(12),
                )
                message.signals.append(signal)
                continue

            message_comment_match = MESSAGE_COMMENT_RE.match(line)
            if message_comment_match:
                message = messages_by_id.get(int(message_comment_match.group(1)))
                if message is not None:
                    message.comment = message_comment_match.group(2)
                continue

            signal_comment_match = SIGNAL_COMMENT_RE.match(line)
            if signal_comment_match:
                message = messages_by_id.get(int(signal_comment_match.group(1)))
                if message is not None:
                    signal_name = signal_comment_match.group(2)
                    for signal in message.signals:
                        if signal.name == signal_name:
                            signal.comment = signal_comment_match.group(3)
                            break
                continue

            value_table_match = VALUE_TABLE_RE.match(line)
            if value_table_match:
                message = messages_by_id.get(int(value_table_match.group(1)))
                if message is not None:
                    signal_name = value_table_match.group(2)
                    values = parse_value_table(value_table_match.group(3))
                    for signal in message.signals:
                        if signal.name == signal_name:
                            signal.values = values
                            break

    return document


def build_extraction_expression(signal: Signal) -> str:
    if signal.is_little_endian:
        if signal.start_bit % 8 == 0 and signal.bit_length % 8 == 0:
            function_name = "bytesToIntLe" if signal.is_signed else "bytesToUIntLe"
            return f"{function_name}(raw, {signal.start_bit // 8}, {signal.bit_length // 8})"

        function_name = "bitsToIntLe" if signal.is_signed else "bitsToUIntLe"
        return f"{function_name}(raw, {signal.start_bit}, {signal.bit_length})"

    big_endian_offset = (signal.start_bit // 8) * 8 + (7 - (signal.start_bit % 8))
    if signal.start_bit % 8 == 7 and signal.bit_length % 8 == 0:
        function_name = "bytesToInt" if signal.is_signed else "bytesToUInt"
        return f"{function_name}(raw, {signal.start_bit // 8}, {signal.bit_length // 8})"

    function_name = "bitsToInt" if signal.is_signed else "bitsToUInt"
    return f"{function_name}(raw, {big_endian_offset}, {signal.bit_length})"


def apply_scaling(expression: str, signal: Signal) -> str:
    formula = expression

    if signal.factor != Decimal("1"):
        formula = f"{formula} * {format_decimal(signal.factor)}"

    if signal.offset != Decimal("0"):
        offset_text = format_decimal(abs(signal.offset))
        operator = "+" if signal.offset >= 0 else "-"
        formula = f"{formula} {operator} {offset_text}"

    return formula


def format_can_id(can_id: int) -> str:
    return f"{can_id} (0x{can_id:X})"


def build_notes(message: Message, signal: Signal) -> str:
    parts: list[str] = []
    parts.append(f"Message {message.name} ({message.sender})")

    if message.comment:
        parts.append(message.comment)

    byte_order = "little-endian" if signal.is_little_endian else "big-endian"
    signedness = "signed" if signal.is_signed else "unsigned"
    parts.append(
        f"{byte_order}, {signedness}, {signal.bit_length} bit, start bit {signal.start_bit}"
    )

    if signal.unit:
        parts.append(f"Unit: {signal.unit}")

    if signal.minimum or signal.maximum:
        parts.append(f"DBC range: {signal.minimum}..{signal.maximum}")

    if signal.multiplexer:
        parts.append(f"Multiplexer token: {signal.multiplexer}")

    if signal.comment:
        parts.append(signal.comment)

    if signal.values:
        formatted_values = ", ".join(
            f"{raw_value}={label if label else '(empty)'}"
            for raw_value, label in signal.values
        )
        parts.append(f"Values: {formatted_values}")

    return "; ".join(parts)


def build_title(document: DbcDocument) -> str:
    if document.version:
        prefix = document.version.split("_", 1)[0].replace("-", " ").strip()
        if prefix:
            return prefix

    return document.source_path.stem.replace("-", " ").replace("_", " ").upper()


def iter_rows(document: DbcDocument) -> Iterable[str]:
    for message in document.messages:
        for signal in message.signals:
            formula = apply_scaling(build_extraction_expression(signal), signal)
            notes = build_notes(message, signal)
            yield " | ".join(
                [
                    escape_markdown_cell(signal.name),
                    escape_markdown_cell(format_can_id(message.can_id)),
                    f"`{formula}`",
                    escape_markdown_cell(notes),
                ]
            )


def generate_markdown(document: DbcDocument) -> str:
    total_signal_count = sum(len(message.signals) for message in document.messages)
    message_count_with_signals = sum(1 for message in document.messages if message.signals)
    lines = [f"# {build_title(document)}", ""]

    lines.append(
        "Automatically generated from the DBC file into RaceChrono formula syntax."
    )
    lines.append(
        "The formulas reconstruct each DBC signal value directly; for RaceChrono channels that expect a different internal unit, you may still want to adapt the formula manually."
    )
    lines.append("")
    lines.append(f"Generated by `tools/generate_racechrono_can_db.sh`")
    lines.append(f"Source DBC: `{document.display_source_path or document.source_path.as_posix()}`")
    lines.append(f"Messages with signals: {message_count_with_signals}")
    lines.append(f"Signals: {total_signal_count}")

    if document.global_comments:
        lines.append("")
        lines.append("DBC notes:")
        for comment in document.global_comments:
            lines.append(f"- {comment}")

    lines.append("")
    lines.append("## All DBC signals")
    lines.append("")
    lines.append("Channel name | CAN ID | Equation | Notes")
    lines.append("------------ | ------ | -------- | -----")
    lines.extend(iter_rows(document))
    lines.append("")
    return "\n".join(lines)


def resolve_input_files(input_path: Path) -> list[Path]:
    if input_path.is_file():
        return [input_path]

    return sorted(input_path.glob("*.dbc"))


def main() -> int:
    args = parse_args()

    script_path = Path(__file__).resolve()
    repo_root = script_path.parent.parent
    input_path = (repo_root / args.input).resolve() if not Path(args.input).is_absolute() else Path(args.input)
    output_path = (repo_root / args.output).resolve() if not Path(args.output).is_absolute() else Path(args.output)

    dbc_files = resolve_input_files(input_path)
    if not dbc_files:
        raise SystemExit(f"No .dbc files found under {input_path}")

    output_path.mkdir(parents=True, exist_ok=True)

    for dbc_file in dbc_files:
        document = parse_dbc(dbc_file)
        try:
            document.display_source_path = dbc_file.relative_to(repo_root).as_posix()
        except ValueError:
            document.display_source_path = dbc_file.as_posix()
        markdown = generate_markdown(document)
        output_file = output_path / f"{dbc_file.stem}.md"
        output_file.write_text(markdown, encoding="utf-8")
        print(f"Generated {output_file}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())