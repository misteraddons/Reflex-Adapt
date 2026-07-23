#!/usr/bin/env python3
"""Release-surface audit for the Classic2USB firmware profile.

This is intentionally source-based. It catches the common release regressions:
new modes without labels, menu items without catalog rows, settings IDs without
EEPROM storage, and browser fallback catalogs drifting away from firmware.
"""

from __future__ import annotations

import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//.*", "", text)


def enum_members(text: str, enum_name: str) -> list[str]:
    match = re.search(rf"enum(?:\s+class)?\s+{re.escape(enum_name)}[^\{{]*\{{(.*?)\}};", text, re.S)
    if not match:
        raise AssertionError(f"Missing enum {enum_name}")
    body = strip_comments(match.group(1))
    body = "\n".join(line for line in body.splitlines() if not line.strip().startswith("#"))
    members: list[str] = []
    for raw in body.split(","):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        name = line.split("=", 1)[0].strip()
        if re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", name):
            members.append(name)
    return members


def struct_fields(text: str, struct_name: str) -> set[str]:
    match = re.search(rf"struct\s+{re.escape(struct_name)}\s*\{{(.*?)\}};", text, re.S)
    if not match:
        raise AssertionError(f"Missing struct {struct_name}")
    return set(re.findall(r"\b(?:u?int(?:8|16|32)_t|int8_t)\s+([A-Za-z_][A-Za-z0-9_]*)\b", match.group(1)))


def function_block(text: str, signature: str) -> str:
    start = text.find(signature)
    if start < 0:
        raise AssertionError(f"Missing function {signature}")
    brace = text.find("{", start)
    depth = 0
    for index in range(brace, len(text)):
        char = text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return text[brace:index + 1]
    raise AssertionError(f"Unterminated function {signature}")


def case_names(text: str, signature: str, prefix: str) -> set[str]:
    block = function_block(text, signature)
    return set(re.findall(rf"\bcase\s+({re.escape(prefix)}[A-Za-z0-9_]+)\s*:", block))


def case_return_new_modules(text: str) -> dict[str, str]:
    modules: dict[str, str] = {}
    for match in re.finditer(r"\bcase\s+(RZORD_[A-Z0-9_]+)\s*:", text):
        start = match.end()
        next_case = re.search(r"\b(?:case\s+RZORD_[A-Z0-9_]+\s*:|default\s*:)", text[start:])
        end = start + next_case.start() if next_case else len(text)
        case_body = text[start:end]
        module = re.search(r"\breturn\s+new\s+(RZInput[A-Za-z0-9_]+)\s*\(", case_body)
        if module:
            modules[match.group(1)] = module.group(1)
    return modules


def input_module_descriptions(text: str) -> dict[str, list[str]]:
    descriptions: dict[str, list[str]] = {}
    for match in re.finditer(r"const\s+char\*\s+(RZInput[A-Za-z0-9_]+)::getDescription\(\)", text):
        brace = text.find("{", match.end())
        if brace < 0:
            continue
        depth = 0
        end = brace
        for index in range(brace, len(text)):
            char = text[index]
            if char == "{":
                depth += 1
            elif char == "}":
                depth -= 1
                if depth == 0:
                    end = index + 1
                    break
        body = text[brace:end]
        descriptions[match.group(1)] = re.findall(r"\"([^\"]+)\"", body)
    return descriptions


def setting_specs(text: str) -> list[dict[str, str]]:
    match = re.search(r"kSettingSpecs\[\]\s*=\s*\{(.*?)\n\};", text, re.S)
    if not match:
        raise AssertionError("Missing kSettingSpecs")
    specs = []
    for line in match.group(1).splitlines():
        line = line.strip().rstrip(",")
        if not line.startswith("{SettingScope::"):
            continue
        offset = re.search(r"offsetof\(([^,]+),\s*([^)]+)\)", line)
        if not offset:
            raise AssertionError(f"Setting spec without offsetof: {line}")
        default_part = line.split("),", 1)[1].split(",", 1)[0].strip()
        specs.append({
            "line": line,
            "record": offset.group(1).strip(),
            "field": offset.group(2).strip(),
            "default": default_part,
        })
    return specs


def parse_adapt_output_catalog(text: str) -> tuple[list[str], set[int]]:
    modes_match = re.search(r"const OUTPUT_MODES\s*=\s*\[(.*?)\];", text, re.S)
    hidden_match = re.search(r"const HIDDEN_OUTPUT_MODES\s*=\s*\[(.*?)\];", text, re.S)
    if not modes_match or not hidden_match:
        raise AssertionError("Adapt.html output catalog is missing")
    modes = re.findall(r"'([^']+)'", modes_match.group(1))
    hidden = {int(value) for value in re.findall(r"\b(\d+)\s*,?\s*//", hidden_match.group(1))}
    return modes, hidden


