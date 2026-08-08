from __future__ import annotations

import os
import shlex
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TEST = ROOT / "host_tests" / "analog_stick_trace_tests.cpp"
SOURCES = (
    TEST,
    ROOT / "firmware/core/controller_frame_state.cpp",
    ROOT / "firmware/core/controller_output_cache_state.cpp",
)
BUILD_DIR = ROOT / "build" / "host_tests"
OUTPUT = BUILD_DIR / (
    "analog_stick_trace_tests.exe"
    if os.name == "nt"
    else "analog_stick_trace_tests"
)


def source_guards() -> None:
    runtime = (ROOT / "firmware/core/controller_runtime_core.cpp").read_text(
        encoding="utf-8"
    )
    render = (ROOT / "firmware/menu/quick_config_render.cpp").read_text(
        encoding="utf-8"
    )
    shared_renderer = (
        ROOT / "firmware/menu/analog_test_renderer.cpp"
    ).read_text(encoding="utf-8")
    shared_renderer_header = (
        ROOT / "firmware/menu/analog_test_renderer.h"
    ).read_text(encoding="utf-8")
    trace_header = (
        ROOT / "firmware/menu/analog_stick_trace.h"
    ).read_text(encoding="utf-8")
    standard_test = (
        ROOT / "firmware/menu/menu_analog_test.cpp"
    ).read_text(encoding="utf-8")
    visibility = (
        ROOT / "firmware/menu/menu_helpers_visibility.cpp"
    ).read_text(encoding="utf-8")
    navigation = (
        ROOT / "firmware/menu/quick_config_navigation.cpp"
    ).read_text(encoding="utf-8")
    psx_poll = (ROOT / "firmware/input/psx/input_psx_poll.cpp").read_text(
        encoding="utf-8"
    )
    dreamcast_poll = (
        ROOT / "firmware/input/dreamcast/input_dreamcast_poll.cpp"
    ).read_text(encoding="utf-8")
    joybus_poll = (
        ROOT / "firmware/input/gc64/input_gc64_poll.cpp"
    ).read_text(encoding="utf-8")
    wii_poll = (
        ROOT / "firmware/input/wii/input_wii_poll.cpp"
    ).read_text(encoding="utf-8")

    capture = "captureRawAnalogInputSnapshots(deviceMode);"
    centering = "updateStickCenteringFromPoll(updated);"
    if capture not in runtime or runtime.index(capture) > runtime.index(centering):
        raise AssertionError("raw axes must be captured before stick centering")

    for token in (
        "analogTraceUsesOctagonalGate(",
        "deviceMode, frame.controller_type_name",
        "range_trace[0].sample",
        "range_trace[1].sample",
        "renderAnalogStickTraceScreen",
        "analogDiagnosticTargetForPort",
        "range_test_target.port",
        "getRawAnalogInputSnapshot",
    ):
        if token not in render:
            raise AssertionError(f"analog trace integration is missing {token}")

    for token in (
        "trace.dotAt(",
        "u8g2.drawPixel",
        "u8g2.sendBuffer",
        "P%u RAW %s",
        "UD:port",
    ):
        if token not in shared_renderer:
            raise AssertionError(f"shared analog renderer is missing {token}")
    if "u8g2.drawLine(" in shared_renderer:
        raise AssertionError("analog trace must render sampled dots, not averaged lines")
    if "ANALOG_TEST_FRAME_INTERVAL_MS" not in shared_renderer_header:
        raise AssertionError("shared analog renderer is missing its frame interval")

    for token in ("kMaxPlotDots = 500", "dotCount() const"):
        if token not in trace_header:
            raise AssertionError(
                f"analog dot trace capacity guard is missing {token}"
            )

    for token in (
        "analogDiagnosticDefaultTarget",
        "analogDiagnosticNextPortTarget",
        "analogDiagnosticNextStickTarget",
        "getRawAnalogInputSnapshot",
        "getClassicAnalogRangeSnapshot",
        "renderAnalogStickTraceScreen",
        "renderAnalogValueTestScreen",
    ):
        if token not in standard_test:
            raise AssertionError(f"standard analog test is missing {token}")

    if "menuModeHasLiveAnalogDiagnostic(menu_input)" not in visibility:
        raise AssertionError(
            "System Analog Test must require a live supported controller"
        )

    if "range_test_right_stick = true;" not in navigation:
        raise AssertionError("right navigation must select the auxiliary stick")
    if "range_test_right_stick = false;" not in navigation:
        raise AssertionError("left navigation must select the main stick")

    for token in (
        "psxProtocolHasDualSticks",
        "protocol == PSPROTO_DUALSHOCK",
        "protocol == PSPROTO_DUALSHOCK2",
        "protocol == PSPROTO_FLIGHTSTICK",
        "hasDualSticks && hasLeftStick",
        "hasDualSticks && hasRightStick",
    ):
        if token not in psx_poll:
            raise AssertionError(f"PSX stick capability integration is missing {token}")

    dual_stick_helper = psx_poll.split(
        "bool psxProtocolHasDualSticks", 1
    )[1].split("bool isAutoResolvedPsxPort", 1)[0]
    for single_axis_protocol in (
        "PSPROTO_NEGCON",
        "PSPROTO_JOGCON",
        "PSPROTO_FISHING",
    ):
        if single_axis_protocol in dual_stick_helper:
            raise AssertionError(
                f"{single_axis_protocol} must not advertise a second stick"
            )

    for token in (
        "frame.HAS_ANALOG_STICK_MAIN = hasMainX || hasMainY;",
        "frame.HAS_ANALOG_TRIGGERS = hasLeftTrigger || hasRightTrigger;",
    ):
        if token not in dreamcast_poll:
            raise AssertionError(
                f"Dreamcast live analog capability is missing {token}"
            )

    if "frame.HAS_ANALOG_STICK_MAIN = false;" not in joybus_poll:
        raise AssertionError(
            "Joybus must clear its setup-time stick flag before subtype mapping"
        )
    for token in (
        "frame.HAS_ANALOG_STICK_MAIN = false;",
        "frame.HAS_ANALOG_STICK_AUX = false;",
        "frame.HAS_ANALOG_TRIGGERS = false;",
    ):
        if token not in wii_poll:
            raise AssertionError(
                f"Wii must clear stale subtype capability {token}"
            )


def windows_to_wsl(path: Path) -> str:
    resolved = str(path.resolve()).replace("\\", "/")
    return f"/mnt/{resolved[0].lower()}{resolved[2:]}"


def native() -> int:
    compiler = shutil.which("g++") or shutil.which("clang++")
    if compiler is None:
        return 127
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [
            compiler,
            "-std=c++17",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-I",
            str(ROOT),
            "-o",
            str(OUTPUT),
            *(str(source) for source in SOURCES),
        ],
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
    root = shlex.quote(windows_to_wsl(ROOT))
    sources = " ".join(shlex.quote(windows_to_wsl(source)) for source in SOURCES)
    command = (
        f"mkdir -p {build_dir} && "
        f"g++ -std=c++17 -Wall -Wextra -Werror -I {root} "
        f"-o {output} {sources} && {output}"
    )
    subprocess.run(["wsl.exe", "bash", "-lc", command], cwd=ROOT, check=True)
    return 0


def main() -> int:
    try:
        source_guards()
        result = native()
        return wsl() if result == 127 else result
    except subprocess.CalledProcessError as exc:
        return exc.returncode


if __name__ == "__main__":
    raise SystemExit(main())
