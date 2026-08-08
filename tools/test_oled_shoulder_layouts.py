#!/usr/bin/env python3
"""Release guard for OLED shoulder-button spacing and layout routing."""

from __future__ import annotations

import ast
import re
import unittest
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MENU = ROOT / "firmware" / "menu"
LAYOUT_SOURCES = (
    MENU / "menu_pad_layouts_data.cpp",
    MENU / "menu_pad_layouts_data_arcade.cpp",
    MENU / "menu_pad_layouts_data_classic.cpp",
    MENU / "menu_pad_layouts_data_sega.cpp",
    MENU / "menu_virtual_output_pad.cpp",
)

CHARACTER_WIDTH = 6
NORMAL_GLYPH_WIDTH = 5
WIDE_GLYPH_WIDTH = 11
HALF_CHARACTER = CHARACTER_WIDTH // 2
MINIMUM_BUTTON_CLEARANCE = 1
SHOULDER_MASKS = ("GPAD_L1", "GPAD_R1", "GPAD_L2", "GPAD_R2")
# Not displayed by the current Classic2USB home-screen policy.
SPACING_AUDIT_DISABLED_LAYOUTS = {
    "padLayoutModernOutput",
    "padLayoutUsbModern",
}
FOUR_SHOULDER_LAYOUTS = {
    "padLayoutModernOutput",
    "padLayoutPS3",
    "padLayoutPSX",
    "padLayoutPSXDigital",
    "padLayoutPSXDualShock",
    "padLayoutUsbModern",
    "padLayoutUsbSwitch",
    "padLayoutWii",
}

ARRAY_PATTERN = re.compile(
    r"const\s+PadButton\s+(padLayout[A-Za-z0-9_]+)\s*\[\]\s*=\s*"
    r"\{(.*?)\n\};",
    flags=re.DOTALL,
)
ENTRY_PATTERN = re.compile(
    r"\{\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^}]+)\}"
)


@dataclass(frozen=True)
class Entry:
    mask: str
    row: int
    col: int
    on: str
    off: str


def integer_expression(text: str) -> int:
    node = ast.parse(text.strip(), mode="eval").body

    def evaluate(current: ast.AST) -> int:
        if isinstance(current, ast.Constant) and isinstance(current.value, int):
            return current.value
        if isinstance(current, ast.UnaryOp) and isinstance(current.op, ast.USub):
            return -evaluate(current.operand)
        if isinstance(current, ast.BinOp):
            left = evaluate(current.left)
            right = evaluate(current.right)
            if isinstance(current.op, ast.Add):
                return left + right
            if isinstance(current.op, ast.Sub):
                return left - right
            if isinstance(current.op, ast.Mult):
                return left * right
        raise ValueError(f"unsupported integer expression: {text}")

    return evaluate(node)


MACRO_REFERENCE_PATTERN = re.compile(r"\bPAD_LAYOUT_[A-Z0-9_]+_ENTRIES\b")


def parse_entries(body: str) -> list[Entry]:
    entries: list[Entry] = []
    for entry_match in ENTRY_PATTERN.finditer(body):
        mask, row, col, on, off = (
            value.strip() for value in entry_match.groups()
        )
        if not mask.startswith("GPAD_"):
            continue
        entries.append(
            Entry(
                mask=mask,
                row=integer_expression(row),
                col=integer_expression(col),
                on=on,
                off=off,
            )
        )
    return entries


def read_macro_bodies(source: str) -> dict[str, str]:
    macros: dict[str, str] = {}
    lines = source.splitlines()
    index = 0
    while index < len(lines):
        match = re.match(
            r"\s*#define\s+(PAD_LAYOUT_[A-Z0-9_]+_ENTRIES)\s+\\\s*$",
            lines[index],
        )
        if match is None:
            index += 1
            continue
        name = match.group(1)
        body_lines: list[str] = []
        index += 1
        while index < len(lines):
            line = lines[index].rstrip()
            continued = line.endswith("\\")
            if continued:
                line = line[:-1]
            body_lines.append(line)
            index += 1
            if not continued:
                break
        macros[name] = "\n".join(body_lines)
    return macros


def expand_entries(
    body: str,
    macros: dict[str, str],
    stack: tuple[str, ...] = (),
) -> list[Entry]:
    entries = parse_entries(body)
    for reference in MACRO_REFERENCE_PATTERN.findall(body):
        if reference not in macros:
            raise AssertionError(f"missing OLED layout macro: {reference}")
        if reference in stack:
            raise AssertionError(f"recursive OLED layout macro: {reference}")
        entries.extend(
            expand_entries(macros[reference], macros, stack + (reference,))
        )
    return entries


