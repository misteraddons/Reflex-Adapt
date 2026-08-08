#!/usr/bin/env python3
"""Regression guards for independent Windows and MiSTer/Linux Auto outputs."""

from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def source(*parts: str) -> str:
    return ROOT.joinpath(*parts).read_text(encoding="utf-8")


class HostOutputPreferenceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.store = source("firmware", "core", "settings_store.h")
        cls.registry = source("firmware", "core", "settings_registry.h")
        cls.boot = source("firmware", "core", "boot", "boot_storage_runtime.cpp")
        cls.runtime = source("firmware", "output", "output_runtime_state.cpp")
        cls.autodetect = source(
            "firmware", "output", "autodetect", "output_autodetect_runtime.cpp")
        cls.mode_labels = source("firmware", "menu", "menu_mode_labels.cpp")
        cls.home_line = source("firmware", "menu", "menu_home_mode_line.cpp")
        cls.descriptors = source("firmware", "menu", "menu_descriptors_data.cpp")
        cls.visibility = source("firmware", "menu", "menu_helpers_visibility.cpp")
        cls.serial_core = source("firmware", "core", "serial_core_commands.cpp")
        cls.serial_native = source(
            "firmware", "core", "runtime", "runtime_serial_debug.cpp")
        cls.webhid = source(
            "firmware", "output", "usb", "webhid", "webhid_settings_reports.cpp")
        cls.webhid_protocol = source(
            "firmware", "output", "usb", "webhid", "webhid_protocol.h")
        cls.web = source("web", "Adapt.html")
        cls.manager = source("tools", "adapt_manager", "reflex_adapt_manager.py")

    def test_current_eeprom_layout_and_setting_ids_are_consistent(self) -> None:
        self.assertIn(
            'static_assert(sizeof(GlobalSettingsRecord) == 20', self.store)
        self.assertIn("SETTINGS_SCHEMA_VERSION = 2", self.store)
        self.assertNotIn("ReservedPsxPeriph", self.store)
        self.assertNotIn("reserved_psx_periph", self.store)
        self.assertRegex(
            self.store,
            r"SystemMenuHotkey,\s+KioskMode,\s+MisterOutput,\s+Count,")
        self.assertIn(
            "offsetof(GlobalSettingsRecord, win_output)", self.registry)
        self.assertGreaterEqual(
            self.registry.count("offsetof(GlobalSettingsRecord, win_output)"), 2)

    def test_packed_preferences_are_independent_and_legacy_safe(self) -> None:
        self.assertIn("WINDOWS_OUTPUT_PREFERENCE_MASK = 0x03", self.store)
        self.assertIn("MISTER_OUTPUT_PREFERENCE_SHIFT = 2", self.store)
        self.assertIn("MISTER_OUTPUT_PREFERENCE_MASK = 0x0C", self.store)
        for legacy_raw in (0, 1, 2):
            self.assertEqual((legacy_raw & 0x0C) >> 2, 0)
        for windows in range(3):
            for mister in range(3):
                raw = windows | (mister << 2)
                self.assertEqual(raw & 0x03, windows)
                self.assertEqual((raw & 0x0C) >> 2, mister)
        for setting in ("WinOutput", "MisterOutput"):
            self.assertIn(f"id == SettingId::{setting}", self.registry)
        self.assertIn(
            "encodeWindowsOutputPreference(target, (uint8_t)value)", self.registry)
        self.assertIn(
            "encodeMisterOutputPreference(target, (uint8_t)value)", self.registry)

    def test_menu_and_boot_load_both_preferences(self) -> None:
        self.assertIn(
            "readRecordSettingValue(globalSettings, SettingId::WinOutput)",
            self.boot)
        self.assertIn(
            "readRecordSettingValue(globalSettings, SettingId::MisterOutput)",
            self.boot)
        self.assertIn(".id = menu_item_mister_output", self.descriptors)
        self.assertIn(".menu_var = &menu_mister_output", self.descriptors)
        self.assertIn(".setting_id = SettingId::MisterOutput", self.descriptors)
        self.assertIn("case menu_item_mister_output:", self.visibility)

    def test_generic_fallback_uses_preference_without_new_host_detection(self) -> None:
        getter = self.runtime.split(
            "outputMode_t get_configured_mister_auto_output_mode()", 1)[1].split(
                "bool output_supports_dpad_buttons", 1)[0]
        self.assertIn("switch (menu_mister_output)", getter)
        self.assertIn("return OUTPUT_MISTER", getter)
        self.assertIn("return OUTPUT_XINPUT2P", getter)
        self.assertIn("return OUTPUT_KEYBOARD", getter)
        self.assertIn("!is_xinput2p_output_enabled()", getter)

        fallback = self.autodetect.split(
            "static void auto_detect_trigger_fallback_hid()", 1)[1].split(
                "static void auto_detect_trigger_ps3_hid_fallback", 1)[0]
        self.assertIn(
            "get_configured_mister_auto_output_mode()", fallback)
        self.assertIn("auto_detect_trigger_reenum((uint32_t)fallbackOutput)", fallback)
        self.assertNotIn(
            "auto_detect_trigger_reenum((uint32_t)OUTPUT_MISTER)", fallback)
        self.assertIn(
            "AUTO_STATE_FALLBACK_HID &&", self.runtime)
        self.assertIn(
            "get_configured_mister_auto_output_mode(), autoDetectState",
            self.autodetect)
        self.assertIn(
            "case AUTO_STATE_FALLBACK_HID: return get_configured_mister_auto_output_mode()",
            self.mode_labels)
        for label in ("MiSTer DIn", "MiSTer XIn", "MiSTer Kbd"):
            self.assertIn(label, self.home_line)

    def test_xinput_transport_is_host_specific(self) -> None:
        windows_getter = self.runtime.split(
            "outputMode_t get_configured_windows_auto_output_mode()", 1
        )[1].split(
            "outputMode_t get_configured_mister_auto_output_mode()", 1
        )[0]
        mister_getter = self.runtime.split(
            "outputMode_t get_configured_mister_auto_output_mode()", 1
        )[1].split(
            "bool output_supports_dpad_buttons", 1
        )[0]
        self.assertIn("return OUTPUT_XINPUTW", windows_getter)
        self.assertNotIn("return OUTPUT_XINPUT2P", windows_getter)
        self.assertIn("return OUTPUT_XINPUT2P", mister_getter)
        self.assertNotIn("return OUTPUT_XINPUTW", mister_getter)

        canonical = self.runtime.split(
            "outputMode_t canonicalizeOutputMode(outputMode_t mode)", 1
        )[1].split(
            "outputMode_t sanitizeConfiguredOutputMode", 1
        )[0]
        self.assertNotIn("OUTPUT_XINPUTW", canonical)
        self.assertIn(
            "if (value == OUTPUT_XINPUTW)", self.registry)
        self.assertIn(
            "return OUTPUT_XINPUTW;", self.registry)
    def test_management_surfaces_expose_and_preserve_both_preferences(self) -> None:
        self.assertIn(
            'case SettingId::MisterOutput: return "MISTER_OUTPUT"',
            self.serial_core)
        self.assertIn("MISTER <MODE>", self.serial_native)
        self.assertIn("OK:MISTER_OUT=", self.serial_native)
        self.assertIn(
            "saveSystemSettingByte(SettingId::WinOutput, newWinOutput)",
            self.serial_native)
        self.assertNotIn("globalSettings.win_output = newWinOutput", self.serial_native)
        self.assertIn("WEBHID_SETTING_MISTER_OUTPUT        49", self.webhid_protocol)
        self.assertIn("WEBHID_SETTINGS_MISTER_OUTPUT_MASK  0x60", self.webhid_protocol)
        self.assertIn("case WEBHID_SETTING_MISTER_OUTPUT:", self.webhid)
        self.assertIn("SettingId::MisterOutput", self.webhid)

    def test_web_and_manager_present_all_three_choices(self) -> None:
        self.assertIn('id="settings-mister-output"', self.web)
        self.assertIn("WEBHID_SETTINGS_MISTER_OUTPUT_MASK", self.web)
        self.assertIn(
            "[[WEBHID_SETTING_MISTER_OUTPUT, misterOutput], [33, winOutput]]",
            self.web)
        self.assertIn(
            "[[33, winOutput], [WEBHID_SETTING_MISTER_OUTPUT, misterOutput]]",
            self.web)
        self.assertIn(
            "lastSettingsSnapshot?.outputMode === 0",
            self.web)
        self.assertIn("settings.push(...hostOutputSettings)", self.web)
        self.assertIn("updateMisterOutputWarning", self.web)
        self.assertIn('"MiSTer/Linux Output"', self.manager)
        self.assertIn("def host_output_modes_flow", self.manager)
        self.assertIn("ordered_ids = (15, 58)", self.manager)
        self.assertIn("host_output_modes_flow(ui, port_name, mode)", self.manager)
        self.assertRegex(
            self.manager,
            r"58:\s*tuple\(enumerate\(\(\"DInput\", \"XInput\", \"Keyboard\"\)\)\)")


if __name__ == "__main__":
    unittest.main()
