#!/usr/bin/env python3
"""Generates board/chip pin-capability data from tools/devicegen/boards/<chip>/*.board.yaml.

Split by design, not by accident:
  - Pin *validity* (can this GPIO be ADC1/output/etc) is a fact about the chip's silicon,
    identical across every board built on that chip. The firmware only ever runs on one
    chip per compiled binary, so it only ever needs ONE chip's table -- not the other
    four, and not per-board detail (a board only differs in which of the chip's real
    pins happen to be broken out on its particular header, which the firmware cannot
    observe or enforce anyway).
  - Which *board model* the user is holding (and therefore which of the chip's pins are
    actually wired to a header, an onboard LED, a display, etc) is a UI-picker concern,
    not a validation concern. The SPA is not compiled per chip -- it's fine for it to
    carry the full catalog across every chip/board researched.

Modes:
  generate --chip <chip>   -- writes src/devices/core/BoardPinCapabilities.h for that one
                               chip's pin table (union of all its boards' pins). Defaults
                               to 'esp32', the only chip this project actually builds today
                               (see platformio.ini). A future esp32s3/esp32c3/... build
                               environment would regenerate this file for its own chip
                               before compiling (analogous to how
                               scripts/generate_version_header.py runs pre-build) -- not
                               wired up yet since no such environment exists yet.
  generate-spa              -- writes portal-spa/src/data/board-pin-capabilities.ts with
                               every board from every chip (see reasoning above).
  check --chip <chip>       -- regenerates the firmware header in memory, diffs against
                               the committed file, non-zero exit on drift.
  check-spa                 -- same, for the SPA data file.
"""
import argparse
import pathlib
import subprocess
import sys

import yaml

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
BOARDS_DIR = pathlib.Path(__file__).resolve().parent / "boards"
CPP_OUT = REPO_ROOT / "src" / "platform" / "BoardPinCapabilities.h"
TS_OUT = REPO_ROOT / "portal-spa" / "src" / "data" / "board-pin-capabilities.ts"

DEFAULT_CHIP = "esp32"

# Closed role vocabulary -- order fixes the bit position in the generated C++ mask.
ROLES = ["output", "input", "adc1", "adc2", "strapping", "reservedFlash"]
ROLE_CONST = {
    "output": "Output",
    "input": "Input",
    "adc1": "Adc1",
    "adc2": "Adc2",
    "strapping": "Strapping",
    "reservedFlash": "ReservedFlash",
}


def load_all_boards() -> list[dict]:
    boards = []
    for path in sorted(BOARDS_DIR.glob("*/*.board.yaml")):
        board = yaml.safe_load(path.read_text())
        known_gpios = {pin["gpio"] for pin in board["pins"]}
        for pin in board["pins"]:
            unknown = set(pin["roles"]) - set(ROLES)
            if unknown:
                raise ValueError(f"{path}: pin {pin['gpio']} has unknown role(s) {unknown}")
        layout = board.get("layout")
        if layout:
            for side in ("left", "right"):
                for entry in layout[side]:
                    if isinstance(entry, int) and entry not in known_gpios:
                        raise ValueError(f"{path}: layout.{side} references gpio {entry} not present in pins")
        boards.append(board)
    if not boards:
        raise ValueError(f"no *.board.yaml files found under {BOARDS_DIR}/*/")
    return boards


def boards_for_chip(boards: list[dict], chip: str) -> list[dict]:
    matched = [b for b in boards if b["chip"] == chip]
    if not matched:
        known = sorted({b["chip"] for b in boards})
        raise ValueError(f"no boards found for chip '{chip}' (known chips: {known})")
    return matched


def role_mask(roles: list[str]) -> int:
    mask = 0
    for role in roles:
        mask |= 1 << ROLES.index(role)
    return mask


def board_id_pascal_case(board_id: str) -> str:
    return "".join(part.capitalize() for part in board_id.split("-"))


def default_board_id(chip: str, chip_boards: list[dict]) -> str:
    defaults = [b["boardId"] for b in chip_boards if b.get("isDefault") is True]
    if len(defaults) != 1:
        raise ValueError(f"chip '{chip}' must have exactly one board with isDefault: true, found {len(defaults)}")
    return defaults[0]


