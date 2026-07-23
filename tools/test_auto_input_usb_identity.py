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


class AutoInputUsbIdentityTest(unittest.TestCase):
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
        generic_bcd = (1 << 10) | (values["BCD_PLAT_N64"] << 4)

        self.assertEqual(generic_bcd, 0x0400)
        self.assertEqual(values["BCD_PLAT_SNES"], 7)
        self.assertEqual((1 << 10) | (values["BCD_PLAT_SNES"] << 4), 0x0470)
        for mode, platform in expected.items():
            self.assertEqual(cases.get(mode), platform, mode)
            mode_bcd = (1 << 10) | (values[platform] << 4)
            if mode != "RZORD_N64":
                self.assertNotEqual(mode_bcd, generic_bcd, mode)

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

        self.assertIn("psxPeripheralHotSwapChangedUsbDescriptor", hotswap)
        self.assertIn("outputModeBeforeInputSetup", hotswap)
        self.assertIn(
            "psxPeripheralHotSwapChangedUsbDescriptor(outputModeBeforeInputSetup) ||",
            hotswap,
        )
        self.assertIn("restoreGenericMisterOutputAfterPsxPeripheralDisconnect", hotswap)
        self.assertIn("outputMode = OUTPUT_MISTER;", hotswap)
        self.assertIn("psxOutputDescriptorReset || autoHomeNeedsUsbDescriptorReenumeration()", hotswap)

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
