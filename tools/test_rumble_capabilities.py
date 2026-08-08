from __future__ import annotations

import os
import shlex
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TEST = ROOT / "host_tests" / "rumble_capability_tests.cpp"
BUILD_DIR = ROOT / "build" / "host_tests"
OUTPUT = BUILD_DIR / (
    "rumble_capability_tests.exe"
    if os.name == "nt" else "rumble_capability_tests"
)


def source_guards() -> None:
    menu_caps = (
        ROOT / "firmware" / "menu" / "menu_capabilities.cpp"
    ).read_text(encoding="utf-8")
    menu_visibility = (
        ROOT / "firmware" / "menu" / "menu_helpers_visibility.cpp"
    ).read_text(encoding="utf-8")
    quick_caps = (
        ROOT / "firmware" / "menu" / "quick_config_capabilities.cpp"
    ).read_text(encoding="utf-8")
    quick_actions = (
        ROOT / "firmware" / "menu" / "quick_config_actions.cpp"
    ).read_text(encoding="utf-8")
    quick_render = (
        ROOT / "firmware" / "menu" / "quick_config_render.cpp"
    ).read_text(encoding="utf-8")
    quick_visibility = (
        ROOT / "firmware" / "menu" / "quick_config_visibility.cpp"
    ).read_text(encoding="utf-8")

    required = (
        (menu_caps, "rumbleCapabilitiesForController("),
        (menu_caps, "combineRumbleCapabilities("),
        (menu_visibility, "return !input_has_variable_rumble_strength();"),
        (quick_caps, "cycleRumbleLevelForSupport("),
        (quick_render, 'display.print(F("Enabled: "));'),
        (quick_visibility, 'return temp_rumble == 0 ? "Off" : "On";'),
    )
    for source, token in required:
        if token not in source:
            raise AssertionError(f"rumble capability integration is missing {token}")

    for invalid_cycle in (
        "temp_rumble = (temp_rumble + 1) % 4;",
        "temp_rumble = (temp_rumble == 0) ? 3 : temp_rumble - 1;",
    ):
        if invalid_cycle in quick_actions:
            raise AssertionError(
                "Quick Config bypasses the tested rumble capability policy"
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
        f"g++ -std=c++17 -Wall -Wextra -Werror -I {root} "
        f"-o {output} {test} && {output}"
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
