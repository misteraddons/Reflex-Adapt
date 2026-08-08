from __future__ import annotations

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CHECKS = (
    "tools/test_serial_management_commands.py",
    "tools/test_classic2usb_source_guards.py",
    "tools/test_factory_defaults.py",
    "tools/test_button_map_policy.py",
    "tools/test_n64_output_compatibility.py",
    "tools/test_analog_stick_trace.py",
    "tools/test_classic_genesis_nso_mode.py",
    "tools/test_rumble_capabilities.py",
    "tools/test_oled_shoulder_layouts.py",
    "tools/test_auto_input_usb_identity.py",
    "tools/test_host_output_preferences.py",
    "tools/audit_classic2usb_release.py",
    "tools/test_release_integrity.py",
    "tools/test_adapt_manager.py",
)


def main() -> int:
    for check in CHECKS:
        print(f"==> {check}", flush=True)
        subprocess.run([sys.executable, check], cwd=ROOT, check=True)
    print("OK: all Classic2USB release checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