def chip_pin_union(chip_boards: list[dict]) -> list[dict]:
    """Merges every board's pins for one chip into a single gpio->roleMask table.

    Pin validity is a chip-level electrical fact, so the same gpio number must carry the
    same roles on every board of that chip; this ORs the masks together (a no-op if the
    per-board YAMLs agree, which they should) rather than trusting a single board to have
    documented every real pin.
    """
    merged: dict[int, int] = {}
    for board in chip_boards:
        for pin in board["pins"]:
            gpio = pin["gpio"]
            merged[gpio] = merged.get(gpio, 0) | role_mask(pin["roles"])
    return [{"gpio": gpio, "roleMask": mask} for gpio, mask in sorted(merged.items())]


def render_cpp(chip: str, chip_boards: list[dict]) -> str:
    pins = chip_pin_union(chip_boards)
    board_ids = sorted(b["boardId"] for b in chip_boards)
    default_id = default_board_id(chip, chip_boards)
    lines = []
    lines.append("#pragma once")
    lines.append("")
    lines.append(f"// GENERATED by tools/devicegen/generate_boards.py for chip: {chip}")
    lines.append("// DO NOT EDIT BY HAND -- see docs/pin-configuration-conventions.md")
    lines.append("//")
    lines.append("// One table per compiled chip, not per board model: pin validity (can this GPIO be")
    lines.append("// ADC1/output/etc) is a fact about the chip's silicon, shared by every board built on")
    lines.append("// it. Which of the chip's pins a specific board actually breaks out on its header is a")
    lines.append("// UI-picker concern handled by the SPA's full board catalog, not by firmware validation.")
    lines.append("")
    lines.append("#include <cstddef>")
    lines.append("#include <cstdint>")
    lines.append("#include <cstring>")
    lines.append("")
    lines.append("namespace ewfm {")
    lines.append("")
    lines.append(f'constexpr const char* kChipId = "{chip}";')
    lines.append("")
    for i, role in enumerate(ROLES):
        lines.append(f"constexpr uint8_t kPinRole{ROLE_CONST[role]} = 1U << {i}U;")
    lines.append("")
    lines.append("struct BoardPinCapability {")
    lines.append("    uint8_t gpio;")
    lines.append("    uint8_t roleMask;")
    lines.append("};")
    lines.append("")
    lines.append("constexpr BoardPinCapability kChipPins[] = {")
    for pin in pins:
        mask_expr = " | ".join(f"kPinRole{ROLE_CONST[r]}" for r in ROLES if pin["roleMask"] & (1 << ROLES.index(r))) or "0U"
        lines.append(f"    {{{pin['gpio']}U, {mask_expr}}},")
    lines.append("};")
    lines.append("")
    lines.append("constexpr size_t kChipPinCount = sizeof(kChipPins) / sizeof(kChipPins[0]);")
    lines.append("")
    lines.append("inline bool boardPinHasRole(uint8_t gpio, uint8_t roleMask) {")
    lines.append("    for (size_t i = 0; i < kChipPinCount; ++i) {")
    lines.append("        if (kChipPins[i].gpio == gpio) {")
    lines.append("            return (kChipPins[i].roleMask & roleMask) == roleMask;")
    lines.append("        }")
    lines.append("    }")
    lines.append("    return false;")
    lines.append("}")
    lines.append("")
    lines.append("// Short model-id strings only (matches BoardDefinition.boardId in the SPA's full")
    lines.append("// catalog) -- lets the firmware report which board models it recognizes for its own")
    lines.append("// chip (e.g. over the system status API) without carrying any of the per-board label/")
    lines.append("// note/exposed-pin detail, which stays SPA-only by design (see file header comment).")
    lines.append(f"constexpr const char* kSupportedBoardIds[] = {{{', '.join(chr(34) + b + chr(34) for b in board_ids)}}};")
    lines.append("constexpr size_t kSupportedBoardIdCount = sizeof(kSupportedBoardIds) / sizeof(kSupportedBoardIds[0]);")
    lines.append("")
    lines.append("// Enum, not a string, is what actually gets persisted (DeviceConfig.boardModel, 1 byte in")
    lines.append("// NVS) -- compiler-checked, no risk of a garbage board id ending up in storage. The")
    lines.append("// string form only exists at the REST boundary, via BoardModelFromString/Name below.")
    lines.append("// Same shape as gpioInputPullModeFromString/Name (BinarySensorDeviceConfig.cpp).")
    lines.append("enum class BoardModel : uint8_t {")
    for i, board_id in enumerate(board_ids):
        lines.append(f"    {board_id_pascal_case(board_id)} = {i}U,")
    lines.append("};")
    lines.append("")
    lines.append(f"constexpr BoardModel kDefaultBoardModel = BoardModel::{board_id_pascal_case(default_id)};")
    lines.append("")
    lines.append("inline bool boardModelFromString(const char* value, BoardModel& out) {")
    lines.append("    if (value == nullptr) {")
    lines.append("        return false;")
    lines.append("    }")
    lines.append("    for (size_t i = 0; i < kSupportedBoardIdCount; ++i) {")
    lines.append("        if (std::strcmp(value, kSupportedBoardIds[i]) == 0) {")
    lines.append("            out = static_cast<BoardModel>(i);")
    lines.append("            return true;")
    lines.append("        }")
    lines.append("    }")
    lines.append("    return false;")
    lines.append("}")
    lines.append("")
    lines.append("inline const char* boardModelName(BoardModel model) {")
    lines.append("    const size_t index = static_cast<size_t>(model);")
    lines.append("    return index < kSupportedBoardIdCount ? kSupportedBoardIds[index] : \"\";")
    lines.append("}")
    lines.append("")
    lines.append("} // namespace ewfm")
    lines.append("")
    return "\n".join(lines)


