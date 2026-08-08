#!/usr/bin/env python3
"""Classic2USB source guards for the shared serial management surface."""

from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def source(*parts: str) -> str:
    return ROOT.joinpath(*parts).read_text(encoding="utf-8")


class ClassicSerialManagementTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.parser = source("firmware", "core", "serial_command_parser.cpp")
        cls.core = source("firmware", "core", "serial_core_commands.cpp")
        cls.management = source(
            "firmware", "core", "serial_management_commands.cpp")
        cls.runtime = source("firmware", "core", "serial_debug_runtime.cpp")
        cls.native = source(
            "firmware", "core", "runtime", "runtime_serial_debug.cpp")
        cls.rumble = source(
            "firmware", "core", "serial_rumble_commands.cpp")
        cls.menu = source(
            "firmware", "platform", "runtime", "platform_menu_runtime.cpp")

    def test_exact_parser_guards_lifecycle_commands(self) -> None:
        exact = self.parser.split(
            "bool serialTextEqualsExact", 1)[1].split(
                "bool serialCommandStartsWith", 1)[0]
        self.assertIn("return *text == '\\0';", exact)
        boot = self.core.split(
            "bool handleSerialBootCommand", 1)[1].split(
                "bool handleSerialStateCommand", 1)[0]
        self.assertIn('serialTextEqualsExact(command, "RESET")', boot)
        self.assertIn('serialTextEqualsExact(command, "BOOT")', boot)
        self.assertNotIn("serialTokenEquals(command", boot)

    def test_both_serial_paths_discover_management(self) -> None:
        self.assertIn(
            "handleSerialManagementCommand(command, out)", self.runtime)
        self.assertIn("appendSerialManagementHelp(out)", self.runtime)
        self.assertIn(
            "appendSerialManagementHelp(runtimeDebugCdc)", self.native)

    def test_every_release_setting_has_a_name(self) -> None:
        enum_body = source(
            "firmware", "core", "settings_store.h").split(
                "enum class SettingId", 1)[1].split("};", 1)[0]
        ids = []
        for line in enum_body.splitlines():
            match = re.match(r"\s*([A-Za-z0-9_]+)(?:\s*=.*)?,$", line)
            if match and match.group(1) != "Count":
                ids.append(match.group(1))
        switch = self.core.split(
            "const char* settingIdName", 1)[1].split(
                "char normalizeSettingNameChar", 1)[0]
        self.assertEqual(
            [], [item for item in ids if f"SettingId::{item}" not in switch])
        named_cases = [
            item for item in re.findall(
                r"case SettingId::([A-Za-z0-9_]+):", switch)
            if item != "Count"
        ]
        self.assertEqual(len(ids), len(named_cases))
        self.assertNotIn("TouchpadMouse", switch)
        self.assertIn("SET GET <ID|NAME>", self.core)

    def test_classic_command_surface_is_product_appropriate(self) -> None:
        for command in (
                "INPUT", "OUTPUT", "PLAYER", "MENU", "CONFIG", "TURBO",
                "REMAP", "AUTH", "STATS", "HISTORY"):
            self.assertIn(f'"{command}"', self.management)
        for excluded in (
                "TRANSPORT", "HIDMAP", "CONTROLLER LED",
                "ESP32", "WIFI", "BT PAIR"):
            self.assertNotIn(f'"{excluded}"', self.management)
        self.assertNotIn(
            'serialCommandStartsWith(command, "DINPUT"', self.management)
        for excluded_include in (
                "controller_selection_state", "hid_mapping_profiles",
                "usb_bt_composite", "esp32_spi"):
            self.assertNotIn(excluded_include, self.management)
        self.assertIn("ERR:PLAYER_LIST_ONLY", self.management)
        self.assertNotIn("setSelectedControllerPlayer", self.management)

    def test_destructive_operations_require_exact_confirmation(self) -> None:
        for marker in (
                "OK:FACTORY_RESET", "OK:TURBO_CLEAR", "OK:REMAP_CLEAR",
                "OK:AUTH_PS4_CLEARED", "OK:STATS_CLEAR",
                "OK:HISTORY_CLEAR"):
            location = self.management.index(marker)
            preceding = self.management[max(0, location - 1800):location]
            self.assertIn("CONFIRM", preceding, marker)
        self.assertIn(
            'serialTextEqualsExact(text, "DEFAULTS CONFIRM")', self.core)
        self.assertIn(
            'serialTextEqualsExact(confirmation, "CONFIRM")', self.core)

    def test_settings_are_strict_and_readback_verified(self) -> None:
        self.assertIn(
            "bool parseSettingModeOrCurrent(char*& text, DeviceEnum* mode)",
            self.core)
        self.assertIn("ERR:USE_INPUT_SET", self.core)
        self.assertIn("ERR:USE_OUTPUT_SET", self.core)
        self.assertIn("ERR:SET_WRITE_FAILED", self.core)
        self.assertIn("VERIFY=1 LIVE=", self.core)
        self.assertIn("ERR:SETTING_REQUIRES_CONCRETE_MODE", self.core)

    def test_classic_feedback_and_menu_controls_are_bounded(self) -> None:
        self.assertIn("RUMBLE TEST PLAYER <1-N>", self.rumble)
        self.assertIn("rumbleRuntimeStartTest(", self.rumble)
        legacy_test = self.rumble.split(
            'serialCommandStartsWith(text, "TEST"', 1)[1].split(
                "rumbleTestStart(", 1)[0]
        self.assertLess(
            legacy_test.index("serialParseLongToken(remainder, &rawMs)"),
            legacy_test.rindex("*serialSkipSpaces(remainder) != '\\0'"))
        self.assertIn("LED MODE <OFF|STATIC|BREATHING", self.runtime)
        self.assertIn("LED BRIGHTNESS <0-254>", self.runtime)
        self.assertNotIn("LED COLOR", self.runtime)
        self.assertNotIn("USBPORTS", self.runtime)
        for function in (
                "openControllerMenuFromSerial", "openSystemMenuFromSerial"):
            block = self.menu.split(
                f"void {function}()", 1)[1].split("\n}", 1)[0]
            self.assertIn("closeMenusFromSerial();", block)
        close = self.menu.split(
            "void closeMenusFromSerial()", 1)[1].split("\n}", 1)[0]
        self.assertIn("quickConfig.discard()", close)


if __name__ == "__main__":
    unittest.main()
