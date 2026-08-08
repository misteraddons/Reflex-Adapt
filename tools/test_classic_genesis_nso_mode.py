from __future__ import annotations

import hashlib
import json
import os
import shlex
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TEST = ROOT / "host_tests" / "classic_genesis_nso_mode_tests.cpp"
BUILD_DIR = ROOT / "build" / "host_tests"
OUTPUT = BUILD_DIR / (
    "classic_genesis_nso_mode_tests.exe"
    if os.name == "nt" else "classic_genesis_nso_mode_tests"
)

OFFICIAL_FW420_BUTTON_TABLE = bytes.fromhex(
    "00000200000001000000040000000800"
    "00200000001000000002000080000000"
    "08000000040000004000000001000000"
)
OFFICIAL_FW420_EVENT_BITS = (
    1 << 17, 1 << 16, 1 << 18, 1 << 19,
    1 << 13, 1 << 12, 1 << 9, 1 << 7,
    1 << 3, 1 << 2, 1 << 6, 1 << 0,
)


def source_guards() -> None:
    saturn_input = (
        ROOT / "firmware" / "input" / "saturn" / "input_saturn_poll.cpp"
    ).read_text(encoding="utf-8")
    saturn_header = (
        ROOT / "firmware" / "input" / "saturn" / "Input_Saturn.h"
    ).read_text(encoding="utf-8")
    saturn_setup = (
        ROOT / "firmware" / "input" / "saturn" / "input_saturn_setup.cpp"
    ).read_text(encoding="utf-8")
    dual_merge = (
        ROOT / "firmware" / "core" / "classic_dual_merge_config.cpp"
    ).read_text(encoding="utf-8")
    pad_layouts = (
        ROOT / "firmware" / "menu" / "menu_pad_layouts_player.cpp"
    ).read_text(encoding="utf-8")
    saturn_lib = (
        ROOT / "third_party" / "firmware_libraries"
        / "SaturnLib" / "SaturnLib.h"
    ).read_text(encoding="utf-8")
    megadrive_decode = (
        ROOT / "third_party" / "firmware_libraries"
        / "SaturnLib" / "MegadriveSixButtonDecode.h"
    ).read_text(encoding="utf-8")
    mapping = (
        ROOT / "firmware" / "output" / "switch"
        / "output_switchpro_mapping_runtime.h"
    ).read_text(encoding="utf-8")
    genesis_mapping = (
        ROOT / "firmware" / "output" / "switch"
        / "output_switch_genesis_nso_mapping.h"
    ).read_text(encoding="utf-8")
    setup = (
        ROOT / "firmware" / "output" / "usb"
        / "output_usb_mode_setup_console_runtime.h"
    ).read_text(encoding="utf-8")
    subcommands = (
        ROOT / "firmware" / "output" / "switch"
        / "output_switch_subcommand_runtime.cpp"
    ).read_text(encoding="utf-8")

    input_tokens = (
        "frame.A = sc.digitalPressed(SAT_A);",
        "frame.B = sc.digitalPressed(SAT_B);",
        "frame.X = sc.digitalPressed(SAT_X);",
        "frame.Y = sc.digitalPressed(SAT_Y);",
        "frame.L1 = sc.digitalPressed(SAT_Z);",
        "frame.R1 = sc.digitalPressed(SAT_C);",
        "frame.R2 = sc.digitalPressed(SAT_R);",
        "frame.START = sc.digitalPressed(SAT_START);",
        "frame.HOME = sc.digitalPressed(SAT_M30_HOME);",
        "frame.CAPTURE = sc.digitalPressed(SAT_M30_STAR);",
    )
    for token in input_tokens:
        if token not in saturn_input:
            raise AssertionError(
                f"Classic2USB Genesis neutral mapping is missing {token}")
    if "//11MXYZ" not in saturn_lib or (
            "setControlValues(sc, 2, sixButtonExtraPage & 0b00001111);"
            not in saturn_lib):
        raise AssertionError(
            "Classic2USB must decode Mode/X/Y/Z from the qualified Mega6 page")
    for token in (
        "saturnlib_megadrive::six_button_id_phase(nibble_0)",
        "saturnlib_megadrive::six_button_marker_valid(",
        "saturnlib_megadrive::m30_aux_control_page(",
        "SAT_M30_HOME  = 0x1000",
        "SAT_M30_STAR  = 0x4000",
    ):
        if token not in saturn_lib:
            raise AssertionError(
                f"Classic2USB Mega6 production path is missing {token}")
    for token in (
        "const uint8_t fixed_mask = allow_m30_auxiliary ? 0x0A : 0x0F;",
        "return marker_page & 0x0F;",
    ):
        if token not in megadrive_decode:
            raise AssertionError(
                f"Classic2USB M30 auxiliary decoder is missing {token}")
    if (
        "if (dtype[i] == SAT_DEVICE_MEGA6) {\n"
        "              frame.HOME = sc.digitalPressed(SAT_M30_HOME);\n"
        "              frame.CAPTURE = sc.digitalPressed(SAT_M30_STAR);"
        not in saturn_input
    ):
        raise AssertionError(
            "M30 auxiliary controls must remain scoped to qualified Mega6 pads")
    for token in (
        "m30Identity[i].observe(\n"
        "          detectedType == SAT_DEVICE_MEGA6,\n"
        "          sc.digitalPressed(SAT_M30_HOME),\n"
        "          sc.digitalPressed(SAT_M30_STAR));",
        'frame, m30Identity[i].identified() ? "M30" : "Mega6");',
        "if (m30JustIdentified) {\n"
        '          setInputFrameTypeName(frame, "M30");',
        "m30Identity[i].reset();",
    ):
        if token not in saturn_input:
            raise AssertionError(
                f"Classic2USB sticky M30 identity is missing {token}")
    if "M30IdentityLatch m30Identity[MAX_USB_OUT]" not in saturn_header:
        raise AssertionError("Classic2USB must keep M30 identity per input slot")
    if "m30Identity[i].reset();" not in saturn_setup:
        raise AssertionError("M30 identity must start clear when the module initializes")
    if saturn_input.count("m30Identity[i].reset();") < 2 or (
            "m30Identity[slotBase].reset();" not in saturn_input):
        raise AssertionError(
            "Every Saturn slot-disconnect cleanup path must clear M30 identity")
    for source, description in (
        (dual_merge, "Classic dual-merge masks"),
        (pad_layouts, "Controller display layout"),
    ):
        if 'strcmp(controllerTypeName, "M30")' not in source and (
                'strcmp(typeName, "M30")' not in source):
            raise AssertionError(
                f"{description} must treat M30 like a Mega6 controller")

    if '#include "output_switch_genesis_nso_mapping.h"' not in mapping:
        raise AssertionError("Switch output must include the Genesis NSO packer")
    call = (
        "switch_genesis_nso::apply_button_bits(\n"
        "      frame, switchpro[port]->switchCommon->_switchReport);"
    )
    if call not in mapping:
        raise AssertionError(
            "Classic2USB Genesis output must use the tested NSO packer")
    for token in (
        "switch_genesis_nso::apply_six_button_position_bits(",
        "switch_gamecube::map_left_shoulder(",
        "gamecube_l_switch_mode",
    ):
        if token not in mapping:
            raise AssertionError(
                f"Classic2USB Switch mapping is missing {token}")
    if "frame.HOME || frame.L2" in mapping:
        raise AssertionError(
            "M30 Home must not alias the generic L2 shoulder")
    if mapping.index(call) > mapping.index(
            "int16_t lx = convertAnalogPrecision"):
        raise AssertionError(
            "Genesis packet must be applied before D-pad mode processing")
    for token, description in (
        ("if (frame.A) bits |= kY;", "Genesis A -> Switch Pro Y"),
        ("if (frame.B) bits |= kB;", "Genesis B -> Switch Pro B"),
        ("if (frame.R1) bits |= kA;", "Genesis C -> Switch Pro A"),
        ("if (frame.X) bits |= kL;", "Genesis X -> Switch Pro L"),
        ("if (frame.Y) bits |= kX;", "Genesis Y -> Switch Pro X"),
        ("if (frame.L1) bits |= kR;", "Genesis Z -> Switch Pro R"),
        ("if (frame.R1) bits |= kY;", "PCE III -> Switch Pro Y"),
        ("if (frame.A) bits |= kB;", "PCE II -> Switch Pro B"),
        ("if (frame.B) bits |= kA;", "PCE I -> Switch Pro A"),
        ("if (frame.L1) bits |= kL;", "PCE IV -> Switch Pro L"),
        ("if (frame.X) bits |= kX;", "PCE V -> Switch Pro X"),
        ("if (frame.Y) bits |= kR;", "PCE VI -> Switch Pro R"),
    ):
        if token not in genesis_mapping:
            raise AssertionError(
                f"Empirical Pro-identity mapping is missing {description}")
    if "validated independently by tools/test_classic_genesis_nso_mode.py" not in (
            genesis_mapping):
        raise AssertionError(
            "Production mapping must remain distinct from official capture validation")
    if (
        "case SWITCHPRO_PRO:\n      TinyUSBDevice.setID(0x057E, 0x2009);"
        not in setup
    ):
        raise AssertionError(
            "Classic2USB Genesis must retain the shared Switch Pro identity")
    if "case SWITCHPRO_PRO:\n      _report[18] = 0x03;" not in subcommands:
        raise AssertionError(
            "Classic2USB Genesis must return the Switch Pro device-info type")


