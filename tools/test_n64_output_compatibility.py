from __future__ import annotations

import os
import shlex
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TEST = ROOT / "host_tests" / "n64_output_compatibility_tests.cpp"
BUILD_DIR = ROOT / "build" / "host_tests"
OUTPUT = BUILD_DIR / (
    "n64_output_compatibility_tests.exe"
    if os.name == "nt" else "n64_output_compatibility_tests"
)


def source_guards() -> None:
    capabilities = (
        ROOT / "firmware" / "output" / "output_capabilities.h"
    ).read_text(encoding="utf-8")
    required = (
        '#include "n64_output_compatibility.h"',
        "return output_n64_c_backing_r2(\n"
        "    controllerFrameTypeNameIsN64(frame),",
        "return output_n64_c_backing_select(\n"
        "    controllerFrameTypeNameIsN64(frame),",
        "frame.R2,\n    frame.L3);",
        "frame.SELECT,\n    frame.R3);",
    )
    for token in required:
        if token not in capabilities:
            raise AssertionError(f"N64 compatibility integration is missing {token}")


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