from __future__ import annotations

import json
import re
import sys
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str, failures: list[str]) -> None:
    if not condition:
        failures.append(message)


def duplicates(values: list[str]) -> list[str]:
    return sorted(value for value, count in Counter(values).items() if count > 1)


def audit() -> list[str]:
    failures: list[str] = []

    protocol = read("firmware/output/usb/webhid/webhid_protocol.h")
    size_match = re.search(r"#define\s+WEBHID_REPORT_SIZE\s+(\d+)", protocol)
    require(size_match is not None, "WebHID report size is not defined", failures)
    report_size = int(size_match.group(1)) if size_match else 0
    for path in sorted((ROOT / "firmware/output/usb/webhid").glob("*.cpp")):
        text = path.read_text(encoding="utf-8")
        for index_text in re.findall(r"buffer\[(\d+)\]", text):
            index = int(index_text)
            require(
                index < report_size,
                f"{path.relative_to(ROOT)} writes buffer[{index}] in a {report_size}-byte report",
                failures,
            )

    dispatch = read("firmware/output/usb/webhid/webhid_report_dispatch.cpp")
    require("uint8_t report[WEBHID_REPORT_SIZE] = {};" in dispatch,
            "WebHID GET_REPORT must build into a full-size local buffer", failures)
    require("responseLen = builtLen < reqlen ? builtLen : reqlen" in dispatch,
            "WebHID GET_REPORT must clamp its response to reqlen", failures)
    settings = read("firmware/output/usb/webhid/webhid_settings_reports.cpp")
    require("WEBHID_SETTINGS_HOTKEY_FLAGS_INDEX" in settings,
            "WebHID settings must use the in-range hotkey flags field", failures)
    require("WEBHID_SETTINGS_GUNCON_X_INDEX" in settings and
            "WEBHID_SETTINGS_GUNCON_Y_INDEX" in settings,
            "WebHID settings must expose both GunCon alignment axes", failures)
    require("buffer[63]" not in settings, "WebHID settings must not write byte 63", failures)
    adapt = read("web/Adapt.html")
    require("data[63]" not in adapt, "Adapt.html must not read outside the 63-byte feature report", failures)
    require('id="settings-guncon-x" min="-50" max="50"' in adapt and
            'id="settings-guncon-y" min="-50" max="50"' in adapt,
            "Adapt.html must expose normalized GunCon alignment controls", failures)
    require("const RELEASE_CONTROLLER_VISUALIZER_ENABLED = true;" in adapt,
            "Release Adapt.html must enable the validated output controller visualizer", failures)
    require("const RELEASE_INPUT_CONTROLLER_ART_ENABLED = false;" in adapt,
            "Release Adapt.html must hide unvalidated input controller artwork", failures)
    require("controllerTestCard.classList.toggle('hidden', !RELEASE_CONTROLLER_VISUALIZER_ENABLED);" in adapt,
            "Release Adapt.html must control the hybrid visualizer after connect", failures)
    require("if (!RELEASE_CONTROLLER_VISUALIZER_ENABLED || testInterval) return;" in adapt,
            "Release Adapt.html must poll only when the hybrid visualizer is enabled", failures)
    require('id="oled-preview-canvas"' in adapt and 'id="input-controller-container" class="hidden"' in adapt,
            "Release Adapt.html must show OLED input while hiding the input SVG container", failures)
    for token in (
        "const CONTROLLER_ART =",
        "const CONTROLLER_SVG_MAP =",
        "function createNESControllerSVG",
        "function createArtDreamcastControllerSVG",
        "function getInputControllerSVG",
    ):
        require(token not in adapt, f"Release Adapt.html must omit input artwork: {token}", failures)
    for token in (
        "function createXboxControllerSVG",
        "function createOGXboxControllerSVG",
        "function createPS3ControllerSVG",
        "function createPS4ControllerSVG",
        "function createSwitchProControllerSVG",
        "function createPS2ControllerSVG",
        "function createGameCubeControllerSVG",
        "function createGenesisControllerSVG",
        "function getOutputControllerSVG",
    ):
        require(token in adapt, f"Release Adapt.html must retain output artwork: {token}", failures)

    hid_reports = read("firmware/output/usb/output_usb_hid_report_runtime.h")
    for token, label in (
        ("report_id == 2 && bufsize >= 2", "DInput report 2 length guard"),
        ("report_id == 0 && bufsize >= 3", "DInput report 0 length guard"),
        ("bufsize >= 6 && buffer[0] == 0x05", "PS4 output length guard"),
        ("report_id == 0x11 && bufsize >= 4", "GC adapter report length guard"),
        ("const uint8_t* rumbleData = nullptr;", "GC adapter normalized report pointer"),
    ):
        require(token in hid_reports, f"Missing {label}", failures)

    boot_output = read("firmware/output/runtime/output_boot_runtime.cpp")
    capabilities = read("firmware/output/output_capabilities.h")
    psx_setup = read("firmware/input/psx/input_psx_setup.cpp")
    require("output_is_specialized_mister_psx_mode(outputMode)" in boot_output,
            "Auto MiSTer boot must preserve PSX specialized output promotion", failures)
    for mode in ("OUTPUT_MISTER_JOGCON", "OUTPUT_MISTER_NEGCON", "OUTPUT_MISTER_GUNCON"):
        require(mode in capabilities, f"PSX specialized mode helper missing {mode}", failures)
    require("output_promote_psx_peripheral_mode(isJogcon, isNeGcon, isGuncon);" in psx_setup,
            "PSX setup must promote detected MiSTer peripherals", failures)

    input_autodetect = read("firmware/input/autodetect/input_autodetect_runtime.cpp")
    require("is_auto_output_mode_selected() && autoDetectState == AUTO_STATE_IDLE" in input_autodetect,
            "Input Auto must not block unresolved USB host detection", failures)

    platformio = read("platformio.ini")
    environments = re.findall(r"^\[env:([^\]]+)\]", platformio, flags=re.MULTILINE)
    require(environments == ["classic2usb"],
            f"Release platformio.ini must expose only classic2usb, found {environments}", failures)
    require(re.search(r"platform\s*=\s*https://github\.com/maxgerhardt/platform-raspberrypi\.git#[0-9a-f]{40}",
                      platformio) is not None,
            "Release PlatformIO platform must be pinned to an exact commit", failures)
    target_only_paths = (
        "third_party/Pico_PIO_USB",
        "firmware/features/bluetooth",
        "firmware/input/usb_host",
        "firmware/input/usb_bt",
        "firmware/input/esp32_spi",
        "firmware/input/jvs",
        "firmware/output/jvs",
        "firmware/platform/usb_host_bridge.cpp",
        "firmware/platform/usb_host_bridge.h",
    )
    require(all(not (ROOT / path).exists() for path in target_only_paths),
            "Future-product implementations belong in Reflex-Adapt-Dev, not the release repository", failures)

    gitignore = read(".gitignore")
    require(".vscode/" in gitignore,
            "Release repository must ignore editor-local VS Code configuration", failures)
    private_ignore_entries = ("/output/", "*.pem", "*.p12", "*.pfx", "*.key",
                              "serial.txt", "sig.bin")
    require(all(entry in gitignore for entry in private_ignore_entries),
            "Release repository must ignore generated output and PS4 credential files", failures)
    require(not (ROOT / "docs" / "social").exists(),
            "Social drafts belong in Reflex-Adapt-Dev, not the release repository", failures)
    notice = read("docs/NOTICE.md")
    require("https://github.com/sonik-br/RetroZordAdapter" in notice and
            "https://github.com/sonik-br/RetroZord)" not in notice,
            "Release notices must use the canonical RetroZordAdapter URL", failures)

    workflows = read(".github/workflows/build.yml") + "\n" + read(".github/workflows/classic2usb-release.yml")
    require("tools/requirements-release.txt" in workflows,
            "CI must install the release test dependencies", failures)
    require("python tools/run_release_checks.py" in workflows,
            "CI must use the unified release check runner", failures)
    require("platformio==6.1.19" in read("tools/requirements-release.txt"),
            "Release dependencies must pin PlatformIO Core", failures)
    for action, ref in re.findall(r"uses:\s+([^@\s]+)@([^\s]+)", workflows):
        require(re.fullmatch(r"[0-9a-f]{40}", ref) is not None,
                f"GitHub Action {action} must use an immutable commit SHA, found {ref}", failures)
    build_workflow = read(".github/workflows/build.yml")
    require("workflow_dispatch:" in build_workflow and
            "\n  push:" not in build_workflow and
            "\n  pull_request:" not in build_workflow,
            "Build workflow must be manual-only", failures)
    require("if: success()" in build_workflow and "if-no-files-found: error" in build_workflow,
            "Build artifacts must upload only after success and fail when missing", failures)
    release_workflow = read(".github/workflows/classic2usb-release.yml")
    require("workflow_dispatch:" in release_workflow and
            "\n  push:" not in release_workflow and
            "\n  pull_request:" not in release_workflow,
            "Release workflow must be manual-only", failures)
    require(workflows.count("cache: pip") ==
            workflows.count("cache-dependency-path: tools/requirements-release.txt"),
            "Every pip cache must use the release requirements file", failures)
    require("permissions:\n  contents: read" in release_workflow and
            "publish:" in release_workflow and
            "permissions:\n      contents: write" in release_workflow,
            "Only the tag publish job may receive contents: write", failures)
    require("actions/download-artifact@" in release_workflow,
            "Tag publishing must consume the validated package artifact", failures)
    require("startsWith(github.ref, 'refs/tags/v')" in release_workflow,
            "Release publishing must be tag-only", failures)
    require("softprops/action-gh-release@" in release_workflow,
            "Tag workflow must publish GitHub Release assets", failures)
    require(workflows.count("timeout-minutes:") >= 3,
            "Build, package, and publish jobs need execution timeouts", failures)

    package_release = read("tools/package_release.py")
    require('"dirty": "yes" if dirty_status else "no"' in package_release,
            "Package provenance must record dirty Git state", failures)
    require("args.require_clean or args.require_tag_match" in package_release,
            "Tagged packages must reject dirty worktrees", failures)

    release_targets = read("tools/release_targets.json")
    release_target_config = json.loads(release_targets)["targets"]["classic2usb"]
    hardware = release_target_config.get("hardware_compatibility", {})
    require(hardware.get("policy") == "all-published-revisions" and
            hardware.get("product_family") == "Classic2USB",
            "Classic2USB releases must declare all-published-revisions compatibility", failures)
    package_destinations = {
        item["destination"] for item in release_target_config["package_files"]
    }
    require(not any(Path(destination).name == "Adapt.html" for destination in package_destinations),
            "Release package must not include Adapt.html before its UI is validated", failures)
    require("mister/Scripts/reflex_adapt_manager.sh" in release_targets,
            "Release package must include the validated Adapt Manager", failures)
    menu_media = {
        "docs/media/classic2usb/reflex-home-menu.png",
        "docs/media/classic2usb/quick-settings-psx.png",
        "docs/media/classic2usb/system-settings-menu.png",
        "docs/media/classic2usb/system-settings-menu.mp4",
    }
    require(menu_media.issubset(package_destinations) and
            all((ROOT / path).is_file() for path in menu_media),
            "Release package must include all README menu media", failures)
    require("adapt-manager-release.json" in release_workflow,
            "Tag workflow must publish the exact-hash manager release descriptor", failures)
    require("generate_release_descriptor.py" in package_release,
            "Release packaging must generate Adapt Manager firmware metadata", failures)
    require('hardware_compatibility' in package_release,
            "Release notes must include declared hardware compatibility", failures)
    descriptor_generator = read("tools/adapt_manager/generate_release_descriptor.py")
    require('"hardware_compatibility"' in descriptor_generator and
            '"all-published-revisions"' in descriptor_generator,
            "Adapt Manager release metadata must declare hardware compatibility", failures)
    readme = read("README.md")
    for term in ("MAJOR", "MINOR", "PATCH", "every published Classic2USB"):
        require(term in readme, f"README version policy is missing {term}", failures)

    menu_visibility = read("firmware/menu/menu_helpers_visibility.cpp")
    require("case menu_item_latency_test:" in menu_visibility and
            "#if defined(ADAPT_ENABLE_LATENCY_TEST)" in menu_visibility,
            "Retail menu must hide latency controls without the diagnostic build flag", failures)
    require('data-tab="latency"' not in adapt and 'id="latency-test"' not in adapt,
            "Release Adapt.html must not contain latency-test UI", failures)
    require("/guncon|reflexpsgun/i" in adapt and
            all(setting_id in adapt for setting_id in (
                "setting-deadzone", "setting-trigger", "setting-switch-rstick",
                "setting-dpad", "setting-stick-invert",
            )),
            "GunCon mode must suppress settings that do not apply to a light gun", failures)
    memory_visibility = adapt.split("function updateMemoryCardVisibility()", 1)[1].split(
        "function requireDreamcastMemoryMode()", 1
    )[0]
    require("scheduleMemoryCardAutoWorkflow" in adapt and
            "memoryAutoWorkflowMode = -1" in adapt and
            "await memorySendCommand('CARD STATUS'" in adapt and
            "if (!await memoryScan({ auto: true })) return;" in adapt and
            "await loadVmuImage({ auto: true })" in adapt and
            "if (title.classList.contains('expanded'))" in adapt and
            "if (connected) scheduleMemoryCardAutoWorkflow();" not in adapt and
            "scheduleMemoryCardAutoWorkflow();" not in memory_visibility and
            "memoryCardExpansionUserSet = false;" not in memory_visibility and
            "if (!memoryCardExpansionUserSet)" in memory_visibility and
            "updateMemoryCardExpansion(true)" not in adapt,
            "Memory-card auto workflow must preserve the user's expansion state", failures)
    require("function updateWindowsOutputWarning" in adapt and
            "settings-win-output-warning" in adapt,
            "Windows output warning helper must remain defined for settings refresh", failures)
    require('id="oled-preview-card"' in adapt and "Pop Out OLED" in adapt,
            "OLED remote controls must remain available in the controller view", failures)
    require(
        adapt.index('id="input-controller-container"')
        < adapt.index('id="oled-preview-card"')
        < adapt.index('id="output-controller-container"'),
        "OLED remote controls must remain under the input-side OLED preview",
        failures,
    )

    quick_caps = read("firmware/menu/quick_config_capabilities.cpp")
    require("if (isSnesMode(mode)) {\n    return true;" in quick_caps,
            "SNES rumble must not depend on the retired RumbleTech setting", failures)
    require("menu_snes_rumbletech == 0" not in menu_visibility,
            "System rumble visibility must not use the retired RumbleTech setting", failures)
    quick_visibility = read("firmware/menu/quick_config_visibility.cpp")
    require("addVisibleItem(QCI_RUMBLETECH)" not in quick_visibility,
            "RumbleTech must not return as a visible opt-in setting", failures)

    public_docs = "\n".join(
        read(path)
        for path in (
            "README.md",
            "docs/classic2usb/Classic2USB.md",
            "docs/classic2usb/Classic2USB-Quick-Start.md",
            "docs/classic2usb/Classic2USB-Input-Reference.md",
            "tools/release_targets.json",
        )
    )
    for token in ("GC/Wii U Adapter", "WUP-028", "latency export", "Run latency tests"):
        require(token not in public_docs, f"Public release surface still advertises {token}", failures)
    require("Flight Stick (Untested)" in public_docs,
            "3DO Flight Stick must be marked untested", failures)
    require("Wii guitar | Untested" in public_docs and "Wii drums | Untested" in public_docs,
            "Wii guitar and drums must be marked untested", failures)
    require("docs/media/classic2usb/reflex-home-menu.png" in read("README.md") and
            (ROOT / "docs/media/classic2usb/reflex-home-menu.png").is_file(),
            "README must include the Reflex Home menu screenshot", failures)
    require("| Players | Detection | Mean Latency |" in read("README.md"),
            "README input table must state the player count", failures)
    require("tools/release_assets/adapt-manager/mister/Scripts/reflex_adapt_manager.sh" in read("README.md"),
            "README must link the actual Reflex Adapt Manager script", failures)
    require("Left device button (Reset)" in read("README.md") and
            "Right device button (Mode)" in read("README.md") and
            "Reflex Kiosk Mode" in read("README.md"),
            "README must document physical controls and Kiosk Mode", failures)

    about_screen = read("firmware/menu/menu_about.cpp")
    menu_tools = read("firmware/menu/menu_bridge_tools.cpp")
    menu_bridge = read("firmware/menu/menu_bridge.cpp")
    about_handler = read("firmware/menu/menu_item_handlers_submenus.cpp")
    require("display.clear();" in about_screen and "u8g2." not in about_screen,
            "About must render through the active menu display backend", failures)
    require("if (about_screen_active)" in menu_tools and
            "redrawMenuAfterOverlayExit" in menu_tools,
            "About must remain active until explicitly dismissed", failures)
    require("about_screen_active = false;" in menu_bridge and
            "case item_action_change_prev:" in about_handler,
            "About state must reset on menu close and accept either change direction", failures)

    menu_defs = read("firmware/menu/menu_item_defs.h")
    menu_catalog = read("firmware/menu/menu_catalog.cpp")
    enum_match = re.search(
        r"enum\s+menu_item_enum(?:\s*:\s*[A-Za-z0-9_]+)?\s*\{(.*?)\};",
        menu_defs,
        flags=re.DOTALL,
    )
    enum_items = re.findall(r"\b(menu_item_[A-Za-z0-9_]+)\b", enum_match.group(1) if enum_match else "")
    catalog_rows = re.findall(r"\{\s*(menu_item_[A-Za-z0-9_]+)\s*,\s*\"([^\"]+)\"\s*\}", menu_catalog)
    require(not duplicates(enum_items), f"Duplicate menu enum items: {duplicates(enum_items)}", failures)
    require(not duplicates([item for item, _ in catalog_rows]),
            f"Duplicate menu catalog IDs: {duplicates([item for item, _ in catalog_rows])}", failures)
    unknown_catalog = sorted(set(item for item, _ in catalog_rows) - set(enum_items))
    require(not unknown_catalog, f"Menu catalog references unknown items: {unknown_catalog}", failures)

    descriptor_source = read("firmware/menu/menu_descriptors_data.cpp")
    descriptor_item_list = re.findall(r"\.id\s*=\s*(menu_item_[A-Za-z0-9_]+)", descriptor_source)
    descriptor_items = set(descriptor_item_list)
    require(not duplicates(descriptor_item_list),
            f"Duplicate menu descriptors: {duplicates(descriptor_item_list)}", failures)
    unknown_descriptors = sorted(descriptor_items - set(enum_items))
    require(not unknown_descriptors,
            f"Menu descriptors reference unknown items: {unknown_descriptors}", failures)
    setting_ids_source = read("firmware/core/settings_store.h")
    setting_match = re.search(
        r"enum\s+class\s+SettingId(?:\s*:\s*[A-Za-z0-9_]+)?\s*\{(.*?)\};",
        setting_ids_source,
        flags=re.DOTALL,
    )
    setting_ids = set(re.findall(r"\b([A-Za-z][A-Za-z0-9_]*)\b", setting_match.group(1) if setting_match else ""))
    descriptor_settings = set(re.findall(r"\.setting_id\s*=\s*SettingId::([A-Za-z0-9_]+)", descriptor_source))
    unknown_descriptor_settings = sorted(descriptor_settings - setting_ids)
    require(not unknown_descriptor_settings,
            f"Menu descriptors reference unknown settings: {unknown_descriptor_settings}", failures)
    handler_source = "\n".join(
        path.read_text(encoding="utf-8")
        for path in sorted((ROOT / "firmware/menu").glob("menu_item_handlers*.cpp"))
    )
    handler_items = set(re.findall(
        r"(?:case\s+|==\s*|!=\s*)(menu_item_[A-Za-z0-9_]+)",
        handler_source,
    ))
    unknown_handler_items = sorted(handler_items - set(enum_items))
    require(not unknown_handler_items,
            f"Menu handlers reference unknown items: {unknown_handler_items}", failures)
    uncovered_menu_items = sorted(
        item for item, _ in catalog_rows
        if item not in descriptor_items and item not in handler_source
        and item not in {"menu_item_bluetooth_pairing", "menu_item_bluetooth_forget"}
    )
    require(not uncovered_menu_items,
            f"Catalog menu items have no descriptor or explicit handler: {uncovered_menu_items}",
            failures)

    quick_header = read("firmware/menu/quick_config.h")
    quick_match = re.search(
        r"enum\s+QuickConfigItem(?:\s*:\s*[A-Za-z0-9_]+)?\s*\{(.*?)\};",
        quick_header,
        flags=re.DOTALL,
    )
    quick_items = list(dict.fromkeys(re.findall(
        r"\b(QCI_[A-Z0-9_]+)\b",
        quick_match.group(1) if quick_match else "",
    )))
    quick_actions = "\n".join(
        path.read_text(encoding="utf-8")
        for path in sorted((ROOT / "firmware/menu").glob("quick_config*.cpp"))
    ) + "\n" + quick_header
    quick_visibility = read("firmware/menu/quick_config_visibility.cpp")
    declared_quick_items = set(re.findall(r"\b(QCI_[A-Z0-9_]+)\b", quick_header))
    quick_references = set(re.findall(r"\b(QCI_[A-Z0-9_]+)\b", quick_actions))
    unknown_quick_references = sorted(quick_references - declared_quick_items)
    require(not unknown_quick_references,
            f"Quick Config code references unknown items: {unknown_quick_references}", failures)
    require(not duplicates(quick_items),
            f"Duplicate Quick Config enum items: {duplicates(quick_items)}", failures)
    unnamed_quick_items = sorted(
        item for item in quick_items
        if item != "QCI_COUNT" and item not in quick_visibility
    )
    require(not unnamed_quick_items,
            f"Quick Config items have no name/visibility coverage: {unnamed_quick_items}", failures)
    no_action_quick_items = sorted(
        item for item in quick_items
        if item not in {"QCI_COUNT", "QCI_DISCARD_EXIT", "QCI_EXIT"}
        and item not in quick_actions
    )
    require(not no_action_quick_items,
            f"Quick Config items have no navigation/action coverage: {no_action_quick_items}", failures)

    targets = json.loads(read("tools/release_targets.json"))
    validations = targets["targets"]["classic2usb"].get("validation_commands", [])
    require(any("run_release_checks.py" in " ".join(command) for command in validations),
            "Release package must run the same unified checks as CI", failures)

    return failures


def main() -> int:
    failures = audit()
    if failures:
        print("Release integrity failures:", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print("OK: Classic2USB release integrity checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