def read_layouts() -> dict[str, list[Entry]]:
    sources = {
        path: path.read_text(encoding="utf-8") for path in LAYOUT_SOURCES
    }
    macros: dict[str, str] = {}
    for source in sources.values():
        macros.update(read_macro_bodies(source))

    layouts: dict[str, list[Entry]] = {}
    for source in sources.values():
        for match in ARRAY_PATTERN.finditer(source):
            name, body = match.groups()
            if name in layouts:
                raise AssertionError(f"duplicate OLED pad layout: {name}")
            layouts[name] = expand_entries(body, macros)
    return layouts


def rendered_entry_width(entry: Entry) -> int:
    return (
        WIDE_GLYPH_WIDTH
        if entry.on.startswith("PAD_WIDE_")
        else NORMAL_GLYPH_WIDTH
    )


def intentional_analog_button_pair(
    layout_name: str, left: Entry, right: Entry
) -> bool:
    return (
        layout_name in {"padLayoutGC", "padLayoutWii"}
        and left.mask == right.mask
        and {left.on, right.on} == {"PAD_SHOULDER_ON", "PAD_RECT_ON"}
    )


def one(entries: list[Entry], mask: str) -> Entry:
    matches = [entry for entry in entries if entry.mask == mask]
    if len(matches) != 1:
        raise AssertionError(
            f"expected exactly one {mask}, found {len(matches)}"
        )
    return matches[0]


