import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read_text(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"{label}: missing {needle!r}")


def require_any(text: str, needles: tuple[str, ...], label: str) -> None:
    if not any(needle in text for needle in needles):
        raise AssertionError(f"{label}: missing one of {needles!r}")


def reject(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise AssertionError(f"{label}: unexpected {needle!r}")


def require_file(path: str, label: str) -> None:
    target = ROOT / path
    if not target.is_file() or target.stat().st_size == 0:
        raise AssertionError(f"{label}: missing or empty {path!r}")


def reject_file(path: str, label: str) -> None:
    if (ROOT / path).exists():
        raise AssertionError(f"{label}: unexpected {path!r}")


def test_oled_pad_layouts_have_one_active_registry() -> None:
    reject_file(
        "firmware/menu/pad_layouts.h",
        "Legacy duplicate OLED pad-layout table",
    )
    require_file(
        "firmware/menu/menu_pad_layouts_internal.h",
        "Canonical OLED pad-layout registry",
    )


def test_gamecube_z_and_triggers_are_distinct() -> None:
    source = read_text("firmware/input/gc64/input_gc64_poll.cpp")
    button_masks = read_text("firmware/menu/menu_pad_button_masks.cpp")
    graphics = read_text("firmware/menu/controller_graphics_controllers.cpp")
    pad_layouts = read_text("firmware/menu/menu_pad_layouts_data.cpp")
    require_any(
        source,
        (
            "frame.R1 = joybus[port]->digitalPressed(GCB_Z_TRIGGER);",
            "frame.R1 = (buttons & GCB_Z_TRIGGER) != 0;",
        ),
        "GameCube Z should be R1",
    )
    require(source, "frame.SELECT = 0;", "GameCube Z should not also be Select")
    require_any(
        source,
        (
            "frame.R2 = joybus[port]->digitalPressed(GCB_R_TRIGGER);",
            "frame.R2 = (buttons & GCB_R_TRIGGER) != 0;",
        ),
        "GameCube R should be R2",
    )
    require(button_masks, "if (deviceMode == RZORD_GAMECUBE && r.R1)", "GameCube OLED should read Z from R1")
    require(button_masks, "mask |= 0x2000;", "GameCube compact OLED should draw Z in its dedicated slot")
    require(graphics, "state & GFX_BTN_R1) u8g2.drawBox(x + 38, y + 5, 6, 2)", "GameCube graphic should read Z from R1")
    require(graphics, "state & GFX_BTN_L2) u8g2.drawBox(x + 5, y + 1, 10, 3)", "GameCube graphic should read L from L2")
    require(graphics, "state & GFX_BTN_R2) u8g2.drawBox(x + 35, y + 1, 10, 3)", "GameCube graphic should read R from R2")
    require(
        pad_layouts,
        "{ GPAD_L2, 0, 9, PAD_SHOULDER_ON, PAD_SHOULDER_OFF }",
        "Active GameCube OLED L gauge should move inward by 1.5 characters",
    )
    require(
        pad_layouts,
        "{ GPAD_R2, 0, 45, PAD_SHOULDER_ON, PAD_SHOULDER_OFF }",
        "Active GameCube OLED R gauge should move inward by 1.5 characters",
    )
    require(
        pad_layouts,
        "{ GPAD_L2, 0, 0 * 6, PAD_RECT_ON, PAD_RECT_OFF }",
        "GameCube digital L should sit outside the analog L gauge",
    )
    require(
        pad_layouts,
        "{ GPAD_R2, 0, 9 * 6, PAD_RECT_ON, PAD_RECT_OFF }",
        "GameCube digital R should sit outside the analog R gauge",
    )


def test_mister_dinput_rumble_matches_reflex_driver() -> None:
    descriptor = read_text(
        "firmware/output/output_descriptors_generic_gamepad_runtime.h"
    )
    callback = read_text(
        "firmware/output/usb/output_usb_hid_report_runtime.h"
    )
    rumble_report = descriptor.split("Output rumble left, right (Stadia style)", 1)[1].split(
        "Player index plus padding", 1
    )[0]
    report_three = descriptor.split("Player index plus padding", 1)[1].split(
        "REFLEX_WEBHID_FEATURE_REPORT_DESC", 1
    )[0]
    require(rumble_report, "HID_USAGE_PAGE_N   ( HID_USAGE_PAGE_PID, 2", "Stadia PID usage page")
    require(rumble_report, "HID_REPORT_ID      ( 5", "MiSTer DInput rumble report ID")
    require(rumble_report, "HID_USAGE_PID_DC_ENABLE_ACTUATORS", "Stadia actuator usage")
    require(rumble_report, "HID_LOGICAL_MAX_N  ( 65534, 2", "Stadia rumble magnitude maximum")
    require(rumble_report, "HID_REPORT_COUNT   ( 2", "Stadia rumble motor count")
    require(rumble_report, "HID_REPORT_SIZE    ( 16", "Stadia rumble motor width")
    require(report_three, "HID_REPORT_ID      ( 3", "MiSTer player-index report ID")
    require(callback, "if (report_id == 5)", "Stadia control-transfer report framing")
    require(callback, "report_id == 0 && bufsize >= 1 && buffer[0] == 5", "Stadia interrupt-OUT report framing")
    require(callback, "rumblePayloadSize >= 4", "Stadia FF payload length guard")
    require(callback, "(uint16_t)rumblePayload[0]", "Stadia strong motor decode")
    require(callback, "(uint16_t)rumblePayload[2]", "Stadia weak motor decode")
    require(callback, "stadiaMagnitudeToU8(strong)", "Stadia strong motor scaling")
    require(callback, "stadiaMagnitudeToU8(weak)", "Stadia weak motor scaling")
    reject(callback, "report_id == 2 && bufsize", "obsolete Reflex four-byte output report")


def test_psx_memory_card_uses_ack_aware_hardware_spi() -> None:
    setup = read_text("firmware/input/psx/input_psx_setup.cpp")
    source = read_text("firmware/input/psx/input_psx_memcard.cpp")
    require(
        setup,
        "#include <PsxNewLib/PsxDriverHwSpiWithAck.h>",
        "PSX memory-card bridge should use the ACK-aware hardware-SPI driver",
    )
    require(
        setup,
        "psxDriver[i] = new PsxDriverHwSpiWithAck",
        "PSX card traffic should remain on hardware SPI instead of remuxing the shared bus",
    )
    reject(setup, "new PsxDriverBitBang", "PSX card traffic must not remux the shared bus to SIO")
    require(
        source,
        "const PSXMemoryCardBlockResult result = readPSXMemoryCardFrame(port, slot, 0, header);",
        "PSX card info probes should use the guarded read path",
    )

def test_psx_flightstick_preserves_adapt_v1_native_assignments() -> None:
    source = read_text("firmware/input/psx/input_psx_poll.cpp")
    reject(
        source,
        "mapPsxFlightstickButtons",
        "Flight-stick mode must not relabel Adapt V1 protocol assignments",
    )
    require(
        source,
        "frame.X = cont.buttonPressed(PSB_SQUARE);",
        "Multitap flight-stick Square should retain its native PSX assignment",
    )
    require(
        source,
        "frame.L1 = cont.buttonPressed(PSB_L1);",
        "Multitap flight-stick L1 should retain its native PSX assignment",
    )
    require(
        source,
        "frame.X = psx[i]->buttonPressed(PSB_SQUARE);",
        "Direct flight-stick Square should retain its native PSX assignment",
    )
    require(
        source,
        "frame.L1 = psx[i]->buttonPressed(PSB_L1);",
        "Direct flight-stick L1 should retain its native PSX assignment",
    )


def test_adapt_html_chunks_full_psx_memory_card_reads() -> None:
    source = read_text("web/Adapt.html")
    require(source, "const PSX_MEMCARD_READ_CHUNK_FRAMES = 16;", "PSX range-read chunk size")
    require(source, "async function readMemoryCardRangeChunked", "chunked memory-card range helper")
    require(
        source,
        "const chunkBlocks = isPsxMem ? PSX_MEMCARD_READ_CHUNK_FRAMES : slotInfo.blocks;",
        "PSX full-card reads should use small serial chunks",
    )
    require(source, "`Reading Blocks ${complete}/${total}`", "chunked reads should report live progress")
    require(source, "await delayMs(0);", "chunked reads should yield to the browser between chunks")


def test_transfer_pak_requires_a_validated_game_boy_header() -> None:
    source = read_text("firmware/input/gc64/input_gc64_n64pak.cpp")
    header = read_text("firmware/input/gc64/Input_GC64.h")
    serial = read_text("firmware/core/serial_memcard_commands.cpp")
    adapt = read_text("web/Adapt.html")
    require(source, "kGbHeaderReadAttempts = 5", "Transfer Pak header reads should tolerate intermittent contacts")
    require(source, "gbHeaderLogoValid(header)", "Transfer Pak should validate the Nintendo logo")
    require(source, "gbHeaderChecksumValid(header)", "Transfer Pak should validate the Game Boy header checksum")
    require(source, "local.headerValid && transferPakMbcSupported", "Mapper support should require a valid header")
    require(header, "bool headerValid = false;", "Transfer Pak info should report header validity")
    require(serial, 'out.print(F(" HEADER_VALID="));', "Serial API should report header validity")
    require(adapt, "Cartridge header unreadable", "Adapt.html should distinguish invalid headers from unsupported mappers")
    require(adapt, "selectedGbInfo?.headerValid", "Adapt.html should gate reads on a validated header")


def test_idle_output_loop_skips_frame_finalization() -> None:
    source = read_text("firmware/core/runtime/runtime_loop.cpp")
    require(
        source,
        "bool __not_in_flash_func(outputFramePreparationRequired)()",
        "Output runtime should centralize the pending-frame fast path",
    )
    require(
        source,
        "if (controllerFrameNeedsDelivery(i))",
        "Pending controller frames must force output preparation",
    )
    require(
        source,
        "if (!outputFramePreparationRequired()) {\n    sendPreparedOutputFrame();\n    return;\n  }",
        "Idle loops should skip frame copy/finalize/restore work",
    )
    require(
        source,
        "if (latencyTest.isEnabled() || outputMode == OUTPUT_PS3)",
        "Continuous-report and latency modes must retain preparation",
    )


def test_controller_runtime_caches_active_transform_groups() -> None:
    core = read_text("firmware/core/controller_runtime_core.cpp")
    finalize = read_text("firmware/core/controller_runtime_output_finalize.cpp")
    hotkeys = read_text("firmware/core/controller_runtime_hotkeys.cpp")
    require(
        core,
        "refreshRuntimeFeatureMask(updated);",
        "Runtime feature mask should refresh when an input poll changes state",
    )
    for token in (
        "RUNTIME_FEATURE_BUTTON_REMAP",
        "RUNTIME_FEATURE_CHORD_REMAP",
        "RUNTIME_FEATURE_TURBO",
        "RUNTIME_FEATURE_CLASSIC_MERGE",
        "RUNTIME_FEATURE_ANALOG_CALIBRATION",
        "RUNTIME_FEATURE_OUTPUT_BUTTON_MAP",
    ):
        require(core, token, f"Runtime feature mask missing {token}")
    require(
        core,
        "controller_runtime_internal::rebuildDigitalButtonsForOutputFrame(\n"
        "      runtimeFeatureEnabled(RUNTIME_FEATURE_BUTTON_REMAP),",
        "Output finalization should reuse the cached transform mask",
    )
    require(
        finalize,
        "if (applyVirtualHotkeys) {\n        frame.digital_buttons &= ~suppressedControllerHotkeySourceButtons(i);",
        "Disabled virtual hotkeys should skip source-button suppression",
    )
    require(
        hotkeys,
        "bool __not_in_flash_func(virtualControllerHotkeysRequireService)()",
        "Virtual hotkeys must keep servicing an active held combo after settings change",
    )
    require(
        hotkeys,
        "const uint32_t transitionButtons = physicalButtons | previousPhysicalButtons[i];",
        "Held hotkeys should accept either physical button press order across adjacent polls",
    )
    require(
        hotkeys,
        "const uint32_t homeButtons = homeHotkeyState[i].active ? physicalButtons : transitionButtons;",
        "Held hotkeys should use transition bridging only for activation",
    )


def test_serial_setting_registry_matches_persistent_storage() -> None:
    registry = read_text("firmware/core/settings_registry.h")
    require(
        registry,
        "{SettingScope::PerMode, SettingValueType::UInt8,  "
        "(uint16_t)offsetof(PerModeSettingsRecord, reserved_musical_buttons), 0,"
        "                          0, 1,",
        "Two-player merge must be exposed as a per-mode setting",
    )
    require(
        registry,
        "reserved_musical_buttons), 0,                         0, 0,",
        "Retired musical-button setting must be immutable",
    )
    require(
        registry,
        "analog_mouse_mode), ANALOG_MOUSE_OFF, ANALOG_MOUSE_OFF, ANALOG_MOUSE_RIGHT,",
        "Analog mouse mode must be pinned Off/Left/Right",
    )
    require(
        registry,
        "if (id == SettingId::ClassicDualMerge) return 0;",
        "External two-player merge storage must not alias a settings record field",
    )
    require(
        registry,
        "if (id == SettingId::ClassicDualMerge) break;",
        "External two-player merge writes must not alias a settings record field",
    )


def test_switch_rumble_requires_host_enable_and_ignores_neutral_packets() -> None:
    common_header = read_text("firmware/output/switch/out_SwitchCommon.h")
    subcommands = read_text("firmware/output/switch/output_switch_subcommand_runtime.cpp")
    report_runtime = read_text("firmware/output/switch/output_switch_report_runtime.cpp")
    common_source = read_text("firmware/output/switch/out_SwitchCommon.cpp")
    usb_source = read_text("firmware/output/switch/out_SwitchUsb.cpp")
    host_runtime = read_text("firmware/core/rumble_test_runtime.cpp")
    usb_output = read_text("firmware/output/usb/out_usb.h")
    output_loop = read_text("firmware/output/runtime/output_loop_runtime.cpp")
    main_display = read_text("firmware/menu/menu_main_display.cpp")
    menu_idle = read_text("firmware/menu/menu_idle_runtime.cpp")

    require(
        common_header,
        "bool vibration_enabled() const { return _vibration_enabled; }",
        "Switch rumble callback should be able to honor the host vibration-enable state",
    )
    require(
        subcommands,
        "_vibration_enabled = _switchRequestReport[11] != 0;",
        "Switch vibration subcommand should support both enable and disable",
    )
    require(
        report_runtime,
        "!inst->vibration_enabled() ||",
        "Switch output reports must not drive motors before vibration is enabled",
    )
    require(
        report_runtime,
        "kNeutralRumble[4] = {0x00, 0x01, 0x40, 0x40}",
        "Switch neutral rumble packets should be recognized explicitly",
    )
    require(
        report_runtime,
        "inst->set_controller_rumble(itf, 0, 0);",
        "Disabled or neutral Switch rumble should actively stop controller motors",
    )
    require(
        report_runtime,
        "report[10] == ENABLE_VIBRATION",
        "Switch vibration-disable command should stop motors in the same callback",
    )
    require(
        report_runtime,
        "(r0.low_band_amp > r1.low_band_amp) ? r0.low_band_amp : r1.low_band_amp",
        "Switch heavy rumble must combine low-frequency energy from both actuators",
    )
    require(
        report_runtime,
        "(r0.high_band_amp > r1.high_band_amp) ? r0.high_band_amp : r1.high_band_amp",
        "Switch light rumble must combine high-frequency energy from both actuators",
    )
    require(
        report_runtime,
        "uint8_t heavy = static_cast<uint8_t>(255 * low_band_f);",
        "Switch heavy rumble must use decoded low-frequency amplitude only",
    )
    require(
        report_runtime,
        "uint8_t light = static_cast<uint8_t>(255 * high_band_f);",
        "Switch light rumble must use decoded high-frequency amplitude only",
    )
    require(
        report_runtime,
        "inst->set_controller_rumble(itf, heavy, light);",
        "Switch rumble must pass independent heavy and light motor channels downstream",
    )
    reject(report_runtime, "left_f", "Switch rumble must not map actuator side directly to motor type")
    reject(report_runtime, "right_f", "Switch rumble must not map actuator side directly to motor type")
    for stale in ("left_raw", "right_raw", "best_left", "best_right"):
        reject(
            report_runtime,
            stale,
            "Encoded Switch HD-rumble bytes must not become linear motor strength",
        )
    require(
        usb_output,
        "output_is_generic_mister_hid_mode(output_effective_mode())",
        "Short light-pulse extension should cover generic MiSTer DInput output",
    )
    require(
        usb_output,
        "output_effective_mode_is_any(OUTPUT_XINPUT, OUTPUT_XINPUT2P)",
        "Short light-pulse extension should cover canonical XInput outputs",
    )
    reject(
        usb_output,
        "bridgeBriefZeroReports",
        "XInput must not retain a zero-report bridge",
    )
    reject(
        host_runtime,
        "kHostZeroStopConfirmMs",
        "Host zero reports must not be delayed",
    )
    require(
        host_runtime,
        "kHostLightMinimumPulseMs = 40",
        "DInput and XInput light effects should receive a perceptible minimum pulse",
    )
    require(
        host_runtime,
        "state.effectiveLeft = left;",
        "Heavy host rumble must still follow zero reports immediately",
    )
    require(
        host_runtime,
        "extendShortLightPulse && state.effectiveRight != 0",
        "Only an already-active light effect may finish its minimum pulse",
    )
    require(
        host_runtime,
        "never extend repeated zeros",
        "Repeated host zero reports must not restart the light pulse",
    )
    require(
        output_loop,
        "extern \"C\" void tud_umount_cb(void) {\n"
        "  usbDeviceDiag.umount_count++;\n"
        "  rumbleRuntimeClearAllHostFeedback();",
        "USB unmount should clear stale host rumble",
    )
    require(
        output_loop,
        "extern \"C\" void tud_suspend_cb(bool remote_wakeup_en) {\n"
        "  (void)remote_wakeup_en;\n"
        "  usbDeviceDiag.suspend_count++;\n"
        "  rumbleRuntimeClearAllHostFeedback();",
        "USB suspend should clear stale host rumble",
    )
    require(
        main_display,
        "const bool enteringVisibleScreensaver =\n"
        "      !idleAnimationActive && !idleDimActive;",
        "Screensaver entry should be detected once for dim and animation modes",
    )
    require(
        main_display,
        "if (enteringVisibleScreensaver) {\n"
        "      rumbleRuntimeSetHostFeedbackSuppressed(true);",
        "Screensaver entry should suppress continuing host rumble",
    )
    require(
        host_runtime,
        "if (hostFeedbackSuppressed) {\n"
        "    state.effectiveLeft = 0;\n"
        "    state.effectiveRight = 0;",
        "Host rumble packets should remain muted throughout the screensaver",
    )
    require(
        menu_idle,
        "if (wasVisibleIdle) {\n"
        "    rumbleRuntimeSetHostFeedbackSuppressed(false);",
        "Controller or UI activity should restore host rumble after wake",
    )
    require(
        read_text("firmware/output/usb/out_usb.h"),
        "if ((left_large | right_small) != 0) {\n"
        "    resetIdleTimer();",
        "Nonzero host rumble should wake the OLED before feedback is applied",
    )
    require(
        common_source,
        "_switchRequestReportSize < 11",
        "Rumble-only Switch reports must not reuse a stale subcommand byte",
    )
    require(
        common_source,
        "_switchRequestReportSize >= 16",
        "Switch SPI reads must reject truncated request packets",
    )
    require(
        usb_source,
        "_switchRequestReportSize >= 2",
        "Switch USB commands must validate their packet length",
    )
    require(
        subcommands,
        "kMaxSpiReadLength = sizeof(_report) - 21",
        "Switch SPI replies must fit inside the fixed report buffer",
    )

def test_switch_controller_specific_mappings_honor_name_and_position() -> None:
    mapping = read_text("firmware/output/switch/output_switchpro_mapping_runtime.h")
    genesis_mapping = read_text(
        "firmware/output/switch/output_switch_genesis_nso_mapping.h"
    )
    capabilities = read_text("firmware/output/output_capabilities.h")
    saturn_setup = read_text("firmware/input/saturn/input_saturn_setup.cpp")
    output_mapping = read_text("firmware/output/usb/output_hid_mapping_runtime.h")
    output_cache = read_text("firmware/core/controller_runtime_output_finalize.cpp")
    full_menu = read_text("firmware/menu/menu_helpers_visibility.cpp")
    quick_menu = read_text("firmware/menu/quick_config_visibility.cpp")
    settings_store = read_text("firmware/core/settings_store.h")
    settings_registry = read_text("firmware/core/settings_registry.h")
    settings_defaults = read_text("firmware/core/settings_store_per_mode_defaults.cpp")
    button_map_mode = read_text("firmware/core/button_map_mode.h")

    gamecube_mapping = read_text(
        "firmware/output/switch/output_switch_gamecube_mapping.h"
    )

    reject(
        output_mapping,
        "get_effective_output_mode() != OUTPUT_SWITCHPRO",
        "Switch must honor the selected Name/Position mapping",
    )
    reject(
        output_cache,
        "get_effective_output_mode() != OUTPUT_SWITCHPRO",
        "Switch diagnostics must reflect the selected Name/Position mapping",
    )
    require(
        full_menu,
        "return !menuInputSupportsSelectableButtonMapMode();",
        "Full menu input/output Button Map visibility",
    )
    require(
        quick_menu,
        "buttonMapModeIsUserSelectable(",
        "Quick Config input/output Button Map visibility",
    )
    require(mapping, "const bool position_button_map = isPositionButtonMapActive();",
            "Switch mapping must consume the shared Name/Position selection")
    require(mapping, "} else if (position_button_map) {",
            "Switch mapping must retain the position-layout branch")
    require(mapping, "_switchReport.a = (faceButtons & INPUT_A) != 0;",
            "Switch mapping must retain the name-layout branch")
    require(mapping, "gamecube_on_switch = (deviceMode == RZORD_GAMECUBE);", "GameCube Switch mode")
    require(mapping, "_switchReport.a = (faceButtons & INPUT_A) != 0;", "GameCube A name mapping")
    require(mapping, "_switchReport.b = (faceButtons & INPUT_B) != 0;", "GameCube B name mapping")
    for token, label in (
        ("case RZORD_NES:      return 0;", "NES Name default"),
        ("case RZORD_N64:      return 0;", "N64 Name default"),
        ("case RZORD_GAMECUBE: return 0;", "GameCube Name default"),
    ):
        require(settings_defaults, token, label)
    for mode, label in (
        ("RZORD_NES", "NES Position support"),
        ("RZORD_SNES", "SNES Position support"),
        ("RZORD_WII", "Wii Classic Position support"),
        ("RZORD_VBOY", "Virtual Boy Position support"),
    ):
        require(button_map_mode, mode, label)
    for token, label in (
        ("n64_on_switch = deviceMode == RZORD_N64;",
         "N64 Switch Pro mapping gate"),
        ("(position_button_map ? frame.B : frame.A) || frame.R3;",
         "N64 A plus C-Right face overlay"),
        ("(position_button_map ? frame.A : frame.B) || frame.L3;",
         "N64 B plus C-Down face overlay"),
        ("_switchReport.x = frame.X;",
         "N64 C-Up and Virtual Boy right-up face overlay"),
        ("_switchReport.y = frame.Y;",
         "N64 C-Left and Virtual Boy right-left face overlay"),
        ("_switchReport.a = frame.A || frame.R2;",
         "Virtual Boy named A plus right-D-pad-right overlay"),
        ("_switchReport.b = frame.B || frame.L2;",
         "Virtual Boy named B plus right-D-pad-down overlay"),
        ("} else if (nes_on_switch) {",
         "NES standard Switch Pro mapping gate"),
        ("} else if (pce_on_switch && !pce_six_button_on_switch) {",
         "PCE two-button standard Switch Pro mapping gate"),
        ("_switchReport.a = frame.B;",
         "PCE physical I to Switch A mapping"),
        ("_switchReport.b = frame.A;",
         "PCE physical II to Switch B mapping"),
    ):
        require(mapping, token, label)
    for source, token, label in (
        (settings_store, "uint8_t gamecube_l_switch_mode;",
         "GameCube left-shoulder assignment must persist per input mode"),

        (settings_store, "GameCubeLSwitchMode,",
         "GameCube left-shoulder assignment must have a stable setting id"),
        (settings_registry,
         "offsetof(PerModeSettingsRecord, gamecube_l_switch_mode)",
         "GameCube left-shoulder assignment must be registered"),
        (settings_defaults, "mode != RZORD_GAMECUBE",
         "Non-GameCube modes must sanitize the GameCube-only setting"),
        (full_menu, "case menu_item_gamecube_l_switch:",
         "Full menu must gate the GameCube left-shoulder setting"),
        (quick_menu, "if (isGameCube(mode)) {\n    addVisibleItem(QCI_GC_L_SWITCH);",
         "Quick Config must always show the assignment for GameCube"),
        (quick_menu, 'case QCI_GC_L_SWITCH: return "L Shoulder";',
         "Quick Config must use the requested GameCube shoulder label"),
        (mapping, "switch_gamecube::map_left_shoulder(",
         "Switch output must use the exclusive GameCube shoulder mapper"),
        (gamecube_mapping, "return {pressed, false};",
         "GameCube L assignment must not also emit ZL"),
        (gamecube_mapping, "return {false, pressed};",
         "GameCube ZL assignment must not also emit L"),
    ):
        require(source, token, label)
    reject(
        quick_menu,
        "isGameCube(mode) && selectedOutput == OUTPUT_SWITCHPRO",
        "GameCube Quick Config must not hide the shoulder assignment by output mode",
    )

    require(mapping, "megadrive_on_switch = (deviceMode == RZORD_MEGADRIVE);", "Genesis Switch mode")
    require(
        mapping,
        "genesis_nso_on_switch =\n"
        "    megadrive_on_switch && output_uses_switch_genesis_profile();",
        "Genesis NSO mapping should follow the enumerated native profile",
    )
    require(
        mapping,
        "genesis_pro_layout_on_switch = megadrive_on_switch && !genesis_nso_on_switch;",
        "Genesis should retain its compensated Switch Pro fallback",
    )
    for token, label in (
        ("switch_genesis_nso::apply_button_bits(",
         "Genesis should use the empirical Pro-identity packer"),
        ("_switchReport.zr = frame.R2;  // Mode",
         "Genesis Mode should emit Switch ZR"),
        ("_switchReport.home =\n    frame.HOME;",
         "M30 Home should not alias the generic L2 shoulder"),
    ):
        require(mapping, token, label)
    for token, label in (
        ("if (frame.A) bits |= kY;", "Genesis A should emit Switch Pro Y"),
        ("if (frame.B) bits |= kB;", "Genesis B should emit Switch Pro B"),
        ("if (frame.R1) bits |= kA;", "Genesis C should emit Switch Pro A"),
        ("if (frame.X) bits |= kL;", "Genesis X should emit Switch Pro L"),
        ("if (frame.Y) bits |= kX;", "Genesis Y should emit Switch Pro X"),
        ("if (frame.L1) bits |= kR;", "Genesis Z should emit Switch Pro R"),
        ("validated independently by tools/test_classic_genesis_nso_mode.py",
         "Official Genesis firmware capture should stay a separate validation"),
    ):
        require(genesis_mapping, token, label)
    require(
        capabilities,
        "case RZORD_MEGADRIVE:\n      return SWITCHPRO_GENESIS;",
        "Genesis NSO mode should select its native identity",
    )
    require(
        saturn_setup,
        "output_apply_automatic_switch_profile_for_input_mode(deviceMode);",
        "Genesis setup should apply the profile before USB enumeration",
    )
    require(
        mapping,
        "(deviceMode == RZORD_SATURN) &&\n"
        "    std::strcmp(frame.controller_type_name, \"Saturn\") == 0;",
        "Switch output should retain the tested Pro fallback for Saturn digital pads",
    )
    reject(
        mapping,
        "saturn_digital_on_switch && output_uses_switch_genesis_profile()",
        "Saturn should not claim the Genesis NSO identity",
    )
    require(
        mapping,
        "switch_genesis_nso::apply_six_button_position_bits(",
        "Saturn digital pads should match the tested Mega6 six-button positions",
    )
    require(
        mapping,
        "} else if (saturn_digital_on_switch) {\n"
        "    switchpro[port]->switchCommon->_switchReport.zl = frame.L2;\n"
        "    switchpro[port]->switchCommon->_switchReport.zr = frame.R2;",
        "Saturn L and R shoulders should remain Switch ZL and ZR",
    )


def test_genesis_switch_output_compensates_for_pro_identity() -> None:
    switch_mapping = read_text("firmware/output/switch/output_switchpro_mapping_runtime.h")
    genesis_mapping = read_text(
        "firmware/output/switch/output_switch_genesis_nso_mapping.h"
    )
    capabilities = read_text("firmware/output/output_capabilities.h")
    saturn_setup = read_text("firmware/input/saturn/input_saturn_setup.cpp")

    require(
        switch_mapping,
        "megadrive_on_switch = (deviceMode == RZORD_MEGADRIVE);",
        "Switch output should identify Genesis input",
    )
    require(
        switch_mapping,
        "genesis_nso_on_switch =\n"
        "    megadrive_on_switch && output_uses_switch_genesis_profile();",
        "Genesis NSO mapping should follow the enumerated native profile",
    )
    require(
        switch_mapping,
        "genesis_pro_layout_on_switch = megadrive_on_switch && !genesis_nso_on_switch;",
        "Genesis should retain its compensated Switch Pro fallback",
    )
    for token, label in (
        ("switch_genesis_nso::apply_button_bits(",
         "Genesis should use the empirical Pro-identity packer"),
        ("_switchReport.zr = frame.R2;  // Mode",
         "Genesis Mode should emit Switch ZR"),
        ("_switchReport.home =\n    frame.HOME;",
         "M30 Home should not alias the generic L2 shoulder"),
    ):
        require(switch_mapping, token, label)

    for token, label in (
        ("if (frame.A) bits |= kY;", "Genesis A should emit Switch Pro Y"),
        ("if (frame.B) bits |= kB;", "Genesis B should emit Switch Pro B"),
        ("if (frame.R1) bits |= kA;", "Genesis C should emit Switch Pro A"),
        ("if (frame.X) bits |= kL;", "Genesis X should emit Switch Pro L"),
        ("if (frame.Y) bits |= kX;", "Genesis Y should emit Switch Pro X"),
        ("if (frame.L1) bits |= kR;", "Genesis Z should emit Switch Pro R"),
        ("validated independently by tools/test_classic_genesis_nso_mode.py",
         "Official Genesis firmware capture should stay a separate validation"),
    ):
        require(genesis_mapping, token, label)

    require(
        capabilities,
        "case RZORD_MEGADRIVE:\n      return SWITCHPRO_GENESIS;",
        "Genesis NSO mode should select its native identity",
    )
    require(
        saturn_setup,
        "output_apply_automatic_switch_profile_for_input_mode(deviceMode);",
        "Genesis setup should apply the profile before USB enumeration",
    )
    require(
        switch_mapping,
        "(deviceMode == RZORD_SATURN) &&\n"
        "    std::strcmp(frame.controller_type_name, \"Saturn\") == 0;",
        "Switch output should retain the tested Pro fallback for Saturn digital pads",
    )
    reject(
        switch_mapping,
        "saturn_digital_on_switch && output_uses_switch_genesis_profile()",
        "Saturn should not claim the Genesis NSO identity",
    )
    require(
        switch_mapping,
        "switch_genesis_nso::apply_six_button_position_bits(",
        "Saturn digital pads should match the tested Mega6 six-button positions",
    )
    require(
        switch_mapping,
        "} else if (saturn_digital_on_switch) {\n"
        "    switchpro[port]->switchCommon->_switchReport.zl = frame.L2;\n"
        "    switchpro[port]->switchCommon->_switchReport.zr = frame.R2;",
        "Saturn L and R shoulders should remain Switch ZL and ZR",
    )


def test_pce_six_button_switch_output_matches_genesis_position() -> None:
    pce_input = read_text("firmware/input/pce/input_pce_poll.cpp")
    switch_mapping = read_text("firmware/output/switch/output_switchpro_mapping_runtime.h")
    genesis_mapping = read_text(
        "firmware/output/switch/output_switch_genesis_nso_mapping.h"
    )

    for token, label in (
        ("PCE_2) == 0) ? INPUT_A", "PCE II should use the generic A backing bit"),
        ("PCE_1) == 0) ? INPUT_B", "PCE I should use the generic B backing bit"),
        ("PCE_5) == 0) ? INPUT_X", "PCE V should use the generic X backing bit"),
        ("PCE_6) == 0) ? INPUT_Y", "PCE VI should use the generic Y backing bit"),
        ("PCE_4) == 0) ? INPUT_L1", "PCE IV should use the generic L backing bit"),
        ("PCE_3) == 0) ? INPUT_R1", "PCE III should use the generic R backing bit"),
    ):
        require(pce_input, token, label)

    for token, label in (
        ("pce_on_switch = deviceMode == RZORD_PCE;",
         "Both PCE pad types should use the PCE Switch control policy"),
        ('std::strcmp(frame.controller_type_name, "6-Button") == 0;',
         "Only six-button PCE pads should use the Genesis position map"),
        ("switch_genesis_nso::apply_pce_six_button_position_bits(",
         "Switch output should apply the tested PCE position map"),
        ("} else if (pce_on_switch) {\n"
         "    switchpro[port]->switchCommon->_switchReport.minus = frame.SELECT;",
         "PCE Select should emit Switch Minus for two- and six-button pads"),
    ):
        require(switch_mapping, token, label)

    for token, label in (
        ("if (frame.R1) bits |= kY;", "PCE III should emit Switch Pro Y"),
        ("if (frame.A) bits |= kB;", "PCE II should emit Switch Pro B"),
        ("if (frame.B) bits |= kA;", "PCE I should emit Switch Pro A"),
        ("if (frame.L1) bits |= kL;", "PCE IV should emit Switch Pro L"),
        ("if (frame.X) bits |= kX;", "PCE V should emit Switch Pro X"),
        ("if (frame.Y) bits |= kR;", "PCE VI should emit Switch Pro R"),
        ("bits & ~kGenesisSixButtonPositionMask",
         "PCE Select, Run, Home, Capture, and D-pad bits should be preserved"),
    ):
        require(genesis_mapping, token, label)


def test_n64_c_buttons_remain_unique_for_mister_hid() -> None:
    caps = read_text("firmware/output/output_capabilities.h")
    finalize = read_text("firmware/core/controller_runtime_output_finalize.cpp")
    n64_input = read_text("firmware/input/gc64/input_gc64_poll.cpp")
    labels = read_text("firmware/core/classic_dual_merge_config.cpp")
    input_reference = read_text("docs/classic2usb/Classic2USB-Input-Reference.md")
    release_targets = read_text("tools/release_targets.json")

    require(
        caps,
        "Generic HID/MiSTer has enough button bits for N64 C buttons",
        "N64 C buttons should stay on distinct HID bits for MiSTer",
    )
    require(
        caps,
        "mode == OUTPUT_MISTER || mode == OUTPUT_HID",
        "N64 C-button spatial remapping should be disabled for MiSTer/HID",
    )
    reject(
        finalize,
        "output_apply_n64_c_buttons_to_face_buttons(buttons, frame);",
        "Output finalization must not pre-collapse N64 C buttons before HID mapping",
    )
    require_any(
        n64_input,
        (
            "frame.L3 = joybus[port]->digitalPressed(N64B_C_DOWN);",
            "frame.L3 = (buttons & N64B_C_DOWN) != 0;",
        ),
        "N64 C-Down should use a neutral HID button, not R2",
    )
    require_any(
        n64_input,
        (
            "frame.R3 = joybus[port]->digitalPressed(N64B_C_RIGHT);",
            "frame.R3 = (buttons & N64B_C_RIGHT) != 0;",
        ),
        "N64 C-Right should use a neutral HID button, not Select",
    )
    require(labels, 'if (mask == INPUT_L3) return "C-Down";', "N64 C-Down label")
    require(labels, 'if (mask == INPUT_R3) return "C-Right";', "N64 C-Right label")
    reject(input_reference, "C-Right` overlaps `A`", "N64 docs should not document a firmware collision")
    reject(release_targets, "gamecontrollerdb", "Classic2USB package should not own GameControllerDB files")
    reject(release_targets, "reflex_adapt_serial_bridge", "Classic2USB release package should not include the unvalidated MiSTer serial bridge")
    reject(release_targets, "mister_serial_bridge.py", "Classic2USB release package should not embed the unvalidated MiSTer serial bridge")
    for name in (
        "Jaguar_input_16d0_1460_ReflexJag_v3.map",
        "N64_input_16d0_1460_ReflexN64_v3.map",
        "Saturn_input_16d0_1460_ReflexSat_v3.map",
        "VirtualBoy_input_16d0_1460_ReflexVboy_v3.map",
    ):
        require_file(
            f"tools/release_assets/classic2usb/mister/config/inputs/{name}",
            f"{name} release map",
        )
    reject(
        release_targets,
        "_input_16d0_1460_Reflex_Adapt_Classic2USB_v3.map",
        "Dedicated MiSTer maps should use input-mode serial names, not the generic product serial",
    )


def test_switch_nso_profiles_are_opt_in_and_switch_guarded() -> None:
    runtime_state = read_text("firmware/output/output_runtime_state.cpp")
    capabilities = read_text("firmware/output/output_capabilities.h")
    switch_mapping = read_text("firmware/output/switch/output_switchpro_mapping_runtime.h")
    switch_modes = read_text("firmware/output/switch/out_SwitchCommon.h")
    switch_setup = read_text("firmware/output/usb/output_usb_mode_setup_console_runtime.h")
    switch_subcommands = read_text("firmware/output/switch/output_switch_subcommand_runtime.cpp")
    switch_descriptor = read_text("firmware/output/switch/output_switch_usb_descriptors_runtime.h")
    hid_bridge = read_text("firmware/output/autodetect/output_autodetect_hid_bridge.h")
    n64_input = read_text("firmware/input/gc64/input_gc64_poll.cpp")
    n64_setup = read_text("firmware/input/gc64/input_gc64_setup.cpp")
    snes_setup = read_text("firmware/input/snes/input_snes_setup.cpp")
    mode_save = read_text("firmware/core/settings_store_mode_save.cpp")
    oled_masks = read_text("firmware/menu/menu_pad_button_masks.cpp")
    quick_config = read_text("firmware/menu/quick_config_state.cpp")
    quick_actions = read_text("firmware/menu/quick_config_actions.cpp")
    quick_render = read_text("firmware/menu/quick_config_render.cpp")
    quick_header = read_text("firmware/menu/quick_config.h")
    quick_visibility = read_text("firmware/menu/quick_config_visibility.cpp")
    full_visibility = read_text("firmware/menu/menu_helpers_visibility.cpp")
    menu_catalog = read_text("firmware/menu/menu_catalog.cpp")
    output_boot = read_text("firmware/output/runtime/output_boot_runtime.cpp")
    input_poll = read_text("firmware/input/runtime/input_poll_runtime.cpp")
    hotswap = read_text("firmware/input/autodetect/input_autodetect_hotswap_runtime.cpp")
    per_mode_defaults = read_text("firmware/core/settings_store_per_mode_defaults.cpp")
    settings_registry = read_text("firmware/core/settings_registry.h")
    menu_descriptors = read_text("firmware/menu/menu_descriptors.cpp")
    serial_commands = read_text("firmware/core/serial_core_commands.cpp")
    adapt_html = read_text("web/Adapt.html")


    for token, label in (
        ("three_do_on_switch = deviceMode == RZORD_3DO;",
         "3DO Switch mapping gate"),
        ("_switchReport.a = frame.X;  // 3DO C",
         "3DO C to Switch A"),
        ("_switchReport.b = frame.B;", "3DO B to Switch B"),
        ("_switchReport.x = 0;", "Unused Switch X in 3DO mode"),
        ("_switchReport.y = frame.A;", "3DO A to Switch Y"),
    ):
        require(switch_mapping, token, label)

    for token, label in (
        ("case RZORD_NES:", "NES NSO support"),
        ("case RZORD_SNES:", "SNES NSO support"),
        ("case RZORD_MEGADRIVE:", "Genesis NSO support"),
        ("case RZORD_N64:", "N64 NSO support"),
    ):
        require(runtime_state, token, label)
    require(
        runtime_state,
        "return outputMode == OUTPUT_SWITCHPRO &&\n"
        "         nso_special_effective_for_input_mode(deviceMode);",
        "NSO identities must require Switch Pro output and the effective per-input policy",
    )
    require(
        runtime_state,
        "return connectedPlayers == 1;",
        "Automatic N64 NSO must require exactly one connected player",
    )
    require(
        capabilities,
        "mode, nso_special_effective_for_input_mode(mode)",
        "Switch profile selection must use the player-count-gated NSO state",
    )
    require(
        capabilities,
        "if (!nsoSpecialEnabled) {\n    return SWITCHPRO_PRO;",
        "Disabled NSO mode must retain the Switch Pro identity",
    )
    for token, label in (
        ("case RZORD_NES:\n      return SWITCHPRO_NES;", "NES profile"),
        ("case RZORD_SNES:\n      return SWITCHPRO_SNES;", "SNES profile"),
        ("case RZORD_MEGADRIVE:\n      return SWITCHPRO_GENESIS;", "Genesis profile"),
        ("case RZORD_N64:\n      return SWITCHPRO_N64;", "N64 profile"),
    ):
        require(capabilities, token, label)
    reject(capabilities, "case RZORD_GAMECUBE:",
           "GameCube must stay on the Switch Pro fallback")
    for setup_source, label in ((n64_setup, "N64"), (snes_setup, "NES/SNES")):
        require(setup_source,
                "output_apply_automatic_switch_profile_for_input_mode(deviceMode);",
                f"{label} setup should apply the selected profile before enumeration")

    for profile in (
        "SWITCHPRO_PRO = 0", "SWITCHPRO_NES", "SWITCHPRO_SNES",
        "SWITCHPRO_GENESIS", "SWITCHPRO_N64",
    ):
        require(switch_modes, profile, f"Switch profile enum missing {profile}")
    for token, label in (
        ("TinyUSBDevice.setID(0x057E, 0x2009);", "Switch Pro USB identity"),
        ("TinyUSBDevice.setID(0x057E, 0x2017);", "SNES/NES USB identity"),
        ("TinyUSBDevice.setID(0x057E, 0x201E);", "Genesis USB identity"),
        ("TinyUSBDevice.setID(0x057E, 0x2019);", "N64 USB identity"),
        ('TinyUSBDevice.setProductDescriptor("MD/Gen Control Pad");', "Genesis product string"),
        ("TinyUSBDevice.setID(0x0F0D, 0x0092);", "Explicit Pokken legacy identity"),
    ):
        require(switch_setup, token, label)
    for token, label in (
        ("case SWITCHPRO_PRO:\n      _report[18] = 0x03;", "Pro controller type"),
        ("case SWITCHPRO_NES:\n      _report[18] = 0x09;", "NES controller type"),
        ("case SWITCHPRO_SNES:\n      _report[18] = 0x0B;", "SNES controller type"),
        ("case SWITCHPRO_GENESIS:\n      _report[18] = 0x0D;", "Genesis controller type"),
        ("case SWITCHPRO_N64:\n      _report[18] = 0x0C;", "N64 controller type"),
    ):
        require(switch_subcommands, token, label)

    descriptor_bytes = [
        int(value, 16)
        for value in re.findall(r"0x([0-9A-Fa-f]{2})", switch_descriptor)
    ]
    if len(descriptor_bytes) != 203:
        raise AssertionError(
            f"Nintendo Switch descriptor should be the native 203 bytes, got {len(descriptor_bytes)}"
        )
    if descriptor_bytes[-3:] != [0x91, 0x83, 0xC0]:
        raise AssertionError("Nintendo Switch descriptor has a non-native tail")
    for report_id in (0xE0, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7):
        if any(
            descriptor_bytes[index:index + 2] == [0x85, report_id]
            for index in range(len(descriptor_bytes) - 1)
        ):
            raise AssertionError(f"Switch descriptor still advertises WebHID report 0x{report_id:02X}")
    require(
        hid_bridge,
        "get_effective_output_mode() != OUTPUT_SWITCHPRO",
        "Switch Pro runtime should reject Adapt WebHID feature reports",
    )
    for source, token, label in (
        (full_visibility, "selectedOutput != OUTPUT_SWITCHPRO ||\n"
         "             !input_mode_supports_nso_special(menu_input) ||\n"
         "             !input_mode_allows_nso_special_for_connected_players(",
         "Full menu Switch/input/player-count guard"),
        (quick_visibility, "selectedOutput == OUTPUT_SWITCHPRO &&\n"
         "      input_mode_supports_nso_special(mode) &&\n"
         "      input_mode_allows_nso_special_for_connected_players(",
         "Quick Config Switch/input/player-count guard"),
        (quick_visibility, "addVisibleItem(QCI_NSO_SPECIAL);",
         "Quick Config NSO row"),
        (quick_config, "temp_nso_special != 0 &&\n"
         "         input_mode_supports_nso_special(mode) &&\n"
         "         input_mode_allows_nso_special_for_connected_players(",
         "Quick Config active-toggle/player-count gate"),
        (adapt_html, "return [4, 6, 7, 8].includes(Number(inputMode));",
         "Web UI NES/SNES/Genesis/N64 input guard"),
    ):
        require(source, token, label)
    for source, token, label in (
        (quick_visibility, 'case QCI_NSO_SPECIAL: return "NSO Mode (1P)";',
         "Quick Config one-player NSO label"),
        (menu_catalog, '{ menu_item_nso_special, "NSO Mode (1P)" },',
         "Full menu one-player NSO label"),
        (quick_header, "QC_CONFIRM_NSO_1P",
         "Quick Config native NSO confirmation state"),
        (quick_actions, "saveOrConfirmNsoSinglePlayer()",
         "Quick Config save warning gate"),
        (quick_actions, "!menu_nso_special",
         "Quick Config warning only when native NSO is enabled"),
        (quick_render, 'display.println(F("Player 2 removed"));',
         "Quick Config Player 2 removal warning"),
        (output_boot,
         "nso_special && input_mode_allows_nso_special_for_connected_players(\n"
         "          inputMode, connectedPlayers)",
         "Native N64 NSO must enumerate one player only when one is connected"),
        (input_poll, "kN64NsoPlayerCountStableMs = 750",
         "N64 player-count transitions must be debounced"),
        (input_poll,
         "preserveInputModeForPlayerCountReboot(deviceMode, inputPortCount());",
         "N64 player-count changes must preserve input mode across descriptor reboot"),
        (hotswap, "automaticN64NsoProfileChanged",
         "AUTO input must re-enumerate both N64 NSO player-count directions"),
    ):
        require(source, token, label)
    require(
        mode_save,
        "latch_nso_special_connected_players(\n"
        "    deviceMode, connectedInputFrameCount(inputPortCount()));",
        "Live manual input changes must refresh the N64 NSO player-count latch",
    )
    require(per_mode_defaults,
            "if (mode == RZORD_N64) {\n    return 1;",
            "N64 NSO mode should default on")
    require(per_mode_defaults, "return 0;",
            "Other NSO-capable inputs should default off")
    require(settings_registry, "defaultNsoSpecialValue",
            "Settings registry should expose the per-input NSO default")
    for source, token, label in (
        (quick_config, "nsoDescriptorChanged", "Quick Config NSO reboot"),
        (menu_descriptors, "desc.setting_id == SettingId::NsoSpecial",
         "Full menu NSO reboot"),
        (serial_commands, "id == SettingId::NsoSpecial",
         "Serial NSO reboot"),
    ):
        require(source, token, label)
    require(
        capabilities,
        "mode == OUTPUT_SWITCHPRO && output_uses_switch_n64_profile()",
        "Switch N64 profile should retain unique C-button backing fields",
    )
    require(
        switch_mapping,
        "_switchReport.zl = frame.L2;",
        "N64 Z should map to Switch ZL",
    )
    require(
        switch_mapping,
        "_switchReport.zr = frame.L3;",
        "N64 C-down should map to Switch ZR",
    )
    require(
        switch_mapping,
        "_switchReport.minus = frame.R3;",
        "N64 C-right should map to Switch Minus",
    )
    require(n64_input, "frame.RX = INT8_MIN;", "N64 C-Left should reach full left stick")
    require(n64_input, "frame.RX = INT8_MAX;", "N64 C-Right should reach full right stick")
    require(n64_input, "frame.RY = INT8_MIN;", "N64 C-Up should reach full up stick")
    require(n64_input, "frame.RY = INT8_MAX;", "N64 C-Down should reach full down stick")
    require(
        switch_mapping,
        "nes_nso_on_switch || snes_nso_on_switch",
        "NES and SNES Select should emit native Switch Minus",
    )
    require(switch_mapping, "_switchReport.minus = frame.SELECT;", "NES/SNES Select mapping")
    require(switch_mapping,
            "genesis_pro_layout_on_switch = megadrive_on_switch && !genesis_nso_on_switch;",
            "Genesis Pro fallback should remain available when NSO mode is off")
    reject(runtime_state,
           "case RZORD_GAMECUBE:\n      return true;",
           "GameCube must not expose the undecoded NSO protocol")
    require(oled_masks, "if (r.L3) mask |= 0x0800;", "N64 OLED should read C-down from L3")
    require(oled_masks, "if (r.R3) mask |= 0x2000;", "N64 OLED should read C-right from R3")

def test_n64_analog_learn_expands_raw_range_to_full_scale() -> None:
    n64_header = read_text("firmware/input/gc64/Input_GC64.h")
    n64_poll = read_text("firmware/input/gc64/input_gc64_poll.cpp")
    range_header = read_text("firmware/core/classic_analog_range.h")
    range_impl = read_text("firmware/core/classic_analog_range.cpp")
    finalize = read_text("firmware/core/controller_runtime_output_finalize.cpp")
    visibility = read_text("firmware/menu/menu_helpers_visibility.cpp")
    quick_visibility = read_text("firmware/menu/quick_config_visibility.cpp")
    quick_render = read_text("firmware/menu/quick_config_render.cpp")
    trace_policy = read_text("firmware/menu/analog_stick_trace.h")

    require(n64_header, "N64_ANALOG_RAW_MAX = 80", "N64 normalized range should start from an 80-count raw throw")
    require(n64_header, "N64_ANALOG_RAW_MIN = -N64_ANALOG_RAW_MAX", "N64 normalized range should be symmetric")
    require(range_impl, "kN64AnalogLearnInitialMax = 80", "N64 learn mode should assume an initial +/-80 raw throw")
    require(
        range_impl,
        "return (int8_t)kN64AnalogLearnInitialMax;",
        "N64 learn initialization should not apply the generic 90 percent estimate",
    )
    require(
        n64_poll,
        "frame.LX = applyClassicAnalogLearnAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LX, raw_lx);\n          frame.LY = applyClassicAnalogLearnAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LY, raw_ly);",
        "N64 normal output should always use learned normalization",
    )
    reject(
        n64_poll,
        "if (n64_analog_range == CLASSIC_ANALOG_RANGE_NORMALIZED)",
        "N64 normal output should not expose a Raw/Norm/Cal/Learn branch",
    )
    reject(
        range_header,
        "if (mode == RZORD_N64) return true;",
        "N64 should not expose the generic classic analog range setting",
    )
    require(
        finalize,
        "case RZORD_N64:\n      return false;",
        "N64 should not apply saved stick calibration over learned normalization",
    )
    require(
        visibility,
        "case menu_item_n64_analog_range:\n      return true;",
        "N64 stick range should not be visible in the system menu",
    )
    require(
        quick_visibility,
        'case QCI_RANGE_TEST: return "Analog Test";',
        "Quick Config should use the standard Analog Test label",
    )
    require(
        quick_render,
        "analogTraceUsesOctagonalGate",
        "N64 raw stick test should use the graphical gate diagnostic",
    )
    require(
        trace_policy,
        "mode == RZORD_N64",
        "N64 raw stick test should use eight-direction gate capture",
    )


def test_gba_link_input_stays_disabled_in_release_build() -> None:
    feature_gates = read_text("firmware/config/classic2usb/feature_gates.h")
    platformio = read_text("platformio.ini")

    reject(feature_gates, "#define ENABLE_INPUT_GBA", "Classic2USB release should not expose GBA input mode without the boot ROM")
    require(platformio, "-DJOYBUS_DISABLE_GBA_LINK", "Classic2USB release should disable the GBA Joybus link ROM path")
    require(
        platformio,
        "-<third_party/firmware_libraries/JoybusLib/joybus_gba.cpp>",
        "Classic2USB release should not compile GBA link support",
    )


def test_boot_setup_keeps_home_paint_and_completion_marker() -> None:
    runtime = read_text("firmware/core/runtime/firmware_runtime.cpp")

    require(
        runtime,
        "runPlatformFeedbackServices(false);",
        "Boot setup should paint the home screen before runtime AUTO scans",
    )
    require(
        runtime,
        "autoDetectBenchmarkMark(ADBENCH_BOOT_SETUP_DONE);",
        "Boot setup should mark completion after the first home paint",
    )
    reject(
        runtime,
        "Temporary boot-hang isolation",
        "Temporary boot isolation code should not ship in release firmware",
    )
    reject(
        runtime,
        'showBootTraceMarker("6h skipfb");',
        "Boot setup should not skip the first feedback/home paint",
    )


def test_nes_centered_home_layout_keeps_face_buttons_above_bottom_row() -> None:
    layouts = read_text("firmware/menu/menu_pad_layouts_data_classic.cpp")
    render = read_text("firmware/menu/menu_main_display_pad_render.cpp")
    match = re.search(
        r"const PadButton padLayoutNES\[\] = \{(?P<body>.*?)\n\};",
        layouts,
        re.S,
    )
    if not match:
        raise AssertionError("NES home pad layout not found")
    body = match.group("body")
    require(render, "constexpr uint8_t kHomePadRowOffset = 3;", "Centered home pad row offset should be stable")
    require(body, "{ GPAD_B, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF }", "NES B should render one row above the bottom line")
    require(body, "{ GPAD_A, 2, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF }", "NES A should render one row above the bottom line")
    reject(body, "{ GPAD_B, 3,", "NES B should not render on the bottom line")
    reject(body, "{ GPAD_A, 3,", "NES A should not render on the bottom line")


def test_one_player_analog_home_gauge_is_scoped_and_incremental() -> None:
    render = read_text("firmware/menu/menu_main_display_pad_render.cpp")

    for token, label in (
        ("if (mode == RZORD_PSX)", "PSX dual-stick home-gauge eligibility"),
        ("if (mode == RZORD_SATURN)", "Saturn analog home-gauge eligibility"),
        ("if (mode == RZORD_N64)", "N64 analog home-gauge eligibility"),
        ("if (mode == RZORD_GAMECUBE)", "GameCube analog home-gauge eligibility"),
        ("if (mode == RZORD_WII)", "Wii Classic analog home-gauge eligibility"),
        ("analogDiagnosticKindForFrame(mode, frame) != AnalogDiagnosticKind::Stick",
         "Digital-only controller exclusion"),
        ('std::strcmp(frame.controller_type_name, "Classic") == 0',
         "Wii Classic controller restriction"),
        ('std::strcmp(frame.controller_type_name, "ClassicPro") == 0',
         "Wii Classic Pro controller restriction"),
        ("connectedHomeControllerCount() == 1",
         "Exactly-one-connected-controller gate"),
        ("shouldShowHomeAnalogGauge(centerOutputVirtualPad)",
         "Centered single-pad home-view gate"),
        ("const bool centerOutputVirtualPad =",
         "Single connected controller always uses the centered home layout"),
        ("kHomeAnalogGaugeWidth + kPadGlyphWidth",
         "Controller pad leaves one character between it and the analog gauge"),
        ("? kHomeAnalogPadColOffset",
         "Controller pad uses the separated analog-view column"),
        ("analogTraceUsesOctagonalGate(",
         "Shared round-versus-octagonal gate policy"),
        ("constexpr int16_t kHomeN64AnalogFullScale = 85;",
         "N64 native home-gauge scale"),
        ("writeHomeAnalogGaugeDotSpan(homeAnalogGaugeState.dotX,",
         "Incremental old-dot restoration"),
        ("writeHomeAnalogGaugeDotSpan(dotX, dotX, dotY, octagonal);",
         "Incremental new-dot drawing"),
        ("HomeAnalogGaugeKind::DualAxes",
         "PSX and Wii Classic dual-axis panel selection"),
        ('static const char* const labels[4] = { "LX", "LY", "RX", "RY" };',
         "Dual-stick horizontal gauge labels"),
        ("scaleHomeAnalogAxis(-frame.LY, fullScale)",
         "Left-stick vertical gauge visual direction"),
        ("scaleHomeAnalogAxis(-frame.RY, fullScale)",
         "Right-stick vertical gauge visual direction"),
        ("if (crosshair && distanceSquared <= radiusSquared)",
         "Round-gate crosshair clipping"),
        ("if (crosshair && insideGate)",
         "Octagonal-gate crosshair clipping"),
        ('std::strcmp(frame.controller_type_name, "Nunchuk") == 0',
         "Nunchuk single-stick home gate"),
        ("return frame.HAS_ANALOG_STICK_AUX\n      ? HomeAnalogGaugeKind::DualAxes",
         "GameCube dual-stick home gauges"),
    ):
        require(render, token, label)

    reject(
        render,
        "return HomeAnalogGaugeKind::DualAxes;\n  }\n#endif\n  return HomeAnalogGaugeKind::DualAxes;",
        "Analog home gauge must not become a generic all-controller feature",
    )


def test_3do_home_layout_matches_physical_abc_order() -> None:
    layouts = read_text("firmware/menu/menu_pad_layouts_data_classic.cpp")
    match = re.search(
        r"const PadButton padLayout3DO\[\] = \{(?P<body>.*?)\n\};",
        layouts,
        re.S,
    )
    if not match:
        raise AssertionError("3DO home pad layout not found")
    body = match.group("body")
    expected = (
        "{ GPAD_A, 3, 6 * 6, PAD_FACE_ON, PAD_FACE_OFF }",
        "{ GPAD_B, 2, 7 * 6, PAD_FACE_ON, PAD_FACE_OFF }",
        "{ GPAD_X, 1, 8 * 6, PAD_FACE_ON, PAD_FACE_OFF }",
    )
    positions = tuple(body.index(token) for token in expected)
    if positions != tuple(sorted(positions)):
        raise AssertionError("3DO face buttons must render left-to-right as A, B, C")
    require(body, expected[0], "3DO A should render one row below B")
    require(body, expected[2], "3DO C should render one row above B")


def test_3do_autodetect_uses_stable_signature_before_saturn() -> None:
    header = read_text("firmware/input/autodetect/Input_AutoDetect.h")
    detect_port = read_text(
        "firmware/input/autodetect/input_autodetect_detect_port.cpp"
    )
    probe = read_text(
        "firmware/input/autodetect/input_autodetect_special_probes.cpp"
    )

    reject(
        header,
        "AUTODETECT_ENABLE_3DO_HOTSWAP",
        "Strict 3DO detection must remain available while Auto waits for hot-plug",
    )
    full_detect = detect_port[
        detect_port.index("AutoDetectResult AutoDetector::detectPort(uint8_t port"):
    ]
    pce_probe = full_detect.index("AutoDetectResult pce = probePCE(pins, port);")
    snes_probe = full_detect.index("AutoDetectResult snes = probeSNES(pins, port);")
    three_do_probe = full_detect.index(
        "AutoDetectResult tdo = probe3DO(pins, is_hotswap);"
    )
    saturn_probe = full_detect.index(
        "AutoDetectResult saturn = probeSaturn(pins, port, &saturnBusActive"
    )
    if not (pce_probe < snes_probe < three_do_probe < saturn_probe):
        raise AssertionError(
            "Strict 3DO AUTO probe must retain PCE/SNES priority and run before Saturn"
        )
    require(
        detect_port,
        "AutoDetectResult deferredSharedBusSaturn = AUTODETECT_NONE;",
        "Shared-bus Saturn claims must wait for strict 3DO qualification",
    )
    require(
        probe,
        "gpio_put(pins.tdo_clk, HIGH);\n  gpio_put(pins.tdo_out, HIGH);\n"
        "  gpio_set_dir(pins.tdo_clk, GPIO_OUT);",
        "3DO probe should preload idle-high before driving the shared lines",
    )
    require(
        probe,
        "delayMicroseconds(500);",
        "3DO probe should provide the library-required idle interval",
    )
    require(
        probe,
        "constexpr uint8_t passes = 3;",
        "3DO probe should collect three qualification frames",
    )
    require(
        probe,
        "if (validCount >= 2 && stableValidPairs >= 1)",
        "3DO Auto detection should require repeated stable valid frames",
    )


def test_3do_latency_path_preserves_auto_resolution_on_transient_frames() -> None:
    header = read_text("firmware/input/3do/Input_3do.h")
    setup = read_text("firmware/input/3do/input_3do_setup.cpp")
    poll = read_text("firmware/input/3do/input_3do_poll.cpp")
    library = read_text(
        "third_party/firmware_libraries/ThreedoLib/ThreedoLib.h"
    )

    for source, token, label in (
        (setup, "pollInterval = 500;", "protocol-safe 500 us poll cadence"),
        (header, "discovery_interval_us = 100000", "100 ms empty-port discovery"),
        (header, "active_port_grace_us = 1000000", "active-port grace"),
        (poll, "bool port_sample_valid[input_ports] = {true, true};",
         "per-port sample validity"),
        (poll, "const uint8_t observed_controller_count =",
         "observed controller count"),
        (poll, "port_sample_valid[port] = false;",
         "transient invalid-sample marker"),
        (poll, "const uint8_t max_controllers =",
         "known-chain fast read"),
        (library, '#include "hardware/gpio.h"', "RP2040 GPIO access"),
        (library, "__not_in_flash_func(doClockCycle)", "SRAM clock cycle"),
        (library, "__not_in_flash_func(readControllers)", "SRAM chain read"),
        (library, "__not_in_flash_func(update)", "SRAM update"),
        (library, "gpio_get(tdo_DIN)", "fast GPIO input"),
        (library, "gpio_put(tdo_CLOCK", "fast GPIO clock"),
    ):
        require(source, token, f"3DO latency guard missing {label}")

    transient_guard = poll.index("if (!port_sample_valid[port])")
    disconnect_branch = poll.index("if (port_controller_count[port] == 0)")
    if transient_guard >= disconnect_branch:
        raise AssertionError(
            "3DO transient sample guard must run before disconnect processing"
        )
    require(
        poll,
        "i = next_index < MAX_USB_OUT ? next_index : MAX_USB_OUT;\n"
        "      continue;",
        "Transient 3DO samples must preserve existing output frames",
    )
    require(
        poll,
        "now_us - last_active_at_us[port] < active_port_grace_us",
        "Transient 3DO invalid reads must use the active-port grace",
    )
    reject(
        setup,
        "pollInterval = 375;",
        "375 us 3DO polling breaks controller detection",
    )

    read_start = library.index("__not_in_flash_func(readControllers)")
    read_end = library.index("  public:", read_start)
    hot_read = library[read_start:read_end]
    reject(hot_read, "digitalRead(", "3DO hot read must avoid Arduino GPIO")
    reject(hot_read, "digitalWrite(", "3DO hot read must avoid Arduino GPIO")
    require(
        hot_read,
        "delayMicroseconds(4);",
        "3DO wire timing must retain its proven 4 us phases",
    )




def test_passive_jpc_hotswap_requires_saturn_shared_bus_arbitration() -> None:
    header = read_text("firmware/input/autodetect/Input_AutoDetect.h")
    support_header = read_text(
        "firmware/input/autodetect/input_autodetect_support.h"
    )
    support = read_text("firmware/input/autodetect/input_autodetect_support.cpp")
    detect_port = read_text(
        "firmware/input/autodetect/input_autodetect_detect_port.cpp"
    )
    hotswap = read_text(
        "firmware/input/autodetect/input_autodetect_hotswap_runtime.cpp"
    )

    for source, token, label in (
        (header, "detectPortSaturnSharedBusOnly(", "probe declaration"),
        (support_header, "detectAutoInputPortSaturnSharedBusOnly(",
         "runtime declaration"),
        (support, "AutoDetector::detectPortSaturnSharedBusOnly(port, is_hotswap)",
         "runtime forwarding"),
        (hotswap, "passiveCandidate.result == AUTODETECT_JPC",
         "JPC-only gate"),
        (hotswap, "detectAutoInputPortSaturnSharedBusOnly(\n        passiveCandidate.port, true)",
         "active arbitration"),
        (hotswap, "tryPassiveHotSwapWithSharedBusArbitration(passiveQuick, 0)",
         "quick path arbitration"),
        (hotswap, "tryPassiveHotSwapWithSharedBusArbitration(\n        passiveResult, millis() - passive_started)",
         "fallback arbitration"),
        (hotswap, "return tryHotSwapToDetectedMode(activeMode);",
         "active AUTO resolution"),
        (hotswap, "return tryPassiveHotSwapToDetectedMode(passiveMode);",
         "true JPC assisted resolution"),
    ):
        require(source, token, label)

    shared_probe = detect_port[
        detect_port.index("detectPortSaturnSharedBusOnly("):
        detect_port.index("AutoDetectResult AutoDetector::detectPort(uint8_t port")
    ]
    if shared_probe.index("probe3DO(pins, is_hotswap)") >= shared_probe.index(
        "probeSaturn(pins, port, nullptr, is_hotswap)"
    ):
        raise AssertionError("3DO must arbitrate before Saturn/Genesis")
    reject(hotswap, "passiveCandidate.result == AUTODETECT_SMS",
           "SMS must not use Saturn-family arbitration")


def test_auto_hotplug_shift_register_uses_narrow_second_port_probe() -> None:
    autodetect_header = read_text("firmware/input/autodetect/Input_AutoDetect.h")
    support_header = read_text("firmware/input/autodetect/input_autodetect_support.h")
    support = read_text("firmware/input/autodetect/input_autodetect_support.cpp")
    detect_port = read_text("firmware/input/autodetect/input_autodetect_detect_port.cpp")
    runtime = read_text("firmware/input/autodetect/input_autodetect_runtime.cpp")

    require(
        autodetect_header,
        "static AutoDetectResult detectPortShiftRegisterOnly(uint8_t port, bool is_hotswap = false);",
        "AutoDetect should expose a shift-register-only probe",
    )
    require(
        support_header,
        "AutoDetectResult detectAutoInputPortShiftRegisterOnly(uint8_t port, bool is_hotswap = false);",
        "AutoDetect support wrapper should expose the shift-register-only probe",
    )
    require(
        support,
        "return AutoDetector::detectPortShiftRegisterOnly(port, is_hotswap);",
        "AutoDetect support wrapper should call the shift-register-only probe",
    )
    require(
        detect_port,
        "return finish(probeSNES(pins, port));",
        "Shift-register-only probe should only run the SNES/NES/VB probe",
    )
    require(
        runtime,
        "bool isShiftRegisterAutoDetectResult(AutoDetectResult result)",
        "Auto hotplug should identify shift-register-class detections",
    )
    require(
        runtime,
        "bool runHotswapShiftRegisterQuickPass(AutoDetectResult (&portResults)[INPUT_MIXED_PORT_COUNT]",
        "Auto hotplug should have a shift-register quick pass before the full probe chain",
    )
    require(
        runtime,
        "runHotswapShiftRegisterQuickPass(portResults, portModes, detected_port)",
        "Auto hotplug should try the shift-register quick pass before full auto-detect",
    )
    require(
        runtime,
        "if (is_hotswap && isShiftRegisterAutoDetectResult(portResults[0]))",
        "Auto hotplug should use the narrow second-port path after a first-port shift-register hit",
    )
    require(
        runtime,
        "portResults[1] = detectAutoInputPortShiftRegisterOnly(1, true);",
        "Second port should not run the full auto-detect chain after a first-port NES/SNES/VB hit",
    )
    require(
        runtime,
        'logInputAutodetectDebug("detect-shift", is_hotswap, detected_port, detectedMode);',
        "Auto hotplug trace should distinguish the shift-register fast path",
    )
    require(
        runtime,
        'logInputAutodetectDebug("detect-shift-quick", is_hotswap, detected_port, portModes[detected_port]);',
        "Auto hotplug trace should distinguish the shift-register quick pass",
    )
    require(
        runtime,
        "DeviceEnum runAutoDetectionFastCommonOnly(bool is_hotswap)",
        "Auto no-input quick retry should cover common cheap probes",
    )
    require(
        runtime,
        "runHotswapShiftRegisterQuickPass(portResults, portModes, detected_port) ||\n      runHotswapFastStrictBusPass(portResults, portModes, detected_port)",
        "Auto no-input quick retry should probe cheap NES/SNES/VB before slower serial buses",
    )
    require(
        hotswap := read_text("firmware/input/autodetect/input_autodetect_hotswap_runtime.cpp"),
        "DeviceEnum newMode = runAutoDetectionFastCommonOnly(true);",
        "Auto home hotplug scheduler should use the common quick retry path",
    )


def test_nes_shift_register_polling_uses_fast_cadence() -> None:
    header = read_text("firmware/input/snes/Input_Snes.h")
    setup = read_text("firmware/input/snes/input_snes_setup.cpp")
    snes_lib = read_text("third_party/firmware_libraries/SnesLib/SnesLib.h")
    require(header, "NES_IDLE_POLL_INTERVAL_US = 125", "NES fixed input should use the fastest connected-port cadence")
    require(header, "SNES_IDLE_POLL_INTERVAL_US = 125", "SNES-family idle poll should stay at the fast connected-port cadence")
    require(
        setup,
        "pollInterval = (internalMode == 1) ? NES_IDLE_POLL_INTERVAL_US : SNES_IDLE_POLL_INTERVAL_US;",
        "NES setup should use the fastest cadence while SNES/VB stay on the shared fast cadence",
    )
    reject(
        setup,
        "pollInterval = snesRumbleTechEnabled()",
        "RumbleTech support should not force the slow poll cadence at setup",
    )
    reject(
        setup,
        "? (snesRumbleTechEnabled() ? SNES_RUMBLETECH_POLL_INTERVAL_US : SNES_IDLE_POLL_INTERVAL_US)\n    : STANDARD_POLL_INTERVAL_US;",
        "NES and Virtual Boy must not fall back to the 16 ms standard poll interval",
    )
    require(snes_lib, "STROBE_HIGH_US = 6", "SNES latch pulse should use the tested fast timing")
    require(snes_lib, "STROBE_LOW_SETTLE_US = 2", "SNES latch settle should use the tested fast timing")
    require(snes_lib, "CLOCK_LOW_US = 1", "SNES shift clock low phase should use the tested fast timing")
    require(snes_lib, "CLOCK_HIGH_US = 1", "SNES shift clock high phase should use the tested fast timing")
    require(snes_lib, "RUMBLE_STROBE_HIGH_US = 12", "RumbleTech strobe should keep legacy timing")
    require(snes_lib, "RUMBLE_STROBE_LOW_SETTLE_US = 6", "RumbleTech strobe settle should keep legacy timing")
    require(snes_lib, "RUMBLE_CLOCK_LOW_US = 6", "RumbleTech command clock low phase should keep legacy timing")
    require(snes_lib, "RUMBLE_CLOCK_HIGH_US = 6", "RumbleTech command clock high phase should keep legacy timing")
    require(
        snes_lib,
        "if (queuedRumbleForSinglePad) {\n        sendQueuedRumbleDuringPoll(false);",
        "RumbleTech commands should be sent immediately after the normal SNES read",
    )
    reject(
        snes_lib,
        "readSingleController(queuedRumbleForSinglePad)",
        "RumbleTech command bits must not be interleaved into the normal SNES read clocks",
    )
    require(
        header,
        "NES_FAST_FULL_VALIDATE_POLLS",
        "Standard NES fast polling should periodically validate through the full shift-register path",
    )
    require(
        header,
        "bool tryFastPollStandardNes(uint8_t port, uint8_t index)",
        "Standard NES fast polling should stay guarded in the SNES/NES input module",
    )
    require(
        setup,
        "nes_fast_validate_count[i] = 0;",
        "SNES/NES setup should reset standard NES fast-poll validation counters",
    )
    require(
        snes_lib,
        "fastPollStandardNes",
        "SnesLib should expose a standard NES-only fast poll helper",
    )
    require(
        snes_lib,
        "gpio_get_all()",
        "Standard NES fast poll should use a single raw GPIO port read in the hot loop on RP2040",
    )
    require(
        snes_lib,
        "const uint32_t dataMask",
        "Standard NES fast poll should precompute the data-pin mask outside the hot loop",
    )
    require(
        read_text("firmware/input/snes/input_snes_poll.cpp"),
        "tryFastPollStandardNes(port, port)",
        "NES polling should try the guarded standard-pad fast path before the full SnesLib update",
    )


def test_snes_shift_register_empty_ports_are_slow_scanned() -> None:
    header = read_text("firmware/input/snes/Input_Snes.h")
    setup = read_text("firmware/input/snes/input_snes_setup.cpp")
    module = read_text("firmware/input/base/RZInputModule.cpp")

    require(
        header,
        "SNES_EMPTY_PORT_SCAN_INTERVAL_MS = 16",
        "SNES-family empty ports should use a slow hotplug scan interval",
    )
    require(
        header,
        "bool is_port_connected(const uint8_t index) override;",
        "SNES-family input should report connected physical ports to the shared scheduler",
    )
    require(
        setup,
        "empty_port_behaviour = EMPTY_PORT_USE_INTERVAL;\n  polling_empty_interval_ms = SNES_EMPTY_PORT_SCAN_INTERVAL_MS;",
        "SNES-family setup should keep connected ports fast and slow-scan empty ports",
    )
    require(
        setup,
        "bool RZInputSnes::is_port_connected(const uint8_t index)",
        "SNES-family input should identify active physical ports",
    )
    require(
        module,
        "if (!physical_port_enabled(i) || is_port_connected(i) || i == port)",
        "empty-port arbitration must ignore masked-off ports so AUTO hotplug can poll the resolved port",
    )


def test_genesis_connected_polling_enforces_protocol_safe_cadence() -> None:
    header = read_text("firmware/input/saturn/Input_Saturn.h")
    runtime_state = read_text(
        "firmware/input/saturn/input_saturn_runtime_state.h"
    )
    autodetect = read_text("firmware/input/autodetect/Input_AutoDetect.h")
    features = read_text("firmware/input/saturn/saturnlib_features.h")
    poll = read_text("firmware/input/saturn/input_saturn_poll.cpp")
    setup = read_text("firmware/input/saturn/input_saturn_setup.cpp")
    saturn_lib = read_text("third_party/firmware_libraries/SaturnLib/SaturnLib.h")

    for token, label in (
        ("SATLIB_ENABLE_8BITDO_HOME_BTN", "M30 auxiliary buttons"),
        ("SATLIB_ENABLE_MISSION6", "Mission Stick 6-axis layout"),
        ("SATLIB_ENABLE_MEGATAP", "Mega Drive multitap"),
        ("SATLIB_ENABLE_SATTAP", "Saturn multitap"),
    ):
        require(
            features,
            token,
            f"Shared SaturnLib feature config is missing {label}",
        )
    require(
        header,
        '#include "saturnlib_features.h"',
        "The Saturn input wrapper must use the shared SaturnLib feature config",
    )
    require(
        runtime_state,
        '#include "saturnlib_features.h"\n#include <SaturnLib/SaturnLib.h>',
        "Saturn runtime-state translation units must configure SaturnLib before including it",
    )
    require(
        autodetect,
        '#include "../saturn/saturnlib_features.h"\n  #include <SaturnLib/SaturnLib.h>',
        "Autodetect translation units must configure SaturnLib before including it",
    )
    for source, label in (
        (header, "Saturn input wrapper"),
        (runtime_state, "Saturn runtime-state header"),
        (autodetect, "Autodetect header"),
    ):
        reject(
            source,
            "#define SATLIB_ENABLE_8BITDO_HOME_BTN",
            f"{label} must not define SaturnLib features locally",
        )

    expected_saturnlib_includes = {
        "firmware/input/autodetect/Input_AutoDetect.h",
        "firmware/input/saturn/Input_Saturn.h",
        "firmware/input/saturn/input_saturn_runtime_state.h",
    }
    actual_saturnlib_includes = set()
    for path in (ROOT / "firmware").rglob("*"):
        if path.suffix not in (".cpp", ".h"):
            continue
        source = path.read_text(encoding="utf-8")
        if "#include <SaturnLib/SaturnLib.h>" not in source:
            continue
        relative = path.relative_to(ROOT).as_posix()
        actual_saturnlib_includes.add(relative)
        if "saturnlib_features.h" not in source:
            raise AssertionError(
                f"{relative} includes SaturnLib without the shared feature config"
            )
        if source.index("saturnlib_features.h") > source.index(
            "#include <SaturnLib/SaturnLib.h>"
        ):
            raise AssertionError(
                f"{relative} configures SaturnLib after including it"
            )
    if actual_saturnlib_includes != expected_saturnlib_includes:
        raise AssertionError(
            "SaturnLib direct include sites changed: "
            f"{sorted(actual_saturnlib_includes)}"
        )

    require(
        header,
        "MEGADRIVE_CONNECTED_POLL_INTERVAL_US = 2500",
        "Direct Genesis pads should leave more than 2 ms between acquisitions",
    )
    require(
        header,
        "MEGADRIVE_MIN_TH_HIGH_US = 2100",
        "Genesis polling should have a hard TH-high recovery floor",
    )
    require(
        header,
        "SATURN_CONNECTED_POLL_INTERVAL_US = 1000",
        "Saturn wireless receivers should have a stable one-millisecond poll cadence",
    )
    require(
        poll,
        "bool __not_in_flash_func(RZInputSaturn::poll)()",
        "Saturn/Genesis input polling should stay in RAM for latency",
    )
    require(
        saturn_lib,
        "#include <pico/platform.h>",
        "SaturnLib should include RP2040 RAM-placement attributes for hot-path functions",
    )
    require(
        saturn_lib,
        "void __not_in_flash_func(readSatPort)()",
        "SaturnLib protocol dispatch should stay in RAM for latency",
    )
    require(
        saturn_lib,
        "void __not_in_flash_func(readMegadrivePad)(uint8_t nibble_0, uint8_t nibble_1)",
        "Megadrive pad decode should stay in RAM for latency",
    )
    require(
        saturn_lib,
        "void __not_in_flash_func(update)(){",
        "SaturnLib update should stay in RAM for latency",
    )
    require(
        poll,
        "(uint32_t)(micros() - lastMegadriveTransactionFinishedUs) <\n"
        "            MEGADRIVE_MIN_TH_HIGH_US",
        "Scheduler catch-up must not shorten the Genesis TH-high recovery period",
    )
    require(
        poll,
        "found_connected_megadrive = true;",
        "Genesis cadence should apply in forced and automatic input modes",
    )
    require(
        poll,
        "} else if (found_connected_megadrive) {\n"
        "    pollInterval = MEGADRIVE_CONNECTED_POLL_INTERVAL_US;\n"
        "  } else if (found_fast_saturn) {",
        "Genesis recovery timing must take precedence over faster Saturn polling",
    )
    require(
        setup,
        "lastMegadriveTransactionFinishedUs = 0;",
        "Genesis transaction timing should reset when the input module is initialized",
    )
    require(
        saturn_lib,
        "#include <hardware/gpio.h>",
        "SaturnLib should use RP2040 direct GPIO primitives on the controller hot path",
    )
    require(
        saturn_lib,
        "const uint32_t pins = gpio_get_all();",
        "SaturnLib should read the 4-bit data bus with one raw GPIO read on RP2040",
    )
    for token in (
        "MEGA6_SIGNATURE_CONFIRM_HITS = 4",
        "MEGA6_SIGNATURE_WINDOW_MISSES = 128",
        "MEGADRIVE_TH_SETTLE_US = 8",
        "sixbtn_signature_hits",
        "sixbtn_signature_window_misses",
        "sixbtn_confirmed",
        "basePageValid",
        "sc.sixbtn_confirmed ? SAT_ID_MEGA6 : SAT_ID_MEGA3",
        "setControlValues(sc, 2, 0x0F);",
    ):
        require(
            saturn_lib,
            token,
            f"Genesis six-button detection hardening is missing {token}",
        )
    reject(
        saturn_lib,
        "sixbtn_counter",
        "A single six-button signature must not latch Mega6 through the old counter",
    )
    for token in (
        "MEGA6_EXTRA_PAGE_GRACE_POLLS",
        "sixbtn_missed_polls",
        "MEGA6_TYPE_GRACE_POLLS",
    ):
        reject(
            saturn_lib,
            token,
            f"Confirmed Genesis pads must not use obsolete timeout state: {token}",
        )
    require(
        saturn_lib,
        "saturnlib_megadrive::six_button_id_phase(nibble_0)",
        "Genesis qualification should accept the observed OEM MK-1653 ID-page quirk",
    )
    require(
        saturn_lib,
        "saturnlib_megadrive::six_button_marker_valid(",
        "OEM-compatible ID candidates must still validate the fixed marker page",
    )
    require(
        saturn_lib,
        "saturnlib_megadrive::m30_aux_control_page(",
        "The M30 Home and Star page must use its validated decoder",
    )
    reject(
        saturn_lib,
        "(sixButtonExtraPage & 0b00110000) == 0b00110000",
        "Genesis C/B activity must not be mistaken for invalid fixed MXYZ-page bits",
    )
    reject(
        saturn_lib,
        "} else if (!sc.sixbtn_confirmed) {\n"
        "        sc.sixbtn_signature_hits = 0;",
        "One incomplete Genesis signature must not erase OEM qualification progress",
    )
    for token in (
        "sc.sixbtn_signature_window_misses = 0;",
        "sc.sixbtn_signature_window_misses <\n"
        "            SaturnController::MEGA6_SIGNATURE_WINDOW_MISSES",
        "sc.sixbtn_signature_window_misses >=\n"
        "            SaturnController::MEGA6_SIGNATURE_WINDOW_MISSES",
    ):
        require(
            saturn_lib,
            token,
            f"OEM Mega6 qualification window is missing {token}",
        )
    require(
        saturn_lib,
        "if (sc.sixbtn_signature_hits >= SaturnController::MEGA6_SIGNATURE_CONFIRM_HITS)",
        "Genesis pads must still require multiple validated signatures",
    )
    require(
        saturn_lib,
        "if (sixButtonSignature && sc.sixbtn_confirmed) {",
        "Genesis XYZ/Mode data must stay hidden until Mega6 is confirmed",
    )
    require(
        saturn_lib,
        "setControlValues(sc, 2, sixButtonExtraPage & 0b00001111);",
        "Genesis XYZ/Mode data must come from a validated six-button extension page",
    )
    require(
        saturn_lib,
        "} else if (!sc.sixbtn_confirmed) {\n"
        "        // Do not expose an extension page before the pad is qualified.",
        "Confirmed Mega6 must retain held XYZ/Mode through intermittent probe misses",
    )
    reject(
        header + poll + setup,
        "megadriveTypeDowngradeStartedMs",
        "Confirmed Mega6 must remain latched until physical-disconnect cleanup",
    )
    require(
        poll,
        "if (dtype[i] == SAT_DEVICE_MEGA3) {",
        "Mega3 mapping must suppress transient XYZ/Mode probe data",
    )
    require(
        saturn_lib,
        "if (config.enable_saturn) {\n        // Precharge data lines",
        "Saturn data-line precharge should stay out of the Genesis-only hot path",
    )
    require(
        saturn_lib,
        "for (uint8_t i = 0; i < lastJoyCount; i++)",
        "SaturnLib should copy previous state only for active slots",
    )
    reject(
        saturn_lib,
        "lastState.debugNibbleCount = currentState.debugNibbleCount;",
        "SaturnLib hot path should not copy debug-only nibble metadata into lastState",
    )


def test_latency_trace_gpio_is_default_off_and_wraps_runtime_phases() -> None:
    header = read_text("firmware/platform/latency_trace_gpio.h")
    source = read_text("firmware/platform/latency_trace_gpio.cpp")
    boot = read_text("firmware/core/boot/boot_storage_runtime.cpp")
    input_runtime = read_text("firmware/input/runtime/input_poll_runtime.cpp")
    runtime = read_text("firmware/core/runtime/runtime_loop.cpp")
    usb_send = read_text("firmware/output/usb/output_usb_send_runtime.h")

    for token in (
        "ENABLE_LATENCY_TRACE_GPIO",
        "PIN_LATENCY_TRACE_POLL",
        "PIN_LATENCY_TRACE_PROCESS",
        "PIN_LATENCY_TRACE_PREPARE",
        "PIN_LATENCY_TRACE_SEND",
        "#define PIN_LATENCY_TRACE_POLL LATENCY_TRACE_PIN_UNUSED",
        "inline void latencyTraceGpioBegin() {}",
        "class LatencyTraceGpioScope",
        "class LatencyPhaseTraceScope",
    ):
        require(header, token, f"Latency trace GPIO header missing {token}")
    for token in (
        "gpio_init(pin);",
        "gpio_set_dir(pin, GPIO_OUT);",
        "gpio_put(pin, 0);",
        "gpio_put(pin, high ? 1 : 0);",
    ):
        require(source, token, f"Latency trace GPIO implementation missing {token}")
    require(boot, "latencyTraceGpioBegin();", "Latency trace GPIO should initialize with latency runtime")
    require(source, "LATTRACE SEQ=", "Latency phase trace should dump over serial")
    require(source,
            "ENABLE_N64_LATENCY_FOCUSED_TRACE",
            "N64 focused trace builds should be able to filter noisy background phases")
    platformio = read_text("platformio.ini")
    classic2usb_block = platformio.split("[env:classic2usb]", 1)[1]
    reject(platformio, "[env:classic2usb_diagnostics]", "Release repo should not expose a diagnostics environment")
    reject(platformio, "[env:classic2usb_neogeo_minimal]", "Release repo should not expose a minimal experiment environment")
    for token in (
        "-DADAPT_ENABLE_LATENCY_TEST",
        "-DENABLE_LATENCY_PHASE_TRACE_CDC",
        "-DENABLE_N64_LATENCY_FOCUSED_TRACE",
        "-DLATENCY_PHASE_TRACE_RING_SIZE=1024",
        "-DENABLE_BOOTTRACE_OLED",
    ):
        reject(classic2usb_block, token, f"Retail Classic2USB should not spend SRAM on diagnostics-only {token}")
    require(source, "#if defined(ENABLE_LATENCY_PHASE_TRACE_CDC)\nLatencyPhaseTraceSample phaseTraceRing",
            "Latency phase trace ring should be compiled out of retail firmware")
    require(input_runtime, "LatencyTraceGpioScope pollTrace(PIN_LATENCY_TRACE_POLL);", "Input poll should trace controller read time")
    require(input_runtime, "LatencyPhaseTraceScope pollPhaseTrace(LATENCY_TRACE_PHASE_POLL);", "Input poll should serial-trace controller read time")
    require(input_runtime, "LatencyTraceGpioScope processTrace(PIN_LATENCY_TRACE_PROCESS);", "Input poll should trace processing time")
    require(input_runtime, "LatencyPhaseTraceScope processPhaseTrace(LATENCY_TRACE_PHASE_PROCESS);", "Input poll should serial-trace processing time")
    require(runtime, "LatencyTraceGpioScope prepareTrace(PIN_LATENCY_TRACE_PREPARE);", "Output prepare should be traced")
    require(runtime, "LatencyPhaseTraceScope preparePhaseTrace(LATENCY_TRACE_PHASE_PREPARE);", "Output prepare should serial-trace")
    require(runtime, "LatencyTraceGpioScope sendTrace(PIN_LATENCY_TRACE_SEND);", "Output send should be traced")
    require(runtime, "LatencyPhaseTraceScope sendPhaseTrace(LATENCY_TRACE_PHASE_SEND);", "Output send should serial-trace")
    require(runtime, "LatencyPhaseTraceScope uiPhaseTrace(LATENCY_TRACE_PHASE_UI);", "Post-poll UI should serial-trace")
    reject(
        runtime,
        "if (polled) {\n    pendingRuntimeUiUpdate = pendingRuntimeUiUpdate || updated;\n    return;\n  }",
        "Polled frames must not starve post-poll UI in zero-interval input modes",
    )
    require(runtime, "runOutputTransportSyncTasks();", "USB transport should be serviced before the controller critical path")
    require(runtime, "if (runInputRuntimeCycle(&polled, &updated))", "Controller polling should run before deferred runtime services")
    require(
        runtime,
        "runPostPollOutputTasks(polled);\n  updateKonamiCodeObserver(polled);\n  runActiveInputAdapterAfterOutputFrameSent(polled, updated);",
        "Output should send before deferred input debug/WebHID hooks",
    )
    source_tree = "\n".join(
        path.read_text(encoding="utf-8")
        for path in (ROOT / "firmware").rglob("*")
        if path.suffix in {".cpp", ".h"}
    )
    for token in (
        "FEATURE_MASH_COUNTER",
        "FEATURE_MUSICAL_BUTTONS",
        "FEATURE_MOUSE_TO_ANALOG",
        "runPostOutputFunFeatureObservers",
        "OUTPUT_MOUSE",
    ):
        reject(source_tree, token, f"Retail Classic2USB should not include dormant per-frame feature {token}")
    require_file("firmware/core/konami_code.cpp", "Konami jingle should remain available")
    require(
        runtime,
        "runActiveInputAdapterAfterOutputFrameSent(polled, updated);\n\n  {\n    LatencyPhaseTraceScope prePollUiTrace",
        "Post-send input hooks should run before menu/OLED/UI work",
    )
    require(runtime, "runPostPollUiIfNeeded(polled, updated);\n  runLoopBackgroundTasks();", "MiSTer output should send before post-poll UI/background work")
    require(
        runtime,
        "constexpr uint32_t kActiveFeedbackUiIntervalUs = 33333;",
        "Active post-poll feedback work should be throttled off the controller path",
    )
    require(
        runtime,
        "constexpr uint32_t kIdleFeedbackUiIntervalUs = 16667;",
        "Idle post-poll feedback should be capped at 60Hz",
    )
    require(
        runtime,
        "lastPostPollUiServiceUs",
        "Controller/menu UI should have a bounded service interval for zero-interval input modes",
    )
    require(
        runtime,
        "runPlatformRuntimeControllerUi(uiUpdated);",
        "Deferred controller/menu UI should still run on idle frames",
    )
    require(
        runtime,
        "runPlatformRuntimeFeedbackUi(uiUpdated);",
        "OLED/WebHID feedback work should be separately throttled from deferred UI",
    )
    require(
        runtime,
        "feedbackIntervalUs",
        "Post-poll feedback should be gated even while controller frames are changing",
    )
    snes_poll = read_text("firmware/input/snes/input_snes_poll.cpp")
    require(
        header,
        "LATENCY_TRACE_PHASE_SNES_UPDATE",
        "Latency trace should expose SNES/NES controller read attribution",
    )
    require(
        header,
        "LATENCY_TRACE_PHASE_SNES_MAP",
        "Latency trace should expose SNES/NES post-read mapping attribution",
    )
    require(
        source,
        'return "SNES_UPDATE";',
        "Latency trace should name SNES/NES controller reads",
    )
    require(
        source,
        'return "SNES_MAP";',
        "Latency trace should name SNES/NES post-read mapping",
    )
    require(
        snes_poll,
        "LatencyPhaseTraceScope snesUpdateTrace(LATENCY_TRACE_PHASE_SNES_UPDATE);",
        "SNES/NES poll should trace controller update cost",
    )
    require(
        snes_poll,
        "LatencyPhaseTraceScope snesMapTrace(LATENCY_TRACE_PHASE_SNES_MAP);",
        "SNES/NES poll should trace mapping/filtering cost",
    )
    neogeo_poll = read_text("firmware/input/neogeo/input_neogeo_poll.cpp")
    neogeo_setup = read_text("firmware/input/neogeo/input_neogeo_setup.cpp")
    neogeo_header = read_text("firmware/input/neogeo/Input_Neogeo.h")
    require(
        header,
        "LATENCY_TRACE_PHASE_NEOGEO_READ",
        "Latency trace should expose Neo-Geo GPIO read attribution",
    )
    require(
        header,
        "LATENCY_TRACE_PHASE_NEOGEO_MAP",
        "Latency trace should expose Neo-Geo direct mapping attribution",
    )
    require(
        source,
        'return "NEOGEO_READ";',
        "Latency trace should name Neo-Geo GPIO reads",
    )
    require(
        source,
        'return "NEOGEO_MAP";',
        "Latency trace should name Neo-Geo direct mapping",
    )
    require(
        neogeo_poll,
        "LatencyPhaseTraceScope neogeoReadTrace(LATENCY_TRACE_PHASE_NEOGEO_READ);",
        "Neo-Geo poll should trace GPIO read cost",
    )
    require(
        neogeo_poll,
        "LatencyPhaseTraceScope neogeoMapTrace(LATENCY_TRACE_PHASE_NEOGEO_MAP);",
        "Neo-Geo poll should trace direct mapping cost",
    )
    require(
        neogeo_setup,
        ".debounceMs = 4",
        "Neo-Geo should keep a small bounce lockout window",
    )
    require(
        neogeo_header,
        "uint32_t debounceBlockedUntilMs[input_ports][32]",
        "Neo-Geo should track per-button debounce re-arm windows",
    )
    require(
        neogeo_poll,
        "filterImmediatePressDebounce",
        "Neo-Geo should use immediate-edge debounce instead of delayed registration",
    )
    require(
        neogeo_poll,
        "Accept the first edge immediately",
        "Neo-Geo debounce comment should preserve the latency-sensitive intent",
    )
    require(
        neogeo_poll,
        "acceptedRawState[port] ^= bit;",
        "Neo-Geo debounce should accept the edge immediately",
    )
    require(
        neogeo_poll,
        "debounceBlockedUntilMs[port][pin] = nowMs + debounceMs;",
        "Neo-Geo debounce should only block subsequent bounce edges",
    )
    gc64_poll = read_text("firmware/input/gc64/input_gc64_poll.cpp")
    joybus_header = read_text("third_party/firmware_libraries/JoybusLib/JoybusLib.h")
    joybus_pio = read_text("third_party/firmware_libraries/JoybusLib/joybus_pio.cpp")
    for token in (
        "LATENCY_TRACE_PHASE_GC64_UPDATE",
        "LATENCY_TRACE_PHASE_JOYBUS_PIO_TXRX",
        "LATENCY_TRACE_PHASE_JOYBUS_INFO",
        "LATENCY_TRACE_PHASE_N64_READ",
        "LATENCY_TRACE_PHASE_N64_ACCESSORY",
        "LATENCY_TRACE_PHASE_N64_RUMBLE",
        "LATENCY_TRACE_PHASE_N64_MAP",
    ):
        require(header, token, f"N64/Joybus latency trace header missing {token}")
    for token in (
        'return "GC64_UPDATE";',
        'return "JOYBUS_PIO_TXRX";',
        'return "JOYBUS_INFO";',
        'return "N64_READ";',
        'return "N64_ACCESSORY";',
        'return "N64_RUMBLE";',
        'return "N64_MAP";',
    ):
        require(source, token, f"N64/Joybus latency trace name missing {token}")
    require(
        gc64_poll,
        "LatencyPhaseTraceScope gc64UpdateTrace(LATENCY_TRACE_PHASE_GC64_UPDATE);",
        "GC64 poll should trace Joybus update cost",
    )
    require(
        gc64_poll,
        "LatencyPhaseTraceScope n64MapTrace(LATENCY_TRACE_PHASE_N64_MAP);",
        "N64 poll should trace Adapt mapping cost",
    )
    require(
        joybus_header,
        "LatencyPhaseTraceScope joybusInfoTrace(LATENCY_TRACE_PHASE_JOYBUS_INFO);",
        "Joybus N64 info refresh should be traced",
    )
    require(
        joybus_header,
        "LatencyPhaseTraceScope n64ReadTrace(LATENCY_TRACE_PHASE_N64_READ);",
        "Joybus N64 controller transaction should be traced",
    )
    require(
        joybus_header,
        "LatencyPhaseTraceScope n64AccessoryTrace(LATENCY_TRACE_PHASE_N64_ACCESSORY);",
        "Joybus N64 accessory probe should be traced",
    )
    require(
        joybus_header,
        "LatencyPhaseTraceScope n64RumbleTrace(LATENCY_TRACE_PHASE_N64_RUMBLE);",
        "Joybus N64 rumble command should be traced",
    )
    require(
        joybus_pio,
        "LatencyPhaseTraceScope pioTrace(LATENCY_TRACE_PHASE_JOYBUS_PIO_TXRX);",
        "Joybus PIO transmit/receive should be traced",
    )
    require(
        joybus_header,
        "JoybusPioTimeoutScope timeoutScope(N64_ACCESSORY_TIMEOUT_MS);",
        "N64 Rumble Pak probing should allow full accessory transactions",
    )
    rumble_probe = joybus_header[
        joybus_header.index("bool probeN64RumblePak"):
        joybus_header.index("void __not_in_flash_func(readPort)")
    ]
    require(
        rumble_probe,
        "memset(data, 0x80, sizeof(data));",
        "N64 Rumble Pak detection should write the standard probe value",
    )
    require(
        rumble_probe,
        "memset(data, 0xFE, sizeof(data));",
        "N64 accessory probing should safely power off a possible Transfer Pak first",
    )
    require(
        joybus_pio,
        "static int tx_payload_dma",
        "Long Joybus accessory writes should use a DMA-fed PIO path",
    )
    require(
        joybus_pio,
        "channel_config_set_dreq(&config, pio_get_dreq(instance.pio, instance.sm, true));",
        "Joybus TX DMA should be paced by the active PIO state machine",
    )
    require(
        joybus_pio,
        "const int dma_result = tx_payload_dma(instance, payload, payload_len, response_len);",
        "Joybus transport should route long payloads through DMA",
    )
    reject(
        joybus_pio,
        "if (payload_len == 35)",
        "N64 accessory writes must stay on the controller's active Joybus PIO",
    )
    require(
        joybus_header,
        "static constexpr uint32_t N64_ACCESSORY_TIMEOUT_MS = 10;",
        "N64 accessory and rumble writes should use the scoped timeout",
    )
    require(
        joybus_pio,
        "uint32_t joybus_pio_get_timeout_ms()",
        "Joybus accessory timeout scope should restore the normal poll timeout",
    )
    for token in (
        "LATENCY_TRACE_PHASE_OUTPUT_BG",
        "LATENCY_TRACE_PHASE_PLATFORM_BG",
        "LATENCY_TRACE_PHASE_TURBO",
        "LATENCY_TRACE_PHASE_PREPOLL_UI",
        "LATENCY_TRACE_PHASE_PENDING_OUTPUT",
        "LATENCY_TRACE_PHASE_MODE_BUTTON",
        "LATENCY_TRACE_PHASE_RESET_BUTTON",
        "LATENCY_TRACE_PHASE_MENU_HANDLE",
        "LATENCY_TRACE_PHASE_USB_FEEDBACK",
        "LATENCY_TRACE_PHASE_USB_READY",
        "LATENCY_TRACE_PHASE_USB_BUILD",
        "LATENCY_TRACE_PHASE_USB_SUBMIT",
        "LATENCY_TRACE_PHASE_USB_NOT_READY",
    ):
        require(header, token, f"Latency trace should expose {token}")
    for token in (
        'return "OUTPUT_BG";',
        'return "PLATFORM_BG";',
        'return "TURBO";',
        'return "PREPOLL_UI";',
        'return "PENDING_OUTPUT";',
        'return "MODE_BUTTON";',
        'return "RESET_BUTTON";',
        'return "MENU_HANDLE";',
        'return "USB_FEEDBACK";',
        'return "USB_READY";',
        'return "USB_BUILD";',
        'return "USB_SUBMIT";',
        'return "USB_NOT_READY";',
    ):
        require(source, token, f"Latency trace should name {token}")
    for token in (
        "LatencyPhaseTraceScope outputBgTrace(LATENCY_TRACE_PHASE_OUTPUT_BG);",
        "LatencyPhaseTraceScope platformBgTrace(LATENCY_TRACE_PHASE_PLATFORM_BG);",
        "LatencyPhaseTraceScope turboTrace(LATENCY_TRACE_PHASE_TURBO);",
        "LatencyPhaseTraceScope prePollUiTrace(LATENCY_TRACE_PHASE_PREPOLL_UI);",
        "LatencyPhaseTraceScope pendingOutputTrace(LATENCY_TRACE_PHASE_PENDING_OUTPUT);",
    ):
        require(runtime, token, f"Runtime loop should trace {token}")
    for token in (
        "LatencyPhaseTraceScope feedbackTrace(LATENCY_TRACE_PHASE_USB_FEEDBACK);",
        "LatencyPhaseTraceScope readyTrace(LATENCY_TRACE_PHASE_USB_READY);",
        "LatencyPhaseTraceScope buildTrace(LATENCY_TRACE_PHASE_USB_BUILD);",
        "LatencyPhaseTraceScope submitTrace(LATENCY_TRACE_PHASE_USB_SUBMIT);",
        "LatencyPhaseTraceScope notReadyTrace(LATENCY_TRACE_PHASE_USB_NOT_READY);",
    ):
        require(usb_send, token, f"USB output send should trace {token}")
    platform_ui = read_text("firmware/platform/runtime/platform_runtime_ui.cpp")
    require(
        platform_ui,
        "constexpr uint32_t kResetButtonIdlePollIntervalUs = 16000;",
        "BOOTSEL reset polling should be throttled off the input critical path",
    )
    require(
        platform_ui,
        "if (resetButton.isPressed() ||\n      lastResetButtonIdlePollUs == 0 ||",
        "Pressed BOOTSEL reset should still be polled every loop",
    )
    require(
        platform_ui,
        "LatencyPhaseTraceScope modeButtonTrace(LATENCY_TRACE_PHASE_MODE_BUTTON);",
        "Mode button polling should be traced",
    )
    require(
        platform_ui,
        "LatencyPhaseTraceScope resetButtonTrace(LATENCY_TRACE_PHASE_RESET_BUTTON);",
        "Reset button polling should be traced",
    )
    require(
        platform_ui,
        "LatencyPhaseTraceScope menuHandleTrace(LATENCY_TRACE_PHASE_MENU_HANDLE);",
        "Menu handling should be traced",
    )
    transforms = read_text("firmware/core/controller_runtime_transforms.cpp")
    require(
        transforms,
        "frameHasAnalogStickStateForCentering)(const controller_state_t& frame)",
        "Digital-only controllers should skip analog stick centering work",
    )
    require(
        transforms,
        "if (!frame.connected || !frameHasAnalogStickStateForCentering(frame))",
        "NES/SNES digital-only frames should stay off the analog centering path",
    )
    require(
        transforms,
        "stickCenter.update(i, false, 0, 0, 0, 0);",
        "Skipping centering should clear stale analog centering state",
    )


def test_dreamcast_uses_classic_analog_learn() -> None:
    source = read_text("firmware/input/dreamcast/input_dreamcast_poll.cpp")
    require(source, '#include "../../core/classic_analog_range.h"', "Dreamcast analog range include")
    require(source, "applyClassicAnalogLearnAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LX, raw_lx)", "Dreamcast LX learn")
    require(source, "recordClassicAnalogRangeAxis(analogMode, i, CLASSIC_ANALOG_AXIS_LX", "Dreamcast range telemetry")


def test_buzzer_idle_forces_gpio_low() -> None:
    source = read_text("firmware/platform/buzzer.cpp")
    require(source, "pwm_set_chan_level(slice_num, pwm_gpio_to_channel(pin), 0);", "Buzzer stops PWM output")
    require(source, "gpio_set_function(pin, GPIO_FUNC_SIO);", "Buzzer releases PWM function when idle")
    require(source, "gpio_set_function(pin, GPIO_FUNC_PWM);", "Buzzer restores PWM function before tones")


def test_release_browser_firmware_update_flow_is_packaged() -> None:
    runtime_serial = read_text("firmware/core/runtime/runtime_serial_debug.cpp")
    adapt_html = read_text("web/Adapt.html")
    release_targets = read_text("tools/release_targets.json")

    require(runtime_serial, '#include "../../firmware_build_info.h"', "Serial INFO should use firmware build metadata")
    for token in (
        "INFO PRODUCT=Classic2USB",
        " VERSION=",
        " TAG=",
        " HARDWARE=",
        "FIRMWARE_VERSION_STRING",
        "FIRMWARE_VERSION_TAG",
        "FIRMWARE_HARDWARE_STRING",
    ):
        require(runtime_serial, token, f"Serial INFO metadata missing {token}")
    for token in (
        "REFLEX_RELEASE_REPO = 'misteraddons/Reflex-Adapt'",
        "GITHUB_RELEASES_API",
        "firmware-update-status",
        "firmware-check-update-btn",
        "firmware-update-btn",
        "checkFirmwareUpdate",
        "runFirmwareUpdate",
        "WEBHID_CMD_BOOTLOADER",
        "showDirectoryPicker",
    ):
        require(adapt_html, token, f"Adapt.html update checker missing {token}")
    reject(
        release_targets,
        "reflex_adapt_update.sh",
        "Release package must exclude the unvalidated MiSTer update script",
    )


def test_generic_dinput_serial_uses_input_mode_identity() -> None:
    hid_setup = read_text("firmware/output/usb/output_usb_mode_setup_hid_runtime.h")
    boot = read_text("firmware/output/runtime/output_boot_runtime.cpp")
    identity = read_text("firmware/output/output_identity.cpp")
    input_boot = read_text("firmware/input/runtime/input_boot_runtime.cpp")
    match = re.search(
        r"static void configure_generic_mister_hid_output_runtime\(\) \{(?P<body>.*?)\n\}",
        hid_setup,
        re.S,
    )
    if not match:
        raise AssertionError("Generic DInput setup: function body not found")
    body = match.group("body")
    require(
        body,
        "TinyUSBDevice.setProductDescriptor(PRODUCT_NAME);",
        "Generic DInput product descriptor should stay user-friendly",
    )
    require(
        body,
        "TinyUSBDevice.setSerialDescriptor(get_reflex_input_usb_serial_descriptor());",
        "Generic DInput serial descriptor should identify the input mode for MiSTer maps",
    )
    require(
        boot,
        "TinyUSBDevice.setSerialDescriptor(get_reflex_input_usb_serial_descriptor());",
        "Boot finalization should preserve the same input-mode serial descriptor",
    )
    reject(
        boot,
        "serial_descriptor = PRODUCT_NAME;",
        "Boot finalization must not collapse Generic DInput serial to the product name",
    )
    require(
        identity,
        "const char* get_reflex_input_usb_serial_descriptor()",
        "Input-mode USB serial helper should be defined",
    )
    require(
        identity,
        "return get_reflex_input_product_name();",
        "Input-mode USB serial should use exact mode names such as ReflexNES/SNES/Vboy",
    )
    reject(
        identity,
        "activeInputAdapterUsbIdOr",
        "Input-mode USB serial must not use shared module IDs for NES/SNES/VB",
    )
    require(
        input_boot,
        "resolvedMisterIdentityBoot",
        "Resolved MiSTer DInput should identify Joybus input before USB connects",
    )
    require(
        input_boot,
        "output_is_generic_mister_hid_mode(get_effective_output_mode())",
        "Pre-USB identity resolution should be limited to generic MiSTer HID",
    )
    require(
        input_boot,
        "runAutoDetectionJoybusOnly(true)",
        "Pre-USB MiSTer identity resolution should use the narrow Joybus probe",
    )


def test_windows_dinput_keeps_descriptor_hid_order() -> None:
    port_order = read_text("firmware/output/usb/output_usb_hid_port_order_runtime.h")
    require(
        port_order,
        "return false;",
        "Windows DInput HID order should not reverse current descriptor/source-port order",
    )
    reject(
        port_order,
        "outputPlayers - 1 - sourcePort",
        "Windows DInput source port routing must not reverse physical player order",
    )
    reject(
        port_order,
        "outputPlayers - 1 - hidInterface",
        "Windows DInput OUT report routing must not reverse physical player order",
    )


def test_special_psx_outputs_have_distinct_gamecontrollerdb_identities() -> None:
    capabilities = read_text("firmware/output/output_capabilities.h")
    menu_capabilities = read_text("firmware/menu/menu_capabilities.cpp")
    menu_modes = read_text("firmware/menu/menu_mode_state.cpp")
    menu_catalog = read_text("firmware/menu/menu_catalog.cpp")
    menu_descriptors = read_text("firmware/menu/menu_descriptors_data.cpp")
    settings_store = read_text("firmware/core/settings_store.h")
    auth_msc = read_text("firmware/output/auth/auth_msc_runtime.cpp")
    usb_configure = read_text("firmware/output/usb/output_usb_configure_runtime.h")
    hid_setup = read_text("firmware/output/usb/output_usb_mode_setup_hid_runtime.h")
    hid_mapping = read_text("firmware/output/usb/output_hid_mapping_runtime.h")
    negcon_descriptor = read_text("firmware/output/specialized/output_descriptors_negcon_runtime.h")
    for name, version in (
        ("JogCon", "0x0101"),
        ("NegCon", "0x0102"),
        ("GunCon", "0x0103"),
    ):
        require(
            hid_setup,
            f"kMister{name}DeviceVersion = {version}",
            f"MiSTer {name} should have a distinct GameControllerDB identity",
        )
        require(
            hid_setup,
            f"TinyUSBDevice.setDeviceVersion(kMister{name}DeviceVersion);",
            f"MiSTer {name} USB setup should apply its distinct identity",
        )
    output_menu_order = menu_modes.split(
        "const outputMode_t kOutputModeMenuOrder[] = {", 1
    )[1].split("};", 1)[0]
    for mode in (
        "OUTPUT_MISTER_JOGCON",
        "OUTPUT_MISTER_NEGCON",
        "OUTPUT_MISTER_GUNCON",
    ):
        reject(
            output_menu_order,
            mode,
            f"{mode} must be selected automatically, not offered in the Output menu",
        )
        require(
            menu_capabilities,
            f"case {mode}:",
            f"{mode} must stay hidden from manual output selection",
        )
    reject(
        menu_catalog,
        "menu_item_psx_periph",
        "The single-value PSX Peripheral menu item should be removed",
    )
    reject(
        menu_descriptors,
        "psx_periph_names",
        "The obsolete PSX Peripheral descriptor should be removed",
    )
    reject(
        settings_store,
        "reserved_psx_periph",
        "Pre-production firmware should not retain the obsolete PSX EEPROM field",
    )
    reject(
        settings_store,
        "ReservedPsxPeriph",
        "Pre-production firmware should not retain the obsolete PSX SettingId",
    )
    require(
        settings_store,
        "constexpr uint8_t SETTINGS_SCHEMA_VERSION = 2;",
        "Removing the persisted field must invalidate the old pre-production layout",
    )
    reject(
        settings_store,
        "  PsxPeriph,",
        "The reserved setting slot should no longer imply a selectable PSX peripheral",
    )
    require(
        hid_mapping,
        "_jogcon.buttons |= (uint16_t)(INPUT_HOME | INPUT_CAPTURE);",
        "MiSTer JogCon Home should expose distinct guide and guide2 buttons",
    )
    require(hid_mapping, "_negcon.ps_btn = frame.HOME;", "MiSTer neGcon should expose guide")
    require(hid_mapping, "_negcon.guide2_btn = frame.HOME;", "MiSTer neGcon should expose guide2")
    require(negcon_descriptor, "uint8_t guide2_btn : 1;", "MiSTer neGcon descriptor should expose guide2")
    require(
        capabilities,
        "return output_allows_management_usb_endpoints(effectiveMode) ||\n"
        "         output_is_specialized_mister_psx_mode(effectiveMode);",
        "Specialized MiSTer PSX modes should expose management CDC",
    )
    require(
        usb_configure,
        "runtimeDebugCdcEnabled = managementCdcEnabled;",
        "Control CDC should use its dedicated capability instead of the full management bundle",
    )
    require(
        hid_setup,
        "static uint8_t management_configuration_buffer[512]",
        "Specialized PSX CDC composites should have enough descriptor space",
    )
    require(
        capabilities,
        "return managementDInputMode &&\n"
        "         !output_is_console_clean_usb_mode(effectiveMode) &&\n"
        "         output_is_generic_mister_hid_mode(effectiveMode);",
        "Generic DInput/WebHID management reports must remain DInput-only",
    )
    require(
        capabilities,
        "inline bool output_allows_management_msc(outputMode_t effectiveMode)",
        "MSC should have a capability separate from generic DInput/WebHID endpoints",
    )
    msc_capability = capabilities.split(
        "inline bool output_allows_management_msc", 1
    )[1].split("inline outputMode_t output_effective_mode", 1)[0]
    reject(msc_capability, "autoProbeActive", "MSC must remain enabled during Auto DInput probing")
    reject(msc_capability, "output_allows_management_usb_endpoints", "MSC must not inherit the Auto management-endpoint gate")
    require(
        capabilities,
        "const bool genericDInputMode =\n"
        "    output_is_generic_mister_hid_mode(effectiveMode) &&\n"
        "    !output_is_console_clean_usb_mode(effectiveMode);\n"
        "  return genericDInputMode ||\n"
        "         output_is_specialized_mister_psx_mode(effectiveMode);",
        "Every Windows/MiSTer DInput identity and specialized PSX mode should expose MSC",
    )
    require(
        auth_msc,
        "return output_allows_management_msc(effectiveOutputMode);",
        "MSC setup should use the dedicated specialized-mode capability",
    )



def test_jogcon_quick_menu_controls_persisted_mode() -> None:
    quick_header = read_text("firmware/menu/quick_config.h")
    quick_visibility = read_text("firmware/menu/quick_config_visibility.cpp")
    quick_state = read_text("firmware/menu/quick_config_state.cpp")
    quick_actions = read_text("firmware/menu/quick_config_actions.cpp")
    descriptors = read_text("firmware/menu/menu_descriptors_data.cpp")
    jogcon = read_text("firmware/input/psx/input_psx_jogcon.cpp")
    jogcon_state = read_text("firmware/input/psx/input_psx_jogcon_runtime_state.h")
    output_mapping = read_text("firmware/output/usb/output_hid_mapping_runtime.h")
    settings_registry = read_text("firmware/core/settings_registry.h")

    for source, token, label in (
        (quick_header, "QCI_JOGCON_MODE", "JogCon quick-menu item"),
        (quick_header, "QCI_JOGCON_FORCE", "JogCon force quick-menu item"),
        (quick_header, "uint8_t temp_jogcon_mode", "JogCon temporary menu state"),
        (quick_header, "uint8_t temp_jogcon_force", "JogCon temporary force state"),
        (quick_visibility, "addVisibleItem(QCI_JOGCON_MODE);", "live JogCon visibility"),
        (quick_visibility, "addVisibleItem(QCI_JOGCON_FORCE);", "Paddle/Wheel force visibility"),
        (quick_visibility, "case QCI_JOGCON_MODE: add(SettingId::JogconMode);", "JogCon setting ownership"),
        (quick_visibility, "case QCI_JOGCON_FORCE: add(SettingId::JogconForce);", "JogCon force setting ownership"),
        (quick_visibility, 'case QCI_JOGCON_MODE: return "JogCon Mode";', "JogCon quick-menu label"),
        (quick_visibility, '{ "Spinner", "Paddle", "Wheel", "Fake Spin" }', "JogCon quick-menu values"),
        (quick_visibility, "if (jogconMode == 0 || jogconMode == 3)", "Spinner/Fake Spinner sensitivity visibility"),
        (quick_visibility, "addVisibleItem(QCI_SPINNER_SPEED);", "JogCon Spin Sens setting"),
        (quick_visibility, 'return "Spin Sens";', "JogCon sensitivity label"),
        (quick_state, "temp_jogcon_mode = settings.jogcon_mode;", "persisted JogCon mode load"),
        (quick_state, "temp_jogcon_force = settings.jogcon_force;", "persisted JogCon force load"),
        (quick_state, "menu_jogcon_mode = temp_jogcon_mode;", "live JogCon mode apply"),
        (quick_state, "menu_jogcon_force = temp_jogcon_force;", "live JogCon force apply"),
        (quick_state, "settings.jogcon_mode = temp_jogcon_mode;", "persisted JogCon mode save"),
        (quick_state, "settings.jogcon_force = temp_jogcon_force;", "persisted JogCon force save"),
        (quick_actions, "temp_jogcon_mode = (temp_jogcon_mode + 1) % 4;", "forward JogCon mode cycling"),
        (quick_actions, "temp_jogcon_mode = (temp_jogcon_mode == 0) ? 3 : temp_jogcon_mode - 1;", "reverse JogCon mode cycling"),
        (quick_actions, "rebuildVisibleAndClampSelection();", "JogCon mode-dependent quick-menu rebuild"),
        (descriptors, '{ "Spinner", "Paddle", "Wheel", "Fake Spin" }', "full-menu JogCon mode values"),
        (descriptors, '{ "Left/Right", "L3/R3", "L/R", "Up/Down" }', "Fake Spinner default digital target"),
        (jogcon, "configuredJogconMode(enableMouseMove)", "menu-driven JogCon runtime mode"),
        (jogcon, "init_jogcon();\n  }\n  force = configuredJogconForce();", "menu mode change physical zero reset"),
        (jogcon, "menu_jogcon_force = force;", "legacy force shortcut live-menu synchronization"),
        (jogcon, "if(signedPosition < -sp_half || signedPosition > sp_half)\n        motorDirection = JOGCON_DIR_START;", "Paddle endpoint brake"),
        (jogcon, "motorDirection = JOGCON_DIR_CW;", "active Wheel clockwise return force"),
        (jogcon, "motorDirection = JOGCON_DIR_CCW;", "active Wheel counterclockwise return force"),
        (jogcon, "constexpr int16_t kWheelCenterDeadzone = 4;", "Wheel center hold zone"),
        (jogcon, "const int32_t analogMin = -128L * sp_step;", "Spinner analog sensitivity scaling"),
        (jogcon, "clampJogconAxis((int16_t)(jogcon_spinnerAnalog / sp_step));", "high-resolution Spinner analog fallback"),
        (jogcon_state, "extern int32_t jogcon_spinnerAnalog;", "dedicated Spinner analog accumulator"),
        (jogcon, "mode = configuredJogconMode(enableMouseMove);", "JogCon setup mode load"),
        (jogcon, "buttonPressed(PSB_L2) && psx[0]->buttonPressed(PSB_R2)", "legacy Spinner shortcut"),
        (jogcon, "buttonPressed(PSB_L1) && psx[0]->buttonPressed(PSB_R1)", "legacy Fake Spinner shortcut"),
        (jogcon, "menu_jogcon_mode = mode;", "legacy shortcut live-menu synchronization"),
        (jogcon, "static const uint8_t steps[] = { 16, 8, 4, 2, 1 };", "JogCon 0.25x-through-4x sensitivity"),
        (jogcon, "sp_step = configuredJogconSpinnerStep();", "live JogCon sensitivity application"),
        (jogcon, "case 0: frame.PAD_L |= spinCCW; frame.PAD_R |= spinCW; break;", "Fake Spinner D-pad default"),
        (jogcon, "if (mode == 0)\n          frame.spinner = spinner;\n        else\n          frame.paddle = paddle;", "mode-exclusive neutral axes"),
        (output_mapping, 'std::strcmp(frame.controller_type_name, "JogCon-S") == 0', "Spinner report gating"),
        (output_mapping, 'std::strcmp(frame.controller_type_name, "JogCon-P") == 0', "Paddle report gating"),
        (output_mapping, 'std::strcmp(frame.controller_type_name, "JogCon-W") == 0', "Wheel report gating"),
        (output_mapping, "_jogcon.spinner_axis = spinnerMode ? frame.spinner : 0;", "relative Spinner-only report"),
        (output_mapping, ": analog_mid;", "neutral paddle axis outside Paddle/Wheel"),
        (settings_registry, "offsetof(PerModeSettingsRecord, jogcon_force),        15,", "strong JogCon force default"),
    ):
        require(source, token, label)

    reject(
        jogcon,
        "setJogconMotorMode(JOGCON_DIR_START, nextCmd, force);",
        "JogCon motor control must remain mode-selective",
    )
    reject(
        output_mapping,
        "_jogcon.spinner_axis = frame.spinner;",
        "MiSTer JogCon must not publish relative spinner data in every mode",
    )
    reject(
        output_mapping,
        "_jogcon.paddle_axis = convertAnalogPrecision(frame.LX, input_precision, output_precision) + analog_mid;",
        "MiSTer JogCon must not publish absolute paddle data in every mode",
    )


def test_wii_i2c_probes_both_pin_pairs_and_locks_runtime_pair() -> None:
    wii_header = read_text("firmware/input/wii/Input_Wii.h")
    autodetect_boot = read_text("firmware/input/autodetect/input_autodetect_boot_runtime.cpp")
    autodetect_runtime = read_text("firmware/input/autodetect/input_autodetect_runtime.cpp")
    home_status = read_text("firmware/menu/menu_main_display_status.cpp")
    wii_poll = read_text("firmware/input/wii/input_wii_poll.cpp")
    wii_header = read_text("firmware/input/wii/Input_Wii.h")
    autodetect_boot = read_text("firmware/input/autodetect/input_autodetect_boot_runtime.cpp")
    autodetect_runtime = read_text("firmware/input/autodetect/input_autodetect_runtime.cpp")
    home_status = read_text("firmware/menu/menu_main_display_status.cpp")
    autodetect_header = read_text("firmware/input/autodetect/Input_AutoDetect.h")
    autodetect_support = read_text("firmware/input/autodetect/input_autodetect_support.cpp")
    autodetect_probe = read_text("firmware/input/autodetect/input_autodetect_modern_probes.cpp")

    require(wii_header, "pinPairs[INPUT_WII_PIN_PAIR_COUNT]", "Wii runtime should carry multiple I2C pin pairs")
    require(wii_header, "activePinPair[input_ports]", "Wii runtime should lock the detected I2C pin pair")
    require(wii_header, "WII_CONNECTED_POLL_INTERVAL_US = 500", "Wii connected polling should run every 500 us")
    require(wii_header, "WII_EMPTY_PORT_CONNECT_INTERVAL_US = 500000", "Empty Wii ports should retry every 500 ms")
    require(wii_header, "WII_UPDATE_FAIL_DISCONNECT_THRESHOLD = 200", "Wii disconnect should tolerate brief I2C error bursts")
    require(wii_header, "nextConnectAttemptUs[input_ports]", "Wii runtime should track per-port reconnect timing")
    for token in (
        "{ .sda = 2, .scl = 7 }",
        "{ .sda = 10, .scl = 11 }",
        "{ .sda = 14, .scl = 15 }",
        "{ .sda = 22, .scl = 23 }",
    ):
        require(wii_header, token, f"Wii runtime missing pin pair {token}")
    require(wii_poll, "for (uint8_t pair = 0; pair < INPUT_WII_PIN_PAIR_COUNT; ++pair)", "Wii connect should probe all pin pairs")
    require(wii_poll, "activePinPair[i] = pair;", "Wii connect should latch the working pin pair")
    require(wii_poll, "activePinPair[i] = INPUT_WII_PIN_PAIR_INVALID;", "Wii disconnect should force a future re-probe")
    require(
        wii_poll,
        "nextConnectAttemptUs[i] != 0 &&\n          (int32_t)(nowUs - nextConnectAttemptUs[i]) < 0",
        "Wii reconnect timing should treat zero as immediate across time_us_32 rollover",
    )
    require(wii_poll, "nextConnectAttemptUs[i] = nowUs + WII_EMPTY_PORT_CONNECT_INTERVAL_US;", "Wii reconnect attempts should be throttled")
    require(autodetect_header, "uint8_t wii_alt_sda;", "AutoDetect should expose alternate Wii SDA")
    require(autodetect_header, "uint8_t wii_alt_scl;", "AutoDetect should expose alternate Wii SCL")
    require(autodetect_support, ".wii_sda = HDMI_1_10, .wii_scl = HDMI_1_11", "AutoDetect P1 should prefer direct 3.3 V Wii I2C")
    require(autodetect_support, ".wii_sda = HDMI_2_10, .wii_scl = HDMI_2_11", "AutoDetect P2 should prefer direct 3.3 V Wii I2C")
    require(autodetect_support, ".wii_alt_sda = HDMI_1_02, .wii_alt_scl = HDMI_1_01", "AutoDetect P1 should retain the level-shifted HDMI pins 1/2 route")
    require(autodetect_support, ".wii_alt_sda = HDMI_2_02, .wii_alt_scl = HDMI_2_01", "AutoDetect P2 should retain the level-shifted HDMI pins 1/2 route")
    require(autodetect_probe, "wiiPinPairs[]", "AutoDetect Wii probe should try both pin pairs")
    require(autodetect_probe, "for (const auto& pair : wiiPinPairs)", "AutoDetect Wii probe should loop through pin pairs")


def test_dinput_and_xinput2p_descriptors_are_stable() -> None:
    hid_setup = read_text("firmware/output/usb/output_usb_mode_setup_hid_runtime.h")
    boot_output = read_text("firmware/output/runtime/output_boot_runtime.cpp")
    usb_configure = read_text("firmware/output/usb/output_usb_configure_runtime.h")
    usb_begin = read_text("firmware/output/usb/output_usb_begin_runtime.h")
    usb_control = read_text("firmware/output/usb/output_usb_control_runtime.h")
    mode_setup = read_text("firmware/output/usb/output_usb_mode_setup_console_runtime.h")
    slots = read_text("firmware/output/usb/output_usb_xinput2p_slots_runtime.h")
    wireless_runtime = read_text("firmware/output/xinputw/output_xinputw_runtime.cpp")
    output_runtime = read_text("firmware/output/output_runtime_state.cpp")
    settings_registry = read_text("firmware/core/settings_registry.h")
    platformio = read_text("platformio.ini")
    catalog = read_text("firmware/output/output_mode_catalog.cpp")
    sender = read_text("firmware/output/usb/output_usb_send_runtime.h")
    routing = read_text("firmware/output/xinputw/output_xinputw_slot_routing.cpp")
    driver = read_text("firmware/output/xinputw/output_xinputw_driver_runtime.cpp")
    require(
        hid_setup,
        "management_configuration_buffer[512]",
        "Every management composite should use a 512-byte TinyUSB configuration buffer",
    )
    reject(usb_configure, "tud_deinit(0);",
           "Resolved XInput modes must not tear down the initialized TinyUSB stack")
    prime_start = usb_configure.find("begin_output_usb_interfaces_runtime();")
    prime_end = usb_configure.find(
        "#if defined(ARDUINO_ARCH_RP2040)", prime_start
    )
    prime_body = usb_configure[prime_start:prime_end]
    require(
        prime_body,
        "if (effectiveOutputMode == OUTPUT_XINPUTW && _xinputw)",
        "Only Windows XInputW should prime receiver child presence",
    )
    require(
        prime_body,
        "service_xinputw_slot_routing();",
        "Windows XInputW should compute physical-to-child routes before enumeration",
    )
    require(
        prime_body,
        "_xinputw->set_connected(target, targetConnected);",
        "Windows XInputW should prime receiver child presence before host queries",
    )
    source_count_start = slots.find("inline uint8_t xinputw_physical_source_count()")
    source_count_end = slots.find(
        "inline void service_xinputw_slot_routing()", source_count_start
    )
    source_count_body = slots[source_count_start:source_count_end]
    require(
        source_count_body,
        "min((uint8_t)XINPUT_MULTI_CONTROLLERS, (uint8_t)MAX_USB_OUT)",
        "XInputW routing should scan both fixed Classic2USB physical ports",
    )
    reject(
        source_count_body,
        "output_usb_player_count()",
        "Physical XInputW sources must not be truncated by output count",
    )
    routing_start = slots.find("inline void service_xinputw_slot_routing()")
    routing_end = slots.find(
        "inline uint8_t xinput2p_fixed_usb_controller_count()", routing_start
    )
    routing_body = slots[routing_start:routing_end]
    require(
        routing_body,
        "get_effective_output_mode() != OUTPUT_XINPUTW",
        "Receiver routing must be exclusive to Windows XInputW",
    )
    xinput2p_setup = mode_setup.split(
        "static void configure_xinput_2p_output_runtime()", 1
    )[1].split("static void configure_xid_output_runtime()", 1)[0]
    require(
        xinput2p_setup,
        "_xinput2p = new Adafruit_USBD_XInputMulti(1, controllerCount);",
        "MiSTer/Linux XInput2P should use two wired XUSB interfaces",
    )
    reject(
        xinput2p_setup,
        "configure_xinput_wireless_output_runtime();",
        "MiSTer/Linux XInput2P must not use the wireless receiver transport",
    )
    wireless_setup = mode_setup.split(
        "static void configure_xinput_wireless_output_runtime()", 1
    )[1].split("static uint8_t runtime_xinput_controller_count()", 1)[0]
    require(
        wireless_setup,
        "_xinputw = new Adafruit_USBD_XInputW();",
        "Windows XInput should use the wireless receiver transport",
    )
    require(
        wireless_setup,
        "TinyUSBDevice.setID(0x045E, 0x0719);",
        "Windows XInputW should advertise the Xbox 360 wireless receiver ID",
    )
    require(
        usb_begin,
        "} else if (effectiveOutputMode == OUTPUT_XINPUTW) {\n    _xinputw->begin();",
        "Windows XInputW should begin the wireless driver",
    )
    reject(
        usb_begin.split(
            "} else if (effectiveOutputMode == OUTPUT_XINPUT2P)", 1
        )[1].split(
            "} else if (effectiveOutputMode == OUTPUT_XINPUTW)", 1
        )[0],
        "_xinputw->begin();",
        "MiSTer/Linux XInput2P must not begin the wireless driver",
    )
    require(
        usb_control,
        "return usbd_drivers_xinput_multi;",
        "MiSTer/Linux XInput2P should register the wired multi-controller driver",
    )
    require(
        usb_control,
        "if (effectiveOutputMode == OUTPUT_XINPUTW) {\n"
        "    *driver_count = (sizeof(usbd_drivers_wireless)",
        "Windows XInputW should register the wireless receiver driver",
    )
    canonical = output_runtime.split(
        "outputMode_t canonicalizeOutputMode(outputMode_t mode)", 1
    )[1].split("outputMode_t sanitizeConfiguredOutputMode", 1)[0]
    reject(
        canonical,
        "OUTPUT_XINPUTW",
        "Windows XInputW must remain distinct from MiSTer/Linux XInput2P",
    )
    require(
        settings_registry,
        "if (value == OUTPUT_XINPUTW) {\n"
        "    #ifdef ENABLE_EXPERIMENTAL_XINPUT2P_OUTPUT\n"
        "    return OUTPUT_XINPUTW;",
        "Saved Windows XInputW must not be rewritten to XInput2P",
    )
    classic_env = platformio.split("[env:classic2usb]", 1)[1].split("\n[env:", 1)[0]
    reject(
        classic_env,
        "-DENABLE_XINPUT2P_WIRELESS_TRANSPORT",
        "Classic2USB must not globally replace XInput2P with XInputW",
    )
    require(
        catalog,
        "case OUTPUT_XINPUT2P:         return XINPUT_MULTI_CONTROLLERS;",
        "MiSTer/Linux XInput should remain capped to two wired interfaces",
    )
    require(
        catalog,
        "case OUTPUT_XINPUTW:          return XINPUT_WIRELESS_CONTROLLERS;",
        "Windows XInputW should retain four receiver children",
    )
    reserve_start = boot_output.find("bool shouldReserveClassic2usbUsbSlots")
    reserve_end = boot_output.find("bool classic2usbInputCanReserveDetectedUsbSlots", reserve_start)
    reserve_body = boot_output[reserve_start:reserve_end]
    for mode in ("OUTPUT_XINPUT2P", "OUTPUT_XINPUTW"):
        require(
            reserve_body,
            f"case {mode}:",
            f"Classic2USB {mode} should reserve both physical controller slots",
        )
    require(
        sender,
        "const uint8_t sourceCount = xinputw_physical_source_count();",
        "Windows receiver presence should validate physical source slots",
    )
    require(
        sender,
        "for (uint8_t target = 0; target < XINPUT_WIRELESS_CONTROLLERS; ++target)",
        "Windows receiver presence should service all four fixed children",
    )
    require(
        sender,
        "source = xinputw_source_port_for_target(target)",
        "Windows receiver presence should disconnect unused child slots",
    )
    require(
        routing,
        "const uint8_t target = compactConnectedSources ? nextTarget++ : source;",
        "A lone physical P2 should compact into the first active Windows child",
    )
    require(
        sender,
        "targetSlot = xinputw_target_slot_for_source(port)",
        "Windows XInputW reports should follow compacted physical-to-child routes",
    )
    require(
        sender,
        "submitted = _xinputw->sendReport(",
        "Windows XInputW reports should track compact-child submission",
    )
    require(
        driver,
        "sourcePort = xinputw_source_port_for_target(i)",
        "Windows XInputW rumble should follow child-to-physical routes",
    )
    require(
        driver,
        "rumble_callback(sourcePort, rumble.left, rumble.right)",
        "Windows XInputW rumble should reach the original physical port",
    )
    require(
        wireless_runtime,
        "const bool connected = _xinput_dev->interfaces[itf].connected &&",
        "Receiver status queries should start from live physical connection state",
    )
    require(
        wireless_runtime,
        "connectedChildMayStart(itf)",
        "Receiver status queries should preserve ordered compact child assignment",
    )
    reject(
        wireless_runtime,
        "send_connection_status(itf, _xinput_dev->interfaces[itf].info_state != DISCONNECTED);",
        "Receiver status queries must not replay stale protocol state",
    )
    reject(
        sender,
        "slotConnected = true",
        "Classic2USB must not advertise disconnected Windows XInputW children",
    )
    require(
        hid_setup,
        "TinyUSBDevice.setConfigurationBuffer(",
        "Every management composite should install its expanded configuration buffer",
    )

def test_gcwiiu_is_a_host_agnostic_manual_four_player_output() -> None:
    menu_modes = read_text("firmware/menu/menu_mode_state.cpp")
    menu_capabilities = read_text("firmware/menu/menu_capabilities.cpp")
    mode_catalog = read_text("firmware/output/output_mode_catalog.cpp")
    usb_configure = read_text("firmware/output/usb/output_usb_configure_runtime.h")
    web_ui = read_text("web/Adapt.html")

    menu_order = menu_modes.split(
        "const outputMode_t kOutputModeMenuOrder[] = {", 1
    )[1].split("};", 1)[0]
    hidden_switch = menu_capabilities.split(
        "bool shouldHideMenuOutputMode(outputMode_t mode)", 1
    )[1].split("if (!authOutputModeCanRun(mode))", 1)[0]
    hidden_web = web_ui.split(
        "const HIDDEN_OUTPUT_MODES = [", 1
    )[1].split("];", 1)[0]

    require(
        menu_order,
        "OUTPUT_GCWIIU",
        "GC2WiiU should be offered in the manual output menu",
    )
    reject(
        hidden_switch,
        "case OUTPUT_GCWIIU:",
        "GC2WiiU manual selection must not depend on detected host type",
    )
    reject(
        hidden_web,
        "15, // Wii U GC Adapter",
        "Adapt Manager should expose GC2WiiU manual output",
    )
    require(
        mode_catalog,
        "case OUTPUT_GCWIIU:           return 4;",
        "GC2WiiU should preserve all four dynamic adapter ports",
    )
    require(
        usb_configure,
        "effectiveOutputMode == OUTPUT_GCWIIU",
        "GC2WiiU should configure from an explicitly selected output mode",
    )


def test_output_settings_use_transport_names_and_home_uses_detected_host() -> None:
    catalog = read_text("firmware/output/output_mode_catalog.cpp")
    labels = read_text("firmware/menu/menu_mode_labels.cpp")
    home = read_text("firmware/menu/menu_home_mode_line.cpp")
    require(catalog, 'case OUTPUT_MISTER:           return "DInput";', "Output settings should name the DInput transport")
    require(catalog, 'case OUTPUT_XINPUT2P:         return "XInput";', "Output settings should name the XInput transport")
    require(catalog, 'case OUTPUT_XINPUT:           return "Xbox 360";', "Output settings should preserve the Xbox 360 protocol name")
    require(catalog, 'case OUTPUT_AUTO:             return "Auto";', "Output settings should use concise Auto language")
    require(labels, 'case OUTPUT_AUTO: return "Auto";', "Quick settings should use concise Auto language")
    require(home, '"MiSTer DIn"', "Home status should identify a detected MiSTer DInput host")
    require(home, '"Windows DIn"', "Home status should identify a detected Windows DInput host")


def test_oled_marquee_dwell_and_spinner_width() -> None:
    helpers = read_text("firmware/menu/menu_helpers_render.cpp")
    quick_config = read_text("firmware/menu/quick_config_render.cpp")
    pad_render = read_text("firmware/menu/menu_main_display_pad_render.cpp")
    require(helpers, "kOledMarqueeInitialHoldSteps = 6", "Selected menu values should dwell for 1.5 seconds")
    require(helpers, "now - marquee_start_ms", "Menu marquee timing should be relative to initial display")
    require(quick_config, "quickConfigMarqueeStartMs = now;", "Quick menu redraw should restart marquee dwell")
    require(pad_render, "kSpinnerPaddlePanelWidth =", "Spinner and paddle displays should have a dedicated width")
    require(pad_render, "kDreamcastWheelPanelWidth - kPadGlyphWidth", "Spinner and paddle displays should be one character narrower")


def test_negcon_oled_uses_pressure_and_horizontal_twist_gauge() -> None:
    layouts = read_text("firmware/menu/menu_pad_layouts_data_arcade.cpp")
    player_layout = read_text("firmware/menu/menu_pad_layouts_player.cpp")
    pad_render = read_text("firmware/menu/menu_main_display_pad_render.cpp")
    status = read_text("firmware/menu/menu_main_display_status.cpp")

    match = re.search(
        r"const PadButton padLayoutPSXNeGcon\[\] = \{(.*?)\n\};",
        layouts,
        re.DOTALL,
    )
    if match is None:
        raise AssertionError("neGcon should have a dedicated OLED layout")
    negcon_layout = match.group(1)
    reject(negcon_layout, "GPAD_SELECT", "neGcon OLED layout must not show Select")
    require(negcon_layout, "{ GPAD_L1, 0, 2 * 6", "neGcon L should retain its standard position")
    require(negcon_layout, "{ GPAD_R1, 0, 7 * 6", "neGcon R should retain its standard position")
    require(
        negcon_layout,
        "{ GPAD_START, 3, (4 * 6) - 3, PAD_FACE_ON, PAD_FACE_OFF }",
        "neGcon Start should be a round button on the bottom row",
    )
    require(player_layout, "*layout = padLayoutPSXNeGcon;", "neGcon should select its dedicated OLED layout")
    require(pad_render, "(mask == GPAD_L2) ? frame->ANALOG_X : frame->ANALOG_A", "neGcon I and II meters should use raw pressure values")
    require(pad_render, "return rawPressure;", "neGcon pressure meters should preserve the requested raw OLED direction")
    reject(pad_render, "return (uint8_t)(255 - rawPressure);", "neGcon pressure meters must not invert the requested OLED direction")
    require(pad_render, "kNeGconAnalogGaugeGap = kPadGlyphWidth / 2", "neGcon pressure meters should leave a half-character gap before L and R")
    require(pad_render, "availableWidth - kNeGconAnalogGaugeGap", "neGcon pressure meter width should account for the L/R gap")
    require(pad_render, "kNeGconTwistGaugeRow = 7", "neGcon twist meter should use the bottom OLED row")
    require(pad_render, "kNeGconTwistGaugeWidth = 60", "neGcon twist meter should span the controller panel")
    require(pad_render, "void drawNeGconTwistGauge", "neGcon should have a horizontal twist gauge")
    require(pad_render, "drawCenterOutSignedAxisGauge(frame != nullptr ? frame->LX : 0", "neGcon twist gauge should use the centered twist axis")
    require(pad_render, "primaryNeGconTwistChanged", "neGcon twist gauge should redraw when twist changes")
    require(status, 'std::strcmp(port1.controller_type_name, "neGcon") == 0', "neGcon name should yield the footer to the twist gauge")
    require(status, 'std::strcmp(port2_name, "neGcon") == 0', "neGcon virtual output name should yield the footer to the twist gauge")

def test_negcon_user_facing_branding_matches_controller() -> None:
    psx_poll = read_text("firmware/input/psx/input_psx_poll.cpp")
    mode_labels = read_text("firmware/menu/menu_mode_labels.cpp")
    mode_catalog = read_text("firmware/output/output_mode_catalog.cpp")
    mapping_display = read_text("firmware/menu/mapping_display.cpp")
    analog_test = read_text("firmware/menu/menu_analog_test.cpp")

    require(psx_poll, 'setPsxControllerTypeName(frame, "neGcon");', "Detected controller name should use official neGcon styling")
    require(mode_labels, 'case OUTPUT_MISTER_NEGCON: return "MiSTer-nGc";', "Short MiSTer output name should avoid the ambiguous Neg abbreviation")
    require(mode_labels, 'case OUTPUT_MISTER_NEGCON: return "neGcon";', "Virtual controller name should use official neGcon styling")
    reject(mode_labels, '"MiSTer-Neg"', "Short MiSTer output name must not use the ambiguous Neg abbreviation")
    require(mode_catalog, 'case OUTPUT_MISTER_NEGCON:    return "MiSTer neGcon";', "Full output name should use official neGcon styling")
    require(mapping_display, 'case OUTPUT_MISTER_NEGCON: return "MnGc";', "Mapping display should use an unambiguous compact neGcon name")
    require(analog_test, 'title = "neGcon Test";', "Analog test title should use official neGcon styling")


def test_factory_reset_clears_all_user_configuration() -> None:
    settings = read_text("firmware/core/settings_store_system.cpp")
    handlers = read_text("firmware/menu/menu_item_handlers_system.cpp")
    screens = read_text("firmware/menu/menu_system_screens.cpp")
    require(settings, "void factoryResetSettings()", "Factory reset should not expose a preserve-remaps option")
    require(settings, "stageEepromRangeFill", "Factory reset should clear the settings storage region")
    require(settings, "setDefaultHotkeyBindings(hotkeys);", "Factory reset should restore default hotkeys")
    require(settings, "clearButtonChordRemaps();", "Factory reset should clear button chord remaps")
    reject(settings, "preservedRemaps", "Factory reset must not preserve per-mode remaps")
    require(handlers, "factoryResetSettings();", "Menu factory reset should clear all user settings")
    require(handlers, "factory_reset_selection == 1", "Factory reset dialog should use Reset and Cancel")
    require(handlers, "factory_reset_selection + 1) % 2", "Factory reset dialog should have two choices")
    require(screens, '"Reset all settings?"', "Factory reset prompt should describe its full scope")
    require(screens, 'const char* options[2] = { "RESET", "CANCEL" };', "Factory reset prompt should offer Reset and Cancel")
    reject(screens, "Remaps preserved.", "Factory reset result must not claim remaps were preserved")


def test_windows_msc_callbacks_defer_import_processing() -> None:
    msc = read_text("firmware/output/auth/auth_msc_runtime.cpp")
    output_loop = read_text("firmware/output/runtime/output_loop_runtime.cpp")
    flush_start = msc.find("void mscFlushCallback()")
    ready_start = msc.find("bool mscReadyCallback()", flush_start)
    flush_body = msc[flush_start:ready_start]
    reject(
        flush_body,
        "auth_msc_virtual_drive_process_import();",
        "MSC flush callback must not parse files or touch persistent storage inside TinyUSB",
    )
    require(msc, "g_auth_msc_import_pending = true;", "MSC flush should defer import processing")
    require(msc, 'extern "C" void auth_msc_task()', "MSC should expose deferred background work")
    require(output_loop, "auth_msc_task();", "Output background services should process deferred MSC imports")
    require(msc, "struct MscSectorCursor", "MSC should preserve logical-sector state across RP2040 endpoint chunks")
    require(msc, "g_auth_msc_read_cursor.offset", "MSC reads should advance through each logical sector")
    require(msc, "g_auth_msc_write_cursor.offset", "MSC writes should advance through each logical sector")
    require(msc, "const uint32_t copied = min(bufsize, remaining);", "MSC callbacks should consume partial endpoint chunks")


def test_classic2usb_msc_contains_lean_mister_bootstrap() -> None:
    assets = read_text("firmware/output/auth/auth_msc_assets.cpp")
    drive = read_text("firmware/output/auth/auth_msc_virtual_drive.cpp")
    status = read_text("firmware/output/auth/auth_msc_status_file.cpp")
    require(drive, "kDeviceShortName", "MSC should expose DEVICE.TXT")
    require(drive, "kDownloaderShortName", "MSC should expose the Adapt Downloader bootstrap")
    require(drive, "kPs4AuthHtmlShortName", "MSC should expose the PS4 auth page as PS4AUTH.HTM")
    require(assets, '"  PS4AUTH.HTM - PS4 key setup page', "README should identify the PS4 auth page")
    require(assets, '"  DEVICE.TXT  - device and PS4 key/import status', "README should describe DEVICE.TXT")
    require(assets, '"  ADAPTDL.INI - Reflex Adapt Manager Downloader entry', "README should describe the Downloader bootstrap")
    require(assets, '"[misteraddons/reflex-adapt-manager]\\r\\n"', "MSC should contain the unified manager database section")
    require(assets, 'reflex-adapt-manager.json.zip\\r\\n"', "MSC should point to the manager database")
    require(status, '"Firmware: %s\\r\\n"', "DEVICE.TXT should report firmware version")
    require(status, "Open PS4AUTH.HTM", "DEVICE.TXT should identify the PS4 auth page")
    reject(assets, "Accepted PS4 auth format:", "README should stay focused on drive contents and MiSTer setup")
    reject(assets, "ADAPT.HTM", "MSC should not use the generic Adapt page filename")
    reject(status, "Open ADAPT.HTM", "DEVICE.TXT should not reference the old auth page name")
    reject(status, '"Mode: DInput management drive', "DEVICE.TXT presence already implies DInput")
    reject(assets, "__REFLEX_MANAGER_PYTHON__", "MSC should not duplicate the full manager script")


def test_active_bus_empty_port_probes_use_500ms_cadence() -> None:
    gc64_setup = read_text("firmware/input/gc64/input_gc64_setup.cpp")
    psx_setup = read_text("firmware/input/psx/input_psx_setup.cpp")
    dreamcast_setup = read_text("firmware/input/dreamcast/input_dreamcast_setup.cpp")
    maple = read_text("third_party/firmware_libraries/MapleLib/MapleLib.cpp")

    require(gc64_setup, "polling_empty_interval_ms = 500;", "Joybus empty ports should use the 500 ms cadence")
    reject(gc64_setup, "polling_empty_interval_ms = 16;", "Joybus setup should not begin with a faster transient empty-port cadence")
    require(psx_setup, "polling_empty_interval_ms = 500;", "PSX empty ports should use the 500 ms cadence")
    require(dreamcast_setup, "pollInterval = 500;", "Dreamcast outer polling should be deterministic at 500 us")
    require(maple, "connected ? 500 : 500000", "Maple should poll connected ports at 500 us and empty ports at 500 ms")

def test_joybus_transient_disconnects_do_not_drop_n64_or_gamecube() -> None:
    header = read_text("firmware/input/gc64/Input_GC64.h")
    poll = read_text("firmware/input/gc64/input_gc64_poll.cpp")
    require(header, "slotLastSeenMs[input_ports]", "Joybus inputs should remember the last valid transaction")
    require(header, "SLOT_DISCONNECT_DEBOUNCE_MS = 120", "Joybus disconnects should use the standard short grace")
    require(poll, "if (transient_disconnect)", "Joybus should retain a live frame through transient read failures")
    require(poll, "joybus[port]->stateChanged() || confirmed_disconnect", "Joybus should still finalize a sustained disconnect")




def test_snes_rumbletech_is_automatic_not_user_opt_in() -> None:
    setup = read_text("firmware/input/snes/input_snes_setup.cpp")
    poll = read_text("firmware/input/snes/input_snes_poll.cpp")
    visibility = read_text("firmware/menu/quick_config_visibility.cpp")
    defaults = read_text("firmware/core/settings_store_per_mode_defaults.cpp")
    require(
        setup,
        "return deviceMode == RZORD_SNES;",
        "RumbleTech command support should be automatic in SNES mode",
    )
    require(
        poll,
        "SNES_RUMBLETECH_ACTIVE_POLL_INTERVAL_US",
        "Active RumbleTech command traffic should use the faster keepalive cadence",
    )
    require(
        poll,
        "rumble_poll_active = true;",
        "Active RumbleTech command traffic should keep polling at the active rumble cadence",
    )
    reject(
        visibility,
        "addVisibleItem(QCI_RUMBLETECH)",
        "RumbleTech should not be a user-facing yes/no quick menu setting",
    )
    require(
        defaults,
        "return 3;",
        "SNES rumble level default should be usable without a RumbleTech opt-in",
    )


def test_psx_single_controller_clears_physical_fallback_latches() -> None:
    header = read_text("firmware/input/psx/Input_Psx.h")
    setup = read_text("firmware/input/psx/input_psx_setup.cpp")
    poll = read_text("firmware/input/psx/input_psx_poll.cpp")
    require(header, "void clearPhysicalFallbackLatches();", "PSX fallback helper declaration")
    require(setup, "void RZInputPSX::clearPhysicalFallbackLatches()", "PSX fallback helper implementation")
    require(setup, "clearPhysicalFallbackLatches();\n    rumbleConfiguredProto[i] = PSPROTO_UNKNOWN;", "PSX setup single-controller latch")
    require(poll, "clearPhysicalFallbackLatches();\n        tryEnableAnalogMode(i);", "PSX runtime single-controller latch")


def test_psx_normal_setup_rejects_multitap_without_controller() -> None:
    header = read_text("firmware/input/psx/Input_Psx.h")
    setup = read_text("firmware/input/psx/input_psx_setup.cpp")
    poll = read_text("firmware/input/psx/input_psx_poll.cpp")
    autodetect_header = read_text("firmware/input/autodetect/Input_AutoDetect.h")
    autodetect_probe = read_text("firmware/input/autodetect/input_autodetect_psx_probe.cpp")
    require(header, "bool multitapHasController(uint8_t port);", "PSX multitap controller gate declaration")
    require(setup, "bool RZInputPSX::multitapHasController(uint8_t port)", "PSX multitap controller gate implementation")
    require(
        setup,
        "if (psxmulti[port]->enableMultiTap() && multitapHasController(port))",
        "PSX setup should only accept multitap when a controller is present",
    )
    reject(
        setup,
        "memoryCardOnlyPhysicalPresent",
        "PSX normal setup should not latch memory-card-only as an input mode",
    )
    reject(
        poll,
        "memoryCardOnlyPhysicalPresent",
        "PSX polling should not maintain a memory-card-only input mode",
    )
    reject(
        poll,
        "isMultitap && !hasAnyController()",
        "PSX polling should not keep a no-controller multitap alive",
    )
    reject(
        autodetect_header,
        "probePSXMultitap",
        "AutoDetect should not detect bare PSX multitaps as controllers",
    )
    reject(
        autodetect_probe,
        "probePSXMultitap(pins, port)",
        "AutoDetect should not route bare PSX multitap replies to PSX mode",
    )
    reject(
        autodetect_header,
        "probePSXMemoryCardSlot",
        "AutoDetect should not detect PSX memory cards as controllers",
    )
    reject(
        autodetect_probe,
        "probePSXMemoryCardSlot(pins, port)",
        "AutoDetect should not route PSX memory-card-only replies to PSX mode",
    )


def test_psx_single_controller_setup_runs_before_multitap_probe() -> None:
    setup = read_text("firmware/input/psx/input_psx_setup.cpp")
    require(
        setup,
        "refreshPsxBusDrivers();\n  trySetupSingleController();\n\n  // Try each enabled physical PSX connector",
        "PSX setup should exhaust single-controller setup before multitap probing",
    )
    reject(
        setup,
        "if (autoPsxPortHint != 0xFF) {\n    refreshPsxBusDrivers();\n    trySetupSingleController();\n  }",
        "PSX single-controller setup must not be limited to AUTO-resolved ports",
    )


def test_psx_dualshock_uses_known_compatible_analog_and_rumble_sequence() -> None:
    header = read_text("firmware/input/psx/Input_Psx.h")
    setup = read_text("firmware/input/psx/input_psx_setup.cpp")
    poll = read_text("firmware/input/psx/input_psx_poll.cpp")
    require(header, "PsxControllerProtocol rumbleConfiguredProto[logical_slots]", "PSX rumble init should be idempotent")
    reject(header, "shouldLockDualShockAnalogMode", "PSX should not use the failed locked analog helper")
    reject(header, "shouldRestoreDualShockAnalogMode", "PSX should not use the failed runtime analog restore helper")
    reject(setup, "enableAnalogSticks(true, true)", "PSX should not use locked analog mode for normal DualShock setup")
    require(setup, "const bool enabled = psx[i]->enableAnalogSticks();", "PSX should use the RetroZord-compatible analog enable command")
    require(setup, "delay(1);\n    psx[i]->read();", "PSX analog enable should flush a read after leaving config mode")
    rumble_setup = setup.split("void RZInputPSX::tryEnableRumble", 1)[1].split(
        "bool RZInputPSX::tryEnableAnalogMode", 1
    )[0]
    require(
        setup,
        "bool psxProtocolSupportsDualShockRumble(PsxControllerProtocol proto)",
        "PSX rumble configuration should treat DualShock and DualShock 2 as one family",
    )
    require(
        setup,
        "if (psxProtocolSupportsDualShockRumble(rumbleConfiguredProto[i]))",
        "PSX rumble init should not repeat within the DualShock protocol family",
    )
    reject(
        rumble_setup,
        "rumbleConfiguredProto[i] = PSPROTO_UNKNOWN;",
        "Transient non-DualShock reads must not clear the rumble configuration latch",
    )
    require(
        setup,
        "rumbleConfiguredProto[port] = PSPROTO_UNKNOWN;",
        "A confirmed PSX disconnect should clear the rumble configuration latch",
    )
    setup_body = setup.split("void RZInputPSX::setup()", 1)[1].split(
        "void RZInputPSX::setup2()", 1
    )[0]
    reject(
        setup_body,
        "tryEnableRumble(i);",
        "PSX setup must not arm physical motors before a real host request",
    )
    require(
        poll,
        "if ((effective_left != 0 || effective_right != 0) && !rumble_configured)",
        "PSX rumble should arm only when nonzero feedback is requested",
    )
    require(
        poll,
        "if (rumble_configured) {\n          const uint8_t physical_left",
        "PSX polling must leave an unrequested rumble protocol inert",
    )
    require(
        poll,
        "psx[i]->setRumble(effective_right ? 1 : 0, physical_left);",
        "PSX polling should transmit explicit zero motor values while idle",
    )
    reject(setup, "enableRumble(false)",
           "PSX runtime must not reconfigure the motor-byte mapping at idle")
    reject(poll, "tryDisableRumble",
           "PSX runtime must keep the motor-byte mapping stable after first use")
    psx_library = read_text(
        "third_party/firmware_libraries/PsxNewLib/PsxSingleController.h"
    )
    require(
        psx_library,
        "controller.rumbleEnabled = enabled && ret;",
        "Disabling PSX rumble must clear the library rumble-enabled state",
    )
    require(
        psx_library,
        "out[3] = 0x00;\n\t\tout[4] = 0x00;",
        "Idle PSX polls must transmit explicit zero motor bytes",
    )
    require(
        psx_library,
        "byte *in = driver -> autoShift (out, sizeof(poll));",
        "PSX reads must clock a complete poll containing the explicit motor bytes",
    )
    reject(
        psx_library,
        "autoShift (poll, 3)",
        "PSX reads must not let short polls pad retained motor slots with 0x5A",
    )
    require(setup, "psx[i]->setRumble(false, 0);\n  psx[i]->read();", "PSX stopRumble should flush a zero-rumble read")
    require(setup, "stopRumble(i);\n      if (rumbleEnabled)", "PSX rumble init should end with a flushed stop command")
    require(poll, "clearPhysicalFallbackLatches();\n        tryEnableAnalogMode(i);", "PSX hotplug should do a single analog-enable attempt")
    reject(poll, "shouldRestoreDualShockAnalogMode", "PSX polling should not spam config commands to restore analog")


def test_psx_rumble_preserves_independent_host_motor_channels() -> None:
    header = read_text("firmware/input/psx/Input_Psx.h")
    poll = read_text("firmware/input/psx/input_psx_poll.cpp")

    require(
        poll,
        "psx[i]->setRumble(effective_right ? 1 : 0, physical_left);",
        "PSX should map the host light channel only to the binary small motor and the heavy channel to the variable large motor",
    )
    require(
        poll,
        "constexpr uint8_t kPsxLargeMotorMinimum = 96;",
        "PSX large-motor minimum perceptible strength",
    )
    require(
        poll,
        "effective_left != 0 && effective_left < kPsxLargeMotorMinimum",
        "PSX large-motor nonzero minimum clamp",
    )
    require(
        poll,
        ": effective_left;",
        "PSX large-motor strength above the minimum remains variable",
    )
    reject(header, "PSX_COMBINE_RUMBLE", "PSX builds must not expose a channel-combining fallback")
    reject(poll, "single_channel", "PSX host feedback must not mirror one active motor channel onto both motors")
    reject(
        poll,
        "effective_left | effective_right",
        "PSX physical rumble must not collapse independent host motor amplitudes",
    )


def test_boot_logo_is_held_until_home_screen_ready() -> None:
    platformio = read_text("platformio.ini")
    boot_ui = read_text("firmware/platform/boot/boot_ui_runtime.cpp")
    require_file("firmware/platform/boot/reflex_boot_logo_bitmap.h", "Reflex boot logo bitmap")
    reject_file("firmware/platform/boot/reflex_boot_logo_animation.h", "Unused boot animation")
    reject_file("tools/generate_boot_logo_animation.py", "Unused boot animation generator")
    boot_logo = read_text("firmware/platform/boot/reflex_boot_logo_bitmap.h")
    input_boot = read_text("firmware/input/runtime/input_boot_runtime.cpp")
    home = read_text("firmware/menu/menu_main_display.cpp")

    require(
        boot_logo,
        "constexpr uint8_t kReflexBootLogoWidth = 128;",
        "Boot logo bitmap should cover the full OLED width",
    )
    require(
        boot_logo,
        "constexpr uint8_t kReflexBootLogoHeight = 64;",
        "Boot logo bitmap should cover the full OLED height",
    )
    require(
        boot_logo,
        "const uint8_t kReflexBootLogoBitmap[] PROGMEM",
        "Boot logo bitmap should be available to firmware as program data",
    )
    logo_bytes = re.findall(r"0x[0-9A-Fa-f]{2}", boot_logo)
    if len(logo_bytes) != 1024:
        raise AssertionError(f"Boot logo bitmap should be 128x64 1bpp XBM data, found {len(logo_bytes)} bytes")
    reject(boot_ui, "reflex_boot_logo_animation.h", "Boot UI should not include animation frames")
    reject(boot_ui, "kReflexBootAnimation", "Boot UI should not execute boot animation data")
    reject(boot_ui, "drawBootLogoAnimation", "Boot UI should render a static splash")
    reject(boot_ui, "bootLogoAnimationPlayed", "Boot UI should not track animation playback")
    require(boot_ui, '#include "reflex_boot_logo_bitmap.h"', "Boot UI should include the bitmap logo")
    require(
        boot_ui,
        "u8g2.drawXBMP(0, 0, kReflexBootLogoWidth, kReflexBootLogoHeight, kReflexBootLogoBitmap);",
        "Boot splash should draw the Reflex bitmap logo",
    )
    reject(boot_ui, "getProductBootDisplayTitle()", "Boot splash should not render a text logo")
    reject(boot_ui, 'display.print(F("Detecting..."));', "Boot splash should not render boot progress text")
    require(boot_ui, "bootSplashVisible = true;", "Boot splash should become the active boot surface")
    reject(
        boot_ui,
        "if (!isBootAutoDetectPending()) {\n    bootSplashVisible = false;\n    return;\n  }",
        "Boot splash should not be limited to AUTO host detection",
    )
    reject(platformio, "ENABLE_BOOT_DISPLAY_SETTLE", "Normal boot should not repeatedly refresh OLED at trace markers")
    reject(boot_ui, "serviceBootDisplaySettle", "Boot trace markers should not touch OLED during normal boot")
    require(
        boot_ui,
        "if (isBootSplashScreenVisible() || isBootAutoDetectPending())",
        "USB boot debug info should not draw over the held splash",
    )
    reject(
        boot_ui,
        "display.print(F(\"Detecting...\"));\n  display.flush();\n  Wire.end();\n#endif\n}\n\n}  // namespace\n\nvoid suppressBootUsbDebugInfoOnce()",
        "Boot display settle should not print status text between the logo and home screen",
    )
    require(
        input_boot,
        "if (isBootSplashScreenVisible()) {\n      return;\n    }",
        "Input module description should not overwrite the boot logo",
    )
    require(
        home,
        "if (keepBootSplashActive) {\n    return;\n  }\n\n  if (needsU8g2Clear || firstHardwareClear)",
        "Home render should leave the boot logo untouched until the menu can draw once",
    )
    reject(home, "kColdBootOledRetryDelaysMs", "Home render should not schedule extra cold-boot OLED redraws")
    reject(home, "coldBootOledRetryCount", "Home render should not schedule extra cold-boot OLED redraws")
    require(home, "markBootSplashScreenConsumed();", "Home rendering should consume the boot splash")


def test_bouncing_screensaver_uses_rflx_bitmap() -> None:
    bounce = read_text("firmware/menu/menu_screensaver_bounce.cpp")
    home = read_text("firmware/menu/menu_main_display.cpp")
    require_file("firmware/menu/screensaver_rflx_bitmap.h", "RFLX screensaver bitmap")
    bitmap = read_text("firmware/menu/screensaver_rflx_bitmap.h")
    identity = read_text("firmware/config/classic2usb/product_identity.h")

    require(
        identity,
        '#define PRODUCT_OLED_HOME_TITLE         "RFLX"',
        "Classic2USB home screen should use the compact RFLX title",
    )
    require(
        identity,
        '#define PRODUCT_OLED_BOOT_TITLE         "REFLEX"',
        "Classic2USB boot branding should remain REFLEX",
    )

    require(
        bitmap,
        "constexpr uint8_t kRflxScreensaverLogoWidth = 56;",
        "Bouncing RFLX logo should leave horizontal travel space",
    )
    require(
        bitmap,
        "constexpr uint8_t kRflxScreensaverLogoHeight = 18;",
        "Bouncing RFLX logo should leave vertical travel space",
    )
    logo_bytes = re.findall(r"0x[0-9A-Fa-f]{2}", bitmap)
    if len(logo_bytes) != 126:
        raise AssertionError(
            f"RFLX screensaver bitmap should be 56x18 1bpp XBM data, found {len(logo_bytes)} bytes"
        )
    require(
        bounce,
        '#include "screensaver_rflx_bitmap.h"',
        "Bouncing screensaver should include the branded bitmap",
    )
    require(
        bounce,
        "u8g2.drawXBMP(",
        "Bouncing screensaver should draw the branded bitmap",
    )
    reject(bounce, "getProductScreensaverTitle", "Bouncing screensaver should not use plain text")
    require(
        home,
        '#include "screensaver_rflx_bitmap.h"',
        "Home screen should reuse the branded RFLX bitmap",
    )
    require(
        home,
        "constexpr uint8_t kHomeLogoHeight = 16;",
        "Home RFLX logo should fit above the mode line without clipping",
    )
    require(
        home,
        "drawHomeRflxLogo();",
        "Home screen should draw the branded RFLX bitmap",
    )
    reject(
        home,
        "getProductHomeDisplayTitle()",
        "Home screen should not render RFLX as ordinary text",
    )
    reject(bounce, "u8g2.drawStr", "Bouncing screensaver should not draw a text logo")


def test_xinput_select_r3_and_r2_keep_native_controls() -> None:
    mappings = (
        ("XInput", read_text("firmware/output/xinput/output_xinput_mapping_runtime.h"), "report"),
        ("XInputW", read_text("firmware/output/xinputw/output_xinputw_mapping_runtime.h"), "_xinputw_report[port]"),
    )
    for label, mapping, report in mappings:
        require(mapping, f"{report}.back = n64XinputLayout ? frame.L2 : frame.SELECT;", f"{label} Select should map to Back outside the N64 Adapt V1 layout")
        require(mapping, f"{report}.rs = n64XinputLayout ? 0 : frame.R3;", f"{label} ordinary R3 should map only to right-stick click")
        reject(mapping, f"{report}.back = frame.R3;", f"{label} R3 must not map to Back")
        reject(mapping, f"{report}.back = output_n64_c_backing_select(frame);", f"{label} Back must not use N64 C-Right backing")
        require(mapping, f"{report}.rt = frame.R2 ? 0xff : 0x00;", f"{label} digital R2 should map to RT")
        reject(mapping, f"{report}.rt = output_n64_c_backing_r2(frame) ? 0xff : 0x00;", f"{label} RT must not use N64 C-Down backing")


def test_xinput_n64_c_button_setting_and_virtual_boy_right_stick() -> None:
    common = read_text("firmware/output/usb/output_hid_mapping_runtime.h")
    capabilities = read_text("firmware/output/output_capabilities.h")
    defaults = read_text("firmware/core/settings_store_per_mode_defaults.cpp")
    bridge = read_text("firmware/output/runtime/input_runtime_output_bridge.cpp")
    per_mode = read_text("firmware/core/settings_store_per_mode_quick.cpp")
    registry = read_text("firmware/core/settings_registry.h")
    n64_input = read_text("firmware/input/gc64/input_gc64_poll.cpp")
    vboy_input = read_text("firmware/input/snes/input_snes_poll.cpp")
    visibility = read_text("firmware/menu/menu_helpers_visibility.cpp")
    quick_visibility = read_text("firmware/menu/quick_config_visibility.cpp")
    mappings = (
        ("XInput", read_text("firmware/output/xinput/output_xinput_mapping_runtime.h"), "report"),
        ("XInputW", read_text("firmware/output/xinputw/output_xinputw_mapping_runtime.h"), "_xinputw_report[port]"),
    )

    for mode, label in (("RZORD_N64", "N64"), ("RZORD_VBOY", "Virtual Boy")):
        require(
            common,
            f"if (deviceMode == {mode}) {{\n    return false;",
            f"{label} XInput should start from the controller name mapping",
        )

    for label, mapping, report in mappings:
        require(
            mapping,
            "isPositionButtonMapActiveForXInputOutput()",
            "Both XInput transports should use the Nintendo name policy",
        )
        require(
            mapping,
            "faceButtons = output_apply_n64_c_buttons_to_xinput_face_buttons(faceButtons, frame);",
            f"{label} should apply the Adapt V1 N64 face layout",
        )
        reject(
            mapping,
            "faceButtons = output_apply_n64_c_buttons_to_face_buttons(faceButtons, frame);",
            f"{label} must not use the generic face-button overlay",
        )
        for token, message in (
            ("const bool n64XinputLayout = controllerFrameTypeNameIsN64(frame);", "identify the N64 layout"),
            (f"{report}.lb = n64XinputLayout ? frame.L1", "map N64 L to LB without a Z alias"),
            (f"{report}.ls = n64XinputLayout ? 0 : frame.L3;", "reserve LS outside the N64 layout"),
            (f"{report}.rs = n64XinputLayout ? 0 : frame.R3;", "reserve RS outside the N64 layout"),
            (f"{report}.back = n64XinputLayout ? frame.L2 : frame.SELECT;", "map N64 Z to Back"),
            (f"{report}.lt = frame.B ? 0xff : 0x00;", "map physical N64 B to LT"),
            (f"{report}.rt = frame.A ? 0xff : 0x00;", "map physical N64 A to RT"),
        ):
            require(mapping, token, f"{label} Adapt V1 mapping should {message}")

    n64_policy = capabilities.split(
        "inline bool output_maps_n64_c_buttons_to_face_buttons()", 1
    )[1].split("inline bool output_should_spatialize_n64_c_buttons", 1)[0]
    spatial_policy = capabilities.split(
        "inline bool output_should_spatialize_n64_c_buttons", 1
    )[1].split("inline uint32_t output_apply_n64_c_buttons_to_face_buttons", 1)[0]
    xinput_helper = capabilities.split(
        "inline uint32_t output_apply_n64_c_buttons_to_xinput_face_buttons", 1
    )[1].split("inline uint8_t output_n64_c_backing_r2", 1)[0]
    vboy_policy = capabilities.split(
        "inline bool output_maps_vboy_right_dpad_to_face_buttons()", 1
    )[1].split("inline void output_set_switch_profile", 1)[0]
    require(
        n64_policy,
        "mode == OUTPUT_MISTER || mode == OUTPUT_HID",
        "N64 C-button policy should explicitly cover DInput",
    )
    dinput_policy = n64_policy.split(
        "if (mode == OUTPUT_MISTER || mode == OUTPUT_HID)", 1
    )[1].split("if (mode == OUTPUT_XINPUT || mode == OUTPUT_XINPUT2P)", 1)[0]
    require(
        dinput_policy,
        "return get_effective_n64_cstick_mode() == N64CSTICK_AS_BUTTONS;",
        "DInput should default to buttons while honoring the explicit Stick selection",
    )
    xinput_policy = n64_policy.split(
        "if (mode == OUTPUT_XINPUT || mode == OUTPUT_XINPUT2P)", 1
    )[1].split("if (output_uses_switch_n64_profile())", 1)[0]
    require(
        xinput_policy,
        "return get_effective_n64_cstick_mode() == N64CSTICK_AS_BUTTONS;",
        "N64 XInput Auto should follow the Buttons policy",
    )
    reject(
        xinput_policy,
        "return n64_cstick_mode == N64CSTICK_AS_BUTTONS;",
        "N64 XInput must not make Auto behave like Stick",
    )
    xinput_spatial_policy = spatial_policy.split(
        "if (mode == OUTPUT_XINPUT || mode == OUTPUT_XINPUT2P)", 1
    )[1].split("if (mode == OUTPUT_SWITCHPRO", 1)[0]
    require(
        xinput_spatial_policy,
        "return false;",
        "XInput should preserve all four N64 C buttons as unique digital controls",
    )
    for token, message in (
        ("buttons &= ~(kButtonA | kButtonB | kButtonX | kButtonY);", "remove physical N64 A/B from Xbox face buttons"),
        ("get_effective_n64_cstick_mode() != N64CSTICK_AS_BUTTONS", "leave ABXY clear when Stick is selected"),
        ("if (frame.L3) buttons |= kButtonA;", "map C-Down to Xbox A"),
        ("if (frame.R3) buttons |= kButtonB;", "map C-Right to Xbox B"),
        ("if (frame.X) buttons |= kButtonY;", "map C-Up to Xbox Y"),
        ("if (frame.Y) buttons |= kButtonX;", "map C-Left to Xbox X"),
    ):
        require(xinput_helper, token, f"N64 Adapt V1 XInput mapping should {message}")
    require(
        bridge,
        "return N64CSTICK_AS_BUTTONS;",
        "N64 C-button Auto should resolve to Buttons",
    )
    require(
        vboy_policy,
        "mode == OUTPUT_XINPUT || mode == OUTPUT_XINPUT2P",
        "Virtual Boy right D-pad should continue to use the right stick in XInput",
    )

    for source, tokens, label in (
        (
            n64_input,
            ("frame.RX = INT8_MIN;", "frame.RX = INT8_MAX;",
             "frame.RY = INT8_MIN;", "frame.RY = INT8_MAX;"),
            "N64 C buttons",
        ),
        (
            vboy_input,
            ("frame.RX = -127;", "frame.RX = 127;",
             "frame.RY = -127;", "frame.RY = 127;"),
            "Virtual Boy right D-pad",
        ),
    ):
        for token in tokens:
            require(source, token, f"{label} should drive all right-stick directions")

    require(
        visibility,
        "case menu_item_n64_cstick_mode: {\n      if (is_nso_special_active()) return true;\n      return !menuInputIsN64();",
        "N64 C-button mode should be selectable in every non-NSO output mode",
    )
    reject(
        visibility,
        "if (mode == OUTPUT_HID || mode == OUTPUT_MISTER) return true;",
        "N64 C-button mode must not be hidden in DInput",
    )
    for source, token, label in (
        (visibility,
         "if (mode == OUTPUT_XINPUT || mode == OUTPUT_XINPUT2P) {\n          return true;",
         "system-menu N64 Z setting"),
        (quick_visibility,
         "const bool n64UsesAdaptV1Xinput =",
         "quick-menu N64 XInput detection"),
        (quick_visibility,
         "if ((isN64(mode) || isGameCube(mode)) && !n64UsesAdaptV1Xinput)",
         "quick-menu N64 Z setting"),
    ):
        require(source, token, f"Adapt V1 fixed Z-to-Back mapping should hide the {label}")

    default_policy = defaults.split(
        "uint8_t defaultN64CStickModeForInputMode", 1
    )[1].split("uint8_t defaultSpinnerSpeedForInputMode", 1)[0]
    require(default_policy, "if (mode == RZORD_N64)", "N64 should have a mode-specific C-button default")
    require(default_policy, "return 1;", "N64 C buttons should default to Buttons")
    require(
        per_mode,
        "settings.n64_cstick_mode = defaultN64CStickModeForInputMode(mode);",
        "New N64 per-mode records should use the Buttons default",
    )
    require(
        defaults,
        "settings.n64_cstick_mode = defaultN64CStickModeForInputMode(mode);",
        "Invalid N64 C-button settings should sanitize to the mode default",
    )
    require(
        registry,
        "defaultN64CStickModeValue, nullptr",
        "Serial and menu default commands should use the N64 Buttons default",
    )


def test_switch_host_detection_tolerates_composite_endpoint_drift() -> None:
    source = read_text("firmware/output/autodetect/usb_detect_runtime.cpp")
    callback = source.split("void usb_detect_handle_edpt_clear_stall", 1)[1]
    callback = callback.split("\n}", 1)[0]
    resolver = source.split("uint8_t usb_host_detection_task", 1)[1]

    require(
        callback,
        "host_detection.cleared_input  |= (ep_addr & 0x80u) != 0;",
        "Switch detection should accept the active HID IN endpoint after composite endpoint drift",
    )
    require(
        callback,
        "host_detection.cleared_output |= (ep_addr & 0x80u) == 0 && ep_addr != 0;",
        "Switch detection should accept the active non-control HID OUT endpoint after composite endpoint drift",
    )
    reject(callback, "ep_addr == 0x81", "Switch detection must not require a fixed HID IN endpoint")
    reject(callback, "ep_addr == 0x01", "Switch detection must not require a fixed HID OUT endpoint")
    reject(callback, "usb_detect_mark_result(DETECT_SWITCH1)", "Endpoint callbacks must not resolve Switch before Windows can identify itself")
    require(
        resolver,
        "host_detection.cleared_input &&\n    host_detection.cleared_output &&",
        "Switch resolution should wait for both endpoint directions",
    )
    require(
        resolver,
        "const bool switch_endpoint_candidate =",
        "Switch endpoint clears should remain a candidate while host descriptors settle",
    )
    require(
        resolver,
        "const bool switch_endpoint_candidate_ready =",
        "Switch resolution should use an explicit candidate readiness gate",
    )
    require(
        resolver,
        "!mounted_hid_generic_activity ||\n      (ms_now - started_at_ms) > ps4_feature_grace_ms);",
        "A Switch that reads normal HID descriptors should resolve after the PS4 feature grace",
    )
    reject(
        resolver,
        "host_detection.cleared_input && host_detection.cleared_output &&\n      !mounted_hid_generic_activity &&",
        "Normal Switch descriptor reads must not permanently veto its endpoint signature",
    )
    require(
        resolver,
        "!host_detection.ms_os_string_read &&",
        "Microsoft OS descriptor traffic should continue to block a Switch classification",
    )
    require(
        resolver,
        "!host_detection.vendor_setup_seen &&",
        "Vendor setup traffic should continue to block a Switch classification",
    )


def test_switch_no_traffic_attach_retry_gets_settle_grace() -> None:
    runtime = read_text("firmware/output/autodetect/output_autodetect_runtime.cpp")
    rp2040 = read_text(
        "firmware/output/autodetect/output_autodetect_rp2040_usb_debug.cpp"
    )
    rp2040_header = read_text(
        "firmware/output/autodetect/output_autodetect_rp2040_usb_debug.h"
    )

    require(
        runtime,
        "constexpr uint16_t kGenericNoMountResolveMs = 1000;",
        "Normal generic host detection should keep its one-second no-mount deadline",
    )
    require(
        runtime,
        "constexpr uint16_t kGenericNoMountAfterAttachRetryResolveMs = 2000;",
        "A VBUS-backed attach retry should receive one additional second to enumerate",
    )
    require(
        runtime,
        "output_autodetect_usb_attach_retry_needs_grace()",
        "The no-mount deadline should consult the RP2040 attach-retry state",
    )
    require(
        runtime,
        "auto_detect_elapsed_ms >=\n          auto_detect_generic_no_mount_resolve_ms()",
        "No-connect fallback should use the retry-aware deadline",
    )
    require(
        rp2040,
        "attach_retry_stage >= 2 &&\n         (usb_hw->sie_status & USB_SIE_STATUS_VBUS_DETECTED_BITS) != 0;",
        "Extra grace should require both a completed retry and live host VBUS",
    )
    require(
        rp2040_header,
        "inline bool output_autodetect_usb_attach_retry_needs_grace() { return false; }",
        "Non-RP2040 builds should retain the ordinary no-mount deadline",
    )


def test_release_settings_hotkeys_are_opt_in_and_kiosk_gates_buttons() -> None:
    settings_store = read_text("firmware/core/settings_store.h")
    settings = read_text("firmware/core/settings_registry.h")
    runtime_state = read_text("firmware/menu/menu_runtime_state.cpp")
    menu_defs = read_text("firmware/menu/menu_item_defs.h")
    menu_catalog = read_text("firmware/menu/menu_catalog.cpp")
    menu_visibility = read_text("firmware/menu/menu_helpers_visibility.cpp")
    descriptors = read_text("firmware/menu/menu_descriptors_data.cpp")
    menu_handlers = read_text("firmware/menu/menu_item_handlers.cpp")
    menu_render = read_text("firmware/menu/menu_helpers_render.cpp")
    submenu_hotkeys = read_text("firmware/menu/menu_submenu_hotkeys.cpp")
    submenu_kiosk = read_text("firmware/menu/menu_submenu_kiosk.cpp")
    submenu_bridge = read_text("firmware/menu/menu_bridge_submenus.cpp")
    platform_menu = read_text("firmware/platform/runtime/platform_menu_runtime.cpp")
    button_handler = read_text("firmware/platform/button_handler.h")
    webhid = read_text("firmware/output/usb/webhid/webhid_settings_reports.cpp")
    hotkeys = read_text("firmware/core/hotkey_combo.cpp")
    adapt_html = read_text("web/Adapt.html")

    require(hotkeys, "kDefaultMenuCombo = INPUT_PAD_L | INPUT_START", "Quick menu combo remains available when opted in")
    require(hotkeys, "kDefaultSystemMenuCombo = INPUT_PAD_R | INPUT_START", "System menu combo remains available when opted in")
    require(settings_store, "MENU_HOTKEY_KIOSK_MASK = 0x04", "Kiosk should reuse a spare menu-hotkey flag bit")
    require(settings_store, "KioskMode,\n  MisterOutput,\n  Count", "MiSTer output should be appended after the persisted kiosk setting id")
    require(settings, "SettingId::MenuHotkey", "Menu hotkey setting should still exist")
    require(settings, "SettingId::SystemMenuHotkey", "System menu hotkey setting should still exist")
    require(settings, "SettingId::KioskMode", "Kiosk setting should be registered")
    require(settings, "inline constexpr int32_t defaultKioskModeValue(DeviceEnum) {\n  return 0;\n}", "Kiosk persisted default should be explicitly off")
    require(settings, "defaultKioskModeValue, nullptr", "Kiosk setting spec should use the explicit off default")
    require(settings, "offsetof(GlobalSettingsRecord, capture_hotkey),         0,", "Capture hotkey should default off")
    require(runtime_state, "uint8_t menu_menu_hotkey = 0;", "Quick menu hotkey should default off")
    require(runtime_state, "uint8_t menu_system_menu_hotkey = 0;", "System menu hotkey should default off")
    require(runtime_state, "uint8_t menu_home_hotkey = 1;", "Home hotkey should default on")
    require(runtime_state, "uint8_t menu_capture_hotkey = 0;", "Capture hotkey should default off")
    require(runtime_state, "uint8_t menu_kiosk_mode = 0;", "Kiosk should default off")
    require(menu_defs, "menu_item_kiosk_mode", "Kiosk menu item enum")
    require(menu_catalog, '{ menu_item_kiosk_mode, "Kiosk" }', "Kiosk menu label")
    require(menu_catalog, '{ menu_item_hotkeys, "Hotkey" }', "Hotkey submenu label")
    require(menu_visibility, "case menu_item_kiosk_mode:", "Kiosk should be a system setting")
    require(menu_visibility, "case menu_item_hotkeys:", "Hotkey submenu should be a system setting")
    require(menu_visibility, "case menu_item_hotkey_hold_time:", "Hotkey rows should be hidden from top-level system menu")
    reject(menu_visibility, "case menu_item_hotkeys:\n      return true;", "Hotkey submenu should be visible")
    require(descriptors, ".id = menu_item_kiosk_mode", "Kiosk menu descriptor")
    require(menu_handlers, "item == menu_item_kiosk_mode", "Kiosk should use dialog handler instead of direct toggle")
    require(menu_render, "case menu_item_kiosk_mode:", "Kiosk should render like a submenu row")
    require(menu_render, "case menu_item_hotkeys:", "Hotkey should render like a submenu row")
    require(menu_render, 'display.print(F("[Cancel]"));', "System footer should bracket Cancel only when selected")
    require(menu_render, 'display.print(F(" Cancel "));', "System footer should leave Cancel unbracketed above the bottom row")
    require(menu_render, 'display.print(F("[Save]"));', "System footer should bracket Save only when selected")
    require(menu_render, 'display.print(F(" Save "));', "System footer should leave Save unbracketed above the bottom row")
    reject(menu_render, 'display.print("[Cancel]");', "System footer should not always bracket Cancel")
    reject(menu_render, 'display.print("[Save]");', "System footer should not always bracket Save")
    require(submenu_bridge, "handleHotkeysSubmenu", "Hotkey submenu should be wired")
    require(submenu_bridge, "handleKioskSubmenu", "Kiosk submenu should be wired")
    for token in (
        '"Hotkey Hold"',
        '"Quick Menu"',
        '"System Menu"',
        '"Home"',
        '"Capture"',
        '"<+ST"',
        '">+ST"',
        '"v+ST"',
        '"^+ST"',
        "HotkeyRenderSnapshot",
        "hotkeyRenderSnapshotMatches",
        "if (hotkeyRenderSnapshotMatches(cursor))",
        "menu_hotkey_hold_time",
        "menu_menu_hotkey",
        "menu_system_menu_hotkey",
        "menu_home_hotkey",
        "menu_capture_hotkey",
    ):
        require(submenu_hotkeys, token, f"Hotkey submenu missing {token}")
    for stale in (
        "boolName(menu_menu_hotkey)",
        "boolName(menu_system_menu_hotkey)",
        "boolName(menu_home_hotkey)",
        "boolName(menu_capture_hotkey)",
    ):
        reject(submenu_hotkeys, stale, f"Hotkey submenu should display combo labels instead of {stale}")
    for token in (
        '"Mode x2 = Quick Menu"',
        '"Mode (hold) = Settings"',
        '"Reset x2 = Reboot"',
        '"Reset (hold) = Auto Input"',
        '"On"',
        '"Off"',
        '"Cancel"',
        "menu_kiosk_mode = 1",
        "menu_kiosk_mode = 0",
    ):
        require(submenu_kiosk, token, f"Kiosk dialog missing {token}")
    for stale in (
        '"Mode x2 = Quick"',
        '"Hold = Sys/Auto"',
        '"Current: "',
        '"Set Kiosk:"',
        '"Yes"',
        '"No"',
        '"Enable"',
        '"Disable"',
    ):
        reject(submenu_kiosk, stale, f"Kiosk dialog stale text {stale}")
    require(button_handler, "BTN_SYSTEM_MENU_HOLD_MS 1500", "Mode hold should open System Menu after 1.5 seconds")
    require(button_handler, "BTN_RESET_LONG_PRESS_MS 3000", "Reset hold should keep the 3-second Auto Input threshold")
    require(platform_menu, "kKioskMenuTapCount = 2", "Kiosk should require two quick menu taps")
    require(platform_menu, "kKioskResetTapCount = 2", "Kiosk should require two reset taps")
    require(platform_menu, "event == BTN_EVENT_LONG || event == BTN_EVENT_LONG_RELEASE", "Kiosk should not gate long-press system/autodetect actions")
    require(platform_menu, "event == BTN_EVENT_DOUBLE ? 2 : 1", "Kiosk tap gate should count double taps toward the required tap count")
    reject(platform_menu, "event != BTN_EVENT_SINGLE && event != BTN_EVENT_LONG", "Kiosk must not require taps before Mode long press")
    require(platform_menu, "menu_kiosk_mode != 0", "Kiosk gate should be runtime-controlled")
    require(webhid, "0x10", "WebHID hotkey flags should report kiosk")
    require(webhid, "case 46:", "WebHID should write the kiosk setting")
    require(adapt_html, "settings-kiosk-mode", "Adapt.html should expose kiosk mode")
    require(adapt_html, "[46, kioskMode]", "Adapt.html should save kiosk mode")



def test_classic2usb_excludes_native_ps5_and_auth_sidecar() -> None:
    roles = read_text("firmware/config/classic2usb/product_roles.h")
    feature_gates = read_text("firmware/config/classic2usb/feature_gates.h")
    settings = read_text("firmware/core/settings_registry.h")
    runtime_h = read_text("firmware/output/output_runtime_state.h")
    runtime = read_text("firmware/output/output_runtime_state.cpp")
    boot = read_text("firmware/core/boot/boot_storage_runtime.cpp")
    mode_save = read_text("firmware/core/settings_store_mode_save.cpp")
    serial = read_text("firmware/core/runtime/runtime_serial_debug.cpp")
    menu = read_text("firmware/menu/menu_mode_state.cpp")
    menu_output = read_text("firmware/menu/menu_output_mode.cpp")
    auth = read_text("firmware/output/auth/auth_status.cpp")
    autodetect = read_text(
        "firmware/output/autodetect/output_autodetect_runtime.cpp")
    usb_configure = read_text(
        "firmware/output/usb/output_usb_configure_runtime.h")
    webhid = read_text(
        "firmware/output/usb/webhid/webhid_settings_reports.cpp")
    post_link = read_text("tools/test_classic2usb_link_isolation.py")

    reject(
        roles,
        "ADAPT_HAS_USB_AUTH_SIDECAR",
        "Classic2USB must not claim physically absent USB auth hardware",
    )
    require(
        roles,
        "ADAPT_HAS_MANAGEMENT_MSC",
        "Classic2USB must compile its virtual management drive",
    )
    reject(
        feature_gates,
        "ENABLE_OUTPUT_PS5",
        "Classic2USB must not compile a native PS5 personality",
    )
    require(
        settings,
        "#if !defined(ENABLE_OUTPUT_PS5)\n  if (value == OUTPUT_PS5) {\n    return OUTPUT_PS4;",
        "Stored unsupported PS5 mode must sanitize to PS4",
    )
    require(
        runtime_h,
        "outputMode_t sanitizeConfiguredOutputMode(outputMode_t mode);",
        "Configured output sanitizer declaration",
    )
    require(
        runtime,
        "#if !defined(ENABLE_OUTPUT_PS5)\n  if (mode == OUTPUT_PS5) {\n    return OUTPUT_PS4;",
        "Runtime unsupported PS5 mode must sanitize to PS4",
    )
    for text_value, token, label in (
        (boot, "configuredOutputMode = sanitizeConfiguredOutputMode(", "boot"),
        (mode_save, "selection.newOutputMode = sanitizeConfiguredOutputMode(newOutputMode);", "quick save"),
        (mode_save, "(uint8_t)sanitizeConfiguredOutputMode(configuredOutputMode);", "input save"),
        (serial, "newMode = sanitizeConfiguredOutputMode(newMode);", "runtime serial"),
        (menu_output, "configuredOutputMode = sanitizeConfiguredOutputMode(menu_output);", "OLED menu"),
        (webhid, "outputMode_t newOutputMode = sanitizeConfiguredOutputMode((outputMode_t)value);", "WebHID"),
    ):
        require(text_value, token, f"Unsupported PS5 sanitization at {label}")
    require(
        menu,
        "#ifdef ENABLE_OUTPUT_PS5\n  OUTPUT_PS5,\n#endif",
        "PS5 must be absent from unsupported output menus",
    )
    require(
        usb_configure,
        "#ifdef ENABLE_OUTPUT_PS5\n    case OUTPUT_PS5:\n#endif",
        "PS5 USB mode must remain compile-gated",
    )
    require(
        auth,
        "case OUTPUT_PS5:\n      #if defined(ENABLE_OUTPUT_PS5)",
        "PS5 auth visibility must use the capability gate",
    )
    require(
        autodetect,
        "#if !defined(ENABLE_OUTPUT_PS5)\n  // Builds without donor-assisted native PS5 support",
        "PlayStation Auto must settle directly into PS4",
    )
    for marker in ('b"P5General"', 'b"configure_ps5_general_output_runtime"'):
        require(post_link, marker, "Post-link native PS5 exclusion")


def test_classic2usb_ps4_output_is_stable_and_local_only() -> None:
    descriptor = read_text(
        "firmware/output/playstation/output_descriptors_ps4_runtime.h")
    setup = read_text(
        "firmware/output/usb/output_usb_mode_setup_console_runtime.h")
    report_handler = read_text(
        "firmware/output/usb/output_usb_hid_report_runtime.h")
    send = read_text("firmware/output/usb/output_usb_send_runtime.h")
    state = read_text("firmware/output/usb/output_usb_report_state.h")
    support = read_text(
        "firmware/output/autodetect/output_autodetect_support.cpp")
    auth = read_text("firmware/output/auth/ps4_auth.cpp")
    auth_header = read_text("firmware/output/auth/ps4_auth.h")
    management = read_text("firmware/core/serial_management_commands.cpp")

    for token in (
        "PS4_GAMEPAD_VENDOR_ID = 0x1532",
        "PS4_GAMEPAD_PRODUCT_ID = 0x0401",
        "PS4_GAMEPAD_POLL_INTERVAL_MS = 1",
        "PS4_GAMEPAD_KEEPALIVE_MS = 5",
        "0x09, 0x05, //Usage (Game Pad)",
        "ps4_gamepad_definition_report[47]",
        "static_assert(sizeof(usbout_ps4_report_t) == 64",
    ):
        require(descriptor, token, f"Stable PS4 descriptor missing {token}")
    require(
        setup,
        "TinyUSBDevice.setID(PS4_GAMEPAD_VENDOR_ID, PS4_GAMEPAD_PRODUCT_ID);",
        "Known-good PS4 identity",
    )
    require(
        setup,
        "PS4_GAMEPAD_POLL_INTERVAL_MS,\n    false);",
        "One-endpoint 1 ms PS4 topology",
    )
    require(
        report_handler,
        "memcpy(buffer, ps4_gamepad_definition_report,",
        "Canonical PS4 definition report",
    )
    reject(
        report_handler,
        "auto_promote_ps5_detection",
        "PS4 traffic must not promote into native PS5",
    )
    reject(
        support,
        "auto_promote_ps5_detection",
        "Unsafe PS4-to-PS5 heuristic must be removed",
    )
    for token in (
        "ps4_last_report_ms",
        "ps4_report_clock_started",
    ):
        require(state, token, f"PS4 report clock missing {token}")
    for token in (
        "PS4_GAMEPAD_KEEPALIVE_MS",
        "ps4KeepaliveDue",
        "_ps4_report.axis_timing",
    ):
        require(send, token, f"PS4 keepalive missing {token}")
    for token in (
        "_nonce_pages_received",
        "ps4AuthReportCrc32",
        "writePs4AuthReportCrc",
        "bool PS4Auth::signLocally()",
        "void PS4Auth::writeDiagnostics",
    ):
        require(auth, token, f"Local PS4 auth missing {token}")
    reject(auth, "ESP32", "Classic2USB PS4 auth must remain local-only")
    reject(auth_header, "remote_sign", "Classic2USB must not carry ESP32 signer state")
    require(
        management,
        "ps4Auth.writeDiagnostics(out);",
        "PS4 auth diagnostics must be visible to management clients",
    )


def test_usb_versions_are_binary_coded_decimal() -> None:
    identity = read_text("firmware/output/runtime/input_runtime_output_bridge.h")
    autodetect = read_text("firmware/output/autodetect/output_autodetect_support.cpp")
    hid_setup = read_text("firmware/output/usb/output_usb_mode_setup_hid_runtime.h")
    console_setup = read_text("firmware/output/usb/output_usb_mode_setup_console_runtime.h")

    for token, label in (
        ("pack_bcd_byte", "packed-BCD byte encoder"),
        ("encode_bcd_device_version", "packed-BCD device encoder"),
        ("platform + (25u * platform_sub)", "unique platform/subtype identity"),
        ("current_bcd_device_version", "runtime BCD device version"),
    ):
        require(identity, token, label)
    reject(identity, "uint16_t platform_sub : 4", "Non-BCD identity bitfield")
    reject(identity, "uint16_t composite", "Raw bit-packed device version")
    require(
        autodetect,
        "constexpr uint8_t kAutoProbeProfile = 99",
        "AUTO probe must use the reserved logical profile",
    )
    require(
        autodetect,
        "return encode_bcd_device_version(bcd_device_version.revision,",
        "AUTO probe must preserve the report ABI generation",
    )
    reject(autodetect, "probe_revision", "AUTO probe ABI bump")
    require(
        hid_setup,
        ": current_bcd_device_version());",
        "Generic HID identity must use packed BCD",
    )
    require(
        console_setup,
        "TinyUSBDevice.setDeviceVersion(current_bcd_device_version());",
        "PS3 fallback identity must use packed BCD",
    )
    require(
        console_setup,
        "TinyUSBDevice.setVersion(0x0200);\n  TinyUSBDevice.setDeviceVersion(0x0210);",
        "Pokken bcdUSB must encode USB 2.00 as packed BCD",
    )


def test_quick_menu_variable_rumble_has_heavy_ramp() -> None:
    header = read_text("firmware/core/rumble_test_runtime.h")
    runtime = read_text("firmware/core/rumble_test_runtime.cpp")
    menu_header = read_text("firmware/menu/quick_config.h")
    capabilities = read_text("firmware/menu/quick_config_capabilities.cpp")
    actions = read_text("firmware/menu/quick_config_actions.cpp")
    render = read_text("firmware/menu/quick_config_render.cpp")

    for source, token, label in (
        (header, "void rumbleTestStartHeavyRamp();",
         "Shared heavy-rumble ramp API"),
        (header, "bool rumbleTestGetHeavyRampLevel(uint8_t* level);",
         "Shared live heavy-rumble level API"),
        (runtime, "constexpr uint16_t kHeavyRampStepMs = 500;",
         "500 ms heavy-rumble ramp dwell"),
        (runtime, "constexpr uint8_t kHeavyRampStepCount = 17;",
         "0 through 255 heavy-rumble ramp steps"),
        (runtime, "const uint32_t step = elapsedMs / kHeavyRampStepMs;",
         "Time-driven nonblocking heavy-rumble ramp"),
        (runtime, "? 255u\n      : static_cast<uint8_t>(step * 16u);",
         "Heavy-rumble ramp reaches full strength"),
        (runtime, "testRumble[port].right = 0;",
         "Heavy-rumble ramp keeps the light motor off"),
        (runtime, "testHeavyRampLevel = level;",
         "Heavy-rumble ramp publishes its live level"),
        (menu_header, "QCI_RUMBLE_RAMP_HEAVY,",
         "Quick-menu heavy-rumble ramp item"),
        (capabilities,
         "return enabled && motors >= 1 && hasVariableRumbleStrength();",
         "Heavy-rumble ramp variable-strength capability gate"),
        (actions, "rumbleTestStartHeavyRamp();",
         "Quick-menu heavy-rumble ramp action"),
        (render, "rumbleTestGetHeavyRampLevel(&rampLevel)",
         "Quick-menu live heavy-rumble level refresh"),
        (render, "F(\"Heavy Ramp: \")",
         "Quick-menu heavy-rumble live level label"),
    ):
        require(source, token, label)


def test_no_host_controller_home_mode_and_rumble_are_guarded() -> None:
    autodetect = read_text(
        "firmware/output/autodetect/output_autodetect_runtime.cpp")
    output_enum = read_text("firmware/output/output_mode.h")
    output_state = read_text("firmware/output/output_runtime_state.cpp")
    output_catalog = read_text("firmware/output/output_mode_catalog.cpp")
    menu_order = read_text("firmware/menu/menu_mode_state.cpp")
    menu_labels = read_text("firmware/menu/menu_mode_labels.cpp")
    boot = read_text("firmware/output/runtime/output_boot_runtime.cpp")
    runtime = read_text("firmware/menu/pad_test_runtime.cpp")
    input_autodetect = read_text(
        "firmware/input/autodetect/input_autodetect_runtime.cpp")
    input_poll = read_text(
        "firmware/input/runtime/input_poll_runtime.cpp")
    main_display = read_text("firmware/menu/menu_main_display.cpp")
    status = read_text("firmware/menu/menu_main_display_status.cpp")
    platform_ui = read_text("firmware/platform/runtime/platform_runtime_ui.cpp")

    for source, label in (
        (output_enum, "output enum"),
        (output_state, "output canonicalization"),
        (output_catalog, "output catalog"),
        (menu_order, "output menu order"),
        (menu_labels, "output labels"),
        (boot, "USB boot path"),
    ):
        reject(source, "OUTPUT_PAD_TEST",
               f"Explicit Pad Test must be absent from {label}")

    for source, token, label in (
        (autodetect, "kAutoDetectNoDataHostFallbackMs = 1500;",
         "AUTO-only no-host fallback gate"),
        (autodetect, "tud_connected() || tud_mounted()",
         "USB data-host presence gate"),
        (runtime, "return auto_detect_no_data_host_fallback_active();",
         "No-host controller view shares AUTO fallback state"),
        (runtime, "constexpr uint32_t kNoHostRumblePortMask = 1UL << 0;",
         "Left-connector-only rumble mask"),
        (runtime, "controllerFrameConst(0)",
         "Left-connector-only controller service"),
        (input_autodetect, "!noDataHostFallback) {",
         "Auto Input remains gated until no-host fallback"),
        (input_autodetect, "kNoDataHostInputDisconnectDebounceMs = 1000;",
         "No-host resolved-input disconnect debounce"),
        (input_autodetect, "deferNoDataHostResolvedDisconnect(",
         "No-host resolved-input latch guard"),
        (input_poll, "!noHostControllerTestActive();",
         "No-host home view releases Auto Input"),
        (runtime, "INPUT_START | INPUT_A | INPUT_B",
         "PSX combined heavy-rumble ramp shortcut"),
        (runtime, "kNoHostRumblePortMask, 0, 255",
         "PSX light small-motor test"),
        (runtime, "kNoHostRumblePortMask, 255, 0",
         "PSX heavy large-motor test"),
        (runtime, "rumbleRuntimeStartHeavyRamp(kNoHostRumblePortMask);",
         "PSX heavy-motor ramp test"),
        (runtime, "kNoHostRumblePortMask, 255, 255",
         "N64 and GameCube binary rumble test"),
        (runtime, '"Start+X/O = Rumble"',
         "PSX home-screen rumble instruction"),
        (runtime, '"Start+A = Rumble"',
         "Generic home-screen rumble instruction"),
        (main_display, "const bool showNoHostController = noHostControllerTestActive();",
         "Normal home recognizes no-host controller state"),
        (main_display, "lastShowNoHostController != showNoHostController",
         "No-host transition redraws the normal home"),
        (status, "const char* rumbleHint = noHostControllerRumbleHint();",
         "Normal home bottom row renders rumble instruction"),
        (platform_ui, "if (updateNoHostControllerTest())",
         "No-host rumble shortcut service"),
    ):
        require(source, token, label)

    reject(main_display, "renderStandalonePadTest",
           "No-host mode must not replace the normal home renderer")
    reject(runtime, "controllerFrameConst(1)",
           "No-host controller service must ignore the right connector")
    reject(runtime, "kAllRumblePortsMask",
           "No-host controller service must never target every port")

    debounce_call = input_autodetect.index(
        "if (deferNoDataHostResolvedDisconnect(")
    disconnect_dispatch = input_autodetect.index(
        "return handleDisconnectedAutoDetectHotSwap(waitingForInitialResolve);")
    if debounce_call >= disconnect_dispatch:
        raise AssertionError(
            "No-host disconnect debounce must run before generic AUTO disconnect")

def test_release_firmware_contains_no_esp32_implementation_code() -> None:
    forbidden = ("ESP32", "esp32", "PicoOTA", "LittleFS", "WiFiClass")
    offenders: list[str] = []
    for path in (ROOT / "firmware").rglob("*"):
        if path.suffix not in {".c", ".cpp", ".h", ".hpp"}:
            continue
        text_value = path.read_text(encoding="utf-8")
        if any(token in text_value for token in forbidden):
            offenders.append(str(path.relative_to(ROOT)))
    if offenders:
        raise AssertionError(
            "ESP32/Wi-Fi/OTA code leaked into Classic2USB release: "
            + ", ".join(sorted(offenders))
        )
def test_classic2usb_switch_two_player_policy_and_ui_guards() -> None:
    factory = read_text("firmware/input/factory/input_adapter_factory.cpp")
    input_poll = read_text("firmware/input/runtime/input_poll_runtime.cpp")
    hotswap = read_text("firmware/input/autodetect/input_autodetect_hotswap_runtime.cpp")
    quick_state = read_text("firmware/menu/quick_config_state.cpp")
    quick_visibility = read_text("firmware/menu/quick_config_visibility.cpp")
    quick_actions = read_text("firmware/menu/quick_config_actions.cpp")
    quick_render = read_text("firmware/menu/quick_config_render.cpp")
    menu_catalog = read_text("firmware/menu/menu_catalog.cpp")
    output_boot = read_text("firmware/output/runtime/output_boot_runtime.cpp")

    reject(
        factory,
        "setPhysicalPortMask",
        "Classic2USB AUTO must keep both physical ports available",
    )
    for source, token, label in (
        (input_poll, "kN64NsoPlayerCountStableMs = 750",
         "N64 player-count transition debounce"),
        (input_poll, "preserveInputModeForPlayerCountReboot(deviceMode, inputPortCount());",
         "N64 descriptor reboot input preservation"),
        (hotswap, "automaticN64NsoProfileChanged",
         "AUTO N64 NSO descriptor transition"),
        (quick_state, "input_mode_allows_nso_special_for_connected_players(",
         "Quick menu effective NSO player-count gate"),
        (quick_visibility, 'case QCI_NSO_SPECIAL: return "NSO Mode (1P)";',
         "Quick menu one-player NSO label"),
        (quick_actions, "saveOrConfirmNsoSinglePlayer()",
         "Quick menu one-player confirmation gate"),
        (quick_render, 'display.println(F("Player 2 removed"));',
         "Quick menu Player 2 warning"),
        (menu_catalog, '{ menu_item_nso_special, "NSO Mode (1P)" },',
         "System menu one-player NSO label"),
        (output_boot,
         "nso_special && input_mode_allows_nso_special_for_connected_players(\n"
         "          inputMode, connectedPlayers)",
         "Native NSO one-slot player-count gate"),
    ):
        require(source, token, label)

def test_wii_classic_frames_and_analog_mouse_are_guarded() -> None:
    wii_poll = read_text("firmware/input/wii/input_wii_poll.cpp")
    wii_header = read_text("firmware/input/wii/Input_Wii.h")
    autodetect_boot = read_text("firmware/input/autodetect/input_autodetect_boot_runtime.cpp")
    autodetect_runtime = read_text("firmware/input/autodetect/input_autodetect_runtime.cpp")
    home_status = read_text("firmware/menu/menu_main_display_status.cpp")
    autodetect = read_text(
        "firmware/input/autodetect/input_autodetect_modern_probes.cpp"
    )
    trace_policy = read_text("firmware/menu/analog_stick_trace.h")
    layouts = read_text("firmware/menu/menu_pad_layouts_data.cpp")
    player_layout = read_text("firmware/menu/menu_pad_layouts_player.cpp")
    descriptor = read_text(
        "firmware/output/output_descriptors_generic_gamepad_runtime.h"
    )
    mapping = read_text("firmware/output/usb/output_hid_mapping_runtime.h")
    send = read_text("firmware/output/usb/output_usb_send_runtime.h")
    quick = read_text("firmware/menu/quick_config_visibility.cpp")
    settings = read_text("firmware/core/settings_store_per_mode_defaults.cpp")
    release = read_text("tools/run_release_checks.py")
    menu_input = read_text("firmware/menu/menu_input.cpp")
    quick_navigation = read_text("firmware/menu/quick_config_navigation.cpp")
    platform_menu = read_text(
        "firmware/platform/runtime/platform_menu_runtime.cpp"
    )
    home_render = read_text("firmware/menu/menu_main_display_pad_render.cpp")

    for token, label in (
        ("wii_extension_frame_is_corrupt", "All Wii extensions share corruption guard"),
        ("allZero || allOnes", "Wii all-zero/all-ones corruption guard"),
        ("otherwise plausible frames", "Wii extension variant acceptance"),
    ):
        require(wii_poll, token, label)
    require(
        wii_poll,
        ": wii[i]->connect();",
        "Wii runtime retains normal connect as the handoff fallback",
    )
    reject(
        wii_poll,
        "result = wii[i]->update() && !wii_extension_frame_is_corrupt(wii[i]);",
        "Wii runtime connection must leave report validation to normal polling",
    )
    reject(
        wii_poll,
        "constantByteIndex",
        "Wii Classic detection must not reject valid variants on a reserved bit",
    )
    require(
        autodetect,
        "wiiDetected = knownType;",
        "Wii autodetect accepts a known extension identity",
    )
    reject(
        autodetect,
        "readPlausibleControlFrame",
        "Wii autodetect must not require a premature first control frame",
    )
    reject(
        autodetect,
        "requestControlData",
        "Wii autodetect must leave report validation to normal polling",
    )

    require(trace_policy, 'strcmp(controllerType, "Nunchuk") == 0',
            "Nunchuk octagonal analog gate")
    for token, label in (
        ("constexpr int16_t kMenuAnalogNavigationThreshold = 24;",
         "Wii native-range analog navigation threshold"),
        ("menuControllerUpPressed(report)", "System-menu analog Up navigation"),
        ("menuControllerDownPressed(report)", "System-menu analog Down navigation"),
        ("report.LY <= -kMenuAnalogNavigationThreshold",
         "Analog-stick Up threshold"),
        ("report.LY >= kMenuAnalogNavigationThreshold",
         "Analog-stick Down threshold"),
        ("return report.PAD_L;", "Menu Left remains D-pad only"),
        ("return report.PAD_R;", "Menu Right remains D-pad only"),
    ):
        require(menu_input, token, label)
    for token, label in (
        ("menuControllerUpPressed(primary)", "Quick-menu analog Up navigation"),
        ("menuControllerDownPressed(primary)", "Quick-menu analog Down navigation"),
        ("menuControllerLeftPressed(primary)", "Quick-menu analog Left editing"),
        ("menuControllerRightPressed(primary)", "Quick-menu analog Right editing"),
    ):
        require(platform_menu, token, label)
    require(
        quick_navigation,
        "if (selected_index == 0) {\n          on_bottom_row = true;",
        "Quick-menu Up wraps from its first item to the last row",
    )
    for token, label in (
        ("constexpr int16_t shoulder = 10;", "Point-up home octagon shoulders"),
        ("corner * ax + shoulder * ay", "Point-up home octagon upper edge"),
        ("shoulder * ax + corner * ay", "Point-up home octagon side edge"),
        ("errorA <= edgeTolerance", "Solid point-up octagon upper edge"),
        ("errorB <= edgeTolerance", "Solid point-up octagon side edge"),
    ):
        require(home_render, token, label)
    require(layouts, "const PadButton padLayoutWiiNunchuk[]",
            "Nunchuk two-button layout")
    require(player_layout, 'std::strcmp(typeName, "Nunchuk") == 0',
            "Nunchuk player-specific OLED layout")
    nunchuk_layout = layouts.split("const PadButton padLayoutWiiNunchuk[]", 1)[1]
    nunchuk_layout = nunchuk_layout.split("};", 1)[0]
    if nunchuk_layout.count("{ GPAD_") != 2:
        raise AssertionError("Nunchuk OLED layout must contain exactly two buttons")

    if descriptor.count("HID_UNIT            ( 0 ),") != 2:
        raise AssertionError("Both mouse descriptors must clear inherited HID units")
    if descriptor.count("HID_PHYSICAL_MAX    ( 0 ),") != 2:
        raise AssertionError("Both mouse descriptors must clear inherited physical ranges")
    require(autodetect_boot, "if (mode == RZORD_WII)",
            "Wii runtime negotiation receives a dedicated auto-resolve grace")
    require(autodetect_boot, "return 5000;", "Wii five-second auto-resolve grace")
    require(wii_header, "WII_CONNECT_SETTLE_GRACE_US = 500000",
            "Wii post-connect report grace window")
    require(wii_poll, "time_us_32() - connectedAtUs[i]",
            "Wii startup grace is measured from successful connect")
    require(wii_poll, "setInputFrameConnected(frame, false);",
            "Wii runtime must not publish an unidentified extension")
    require(wii_poll, "conType != ExtensionType::NoController",
            "Wii runtime publishes only after extension identification")
    require(wii_header, "hasPhysicalConnectionForHotSwap() const override",
            "Wii physical connection keeps auto mode latched during identification")
    require(autodetect_runtime, "runHotswapWiiQuickPass",
            "Initial AUTO scans include Wii identity")
    require(autodetect_runtime, "detectAutoInputPortWiiOnly(port, true)",
            "Fast Wii AUTO scan uses the isolated identity probe")
    require(autodetect, "cacheWiiAutodetectIdentity(port, detectedPinPair, id);",
            "Wii autodetect preserves identity and working pin pair")
    require(wii_poll, "peekWiiAutodetectIdentity(i, &handoffPair, handoffIdentity)",
            "Wii runtime consumes the autodetect identity handoff")
    require(wii_poll, "wii[i]->connectFromIdentity(identity)",
            "Wii runtime avoids duplicate extension initialization")
    require(wii_poll, "if (++handoffFailCount[i] < 10)",
            "Wii identity handoff retries transient Classic setup failures")
    require(home_status, "return isAutoDetectMode && (connectedCount == 0);",
            "AUTO UI remains visible until a validated controller is published")

    if descriptor.count("TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(2))") != 2:
        raise AssertionError(
            "Both managed and clean generic HID descriptors must expose mouse report ID 2"
        )
    for token, label in (
        ("analog_mouse_mode == ANALOG_MOUSE_OFF", "Mouse disabled gate"),
        ("frame.HAS_ANALOG_STICK_MAIN", "Live primary-stick gate"),
        ("frame.HAS_ANALOG_STICK_AUX", "Live secondary-stick gate"),
        ("constexpr int16_t kAnalogMouseDeadzone = 24;", "Nunchuk neutral mouse dead zone"),
        ("ANALOG_MOUSE_SPEED_UNITY", "Shared pointer-speed scale"),
        ("analog_mouse_speed", "Live pointer-speed setting"),
        ("axis >= -kAnalogMouseDeadzone", "Analog mouse negative neutral drift guard"),
        ("axis <= kAnalogMouseDeadzone", "Analog mouse positive neutral drift guard"),
        (
            "if (frame.A) {\n    _hidmouse.buttons |= 0x01;",
            "Primary face-button left click",
        ),
        (
            "if (frame.B) {\n    _hidmouse.buttons |= 0x02;",
            "Secondary face-button right click",
        ),
    ):
        require(mapping, token, label)
    for token, label in (
        ("report_id = 2;", "Analog mouse HID report ID"),
        ("analogMouseRepeatDue", "Held-stick relative mouse repeat"),
        ("analogMouseDeltaX != 0 || analogMouseDeltaY != 0",
         "Analog mouse repeat ignores centered-stick drift"),
    ):
        require(send, token, label)
    for token, label in (
        ("addAnalogItem(QCI_ANALOG_MOUSE_MODE)", "Analog submenu mouse mode"),
        ("addAnalogItem(QCI_ANALOG_MOUSE_SPEED)", "Analog submenu pointer speed"),
        ("mouseOutput == OUTPUT_AUTO", "Unresolved AUTO mouse-setting visibility"),
        ("mouseOutput == OUTPUT_HID", "Windows/Linux HID guard"),
        ("mouseOutput == OUTPUT_MISTER", "MiSTer DInput guard"),
        ('return "Left Joy";', "Left-stick mouse label"),
        ('return "Right Joy";', "Right-stick mouse label"),
    ):
        require(quick, token, label)
    actions = read_text("firmware/menu/quick_config_actions.cpp")
    analog_select = actions.split(
        "void QuickConfigMenu::handleAnalogSelect()", 1)[1].split(
        "void QuickConfigMenu::handleAnalogSelectBack()", 1)[0]
    require(analog_select, "case QCI_ANALOG_MOUSE_MODE:",
            "Mouse mode must change through confirm/select")
    require(analog_select, "case QCI_ANALOG_MOUSE_SPEED:",
            "Pointer speed must change through confirm/select")
    require(
        settings,
        "settings.analog_mouse_mode = ANALOG_MOUSE_OFF;",
        "Analog mouse Off factory default",
    )
    require(
        release,
        '"tools/test_classic2usb_source_guards.py"',
        "Source guards remain mandatory release checks",
    )


def test_multiplayer_rumble_does_not_fall_back_across_ports() -> None:
    runtime = read_text("firmware/core/rumble_test_runtime.cpp")
    require(
        runtime,
        "effectiveLeft == 0 && effectiveRight == 0 &&\n"
        "      !output_runtime_has_secondary_player_slot()",
        "Global rumble fallback must be restricted to single-player outputs",
    )


if __name__ == "__main__":
    for name, fn in sorted(globals().items()):
        if name.startswith("test_"):
            fn()