def parse_adapt_input_catalog(text: str) -> dict[int, str | None]:
    match = re.search(r"const INPUT_MODES\s*=\s*\[(.*?)\];", text, re.S)
    if not match:
        raise AssertionError("Adapt.html input catalog is missing")
    entries: dict[int, str | None] = {}
    for line in match.group(1).splitlines():
        item = re.search(r"(?:(?:'([^']+)')|(null))\s*,?\s*//\s*(\d+)", line)
        if item:
            entries[int(item.group(3))] = item.group(1) if item.group(2) is None else None
    return entries


def require(condition: bool, message: str, failures: list[str]) -> None:
    if not condition:
        failures.append(message)


def tracked_paths() -> list[Path]:
    output = subprocess.check_output(
        ["git", "ls-files"], cwd=ROOT, text=True, encoding="utf-8"
    )
    return [Path(line) for line in output.splitlines() if line]


def audit() -> tuple[list[str], list[str]]:
    failures: list[str] = []
    report: list[str] = []

    feature_gates = read("firmware/config/classic2usb/feature_gates.h")
    device_mode = read("firmware/core/device_mode.h")
    output_mode = read("firmware/output/output_mode.h")
    menu_input = read("firmware/menu/menu_input_mode.cpp")
    mode_labels = read("firmware/menu/menu_mode_labels.cpp")
    webhid_input = read("firmware/output/usb/webhid/webhid_input_modes.cpp")
    settings_store = read("firmware/core/settings_store.h")
    settings_registry = read("firmware/core/settings_registry.h")
    menu_defs = read("firmware/menu/menu_item_defs.h")
    menu_catalog = read("firmware/menu/menu_catalog.cpp")
    descriptors = read("firmware/menu/menu_descriptors_data.cpp")
    submenu_bridge = read("firmware/menu/menu_bridge_submenus.cpp")
    item_handlers = read("firmware/menu/menu_item_handlers_submenus.cpp")
    kiosk = read("firmware/menu/menu_submenu_kiosk.cpp")
    input_factory = read("firmware/input/factory/input_adapter_factory.cpp")
    input_sources = "\n".join(path.read_text(encoding="utf-8", errors="ignore") for path in (ROOT / "firmware/input").glob("**/*.cpp"))
    adapt_html = read("web/Adapt.html")
    board_profile = read("firmware/config/classic2usb/board.h")
    workflows = read(".github/workflows/build.yml") + "\n" + read(".github/workflows/classic2usb-release.yml")
    release_targets = read("tools/release_targets.json")

    require("default_envs = classic2usb" in read("platformio.ini"), "PlatformIO default env must stay classic2usb", failures)

    future_target_tokens = ("usb2" + "usb", "jvsio2" + "usb")
    personal_path_tokens = ("c:" + "\\users\\", "/" + "users/", "app" + "data")
    personal_mail_tokens = tuple(
        "@" + domain for domain in ("gmail.", "outlook.", "hotmail.", "proton.")
    )
    allowed_binary_assets = {
        Path("reflex-adapt-manager.json.zip"),
        Path("tools/adapt_manager/mister_maps.bin"),
    }
    generated_suffixes = {".uf2", ".elf", ".hex", ".o", ".obj", ".exe", ".dll", ".pyc", ".bin", ".zip"}

    for relative_path in tracked_paths():
        if not (ROOT / relative_path).exists():
            continue
        lower_parts = tuple(part.lower() for part in relative_path.parts)
        is_generated_path = (
            any(part in {".pio", "build", "dist", "artifacts"} for part in lower_parts)
            or (lower_parts and lower_parts[0] == "output")
        )
        require(not is_generated_path,
                f"Tracked generated-output path: {relative_path.as_posix()}", failures)
        if relative_path.suffix.lower() in generated_suffixes:
            require(relative_path in allowed_binary_assets,
                    f"Tracked binary/build artifact: {relative_path.as_posix()}", failures)
            continue
        if lower_parts and lower_parts[0] == "third_party":
            continue
        try:
            source = (ROOT / relative_path).read_text(encoding="utf-8").lower()
        except UnicodeDecodeError:
            continue
        for token in future_target_tokens:
            require(token not in source,
                    f"Unreleased target token in {relative_path.as_posix()}", failures)
        for token in personal_path_tokens:
            require(token not in source,
                    f"Personal workstation path in {relative_path.as_posix()}", failures)
        for token in personal_mail_tokens:
            require(token not in source,
                    f"Personal contact address in {relative_path.as_posix()}", failures)

    manager_json_path = ROOT / "reflex-adapt-manager.json"
    manager_zip_path = ROOT / "reflex-adapt-manager.json.zip"
    require(not manager_json_path.exists(),
            "Loose reflex-adapt-manager.json should not be published", failures)
    try:
        with ZipFile(manager_zip_path) as archive:
            manager_database = json.loads(archive.read("reflex-adapt-manager.json"))
        manager_script = ROOT / "tools/release_assets/adapt-manager/mister/Scripts/reflex_adapt_manager.sh"
        manager_entry = manager_database["files"]["Scripts/reflex_adapt_manager.sh"]
        require(manager_entry["size"] == manager_script.stat().st_size,
                "Manager database script size is stale", failures)
        require(manager_entry["hash"] == hashlib.md5(manager_script.read_bytes()).hexdigest(),
                "Manager database script hash is stale", failures)
    except (FileNotFoundError, KeyError, OSError, ValueError) as exc:
        failures.append(f"Manager database is invalid: {exc}")

    active_features = set(re.findall(r"^\s*#\s*define\s+(ENABLE_INPUT_[A-Z0-9_]+)\b", feature_gates, re.M))
    device_members = enum_members(device_mode, "DeviceEnum")
    active_inputs = []
    for name in device_members:
        if name in {"RZORD_NONE", "RZORD_LAST"}:
            continue
        feature = "ENABLE_INPUT_" + name.removeprefix("RZORD_")
        if feature in active_features:
            active_inputs.append(name)

    hidden_inputs = {"RZORD_PSX_JOG", "RZORD_PSX_DANCE"}
    input_name_cases = case_names(menu_input, "const char* getInputModeName", "RZORD_")
    input_short_cases = case_names(mode_labels, "const char* getInputShortName", "RZORD_")
    webhid_from_cases = case_names(webhid_input, "uint8_t webhid_input_mode_from_device", "RZORD_")
    webhid_to_cases = case_names(webhid_input, "DeviceEnum webhid_input_mode_to_device", "WEBHID_MODE_")
    webhid_names = set(re.findall(r"\bcase\s+(WEBHID_MODE_[A-Z0-9_]+)\s*:\s*return\s+\"[^\"]+\"", function_block(webhid_input, "const char* webhid_input_mode_name")))

    for mode in active_inputs:
        require(mode in input_name_cases, f"{mode} is enabled but missing getInputModeName()", failures)
        require(mode in input_short_cases, f"{mode} is enabled but missing getInputShortName()", failures)
        require(mode in webhid_from_cases, f"{mode} is enabled but missing WebHID from-device mapping", failures)
        webhid_id = "WEBHID_MODE_" + mode.removeprefix("RZORD_")
        require(webhid_id in webhid_to_cases, f"{mode} is enabled but missing WebHID to-device mapping", failures)
        require(webhid_id in webhid_names, f"{mode} is enabled but missing WebHID display name", failures)
    mode_modules = case_return_new_modules(input_factory)
    module_descriptions = input_module_descriptions(input_sources)
    for mode in active_inputs:
        if mode == "RZORD_AUTODETECT":
            continue
        module = mode_modules.get(mode)
        require(module is not None, f"{mode} is enabled but missing input factory module", failures)
        if module is not None:
            descriptions = module_descriptions.get(module, [])
            require(bool(descriptions), f"{mode} factory module {module} has no literal getDescription() label", failures)
            require(all(label.strip() for label in descriptions), f"{mode} factory module {module} has an empty getDescription() label", failures)
    controller_type_labels = set(re.findall(r"set(?:InputFrame|PsxController)TypeName\([^;]*,\s*\"([^\"]+)\"", input_sources))
    controller_type_labels.update(re.findall(r"return\s+\"([^\"]+)\";", read("firmware/input/dreamcast/input_dreamcast_debug.cpp")))
    for stale in ("DC Pad+VMU", "DC Pad", "PC-FX"):
        require(stale not in controller_type_labels, f"Stale controller type label still present: {stale}", failures)
    report.append(
        f"Input modes checked: {len(active_inputs)} enabled; "
        f"{len(set(mode_modules.values()))} factory modules; "
        f"{len(controller_type_labels)} controller-type labels"
    )

    adapt_inputs = parse_adapt_input_catalog(adapt_html)
    unreleased_input_ids = set(range(20, 31))
    exposed_unreleased = {
        mode: adapt_inputs.get(mode)
        for mode in unreleased_input_ids
        if adapt_inputs.get(mode) is not None
    }
    require(not exposed_unreleased,
            f"Adapt.html exposes unreleased input modes: {exposed_unreleased}", failures)
    require("CLASSIC2USB_RETROZORD_HDMI_PINOUT" not in board_profile and
            "ADAPT_CLASSIC2USB_RETROZORD_OLD_HDMI_PINOUT" not in board_profile,
            "Classic2USB release profile contains the obsolete donor pin map", failures)
    require(not (ROOT / "firmware/platform/boot/reflex_boot_logo_animation.h").exists() and
            not (ROOT / "tools/generate_boot_logo_animation.py").exists(),
            "Release source contains unused boot-animation artifacts", failures)
    require(not (ROOT / "tools/docs/release-checklist.md").exists(),
            "Release source contains the internal maintainer checklist", failures)

    disabled_release_outputs = {
        "OUTPUT_JVS",
        "OUTPUT_ESP32_BT",
        "OUTPUT_CONSOLE_NES",
        "OUTPUT_CONSOLE_SNES",
        "OUTPUT_CONSOLE_N64",
        "OUTPUT_CONSOLE_GC",
        "OUTPUT_CONSOLE_SATURN",
        "OUTPUT_CONSOLE_GENESIS",
        "OUTPUT_CONSOLE_WII",
        "OUTPUT_CONSOLE_AUTO",
        "OUTPUT_DB15_SUPERGUN",
    }
    output_members = [
        name for name in enum_members(output_mode, "outputMode_t")
        if name != "OUTPUT_LAST" and name not in disabled_release_outputs
    ]
    output_name_cases = case_names(mode_labels, "const char* getOutputShortName", "OUTPUT_")
    for mode in output_members:
        require(mode in output_name_cases, f"{mode} missing getOutputShortName()", failures)
    adapt_modes, adapt_hidden = parse_adapt_output_catalog(adapt_html)
    require(len(adapt_modes) == 22, f"Adapt.html fallback output catalog should expose 22 Classic2USB entries, found {len(adapt_modes)}", failures)
    expected_hidden_outputs = {
        0,   # OUTPUT_HID legacy alias
        2,   # OUTPUT_MISTER_JOGCON
        3,   # OUTPUT_MISTER_NEGCON
        4,   # OUTPUT_MISTER_GUNCON
        5,   # OUTPUT_RESERVED_JOGCON
        6,   # OUTPUT_RESERVED_MOUSE
        11,  # OUTPUT_PS5 without release auth
        12,  # OUTPUT_SWITCH Pokken
        14,  # OUTPUT_PANTHERLORD
        15,  # OUTPUT_GCWIIU
        16,  # OUTPUT_MDMINI
        18,  # OUTPUT_XINPUTW
        21,  # OUTPUT_XBOXONE
    }
    require(expected_hidden_outputs.issubset(adapt_hidden), f"Adapt.html hidden outputs missing {sorted(expected_hidden_outputs - adapt_hidden)}", failures)
    report.append(f"Output modes checked: {len(output_members)} enum entries; Adapt fallback hidden={sorted(adapt_hidden)}")

    setting_ids = [name for name in enum_members(settings_store, "SettingId") if name != "Count"]
    specs = setting_specs(settings_registry)
    require(len(specs) == len(setting_ids), f"Setting registry count mismatch: {len(specs)} specs for {len(setting_ids)} IDs", failures)
    global_fields = struct_fields(settings_store, "GlobalSettingsRecord")
    per_mode_fields = struct_fields(settings_store, "PerModeSettingsRecord")
    special_offsets = {
        "MenuHotkey": "menu_hotkey",
        "SystemMenuHotkey": "menu_hotkey",
        "KioskMode": "menu_hotkey",
        "HomeScreenDebug": "home_screen_debug",
        "HomeButtonLabels": "home_screen_debug",
        "HomeJvsView": "home_screen_debug",
        "HotkeyHoldTime": "home_screen_debug",
        "ClassicDualMerge": "reserved_musical_buttons",
    }
    for setting, spec in zip(setting_ids, specs):
        fields = global_fields if spec["record"] == "GlobalSettingsRecord" else per_mode_fields
        require(spec["field"] in fields, f"SettingId::{setting} points at missing field {spec['record']}::{spec['field']}", failures)
        if setting in special_offsets:
            require(spec["field"] == special_offsets[setting], f"SettingId::{setting} should use packed field {special_offsets[setting]}, not {spec['field']}", failures)
            require(f"id == SettingId::{setting}" in settings_registry, f"SettingId::{setting} missing packed read/write handler", failures)
    defaults = {setting: spec["default"] for setting, spec in zip(setting_ids, specs)}
    require(defaults.get("MenuHotkey") == "0", "Quick Menu hotkey must default off", failures)
    require(defaults.get("SystemMenuHotkey") == "0", "System Menu hotkey must default off", failures)
    require(defaults.get("HomeHotkey") == "1", "Home hotkey must default on", failures)
    require(defaults.get("CaptureHotkey") == "0", "Capture hotkey must default off", failures)
    require("inline constexpr int32_t defaultKioskModeValue(DeviceEnum) {\n  return 0;\n}" in settings_registry, "Kiosk mode must default off explicitly", failures)
    report.append(f"EEPROM settings checked: {len(setting_ids)} SettingId entries")

    menu_items = set(enum_members(menu_defs, "menu_item_enum"))
    catalog_items = set(re.findall(r"\{\s*(menu_item_[A-Za-z0-9_]+)\s*,\s*\"[^\"]+\"\s*\}", menu_catalog))
    allowed_unlabeled = {
        "menu_item_exit",
        "menu_item_save_and_reboot",
        "menu_item_reserved_internal",
        "menu_item_pad_test",
        "menu_item_pin_debug",
        "menu_item_autodetect",
    }
    missing_catalog = sorted(menu_items - catalog_items - allowed_unlabeled)
    require(not missing_catalog, f"Menu items missing catalog rows: {missing_catalog}", failures)
    descriptor_items = set(re.findall(r"\.id\s*=\s*(menu_item_[A-Za-z0-9_]+)", descriptors))
    descriptor_settings = set(re.findall(r"\.setting_id\s*=\s*SettingId::([A-Za-z0-9_]+)", descriptors))
    unknown_descriptor_settings = sorted(descriptor_settings - set(setting_ids))
    require(not unknown_descriptor_settings, f"Menu descriptors reference unknown settings: {unknown_descriptor_settings}", failures)
    require("handleHotkeysSubmenu" in submenu_bridge and "hotkeys_submenu_active = true" in item_handlers, "Hotkey submenu must be wired and openable", failures)
    require("handleKioskSubmenu" in submenu_bridge and "kiosk_submenu_active = true" in item_handlers, "Kiosk submenu must be wired and openable", failures)
    for token in (
        '"Mode x2 = Quick Menu"',
        '"Mode (hold) = Settings"',
        '"Reset x2 = Reboot"',
        '"Reset (hold) = Auto Input"',
        '"On"',
        '"Off"',
        '"Cancel"',
        "KioskRenderSnapshot",
        "kioskRenderSnapshotMatches",
    ):
        require(token in kiosk, f"Kiosk submenu missing polished text/render guard {token}", failures)
    for stale in ('"Enable"', '"Disable"', '"Current: "', '"Set Kiosk:"'):
        require(stale not in kiosk, f"Kiosk submenu contains stale language {stale}", failures)
    report.append(f"Menu items checked: {len(menu_items)} enum entries, {len(catalog_items)} catalog rows, {len(descriptor_items)} descriptors")

    forbidden_release_tokens = ("PC-FX", "i2c2oled", "reflex_adapt_serial_bridge", "gamecontrollerdb")
    release_surface = (
        read("docs/classic2usb/Classic2USB.md")
        + "\n" + read("docs/classic2usb/Classic2USB-Quick-Start.md")
        + "\n" + release_targets
    )
    for token in forbidden_release_tokens:
        require(token not in release_surface, f"Release docs/package config should not include {token}", failures)
    require("reflex_adapt_update.sh" not in release_targets, "Release package must exclude the unvalidated MiSTer updater", failures)
    check_runner = read("tools/run_release_checks.py")
    require("run_release_checks.py" in workflows, "CI must run the unified release checks", failures)
    require("audit_classic2usb_release.py" in check_runner, "Unified checks must run the release audit", failures)
    require("test_classic2usb_source_guards.py" in check_runner, "Unified checks must run source guards", failures)
    require("test_auto_input_usb_identity.py" in check_runner, "Unified checks must run Auto input identity tests", failures)
    require("test_release_integrity.py" in check_runner, "Unified checks must run release integrity tests", failures)
    report.append("Release package/docs policy checked")

    return failures, report


def main() -> int:
    failures, report = audit()
    for line in report:
        print(f"OK: {line}")
    if failures:
        print("\nRelease audit failures:", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print("OK: Classic2USB release audit passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