class OledShoulderLayoutReleaseGuard(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.layouts = read_layouts()
        cls.mode_source = (
            MENU / "menu_pad_layouts_mode.cpp"
        ).read_text(encoding="utf-8")
        cls.player_source = (
            MENU / "menu_pad_layouts_player.cpp"
        ).read_text(encoding="utf-8")
        cls.render_source = (
            MENU / "menu_main_display_pad_render.cpp"
        ).read_text(encoding="utf-8")
        cls.status_source = (
            MENU / "menu_main_display_status.cpp"
        ).read_text(encoding="utf-8")

    def test_all_oled_pad_buttons_have_one_blank_pixel(self) -> None:
        for layout_name, entries in sorted(self.layouts.items()):
            if layout_name in SPACING_AUDIT_DISABLED_LAYOUTS:
                continue
            self.assertTrue(entries, f"{layout_name} must expose parsed buttons")
            for row in sorted({entry.row for entry in entries}):
                row_entries = sorted(
                    (entry for entry in entries if entry.row == row),
                    key=lambda entry: entry.col,
                )
                for left, right in zip(row_entries, row_entries[1:]):
                    if intentional_analog_button_pair(
                        layout_name, left, right
                    ):
                        continue
                    clearance = (
                        right.col - left.col - rendered_entry_width(left)
                    )
                    self.assertGreaterEqual(
                        clearance,
                        MINIMUM_BUTTON_CLEARANCE,
                        (
                            f"{layout_name} row {row}: {left.mask}@{left.col} "
                            f"to {right.mask}@{right.col} leaves "
                            f"{clearance}px; minimum is "
                            f"{MINIMUM_BUTTON_CLEARANCE}px"
                        ),
                    )

    def test_all_four_shoulder_layouts_are_inventoried(self) -> None:
        discovered: set[str] = set()
        for name, entries in self.layouts.items():
            if name == "padLayoutPSXNeGcon":
                continue
            shoulder_entries = {
                entry.mask: entry
                for entry in entries
                if entry.mask in SHOULDER_MASKS
                and entry.on == "PAD_SHOULDER_ON"
                and entry.off == "PAD_SHOULDER_OFF"
            }
            if set(shoulder_entries) == set(SHOULDER_MASKS):
                discovered.add(name)
        self.assertEqual(discovered, FOUR_SHOULDER_LAYOUTS)

    def test_four_shoulder_layouts_use_half_character_inner_inset(self) -> None:
        for name in sorted(FOUR_SHOULDER_LAYOUTS):
            with self.subTest(layout=name):
                if name == "padLayoutWii":
                    continue
                entries = self.layouts[name]
                l1 = one(entries, "GPAD_L1")
                r1 = one(entries, "GPAD_R1")
                l2 = one(entries, "GPAD_L2")
                r2 = one(entries, "GPAD_R2")
                for entry in (l1, r1, l2, r2):
                    self.assertEqual(entry.row, 0)
                    self.assertEqual(entry.on, "PAD_SHOULDER_ON")
                    self.assertEqual(entry.off, "PAD_SHOULDER_OFF")
                self.assertEqual(l2.col % CHARACTER_WIDTH, 0)
                self.assertEqual(r2.col % CHARACTER_WIDTH, 0)
                self.assertEqual(l1.col % CHARACTER_WIDTH, HALF_CHARACTER)
                self.assertEqual(r1.col % CHARACTER_WIDTH, HALF_CHARACTER)
                self.assertLess(l2.col, l1.col)
                self.assertLess(l1.col, r1.col)
                self.assertLess(r1.col, r2.col)
                self.assertGreaterEqual(
                    l1.col - l2.col, CHARACTER_WIDTH + HALF_CHARACTER
                )
                self.assertGreaterEqual(
                    r2.col - r1.col, CHARACTER_WIDTH + HALF_CHARACTER
                )
                self.assertEqual(l2.col + r2.col, l1.col + r1.col)

    def test_wii_classic_shoulders_use_requested_insets(self) -> None:
        entries = self.layouts["padLayoutWii"]
        analog = [entry for entry in entries if entry.on == "PAD_SHOULDER_ON"]
        digital = [entry for entry in entries if entry.on == "PAD_RECT_ON"]
        self.assertEqual(
            (one(analog, "GPAD_L1").row, one(analog, "GPAD_L1").col), (0, 9)
        )
        self.assertEqual(
            (one(analog, "GPAD_R1").row, one(analog, "GPAD_R1").col), (0, 51)
        )
        self.assertEqual(
            (one(digital, "GPAD_L1").row, one(digital, "GPAD_L1").col), (0, 0)
        )
        self.assertEqual(
            (one(digital, "GPAD_R1").row, one(digital, "GPAD_R1").col), (0, 60)
        )
        l2 = one(analog, "GPAD_L2")
        r2 = one(analog, "GPAD_R2")
        l1 = one(analog, "GPAD_L1")
        r1 = one(analog, "GPAD_R1")
        self.assertEqual((l2.row, l2.col), (1, 21))
        self.assertEqual((r2.row, r2.col), (1, 39))

        gauge_width = 18
        self.assertEqual(l2.col + CHARACTER_WIDTH, l1.col + gauge_width)
        self.assertEqual(r2.col, r1.col + CHARACTER_WIDTH - gauge_width)

        second_row = sorted(
            (entry for entry in entries if entry.row == 1),
            key=lambda entry: entry.col,
        )
        for left, right in zip(second_row, second_row[1:]):
            self.assertGreaterEqual(
                right.col - (left.col + CHARACTER_WIDTH),
                CHARACTER_WIDTH,
            )
        self.assertIn(
            'std::strcmp(frame->controller_type_name, "Classic") == 0',
            self.render_source,
        )
        self.assertIn("return left ? GPAD_L1 : GPAD_R1;", self.render_source)
        self.assertIn("return left ? GPAD_L2 : GPAD_R2;", self.render_source)

    def test_psx_start_and_select_use_the_second_pad_row(self) -> None:
        for name in ("padLayoutPSX", "padLayoutPSXDigital", "padLayoutPSXDualShock"):
            with self.subTest(layout=name):
                entries = self.layouts[name]
                self.assertEqual(one(entries, "GPAD_SELECT").row, 2)
                self.assertEqual(one(entries, "GPAD_START").row, 2)

    def test_dualshock_without_analog_triggers_spreads_l1_r1_outward(self) -> None:
        start = self.render_source.index("int16_t padButtonColumnForFrame")
        helper = self.render_source[start : start + 900]
        self.assertIn(
            "layout == menu_pad_layouts_internal::padLayoutPSXDualShock",
            helper,
        )
        self.assertIn("!frame->HAS_ANALOG_TRIGGERS", helper)
        self.assertIn("button.mask == GPAD_L1", helper)
        self.assertIn("button.col - kPadGlyphWidth", helper)
        self.assertIn("button.mask == GPAD_R1", helper)
        self.assertIn("button.col + kPadGlyphWidth", helper)
        self.assertIn(
            "padButtonColumnForFrame(button, layout, analogFrame)",
            self.render_source,
        )

    def test_both_player_controller_names_trigger_home_redraw(self) -> None:
        self.assertIn("bool didControllerNamesChange()", self.render_source)
        helper_start = self.render_source.index("bool didControllerNamesChange()")
        helper = self.render_source[helper_start : helper_start + 1200]
        self.assertIn("lastDisplayedControllerType[2][16]", helper)
        self.assertIn("player < 2", helper)
        self.assertIn("controllerFrameConst(player).controller_type_name", helper)
        self.assertNotIn("didPrimaryControllerNameChange", self.render_source)
        main_source = (MENU / "menu_main_display.cpp").read_text(encoding="utf-8")
        self.assertIn(
            "menu_main_display_internal::didControllerNamesChange()",
            main_source,
        )

    def test_three_shoulder_nintendo_layouts_are_inset(self) -> None:
        n64 = self.layouts["padLayoutN64"]
        n64_l = one(n64, "GPAD_L1")
        n64_r = one(n64, "GPAD_R1")
        n64_z = one(n64, "GPAD_L2")
        self.assertEqual((n64_l.row, n64_l.col), (0, 21))
        self.assertEqual((n64_r.row, n64_r.col), (0, 33))
        self.assertEqual(n64_z.on, "PAD_SHOULDER_ON")

        gamecube = self.layouts["padLayoutGC"]
        gc_l = one(
            [entry for entry in gamecube if entry.on == "PAD_SHOULDER_ON"],
            "GPAD_L2",
        )
        gc_r = one(
            [entry for entry in gamecube if entry.on == "PAD_SHOULDER_ON"],
            "GPAD_R2",
        )
        gc_l_digital = one(
            [entry for entry in gamecube if entry.on == "PAD_RECT_ON"], "GPAD_L2"
        )
        gc_r_digital = one(
            [entry for entry in gamecube if entry.on == "PAD_RECT_ON"], "GPAD_R2"
        )
        gc_z = one(gamecube, "GPAD_SELECT")
        self.assertEqual((gc_l.row, gc_l.col), (0, 9))
        self.assertEqual((gc_r.row, gc_r.col), (0, 45))
        self.assertEqual((gc_l_digital.row, gc_l_digital.col), (0, 0))
        self.assertEqual((gc_r_digital.row, gc_r_digital.col), (0, 54))
        self.assertEqual((gc_z.row, gc_z.col), (1, 36))

    def assert_type_routes_to(
        self, controller_type: str, layout_name: str
    ) -> None:
        marker = f'std::strcmp(typeName, "{controller_type}")'
        start = self.mode_source.index(marker)
        self.assertIn(
            f"*layout = {layout_name};",
            self.mode_source[start : start + 500],
        )

    def test_usb_controller_families_route_to_audited_layouts(self) -> None:
        routes = {
            "Switch": "padLayoutUsbSwitch",
            "DualShock3": "padLayoutPS3",
            "DualShock4": "padLayoutPSX",
            "DualSense": "padLayoutPSX",
            "XInput": "padLayoutUsbModern",
            "8BitDo": "padLayoutUsbModern",
        }
        for controller_type, layout_name in routes.items():
            with self.subTest(controller=controller_type):
                self.assert_type_routes_to(controller_type, layout_name)

    def test_negcon_uses_bottom_row_horizontal_twist_gauge(self) -> None:
        route = self.player_source.index('std::strcmp(typeName, "neGcon")')
        self.assertIn("padLayoutPSXNeGcon", self.player_source[route : route + 250])
        self.assertIn("kNeGconTwistGaugeRow = 7", self.render_source)
        self.assertIn("kNeGconTwistGaugeWidth = 60", self.render_source)
        self.assertIn(
            "drawCenterOutSignedAxisGauge(frame != nullptr ? frame->LX : 0",
            self.render_source,
        )
        self.assertIn(
            'std::strcmp(port1.controller_type_name, "neGcon") == 0',
            self.status_source,
        )
        self.assertIn(
            'std::strcmp(port2.controller_type_name, "neGcon") == 0',
            self.status_source,
        )
        self.assertIn('std::strcmp(port2_name, "neGcon") == 0', self.status_source)
    def test_classic_modes_and_playstation_types_route_to_audited_layouts(
        self,
    ) -> None:
        for mode, layout_name in (
            ("RZORD_WII", "padLayoutWii"),
            ("RZORD_GAMECUBE", "padLayoutGC"),
            ("RZORD_PSX", "padLayoutPSX"),
        ):
            with self.subTest(mode=mode):
                start = self.mode_source.index(f"case {mode}:")
                self.assertIn(
                    f"*layout = {layout_name};",
                    self.mode_source[start : start + 250],
                )
        for controller_type, layout_name in (
            ("DualShock", "padLayoutPSXDualShock"),
            ("DualShock2", "padLayoutPSXDualShock"),
            ("Digital", "padLayoutPSXDigital"),
        ):
            with self.subTest(controller=controller_type):
                marker = f'std::strcmp(typeName, "{controller_type}")'
                start = self.player_source.index(marker)
                self.assertIn(
                    f"*layout = {layout_name};",
                    self.player_source[start : start + 500],
                )


if __name__ == "__main__":
    unittest.main()
