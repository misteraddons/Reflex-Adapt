#!/usr/bin/env python3
"""Unit and release-integration guards for Name/Position button-map policy."""

from __future__ import annotations

import os
import shlex
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TEST = ROOT / "host_tests" / "button_map_policy_tests.cpp"
BUILD_DIR = ROOT / "build" / "host_tests"
OUTPUT = BUILD_DIR / (
    "button_map_policy_tests.exe" if os.name == "nt" else "button_map_policy_tests"
)


def require(path: Path, token: str, label: str) -> None:
    source = path.read_text(encoding="utf-8")
    if token not in source:
        raise AssertionError(f"{label} is missing {token!r}")


def reject(path: Path, token: str, label: str) -> None:
    source = path.read_text(encoding="utf-8")
    if token in source:
        raise AssertionError(f"{label} still contains {token!r}")


def source_guards() -> None:
    firmware = ROOT / "firmware"
    require(
        firmware / "output" / "usb" / "output_hid_mapping_runtime.h",
        "return effectivePositionButtonMap(",
        "shared USB output mapping",
    )
    require(
        firmware / "core" / "controller_runtime_output_finalize.cpp",
        "effectivePositionButtonMap(",
        "cached neutral-frame output",
    )

    require(
        firmware / "menu" / "menu_helpers_visibility.cpp",
        "return !menuInputSupportsSelectableButtonMapMode();",
        "OLED settings menu",
    )
    require(
        firmware / "menu" / "quick_config_visibility.cpp",
        "buttonMapModeIsUserSelectable(",
        "quick settings menu",
    )

    switch_mapping = firmware / "output" / "switch" / "output_switchpro_mapping_runtime.h"
    reject(
        switch_mapping,
        "} else if (gamecube_on_switch) {",
        "Switch GameCube face mapping",
    )
    require(
        switch_mapping,
        "_switchReport.a = frame.A || frame.R2;",
        "Virtual Boy named A Switch mapping",
    )

    require(
        switch_mapping,
        "position_button_map ? frame.Y : frame.X;",
        "Virtual Boy right-D-pad Position mapping",
    )

    web = ROOT / "web" / "Adapt.html"
    require(web, "function buttonMapModePolicyForUi(", "Adapt.html")
    require(web, "inputKey === 'snes' || inputKey === 'wii'", "Adapt.html")
    require(web, "buttonMapSetting.style.display = buttonMapSelectable", "Adapt.html")
    require(web, "'nes', 'snes', 'n64', 'gamecube', 'wii', 'vboy'", "Adapt.html")

    runner = ROOT / "tools" / "run_release_checks.py"
    require(runner, "tools/test_button_map_policy.py", runner.name)


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
            "-DPRODUCT_CLASSIC2USB",
            "-I",
            str(ROOT),
            "-o",
            str(OUTPUT),
            str(TEST),
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
    test = shlex.quote(windows_to_wsl(TEST))
    root = shlex.quote(windows_to_wsl(ROOT))
    command = (
        f"mkdir -p {build_dir} && "
        f"g++ -std=c++17 -Wall -Wextra -Werror -DPRODUCT_CLASSIC2USB "
        f"-I {root} -o {output} {test} && {output}"
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