def verify_pinned_firmware_capture_vectors() -> None:
    decoded_event_bits = tuple(
        int.from_bytes(OFFICIAL_FW420_BUTTON_TABLE[index:index + 4], "little")
        for index in range(0, len(OFFICIAL_FW420_BUTTON_TABLE), 4)
    )
    if decoded_event_bits != OFFICIAL_FW420_EVENT_BITS:
        raise AssertionError("Pinned Genesis firmware event mapping changed")


def verify_local_firmware_capture() -> None:
    capture = (
        ROOT / "build" / "hardware_captures"
        / "genesis_nso_057e_201e_fw4.20_patchram.bin"
    )
    metadata_path = capture.with_suffix(capture.suffix + ".json")
    if not capture.exists() or not metadata_path.exists():
        print(
            "SKIP: local Genesis firmware capture absent; "
            "pinned firmware vectors verified")
        return

    firmware = capture.read_bytes()
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    expected_hash = (
        "784e762225283b894dfc35e67f1d6622630684e22635c8c341e9d778373b8216"
    )
    if hashlib.sha256(firmware).hexdigest() != expected_hash:
        raise AssertionError("local Genesis NSO PatchRAM hash changed")
    expected_metadata = {
        "vid": "057E",
        "pid": "201E",
        "usb_release": "0212",
        "product": "MD/Gen Control Pad",
        "firmware": "4.20",
        "controller_type": "0D",
        "size": 98304,
        "sha256": expected_hash,
    }
    for key, expected in expected_metadata.items():
        if metadata.get(key) != expected:
            raise AssertionError(
                f"local Genesis NSO metadata {key} != {expected!r}")
    ram: dict[int, int] = {}
    position = 0
    while position + 3 <= len(firmware):
        record_type = firmware[position]
        record_size = int.from_bytes(
            firmware[position + 1:position + 3], "little")
        payload_start = position + 3
        record_end = payload_start + record_size
        if record_end > len(firmware):
            raise AssertionError("local Genesis NSO PatchRAM record overruns")
        payload = firmware[payload_start:record_end]
        if record_type == 0x0A and len(payload) >= 4:
            address = int.from_bytes(payload[:4], "little")
            for offset, value in enumerate(payload[4:]):
                ram[address + offset] = value
        position = record_end
        if record_type == 0xFE:
            break
    else:
        raise AssertionError("local Genesis NSO PatchRAM has no EOF")

    def read_ram(address: int, size: int) -> bytes:
        try:
            return bytes(ram[address + offset] for offset in range(size))
        except KeyError as exc:
            raise AssertionError(
                f"Genesis NSO RAM byte {exc.args[0]:#x} is absent") from exc

    if read_ram(0x219DEC, 19) != b"MD/Gen Control Pad\0":
        raise AssertionError(
            "official Genesis product string moved in PatchRAM")

    firmware_button_table = read_ram(0x222618, 48)
    if firmware_button_table != OFFICIAL_FW420_BUTTON_TABLE:
        raise AssertionError("Genesis firmware type-0x0D button table changed")
    decoded_event_bits = tuple(
        int.from_bytes(firmware_button_table[index:index + 4], "little")
        for index in range(0, len(firmware_button_table), 4)
    )
    if decoded_event_bits != OFFICIAL_FW420_EVENT_BITS:
        raise AssertionError("Genesis firmware event-to-report mapping changed")

    correction_block = read_ram(0x21B634, 118)
    expected_correction_block = bytes.fromhex(
        "7c4800780d2836d1d4f86402062120f00200c4f864020020"
        "53f665fa08b1002000e00220d4f8641240220843b4f87c12"
        "02ea401221f040011143a4f87c1220f48000c4f864020721"
        "002053f64cfa08b1002001e04ff48000d4f8641210220843"
        "c4f86402b4f87c1202ea904021f010010143a4f87c12"
    )
    if correction_block != expected_correction_block:
        raise AssertionError(
            "Genesis firmware X/Z correction block changed")
    print("OK: local official Genesis firmware 4.20 capture verified")