def render_ts(boards: list[dict]) -> str:
    lines = []
    lines.append("// GENERATED by tools/devicegen/generate_boards.py from tools/devicegen/boards/*/*.board.yaml")
    lines.append("// DO NOT EDIT BY HAND -- see docs/pin-configuration-conventions.md")
    lines.append("//")
    lines.append("// Unlike the firmware header (one chip's table only, matching what's actually")
    lines.append("// compiled), this carries every researched board across every chip -- the SPA isn't")
    lines.append("// compiled per chip, so there's no reason to leave any of it out. Board-specific detail")
    lines.append("// (labels, notes, which of the chip's pins this exact board exposes) lives only here.")
    lines.append("")
    chips = sorted({b["chip"] for b in boards})
    lines.append("export type PinRole = " + " | ".join(f"'{r}'" for r in ROLES))
    lines.append("export type ChipId = " + " | ".join(f"'{c}'" for c in chips))
    lines.append("")
    lines.append("export interface BoardPinCapability {")
    lines.append("  gpio: number")
    lines.append("  roles: PinRole[]")
    lines.append("  fixedDefaultFor?: string")
    lines.append("  note?: string")
    lines.append("}")
    lines.append("")
    lines.append("// Physical top-to-bottom header order, for BoardPinoutDiagram.vue only -- entries are")
    lines.append("// either a GPIO number (must appear in this board's `pins`) or a fixed non-GPIO silkscreen")
    lines.append("// label ('3V3', '5V', 'GND', 'EN', 'NC', ...). Omitted on boards where the real physical")
    lines.append("// order couldn't be confirmed from a source -- see docs/esp32-board-models.md.")
    lines.append("export interface BoardPinoutLayout {")
    lines.append("  left: (number | string)[]")
    lines.append("  right: (number | string)[]")
    lines.append("}")
    lines.append("")
    lines.append("export interface BoardDefinition {")
    lines.append("  chip: ChipId")
    lines.append("  boardId: string")
    lines.append("  label: string")
    lines.append("  pins: BoardPinCapability[]")
    lines.append("  layout?: BoardPinoutLayout")
    lines.append("}")
    lines.append("")
    lines.append("export const BOARD_CATALOG: Record<string, BoardDefinition> = {")
    for board in sorted(boards, key=lambda b: (b["chip"], b["boardId"])):
        lines.append(f"  '{board['boardId']}': {{")
        lines.append(f"    chip: '{board['chip']}',")
        lines.append(f"    boardId: '{board['boardId']}',")
        lines.append(f"    label: '{board['label']}',")
        lines.append("    pins: [")
        for pin in board["pins"]:
            roles_ts = ", ".join(f"'{r}'" for r in pin["roles"])
            extra = ""
            fixed = pin.get("fixedDefaultFor")
            if fixed:
                extra += f", fixedDefaultFor: '{fixed}'"
            note = pin.get("note")
            if note:
                escaped = note.replace("\\", "\\\\").replace("'", "\\'")
                extra += f", note: '{escaped}'"
            lines.append(f"      {{ gpio: {pin['gpio']}, roles: [{roles_ts}]{extra} }},")
        lines.append("    ],")
        layout = board.get("layout")
        if layout:
            def side_ts(entries: list) -> str:
                return ", ".join(str(e) if isinstance(e, int) else f"'{e}'" for e in entries)
            lines.append("    layout: {")
            lines.append(f"      left: [{side_ts(layout['left'])}],")
            lines.append(f"      right: [{side_ts(layout['right'])}],")
            lines.append("    },")
        lines.append("  },")
    lines.append("}")
    lines.append("")
    lines.append("export const CHIP_IDS: ChipId[] = [" + ", ".join(f"'{c}'" for c in chips) + "]")
    lines.append("")
    return "\n".join(lines)


