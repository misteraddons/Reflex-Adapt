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


def test_gamecube_z_and_triggers_are_distinct() -> None:
    source = read_text("firmware/input/gc64/input_gc64_poll.cpp")
    button_masks = read_text("firmware/menu/menu_pad_button_masks.cpp")
    graphics = read_text("firmware/menu/controller_graphics_controllers.cpp")
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
        "reserved_mouse_sensitivity), 5, 5, 5,",
        "Retired mouse sensitivity setting must be immutable",
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


def test_n64_analog_learn_expands_raw_range_to_full_scale() -> None:
    n64_header = read_text("firmware/input/gc64/Input_GC64.h")
    n64_poll = read_text("firmware/input/gc64/input_gc64_poll.cpp")
    range_header = read_text("firmware/core/classic_analog_range.h")
    range_impl = read_text("firmware/core/classic_analog_range.cpp")
    finalize = read_text("firmware/core/controller_runtime_output_finalize.cpp")
    visibility = read_text("firmware/menu/menu_helpers_visibility.cpp")
    quick_visibility = read_text("firmware/menu/quick_config_visibility.cpp")
    quick_render = read_text("firmware/menu/quick_config_render.cpp")

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
        'case QCI_RANGE_TEST: return isN64(temp_input_mode) ? "Raw Stick" : "Range Test";',
        "N64 should label the analog diagnostic as raw stick test",
    )
    require(
        quick_render,
        '= N64 Raw Stick =',
        "N64 raw stick test should show a dedicated diagnostic screen",
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


def test_genesis_connected_polling_uses_dedicated_fast_cadence() -> None:
    header = read_text("firmware/input/saturn/Input_Saturn.h")
    poll = read_text("firmware/input/saturn/input_saturn_poll.cpp")
    saturn_lib = read_text("third_party/firmware_libraries/SaturnLib/SaturnLib.h")

    require(
        header,
        "MEGADRIVE_CONNECTED_POLL_INTERVAL_US = 125",
        "Direct Genesis pads should use the tested faster connected polling cadence",
    )
    require(
        header,
        "SATURN_CONNECTED_POLL_INTERVAL_US = 125",
        "Saturn pads should keep the existing 1 ms connected polling cadence",
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
        "const bool megadriveMode = getInternalMode() == 1;",
        "Genesis cadence should only apply when the Saturn module is in Megadrive mode",
    )
    require(
        poll,
        "found_fast_megadrive = true;",
        "Genesis direct pads should select the faster cadence separately from Saturn pads",
    )
    require(
        poll,
        "pollInterval = MEGADRIVE_CONNECTED_POLL_INTERVAL_US;",
        "Genesis direct pads should apply the dedicated connected polling cadence",
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
        "MEGA6_EXTRA_PAGE_GRACE_POLLS = 8",
        "MEGA6_TYPE_GRACE_POLLS = 2048",
        "sixbtn_signature_hits",
        "sixbtn_confirmed",
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
    require(
        poll,
        "(uint32_t)(now - megadriveTypeDowngradeStartedMs[slot]) >=",
        "Mega6 downgrade debounce must expire instead of restarting every poll",
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
    require(
        capabilities,
        "return output_allows_management_usb_endpoints(effectiveMode) ||\n"
        "         output_is_specialized_mister_psx_mode(effectiveMode);",
        "Specialized MiSTer PSX modes should expose management MSC",
    )
    require(
        auth_msc,
        "return output_allows_management_msc(effectiveOutputMode);",
        "MSC setup should use the dedicated specialized-mode capability",
    )



def test_wii_i2c_probes_both_pin_pairs_and_locks_runtime_pair() -> None:
    wii_header = read_text("firmware/input/wii/Input_Wii.h")
    wii_poll = read_text("firmware/input/wii/input_wii_poll.cpp")
    autodetect_header = read_text("firmware/input/autodetect/Input_AutoDetect.h")
    autodetect_support = read_text("firmware/input/autodetect/input_autodetect_support.cpp")
    autodetect_probe = read_text("firmware/input/autodetect/input_autodetect_modern_probes.cpp")

    require(wii_header, "pinPairs[INPUT_WII_PIN_PAIR_COUNT]", "Wii runtime should carry multiple I2C pin pairs")
    require(wii_header, "activePinPair[input_ports]", "Wii runtime should lock the detected I2C pin pair")
    require(wii_header, "WII_CONNECTED_POLL_INTERVAL_US = 500", "Wii connected polling should run every 500 us")
    require(wii_header, "WII_EMPTY_PORT_CONNECT_INTERVAL_US = 500000", "Empty Wii ports should retry every 500 ms")
    require(wii_header, "WII_UPDATE_FAIL_DISCONNECT_THRESHOLD = 32", "Wii disconnect should tolerate brief I2C error bursts")
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
    require(
        hid_setup,
        "management_configuration_buffer[512]",
        "Every management composite should use a 512-byte TinyUSB configuration buffer",
    )
    require(
        hid_setup,
        "TinyUSBDevice.setConfigurationBuffer(",
        "Every management composite should install its expanded configuration buffer",
    )
    reserve_start = boot_output.find("bool shouldReserveClassic2usbUsbSlots")
    reserve_end = boot_output.find("bool classic2usbInputCanReserveDetectedUsbSlots", reserve_start)
    reserve_body = boot_output[reserve_start:reserve_end]
    require(
        reserve_body,
        "case OUTPUT_XINPUT2P:",
        "Classic2USB XInput2P should reserve both wired USB controller slots",
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
    require(setup, "if (rumbleConfiguredProto[i] == proto)", "PSX rumble init should not repeat for an unchanged protocol")
    require(setup, "psx[i]->setRumble(false, 0);\n  psx[i]->read();", "PSX stopRumble should flush a zero-rumble read")
    require(setup, "stopRumble(i);\n      if (rumbleEnabled)", "PSX rumble init should end with a flushed stop command")
    require(poll, "clearPhysicalFallbackLatches();\n        tryEnableAnalogMode(i);", "PSX hotplug should do a single analog-enable attempt")
    reject(poll, "shouldRestoreDualShockAnalogMode", "PSX polling should not spam config commands to restore analog")


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
    require_file("firmware/menu/screensaver_rflx_bitmap.h", "RFLX screensaver bitmap")
    bitmap = read_text("firmware/menu/screensaver_rflx_bitmap.h")

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
    reject(bounce, "u8g2.drawStr", "Bouncing screensaver should not draw a text logo")


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
    require(settings_store, "KioskMode,\n  Count", "Kiosk setting id should be part of persisted settings")
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
    require(button_handler, "BTN_LONG_PRESS_MS    3000", "Mode and Reset long press should be 3 seconds")
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


if __name__ == "__main__":
    for name, fn in sorted(globals().items()):
        if name.startswith("test_"):
            fn()
