from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parent.parent


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def parse_bcd_platform_values(header: str) -> dict[str, int]:
    enum_body = header.split("enum bcd_input_platform_enum {", 1)[1].split("};", 1)[0]
    values: dict[str, int] = {}
    current = -1
    for raw_line in enum_body.splitlines():
        line = raw_line.split("//", 1)[0].strip().rstrip(",")
        if not line or not line.startswith("BCD_PLAT_"):
            continue
        if "=" in line:
            name, value = [part.strip() for part in line.split("=", 1)]
            current = int(value, 0)
        else:
            name = line
            current += 1
        values[name] = current
    return values


def parse_input_mode_identity_cases(identity: str) -> dict[str, str]:
    return {
        mode: platform
        for mode, platform in re.findall(
            r"case\s+(RZORD_[A-Z0-9_]+):\s+platform\s+=\s+(BCD_PLAT_[A-Z0-9_]+);\s+return true;",
            identity,
        )
    }


def pack_bcd_byte(decimal: int) -> int:
    if not 0 <= decimal <= 99:
        raise ValueError(decimal)
    return ((decimal // 10) << 4) | (decimal % 10)


def encode_bcd_device_version(revision: int, platform: int, platform_sub: int = 0) -> int:
    identity = platform + (25 * platform_sub)
    return (pack_bcd_byte(revision) << 8) | pack_bcd_byte(identity)


class AutoInputUsbIdentityTest(unittest.TestCase):
    def test_auto_boot_chimes_only_after_input_and_output_resolution(self):
        boot_ui = read("firmware/platform/boot/boot_ui_runtime.cpp")
        body = boot_ui.split(
            "void maybePlayResolvedBootJingle", 1
        )[1].split("\n}", 1)[0]
        self.assertIn("deviceMode != RZORD_AUTODETECT", body)
        self.assertIn("configuredOutputMode != OUTPUT_AUTO", body)
        self.assertIn("autoDetectState != AUTO_STATE_IDLE", body)
        self.assertIn("autoOutputResolvedBoot", body)
        self.assertIn(
            "inputModeResolvedAtBoot && outputModeResolvedAtBoot && !autoOutputProbeBoot",
            body,
        )

    def test_mister_input_identities_are_explicit_and_mode_specific(self):
        bridge = read("firmware/output/runtime/input_runtime_output_bridge.h")
        identity = read("firmware/output/runtime/input_usb_identity_runtime.h")

        values = parse_bcd_platform_values(bridge)
        cases = parse_input_mode_identity_cases(identity)
        expected = {
            "RZORD_N64": "BCD_PLAT_N64",
            "RZORD_GAMECUBE": "BCD_PLAT_GC",
            "RZORD_GBA": "BCD_PLAT_GBA",
            "RZORD_MEGADRIVE": "BCD_PLAT_MEGADRIVE",
            "RZORD_SATURN": "BCD_PLAT_SATURN",
            "RZORD_PCE": "BCD_PLAT_PCE",
            "RZORD_NES": "BCD_PLAT_NES",
            "RZORD_SNES": "BCD_PLAT_SNES",
            "RZORD_VBOY": "BCD_PLAT_VBOY",
            "RZORD_NEOGEO": "BCD_PLAT_NEOGEO",
            "RZORD_WII": "BCD_PLAT_WII",
            "RZORD_3DO": "BCD_PLAT_3DO",
            "RZORD_JAGUAR": "BCD_PLAT_JAGUAR",
            "RZORD_DREAMCAST": "BCD_PLAT_DREAMCAST",
            "RZORD_INTV": "BCD_PLAT_INTV",
            "RZORD_PADDLE": "BCD_PLAT_PADDLE",
            "RZORD_DRIVING": "BCD_PLAT_DRIVING",
            "RZORD_GAMEPORT": "BCD_PLAT_GAMEPORT",
            "RZORD_MEMCARD": "BCD_PLAT_MEMCARD",
            "RZORD_SMS": "BCD_PLAT_SMS",
            "RZORD_JPC": "BCD_PLAT_JPC",
            "RZORD_PSX": "BCD_PLAT_PSX",
            "RZORD_PSX_JOG": "BCD_PLAT_PSX",
            "RZORD_PSX_DANCE": "BCD_PLAT_PSX",
            "RZORD_JVS": "BCD_PLAT_JVS",
        }
        generic_bcd = encode_bcd_device_version(1, values["BCD_PLAT_N64"])

        self.assertEqual(generic_bcd, 0x0100)
        self.assertEqual(values["BCD_PLAT_SNES"], 7)
        self.assertEqual(
            encode_bcd_device_version(1, values["BCD_PLAT_SNES"]), 0x0107
        )
        self.assertEqual(encode_bcd_device_version(1, values["BCD_PLAT_WII"]), 0x0110)
        self.assertEqual(encode_bcd_device_version(1, values["BCD_PLAT_3DO"]), 0x0111)
        self.assertEqual(encode_bcd_device_version(1, values["BCD_PLAT_PSX"]), 0x0122)
        versions = set()
        for mode, platform in expected.items():
            self.assertEqual(cases.get(mode), platform, mode)
            mode_bcd = encode_bcd_device_version(1, values[platform])
            self.assertTrue(all(((mode_bcd >> shift) & 0xF) <= 9 for shift in (0, 4, 8, 12)))
            versions.add(mode_bcd)
        self.assertEqual(len(versions), len(set(expected.values())))

    def test_auto_input_hotplug_does_not_reboot_for_usb_identity(self):
        state = read("firmware/input/autodetect/input_autodetect_runtime_state.cpp")
        runtime_header = read("firmware/input/autodetect/input_autodetect_runtime.h")
        hotswap = read("firmware/input/autodetect/input_autodetect_hotswap_runtime.cpp")
        auto_boot = read("firmware/input/autodetect/input_autodetect_boot_runtime.cpp")
        input_boot = read("firmware/input/runtime/input_boot_runtime.cpp")

        self.assertIn("deviceMode = newMode", hotswap)
        self.assertIn("savedDeviceMode = assistedMode ? newMode : RZORD_AUTODETECT", hotswap)
        self.assertIn("initializeInputModuleForRuntimeModeChange()", hotswap)
        self.assertIn("webhid_update_device_mode(deviceMode)", hotswap)
        self.assertNotIn("autoInputNeedsUsbIdentityReenumeration", hotswap)
        self.assertNotIn("disconnectUsbForAutoInputIdentityChange", hotswap)
        self.assertNotIn("reconnectUsbAfterAutoInputIdentityChange", hotswap)
        self.assertNotIn("TinyUSBDevice.setDeviceVersion(bcd_device_version.composite)", hotswap)
        self.assertNotIn("tud_disconnect()", hotswap)
        self.assertNotIn("tud_connect()", hotswap)
        self.assertNotIn("reenumerateUsbForAutoInputIdentityChange", hotswap)
        self.assertNotIn("preserveAutoDetectUsbIdentityForReboot(newMode", hotswap)
        self.assertNotIn("preserveAutoDetectUsbIdentityForReboot", auto_boot)
        self.assertNotIn("kAutoInputResolveSourceUsbIdentityReenum", runtime_header)
        self.assertNotIn("input_auto_usb_identity_reenum_mode", state)
        self.assertNotIn("autoInputUsbIdentityEnumeratedForMode", hotswap)
        self.assertNotIn("kAutoInputScratchUsbIdentityDone", auto_boot)
        self.assertNotIn("kAutoInputScratchUsbIdentityDone", input_boot)
        self.assertNotIn("scratchUsbIdentityDone", input_boot)
        self.assertNotIn("restoreAutoUsbIdentityFromScratch", input_boot)
        self.assertNotIn("inputHotSwapPendingDisconnectRestore", hotswap)
        self.assertNotIn("clearInputHotSwapPendingDisconnectRestore", state)

    def test_resolved_mister_boot_selects_joybus_identity_before_usb_connect(self):
        firmware = read("firmware/core/runtime/firmware_runtime.cpp")
        input_boot = read("firmware/input/runtime/input_boot_runtime.cpp")

        setup = firmware.split("void runFirmwareSetup()", 1)[1]
        resolve_call = (
            "resolveAutomaticSwitchProfileInputAtBoot(\n"
            "      autoOutputBootState.autoOutputResolvedBoot);"
        )
        self.assertLess(setup.index(resolve_call), setup.index("configure_usb_output()"))

        resolver = input_boot.split(
            "void resolveAutomaticSwitchProfileInputAtBoot", 1
        )[1].split("\n}", 1)[0]
        self.assertIn("resolvedMisterIdentityBoot", resolver)
        self.assertIn(
            "output_is_generic_mister_hid_mode(get_effective_output_mode())",
            resolver,
        )
        self.assertIn(
            "!is_auto_output_mode_selected() || autoOutputResolvedBoot",
            resolver,
        )
        self.assertIn("runAutoDetectionJoybusOnly(true)", resolver)
        self.assertIn("detectedMode == RZORD_NONE", resolver)
        self.assertIn("deviceMode = detectedMode;", resolver)
        self.assertNotIn("reboot();", resolver)

    def test_input_scan_waits_for_auto_host_detection_to_resolve(self):
        runtime = read("firmware/input/autodetect/input_autodetect_runtime.cpp")
        output_state = read("firmware/output/output_runtime_state.cpp")

        gate = "is_auto_output_mode_selected() && autoDetectState == AUTO_STATE_IDLE"
        self.assertIn(gate, runtime)
        check_body = runtime.split("bool checkAutoDetectHotSwap()", 1)[1]
        self.assertLess(check_body.index(gate), check_body.index("waitingForInitialResolve"))
        self.assertIn("return configuredOutputMode == OUTPUT_AUTO;", output_state)
        self.assertIn("autoDetectState != AUTO_STATE_IDLE", output_state)

    def test_psx_specialty_hotplug_reenumerates_and_disconnect_restores_mister(self):
        hotswap = read("firmware/input/autodetect/input_autodetect_hotswap_runtime.cpp")
        runtime_state = read("firmware/output/output_runtime_state.cpp")

        self.assertIn("psxPeripheralHotSwapChangedUsbDescriptor", hotswap)
        self.assertIn("outputModeBeforeInputSetup", hotswap)
        self.assertIn(
            "psxPeripheralHotSwapChangedUsbDescriptor(outputModeBeforeInputSetup) ||",
            hotswap,
        )
        self.assertIn("restoreGenericMisterOutputAfterPsxPeripheralDisconnect", hotswap)
        self.assertIn("outputMode = OUTPUT_MISTER;", hotswap)
        self.assertIn("if (psxOutputDescriptorReset ||", hotswap)
        self.assertIn("autoHomeNeedsUsbDescriptorReenumeration())", hotswap)
        self.assertIn(
            "misterAutoOutputAcceptsRuntimeMode(outputMode)",
            runtime_state,
        )
        fallback_guard = runtime_state.split(
            "if (autoDetectState == AUTO_STATE_FALLBACK_HID", 1
        )[1].split("autoDetectProbeStage = AUTO_PROBE_GENERIC;", 1)[0]
        self.assertIn(
            "!misterAutoOutputAcceptsRuntimeMode(outputMode)",
            fallback_guard,
        )
        self.assertIn("case OUTPUT_MISTER_JOGCON:", runtime_state)
        self.assertIn("case OUTPUT_MISTER_NEGCON:", runtime_state)
        self.assertIn("case OUTPUT_MISTER_GUNCON:", runtime_state)

    def test_switch_profile_hotplug_reenumerates_and_disconnect_restores_pro(self):
        hotswap = read("firmware/input/autodetect/input_autodetect_hotswap_runtime.cpp")
        capabilities = read("firmware/output/output_capabilities.h")
        runtime_state = read("firmware/output/output_runtime_state.cpp")
        switch_setup = read("firmware/output/usb/output_usb_mode_setup_console_runtime.h")

        self.assertIn("switchProfileHotSwapChangedUsbDescriptor", hotswap)
        self.assertIn(
            "const switchpro_mode_enum switchProfileBeforeInputSetup = SwitchCommon::switchpro_mode;",
            hotswap,
        )
        self.assertIn(
            "switchProfileHotSwapChangedUsbDescriptor(switchProfileBeforeInputSetup) ||",
            hotswap,
        )
        apply_body = hotswap.split("void applyDetectedInputModeLive", 1)[1]
        apply_body = apply_body.split("\n}", 1)[0]
        initialize_profile = apply_body.index(
            "output_apply_automatic_switch_profile_for_input_mode(deviceMode);"
        )
        self.assertGreater(
            initialize_profile,
            apply_body.index("initializeInputModuleForRuntimeModeChange();"),
        )
        self.assertLess(
            initialize_profile,
            apply_body.index(
                "switchProfileHotSwapChangedUsbDescriptor(switchProfileBeforeInputSetup)"
            ),
        )
        restore_body = hotswap.split("void restoreAutoDetectHomeAfterDisconnect", 1)[1]
        restore_body = restore_body.split("\n}", 1)[0]
        self.assertIn("output_reset_switch_profile_to_default();", restore_body)
        self.assertIn("switchProfileDescriptorReset ||", restore_body)
        self.assertIn("reenumerateUsbForAutoHomePlayerCountChange()", restore_body)

        configure_body = switch_setup.split(
            "static void configure_switchpro_output_runtime()", 1
        )[1].split("\n}", 1)[0]
        select_profile = configure_body.index(
            "output_apply_automatic_switch_profile_for_input_mode(deviceMode);"
        )
        self.assertLess(
            select_profile,
            configure_body.index("switch (SwitchCommon::switchpro_mode)"),
        )

        profile_body = capabilities.split(
            "inline switchpro_mode_enum output_switch_profile_for_input_mode", 1
        )[1].split("\n}", 1)[0]
        self.assertIn("if (!nsoSpecialEnabled)", profile_body)
        for mode, profile in (
            ("RZORD_NES", "SWITCHPRO_NES"),
            ("RZORD_SNES", "SWITCHPRO_SNES"),
            ("RZORD_MEGADRIVE", "SWITCHPRO_GENESIS"),
            ("RZORD_N64", "SWITCHPRO_N64"),
        ):
            self.assertIn(f"case {mode}:", profile_body)
            self.assertIn(f"return {profile};", profile_body)

        supported_body = runtime_state.split(
            "bool input_mode_supports_nso_special", 1
        )[1].split("\n}", 1)[0]
        for mode in ("RZORD_NES", "RZORD_SNES", "RZORD_MEGADRIVE", "RZORD_N64"):
            self.assertIn(f"case {mode}:", supported_body)
        self.assertNotIn("case RZORD_GAMECUBE:", supported_body)

        automatic_body = runtime_state.split(
            "bool input_mode_has_automatic_nso_profile", 1
        )[1].split("\n}", 1)[0]
        self.assertIn("return mode == RZORD_N64;", automatic_body)
        for mode in ("RZORD_NES", "RZORD_SNES", "RZORD_MEGADRIVE"):
            self.assertNotIn(f"case {mode}:", automatic_body)

    def test_n64_profile_handoff_preserves_resolved_switch_and_input(self):
        hotswap = read(
            "firmware/input/autodetect/input_autodetect_hotswap_runtime.cpp")
        output_state = read("firmware/output/output_runtime_state.cpp")
        output_auto = read(
            "firmware/output/autodetect/output_autodetect_runtime.cpp")
        output_boot = read(
            "firmware/output/runtime/output_boot_runtime.cpp")
        input_boot = read(
            "firmware/input/runtime/input_boot_runtime.cpp")

        handoff = hotswap.split(
            "void reenumerateUsbForHotSwapPlayerCountChange", 1
        )[1].split("\n}", 1)[0]
        preserve_host = handoff.index(
            "auto_detect_preserve_known_runtime_mode_for_input_reboot();")
        preserve_input = handoff.index(
            "preserveInputModeForPlayerCountReboot(deviceMode, inputPortCount());")
        reboot = handoff.index(chr(10) + "  reboot();")
        self.assertLess(preserve_host, preserve_input)
        self.assertLess(preserve_input, reboot)
        self.assertIn(
            "if (is_auto_output_mode_selected() && !outputPreserved)",
            handoff,
        )

        resolved = output_state.split(
            "bool auto_detect_preserve_resolved_runtime_mode_for_reboot()", 1
        )[1].split("\n}", 1)[0]
        self.assertIn(
            "auto_detect_persist_runtime_mode(outputMode, autoDetectState);",
            resolved,
        )

        pre_output_reenum = output_auto.split(
            "static void auto_detect_preserve_auto_input_before_output_reenum", 1
        )[1].split("\n}", 1)[0]
        self.assertNotIn("runAutoDetection(", pre_output_reenum)

        output_restore = output_boot.split(
            "auto_output_boot_state_t restoreAutoOutputBootState()", 1
        )[1].split("\n}", 1)[0]
        self.assertLess(
            output_restore.index("restoreDetectedAutoOutputModeAtBoot();"),
            output_restore.index("clearAutoOutputScratchRegisters();"),
        )

        input_restore = input_boot.split(
            "void restoreChainedAutoInputModeFromScratchRegisters()", 1
        )[1].split("\n}", 1)[0]
        self.assertIn("deviceMode = chainedMode;", input_restore)
        self.assertIn(
            "setInputAutoDetectModeActive(savedDeviceMode == RZORD_AUTODETECT);",
            input_restore,
        )

    def test_resolved_switch_boot_selects_nso_input_before_usb_connect(self):
        firmware = read("firmware/core/runtime/firmware_runtime.cpp")
        input_boot = read("firmware/input/runtime/input_boot_runtime.cpp")
        input_runtime = read("firmware/input/autodetect/input_autodetect_runtime.cpp")
        detect_port = read("firmware/input/autodetect/input_autodetect_detect_port.cpp")
        output_boot = read("firmware/output/runtime/output_boot_runtime.cpp")

        setup = firmware.split("void runFirmwareSetup()", 1)[1]
        resolve_call = (
            "resolveAutomaticSwitchProfileInputAtBoot(\n"
            "      autoOutputBootState.autoOutputResolvedBoot);"
        )
        self.assertIn(resolve_call, setup)
        self.assertLess(
            setup.index("restoreAutoOutputBootState()"), setup.index(resolve_call)
        )
        self.assertLess(
            setup.index(resolve_call), setup.index("initializeRuntimeSettingsFromStorage()")
        )
        self.assertLess(
            setup.index(resolve_call), setup.index("initializeInputModuleForBoot()")
        )
        self.assertLess(setup.index(resolve_call), setup.index("configure_usb_output()"))

        resolver = input_boot.split(
            "void resolveAutomaticSwitchProfileInputAtBoot", 1
        )[1].split("\n}", 1)[0]
        for gate in (
            "savedDeviceMode != RZORD_AUTODETECT",
            "deviceMode != RZORD_AUTODETECT",
        ):
            self.assertIn(gate, resolver)
        self.assertIn(
            "autoOutputResolvedBoot && output_runtime_is_switchpro_mode()",
            resolver,
        )
        self.assertIn(
            "if (!resolvedSwitchProfileBoot && !resolvedMisterIdentityBoot)",
            resolver,
        )
        self.assertIn("runAutoDetectionJoybusOnly(true)", resolver)
        self.assertIn("input_mode_has_automatic_nso_profile(detectedMode)", resolver)
        self.assertIn("deviceMode = detectedMode;", resolver)
        self.assertIn("setInputAutoDetectModeActive(true);", resolver)
        self.assertNotIn("reboot();", resolver)

        joybus_only = input_runtime.split(
            "DeviceEnum runAutoDetectionJoybusOnly", 1
        )[1].split("\n}", 1)[0]
        self.assertIn("detectAutoInputPortJoybusOnly", joybus_only)
        self.assertNotIn("detectAutoInputPort(", joybus_only)
        self.assertIn("INPUT_MIXED_PORT_COUNT", joybus_only)
        self.assertIn("AutoDetector::detectPortJoybusOnly", detect_port)

        boot_state = output_boot.split(
            "auto_output_boot_state_t getAutoOutputBootStateFromScratch()", 1
        )[1].split("\n}", 1)[0]
        self.assertIn(
            "watchdog_hw->scratch[AUTO_DETECT_SCRATCH_STATE] & 0xFFu",
            boot_state,
        )

    def test_native_nso_identities_use_one_usb_controller_interface(self):
        output_boot = read("firmware/output/runtime/output_boot_runtime.cpp")
        slot_count = output_boot.split(
            "uint8_t classic2usbBootUsbSlotCountForInput", 1
        )[1].split("\n}", 1)[0]

        self.assertIn(
            "canonicalizeOutputMode(mode) == OUTPUT_SWITCHPRO",
            slot_count,
        )
        self.assertIn("uint8_t connectedPlayers", slot_count)
        self.assertIn(
            "nso_special && input_mode_allows_nso_special_for_connected_players(\n"
            "          inputMode, connectedPlayers)",
            slot_count,
        )
        self.assertNotIn("inputMode == RZORD_N64", slot_count)
        self.assertIn("return 1;", slot_count)
        nso_guard = slot_count.index("output_switch_profile_for_input_mode")
        reserve_slots = slot_count.index("if (shouldReserveClassic2usbUsbSlots(mode))")
        self.assertLess(nso_guard, reserve_slots)

    def test_manual_input_modes_still_reboot_when_bcd_identity_changes(self):
        mode_save = read("firmware/core/settings_store_mode_save.cpp")

        self.assertIn("bool isManualInputOnlyModeChange", mode_save)
        self.assertIn("output_is_generic_mister_hid_mode(get_effective_output_mode())", mode_save)
        self.assertIn("inputModeNeedsBcdDeviceReenumeration(selection.newInputMode)", mode_save)
        self.assertIn("return false;", mode_save)

    def test_auto_home_disconnect_does_not_reboot_for_mode_identity_only(self):
        hotswap = read("firmware/input/autodetect/input_autodetect_hotswap_runtime.cpp")

        self.assertNotIn("autoHomeDisconnectNeedsUsbReenumeration", hotswap)
        restore_body = hotswap.split("void restoreAutoDetectHomeAfterDisconnect", 1)[1]
        restore_body = restore_body.split("\n}", 1)[0]
        self.assertIn("forceMainDisplayRefresh()", restore_body)
        self.assertIn("initializeInputModuleForRuntimeModeChange()", restore_body)
        self.assertIn("clearAutoDetectDisconnectedFrames()", restore_body)
        self.assertIn("autoHomeNeedsUsbDescriptorReenumeration()", restore_body)
        self.assertIn("reenumerateUsbForAutoHomePlayerCountChange()", restore_body)
        self.assertNotIn("disconnectedMode > RZORD_NONE", restore_body)

    def test_auto_resolved_snes_family_missed_edge_can_restore_auto_home(self):
        hotswap = read("firmware/input/autodetect/input_autodetect_hotswap_runtime.cpp")

        self.assertNotIn("autoResolvedShiftRegisterModeAwaitingFirstFrame", hotswap)
        disconnected_body = hotswap.split("bool handleDisconnectedAutoDetectHotSwap", 1)[1]
        missed_edge_body = disconnected_body.split("If the first disconnected edge was missed", 1)[1]
        restore_to_auto = missed_edge_body.index("restoreAutoDetectModeForDisconnect()")
        self.assertLess(missed_edge_body.index("const DeviceEnum disconnectedMode = deviceMode"), restore_to_auto)

    def test_auto_home_hotplug_probes_are_not_suspended_by_idle_ui(self):
        hotswap = read("firmware/input/autodetect/input_autodetect_hotswap_runtime.cpp")

        self.assertNotIn("if (inputAutoDetectSuspendedForIdleUi())", hotswap)
        self.assertEqual(
            hotswap.count("if (!waitingForInitialResolve && inputAutoDetectSuspendedForIdleUi())"),
            2,
        )

    def test_auto_hotplug_trace_marks_probe_timing_and_blockers(self):
        benchmark_header = read("firmware/input/autodetect/input_autodetect_benchmark.h")
        benchmark = read("firmware/input/autodetect/input_autodetect_benchmark.cpp")
        hotswap = read("firmware/input/autodetect/input_autodetect_hotswap_runtime.cpp")
        serial_debug = read("firmware/core/serial_debug_runtime.cpp")

        expected_hotswap_events = [
            "ADBENCH_HOTSWAP_WAIT_DISCONNECT_DELAY",
            "ADBENCH_HOTSWAP_SKIP_IDLE_UI",
            "ADBENCH_HOTSWAP_WAIT_DUE",
            "ADBENCH_HOTSWAP_QUICK_SCAN_START",
            "ADBENCH_HOTSWAP_QUICK_SCAN_END",
            "ADBENCH_HOTSWAP_FULL_SCAN_START",
            "ADBENCH_HOTSWAP_FULL_SCAN_END",
            "ADBENCH_HOTSWAP_CONNECTED_SCAN_START",
            "ADBENCH_HOTSWAP_CONNECTED_SCAN_END",
            "ADBENCH_HOTSWAP_WAIT_AUTO_REVERTED",
            "ADBENCH_HOTSWAP_CLEAR_AUTO_REVERTED",
        ]
        for event in expected_hotswap_events:
            self.assertIn(event, benchmark_header)
            self.assertIn(event.replace("ADBENCH_", ""), benchmark)
            self.assertIn(event, hotswap)

        self.assertIn("ADBENCH_SERIAL_AUTO_REQUEST", benchmark_header)
        self.assertIn("SERIAL_AUTO_REQUEST", benchmark)

        autodetect = read("firmware/input/autodetect/input_autodetect_runtime.cpp")
        self.assertIn("ADBENCH_HOTSWAP_CHECK_ENTER", benchmark_header)
        self.assertIn("HOTSWAP_CHECK_ENTER", benchmark)
        self.assertIn("ADBENCH_HOTSWAP_CHECK_ENTER", autodetect)
        self.assertIn('serialTokenEquals(text, "AUTO")', benchmark)
        self.assertIn("queueAutoDetectRuntimeBenchmark", benchmark)
        self.assertLess(
            serial_debug.index('serialCommandStartsWith(command, "ADBENCH"'),
            serial_debug.index("handleAutodetectSerialCommand(command, out)"),
        )

    def test_auto_reverted_latch_is_not_permanent(self):
        hotswap = read("firmware/input/autodetect/input_autodetect_hotswap_runtime.cpp")

        reverted_body = hotswap.split(
            "if (waitingForInitialResolve && inputHotSwapRevertedToAutoWhileDisconnected())", 1
        )[1].split("\n  }\n", 1)[0]
        self.assertIn("inputHotSwapDetectDue(now)", reverted_body)
        self.assertIn("ADBENCH_HOTSWAP_WAIT_AUTO_REVERTED", reverted_body)
        self.assertIn("ADBENCH_HOTSWAP_CLEAR_AUTO_REVERTED", reverted_body)
        self.assertIn("markInputHotSwapRevertedToAutoWhileDisconnected(false)", reverted_body)
        self.assertIn("skipDisconnectDelay = true", reverted_body)
        disconnected_body = hotswap.split("bool handleDisconnectedAutoDetectHotSwap", 1)[1]
        self.assertLess(
            disconnected_body.index("inputHotSwapRevertedToAutoWhileDisconnected()"),
            disconnected_body.index("shouldDeferAutoDetectHotSwap"),
        )
        self.assertIn("!skipDisconnectDelay", disconnected_body)


if __name__ == "__main__":
    unittest.main()