def clang_format(text: str) -> str:
    """Runs the project's .clang-format over generated C++ so `scripts/lint.sh` never flags the
    output -- e.g. a long kSupportedBoardIds initializer list needs the same reflow a human editing
    this by hand would get from their editor's format-on-save."""
    result = subprocess.run(
        ["clang-format", f"--assume-filename={CPP_OUT.name}"],
        input=text, capture_output=True, text=True, cwd=REPO_ROOT, check=True,
    )
    return result.stdout


def generate_firmware(chip: str) -> int:
    boards = load_all_boards()
    chip_boards = boards_for_chip(boards, chip)
    CPP_OUT.write_text(clang_format(render_cpp(chip, chip_boards)))
    print(f"generate-boards: wrote {CPP_OUT.relative_to(REPO_ROOT)} for chip '{chip}' "
          f"({len(chip_pin_union(chip_boards))} pins, from {len(chip_boards)} board(s))")
    return 0


def generate_spa() -> int:
    boards = load_all_boards()
    TS_OUT.parent.mkdir(parents=True, exist_ok=True)
    TS_OUT.write_text(render_ts(boards))
    print(f"generate-boards: wrote {TS_OUT.relative_to(REPO_ROOT)} ({len(boards)} boards)")
    return 0


def check_firmware(chip: str) -> int:
    boards = load_all_boards()
    chip_boards = boards_for_chip(boards, chip)
    expected = clang_format(render_cpp(chip, chip_boards))
    if not CPP_OUT.exists() or CPP_OUT.read_text() != expected:
        print(f"check-boards: {CPP_OUT.relative_to(REPO_ROOT)} is out of date for chip '{chip}' "
              f"-- run generate_boards.py generate --chip {chip}")
        return 1
    print(f"check-boards: OK -- {CPP_OUT.relative_to(REPO_ROOT)} matches chip '{chip}'")
    return 0


def check_spa() -> int:
    boards = load_all_boards()
    expected = render_ts(boards)
    if not TS_OUT.exists() or TS_OUT.read_text() != expected:
        print(f"check-boards: {TS_OUT.relative_to(REPO_ROOT)} is out of date -- run generate_boards.py generate-spa")
        return 1
    print(f"check-boards: OK -- {TS_OUT.relative_to(REPO_ROOT)} matches board YAML sources")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=["generate", "generate-spa", "check", "check-spa"])
    ap.add_argument("--chip", default=DEFAULT_CHIP, help=f"chip id for 'generate'/'check' (default: {DEFAULT_CHIP})")
    a = ap.parse_args()
    if a.mode == "generate":
        return generate_firmware(a.chip)
    if a.mode == "check":
        return check_firmware(a.chip)
    if a.mode == "generate-spa":
        return generate_spa()
    return check_spa()


if __name__ == "__main__":
    sys.exit(main())