def windows_to_wsl(path: Path) -> str:
    resolved = str(path.resolve()).replace("\\", "/")
    return f"/mnt/{resolved[0].lower()}{resolved[2:]}"


def native() -> int:
    compiler = shutil.which("g++") or shutil.which("clang++")
    if compiler is None:
        return 127
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
         "-I", str(ROOT), "-o", str(OUTPUT), str(TEST)],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(OUTPUT.resolve())], cwd=ROOT, check=True)
    return 0


def wsl() -> int:
    if shutil.which("wsl.exe") is None:
        return 127
    probe = subprocess.run(
        ["wsl.exe", "-l", "-q"], capture_output=True, text=True
    )
    if probe.returncode != 0 or not probe.stdout.strip():
        return 127
    build_dir = shlex.quote(windows_to_wsl(BUILD_DIR))
    output = shlex.quote(windows_to_wsl(OUTPUT))
    test = shlex.quote(windows_to_wsl(TEST))
    root = shlex.quote(windows_to_wsl(ROOT))
    command = (
        f"mkdir -p {build_dir} && "
        f"g++ -std=c++17 -Wall -Wextra -Werror -I {root} "
        f"-o {output} {test} && {output}"
    )
    subprocess.run(["wsl.exe", "bash", "-lc", command], cwd=ROOT, check=True)
    return 0


def main() -> int:
    try:
        verify_pinned_firmware_capture_vectors()
        verify_local_firmware_capture()
        source_guards()
        result = native()
        return wsl() if result == 127 else result
    except subprocess.CalledProcessError as exc:
        return exc.returncode


if __name__ == "__main__":
    raise SystemExit(main())
