#!/usr/bin/env python3
"""Unified Reflex Adapt firmware and settings manager for MiSTer.

The packaged MiSTer script embeds this file and the fallback catalog.  Keep this
module dependency-free: stock MiSTer Python provides everything it uses.
"""

from __future__ import annotations

import argparse
import base64
import contextlib
import hashlib
import json
import os
import re
import select
import shutil
import struct
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request
import zlib
from pathlib import Path
from typing import Any, Callable

try:
    import termios
    import tty
except ImportError:  # The manager runs on MiSTer/Linux; Windows imports support tests.
    termios = None  # type: ignore[assignment]
    tty = None  # type: ignore[assignment]


MANAGER_VERSION = "0.5.0"
CATALOG_SCHEMA_VERSION = 1
DEFAULT_CATALOG_URL = (
    "https://raw.githubusercontent.com/misteraddons/Reflex-Adapt/main/"
    "tools/adapt_manager/catalog.json"
)
CLASSIC_RELEASES_URL = "https://api.github.com/repos/misteraddons/Reflex-Adapt/releases"
STATE_ROOT = Path(os.environ.get(
    "REFLEX_ADAPT_MANAGER_HOME",
    "/media/fat/config/reflex-adapt-manager",
))
CACHE_ROOT = STATE_ROOT / "cache"
CATALOG_CACHE_PATH = STATE_ROOT / "catalog.json"
RECEIPTS_PATH = STATE_ROOT / "receipts.json"
LOG_PATH = STATE_ROOT / "reflex_adapt_manager.log"
MISTER_ROOT = Path(os.environ.get("REFLEX_ADAPT_MISTER_ROOT", "/media/fat"))
DEFAULT_MAP_BUNDLE = Path(__file__).with_name("mister_maps.bin")
SYS_TTY_ROOT = Path(os.environ.get("REFLEX_ADAPT_SYS_TTY", "/sys/class/tty"))
SYS_USB_ROOT = Path(os.environ.get("REFLEX_ADAPT_SYS_USB", "/sys/bus/usb/devices"))
SYS_BLOCK_ROOT = Path(os.environ.get("REFLEX_ADAPT_SYS_BLOCK", "/sys/class/block"))
PROC_MOUNTS_PATH = Path(os.environ.get("REFLEX_ADAPT_PROC_MOUNTS", "/proc/mounts"))

CLASSIC_VID_PID = (0x16D0, 0x1460)
LEGACY_RUNTIME_VID_PID = (0x16D0, 0x127E)
LEGACY_BOOT_VID_PID = (0x2341, 0x0036)
LEGACY_HID_QUIRK = "0x16d0:0x127e:0x040"
LEGACY_NO_MERGE_VID_PID = "0x16d0127e"
OBSOLETE_DOWNLOADER_DATABASES = {
    "misteraddons/reflexadapt": (
        "https://github.com/misteraddons/Reflex-Adapt/raw/main/reflexadapt.json.zip"
    ),
}
# Reflex-Adapt-Legacy remains the maintained Adapt V1 Downloader feed. Manager
# 0.4.0 commented it out as obsolete; restore exactly those disabled sections.
RESTORED_DOWNLOADER_DATABASES = {
    "misteraddons/reflex-adapt-legacy": (
        "https://raw.githubusercontent.com/misteraddons/"
        "Reflex-Adapt-Legacy/main/reflex-adapt-legacy.json.zip"
    ),
}
DOWNLOADER_DISABLED_MARKER = "; Disabled by Reflex Adapt Manager"
AVR109_SOFTWARE_ID = b"CATERIN"
AVR109_PAGE_SIZE = 128
UF2_MAGIC0 = 0x0A324655
UF2_MAGIC1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30
UF2_FLAG_FAMILY_ID = 0x00002000
RP2040_FAMILY_ID = 0xE48BFF56
RP2040_FLASH_BASE = 0x10000000
# The RP2040 boot ROM reports this Board-ID in INFO_UF2.TXT; other UF2
# bootloaders (RP2350, SAMD, nRF) would accept or misinterpret an RP2040 image.
RP2040_BOOTSEL_BOARD_ID = "RPI-RP2"
RP2040_BOOTSEL_VID_PID = (0x2E8A, 0x0003)
# Classic2USB uses 12 KiB LittleFS at the physical flash end. Arduino-Pico's
# EEPROM begins at 0x101fc000; the three preceding sectors hold auth, bonds,
# and learned HID maps. Published application UF2 blocks must stop below them.
CLASSIC_RESERVED_FLASH_BASE = 0x101F9000

SETTING_NAMES = (
    "Input Mode", "Output Mode", "Display Contrast", "Latency Test",
    "Latency Controller Loop", "Latency Host", "Buzzer", "Sound Events",
    "LED Mode", "LED Brightness", "Screensaver", "Screensaver Animation",
    "Quick Menu Hotkey", "Home Hotkey", "Capture Hotkey", "Windows Output",
    "PSX Peripheral", "SNES Rumble", "Home Debug", "Reserved",
    "JVS Home View", "Hotkey Hold", "Deadzone", "SOCD", "D-pad Mode",
    "Stick Invert", "Rumble Level", "Trigger Mode", "Spinner Speed",
    "Button Map", "N64 Z Mode", "N64 C Mode", "Wii Analog Range",
    "N64 Analog Range", "NSO Special", "Power Pad Mode", "GunCon X Alignment",
    "GunCon Y Alignment", "Reserved Musical", "L Shoulder",
    "Reserved Mouse Sensitivity", "Reserved Mouse Stick", "JogCon Mode",
    "JogCon Force", "JogCon Range", "JogCon Digital Map", "JogCon Axis",
    "Cal LX Min", "Cal LX Max", "Cal LY Min", "Cal LY Max", "Cal RX Min",
    "Cal RX Max", "Cal RY Min", "Cal RY Max", "Analog Calibration",
    "2P Merge", "System Menu Hotkey", "Kiosk Mode", "Touchpad Mouse",
    "Output Transport", "BT Output Persona", "MiSTer Output",
    "Multi-bind", "USB Drive", "Game selection", "Paddle Divisor",
    "Rotary Reverse", "Ends", "OLED Inactive", "OLED Wake", "OK / Back",
)

CLASSIC_INPUT_MODE_NAMES = {
    1: "Auto",
    2: "Atari/C64/SMS",
    3: "Atari Driving",
    4: "Genesis",
    5: "Saturn",
    6: "Dreamcast",
    7: "NES",
    8: "SNES",
    9: "N64",
    10: "GameCube",
    11: "Wii Classic",
    12: "Virtual Boy",
    13: "PC Engine",
    14: "PlayStation",
    15: "JogCon",
    16: "Neo Geo",
    17: "3DO",
    18: "Jaguar",
    19: "Japanese PC",
    20: "Intellivision",
    21: "ColecoVision",
}

# Names and values follow the adapter's OLED menus (longer where the OLED
# abbreviates for width). tools/test_config_tool_settings_parity.py checks
# them against the firmware.
SCREENSAVER_ANIMATIONS = (
    (0, "Bouncing Logo"), (1, "Starfield"), (2, "Matrix Rain"), (3, "Auto Snake"),
    (4, "Pong AI"), (6, "Fireworks"), (8, "Maze Generator"), (9, "Breakout AI"),
    (10, "Plasma Effect"), (12, "Rain Drops"), (13, "Spirograph"),
)
OLED_INACTIVE_NAMES = ("Screensaver", "Blank OLED")
OLED_WAKE_NAMES = ("Any input", "Adapt only")
OLED_TIMER_NAMES = ("Off", "1m", "5m", "10m")
SCREENSAVER_DIM = -1
SWITCH_DEVICE_NAMES = ("Switch Pro", "Genesis NSO", "NES NSO", "SNES NSO", "N64 NSO", "GC Adapter")
STICK_RANGE_NAMES = ("Raw", "Norm", "Cal", "Learn")

SETTING_CHOICES: dict[int, tuple[tuple[int, str], ...]] = {
    15: tuple(enumerate(("DInput", "XInput", "Keyboard", "Xbox One", "PS3", "PS4", "PS5", "Switch Pro"))),
    62: tuple(enumerate(("DInput", "XInput", "Keyboard", "Xbox One", "PS3", "PS4", "PS5", "Switch Pro"))),
    2: ((64, "25%"), (128, "50%"), (192, "75%"), (254, "100%")),
    6: tuple(enumerate(("Off", "Low", "High"))),
    11: SCREENSAVER_ANIMATIONS,
    21: tuple(enumerate(("Press", "0.5s", "1.0s", "1.5s", "2.0s", "2.5s", "3.0s"))),
    22: tuple((value, f"{value}%") for value in range(0, 31, 5)),
    24: tuple(enumerate(("DPad", "Left Stick", "Right Stick", "Buttons"))),
    25: tuple(enumerate(("Off", "Left", "Right", "Both"))),
    26: tuple(enumerate(("Off", "Variable", "Max"))),
    27: tuple(enumerate(("Analog", "Digital", "RStick", "Both"))),
    28: tuple(enumerate(("0.25x", "0.5x", "1x", "2x", "4x"))),
    29: ((0, "Name"), (1, "Position")),
    30: ((0, "L1"), (1, "L2")),
    31: tuple(enumerate(("Auto", "Buttons", "Stick"))),
    32: tuple(enumerate(STICK_RANGE_NAMES)),
    33: tuple(enumerate(STICK_RANGE_NAMES)),
    35: tuple(enumerate(("Pad", "PowerPad", "Vaus NES", "Vaus FC"))),
    39: ((0, "ZL"), (1, "L")),
    42: tuple(enumerate(("Spinner", "Paddle", "Wheel", "Fake Spin"))),
    43: ((1, "Low"), (3, "Med-L"), (7, "Med-H"), (15, "High")),
    44: tuple(enumerate(("Fine", "Normal", "Coarse"))),
    45: tuple(enumerate(("Left/Right", "L3/R3", "L/R", "Up/Down"))),
    46: ((0, "X"), (1, "Y")),
    66: (
        (1, "1x"), (2, "1/2"), (3, "1/3"), (4, "1/4"), (5, "1/5"),
        (6, "1/6"), (7, "1/7"), (8, "1/8"), (9, "1/9"), (10, "1/10"),
        (11, "1/11"), (12, "1/12"), (13, "1/13"), (14, "1/14"),
        (15, "1/15"), (16, "1/16"), (17, "2x"), (19, "3/2"), (18, "4x"),
        (20, "8x"), (21, "12x"), (22, "16x"), (23, "3x"),
    ),
    68: ((0, "Stop"), (1, "Wrap")),
    69: tuple(enumerate(OLED_INACTIVE_NAMES)),
    70: tuple(enumerate(OLED_WAKE_NAMES)),
    71: tuple(enumerate(("X / A", "East", "South"))),
}

BOOLEAN_SETTING_IDS = {12, 13, 14, 55, 56, 57, 58, 64, 67}
GLOBAL_DISPLAY_SETTING_IDS = {2}
GLOBAL_SOUND_SETTING_IDS = {6, 7}
GLOBAL_OLED_SETTING_IDS = {10, 11, 69, 70}
# Classic2USB has directional shortcuts: the OLED Hotkey page shows only these.
GLOBAL_HOTKEY_SETTING_IDS = {13, 21}
CALIBRATION_SETTING_IDS = set(range(47, 56))
INPUT_SETTING_IDS = {
    22, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 39, 42, 43, 44, 45, 46, 56, 59, 66, 67, 68,
    *CALIBRATION_SETTING_IDS,
}
ATARI_INPUT_MODE = 2
ATARI_DRIVING_INPUT_MODE = 3
PLAYSTATION_INPUT_MODE = 14
JOGCON_INPUT_MODE = 15
GAMECUBE_INPUT_MODE = 10
WII_INPUT_MODE = 11
VIRTUAL_BOY_INPUT_MODE = 12
GUNCON_SETTING_IDS = {36, 37}
JOGCON_SETTING_IDS = {42, 43, 44, 45, 46, 66, 67}
# Input modes whose Switch device choice is one fixed NSO controller.
NSO_DEVICE_BY_MODE = {4: "Genesis NSO", 7: "NES NSO", 8: "SNES NSO", 9: "N64 NSO", 10: "GC Adapter"}
SOUND_EVENTS = (
    (1 << 0, "Boot"), (1 << 1, "Connect"), (1 << 2, "Disconnect"),
    (1 << 3, "Menu Enter"), (1 << 4, "Menu Nav"), (1 << 5, "Save/Cancel"),
    (1 << 6, "Turbo"), (1 << 7, "Mode Change"), (1 << 8, "Error"),
    (1 << 9, "Factory Reset"), (1 << 10, "Hotkey"),
)

CONFIGURABLE_INPUT_MODES = tuple(
    mode for mode in sorted(CLASSIC_INPUT_MODE_NAMES)
    if mode not in {1, ATARI_DRIVING_INPUT_MODE, JOGCON_INPUT_MODE}
)


def log(message: str) -> None:
    try:
        STATE_ROOT.mkdir(parents=True, exist_ok=True)
        with LOG_PATH.open("a", encoding="utf-8") as handle:
            handle.write(f"{time.strftime('%Y-%m-%d %H:%M:%S')} {message}\n")
    except OSError:
        pass


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def version_key(version: str) -> tuple[int, ...]:
    values = [int(value) for value in re.findall(r"\d+", version)]
    values = values or [0]
    return tuple(values + [0] * max(0, 4 - len(values)))


def load_json(path: Path, default: Any) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return default


def safe_artifact_filename(value: Any) -> str:
    """Return a catalog filename only if it is a plain basename.

    Catalog entries come from the network, and the filename is joined onto the
    download cache and the UF2 volume, so path separators or dot names would
    let a descriptor write outside those directories.
    """
    name = str(value or "")
    if (
        not name
        or name in {".", ".."}
        or name.startswith(".")
        or "/" in name
        or "\\" in name
        or "\0" in name
        or name != Path(name).name
    ):
        raise ValueError(f"unsafe artifact filename: {name!r}")
    return name


def validate_catalog(catalog: dict[str, Any]) -> None:
    if catalog.get("schema_version") != CATALOG_SCHEMA_VERSION:
        raise ValueError("unsupported catalog schema")
    minimum = str(catalog.get("manager_min_version", "0"))
    if not re.fullmatch(r"\d+(?:\.\d+){0,3}", minimum):
        raise ValueError(f"invalid manager_min_version: {minimum!r}")
    if version_key(minimum) > version_key(MANAGER_VERSION):
        raise ValueError(
            f"catalog requires Reflex Adapt Manager v{minimum} or newer "
            f"(running v{MANAGER_VERSION})"
        )
    products = catalog.get("products")
    if not isinstance(products, list) or not products:
        raise ValueError("catalog has no products")
    product_ids: set[str] = set()
    for product in products:
        product_id = product.get("id")
        if not product_id or product_id in product_ids:
            raise ValueError(f"invalid or duplicate product id: {product_id!r}")
        product_ids.add(product_id)
        if product.get("backend") not in {"rp2040-uf2", "avr109"}:
            raise ValueError(f"unsupported backend for {product_id}")
        for release in product.get("releases", []):
            if not release.get("version") or not isinstance(release.get("artifacts"), list):
                raise ValueError(f"invalid release in {product_id}")
            for artifact in release["artifacts"]:
                digest = artifact.get("sha256", "")
                if not re.fullmatch(r"[0-9a-f]{64}", digest):
                    raise ValueError(f"invalid SHA256 for {artifact.get('id')}")
                if not artifact.get("url") or not artifact.get("filename"):
                    raise ValueError(f"incomplete artifact {artifact.get('id')}")
                safe_artifact_filename(artifact["filename"])


def fetch_json(url: str, timeout: float = 10.0) -> Any:
    request = urllib.request.Request(
        url,
        headers={"User-Agent": f"Reflex-Adapt-Manager/{MANAGER_VERSION}"},
    )
    with urllib.request.urlopen(request, timeout=timeout) as response:
        return json.loads(response.read().decode("utf-8"))


def merge_product_releases(base: dict[str, Any], incoming: dict[str, Any]) -> None:
    existing = {
        (release.get("family", ""), release.get("version")): release
        for release in base.setdefault("releases", [])
    }
    for release in incoming.get("releases", []):
        key = (release.get("family", ""), release.get("version"))
        existing[key] = release
    base["releases"] = sorted(
        existing.values(),
        key=lambda release: release.get("version", ""),
        reverse=True,
    )


def merge_catalog(base: dict[str, Any], incoming: dict[str, Any]) -> dict[str, Any]:
    validate_catalog(incoming)
    products = {product["id"]: product for product in base["products"]}
    for product in incoming["products"]:
        if product["id"] in products:
            merge_product_releases(products[product["id"]], product)
        else:
            base["products"].append(product)
    return base


def normalize_recommendations(catalog: dict[str, Any]) -> None:
    for product in catalog["products"]:
        stable_by_family: dict[str, list[dict[str, Any]]] = {}
        for release in product.get("releases", []):
            if release.get("channel") != "stable":
                continue
            family = release.get("family", product["id"])
            stable_by_family.setdefault(family, []).append(release)
        for releases in stable_by_family.values():
            newest = max(releases, key=lambda release: version_key(release["version"]))
            for release in releases:
                release["recommended"] = release is newest


def discover_classic_release_descriptors() -> tuple[list[dict[str, Any]], list[str]]:
    descriptors: list[dict[str, Any]] = []
    errors: list[str] = []
    releases: list[dict[str, Any]] = []
    for page in range(1, 11):
        separator = "&" if "?" in CLASSIC_RELEASES_URL else "?"
        url = f"{CLASSIC_RELEASES_URL}{separator}per_page=100&page={page}"
        try:
            result = fetch_json(url)
            if not isinstance(result, list):
                raise ValueError("GitHub release feed did not return a list")
        except (OSError, ValueError, urllib.error.URLError) as exc:
            message = f"Classic release discovery failed: {exc}"
            log(message)
            errors.append(message)
            return descriptors, errors
        releases.extend(result)
        if len(result) < 100:
            break
    else:
        message = "Classic release discovery stopped after 1,000 releases"
        log(message)
        errors.append(message)
    for release in releases:
        if release.get("draft") or release.get("prerelease"):
            continue
        for asset in release.get("assets", []):
            if asset.get("name") != "adapt-manager-release.json":
                continue
            try:
                descriptor = fetch_json(asset["browser_download_url"])
                validate_catalog(descriptor)
                descriptors.append(descriptor)
            except (KeyError, OSError, ValueError, urllib.error.URLError) as exc:
                message = f"Release descriptor rejected: {exc}"
                log(message)
                errors.append(message)
    return descriptors, errors


def _catalog_for_cache(catalog: dict[str, Any]) -> dict[str, Any]:
    cached = json.loads(json.dumps(catalog))
    cached.pop("_update_check", None)
    return cached


def load_catalog(path: Path, refresh: bool = True) -> dict[str, Any]:
    catalog = load_json(path, {})
    validate_catalog(catalog)
    refresh_errors: list[str] = []
    cache_errors: list[str] = []
    used_cached_catalog = False
    cached = load_json(CATALOG_CACHE_PATH, {})
    cached_valid = False
    if cached:
        try:
            validate_catalog(cached)
            cached_valid = True
        except ValueError as exc:
            log(f"Cached catalog rejected: {exc}")
    if not refresh and cached_valid:
        catalog = merge_catalog(catalog, cached)
        used_cached_catalog = True
    elif refresh:
        try:
            remote = fetch_json(catalog.get("catalog_url", DEFAULT_CATALOG_URL))
            catalog = merge_catalog(catalog, remote)
        except (OSError, ValueError, urllib.error.URLError) as exc:
            log(f"Catalog refresh failed, using embedded catalog: {exc}")
            refresh_errors.append(f"catalog: {exc}")
        descriptors, discovery_errors = discover_classic_release_descriptors()
        refresh_errors.extend(discovery_errors)
        for descriptor in descriptors:
            catalog = merge_catalog(catalog, descriptor)
        if refresh_errors and cached_valid:
            # Use the complete last-known-good snapshot instead of mixing it
            # with a partial refresh that may retain withdrawn releases.
            catalog = load_json(path, {})
            validate_catalog(catalog)
            catalog = merge_catalog(catalog, cached)
            used_cached_catalog = True
    normalize_recommendations(catalog)
    if refresh and not refresh_errors:
        try:
            write_text_atomic(
                CATALOG_CACHE_PATH,
                json.dumps(_catalog_for_cache(catalog), indent=2) + "\n",
            )
        except OSError as exc:
            log(f"Catalog cache update failed: {exc}")
            cache_errors.append(str(exc))
    catalog["_update_check"] = {
        "attempted": refresh,
        "online": refresh and not refresh_errors,
        "errors": refresh_errors,
        "cache_errors": cache_errors,
        "cached": used_cached_catalog,
    }
    return catalog


class BlueUi:
    """MiSTer-native dialog UI matching the system utility scripts."""

    BACKTITLE = "Reflex Adapt Manager"
    HEIGHT = 22
    WIDTH = 76

    def __init__(self) -> None:
        self.dialog = shutil.which("dialog")
        self._pending_status: str | None = None
        self._skip_next_pause = False

    def __enter__(self) -> "BlueUi":
        if not self.dialog:
            raise RuntimeError("MiSTer dialog utility was not found")
        return self

    def __exit__(self, *_args: Any) -> None:
        if self.dialog:
            subprocess.run(
                [self.dialog, "--clear"],
                check=False,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )

    def header(self, subtitle: str = "") -> None:
        self.info(subtitle)

    @staticmethod
    def _option(option: str | tuple[str, bool]) -> tuple[str, bool]:
        return option if isinstance(option, tuple) else (option, True)

    def _run(self, args: list[str], capture: bool = False) -> subprocess.CompletedProcess[str]:
        if not self.dialog:
            raise RuntimeError("MiSTer dialog utility was not found")
        command = [
            self.dialog,
            "--backtitle", self.BACKTITLE,
            "--cr-wrap",
            "--no-collapse",
            *args,
        ]
        return subprocess.run(
            command,
            check=False,
            text=True,
            stdout=None,
            stderr=subprocess.PIPE if capture else subprocess.DEVNULL,
        )

    def info(self, message: str, title: str = "") -> None:
        args = ["--title", title] if title else []
        self._run([*args, "--infobox", f"\n{message}", "7", "70"])

    def message(self, title: str, message: str) -> None:
        self._pending_status = None
        self._run([
            "--title", title,
            "--msgbox", message,
            str(self.HEIGHT), str(self.WIDTH),
        ])

    def choose(
        self,
        title: str,
        options: list[str | tuple[str, bool]],
        allow_back: bool = True,
        initial: int = 0,
    ) -> int | None:
        normalized = [self._option(option) for option in options]
        enabled = [index for index, (_label, active) in enumerate(normalized) if active]
        if not enabled:
            return None
        selected = initial if initial in enabled else enabled[0]
        menu_args = [
            "--title", title,
            "--default-item", str(selected),
        ]
        if allow_back:
            menu_args.extend(["--cancel-label", "Back"])
        else:
            menu_args.append("--nocancel")
        menu_args.extend([
            "--menu", "Use Up/Down and Enter to select.",
            str(self.HEIGHT), str(self.WIDTH),
            str(min(15, max(4, len(enabled)))),
        ])
        for index in enabled:
            menu_args.extend([str(index), normalized[index][0]])
        result = self._run(menu_args, capture=True)
        if result.returncode != 0:
            return None
        try:
            return int(result.stderr.strip())
        except ValueError:
            return None

    def confirm(self, prompt: str) -> bool:
        result = self._run([
            "--title", "Confirm",
            "--defaultno",
            "--yes-label", "Yes",
            "--no-label", "No",
            "--yesno", prompt,
            "14", str(self.WIDTH),
        ])
        return result.returncode == 0

    def input_value(self, title: str, prompt: str, initial: str = "") -> str | None:
        result = self._run([
            "--title", title,
            "--inputbox", prompt,
            "14", str(self.WIDTH), initial,
        ], capture=True)
        return result.stderr.strip() if result.returncode == 0 else None

    def pause(self, message: str = "Press Enter to continue.") -> None:
        if self._skip_next_pause:
            self._skip_next_pause = False
            return
        detail = self._pending_status or message
        self._pending_status = None
        self.message("Reflex Adapt Manager", detail)

    def status(self, message: str) -> None:
        self._pending_status = message
        self.info(message, "Working")

    def error(self, message: str) -> None:
        self._pending_status = None
        self.message("Error", message)
        self._skip_next_pause = True


def _read_hex(path: Path) -> int | None:
    try:
        return int(path.read_text(encoding="ascii").strip(), 16)
    except (OSError, ValueError):
        return None


def _read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="replace").strip()
    except OSError:
        return ""


def usb_parent_for(path: Path) -> Path | None:
    try:
        current = path.resolve()
    except OSError:
        return None
    for parent in (current, *current.parents):
        vid = _read_hex(parent / "idVendor")
        pid = _read_hex(parent / "idProduct")
        if vid is not None and pid is not None:
            return parent
    return None


def usb_identity_for(path: Path) -> tuple[int, int, str] | None:
    parent = usb_parent_for(path)
    if parent is None:
        return None
    vid = _read_hex(parent / "idVendor")
    pid = _read_hex(parent / "idProduct")
    if vid is None or pid is None:
        return None
    return vid, pid, parent.name


def serial_ports(vid_pid: tuple[int, int] | None = None) -> list[dict[str, Any]]:
    ports: list[dict[str, Any]] = []
    sys_class = SYS_TTY_ROOT
    if not sys_class.exists():
        return ports
    for entry in sorted(sys_class.iterdir()):
        if not (entry.name.startswith("ttyACM") or entry.name.startswith("ttyUSB")):
            continue
        identity = usb_identity_for(entry / "device")
        if not identity:
            continue
        vid, pid, topology = identity
        if vid_pid and (vid, pid) != vid_pid:
            continue
        ports.append({
            "port": f"/dev/{entry.name}",
            "vid": vid,
            "pid": pid,
            "topology": topology,
        })
    return ports


def usb_devices() -> list[dict[str, Any]]:
    devices: list[dict[str, Any]] = []
    if not SYS_USB_ROOT.exists():
        return devices
    ports_by_topology: dict[str, list[str]] = {}
    for port in serial_ports():
        ports_by_topology.setdefault(port["topology"], []).append(port["port"])
    for path in sorted(SYS_USB_ROOT.iterdir()):
        vid = _read_hex(path / "idVendor")
        pid = _read_hex(path / "idProduct")
        if vid is None or pid is None:
            continue
        devices.append({
            "vid": vid,
            "pid": pid,
            "bcd_device": _read_hex(path / "bcdDevice"),
            "manufacturer": _read_text(path / "manufacturer"),
            "product_name": _read_text(path / "product"),
            "serial": _read_text(path / "serial"),
            "topology": path.name,
            "ports": ports_by_topology.get(path.name, []),
        })
    return devices


def probe_classic_info(port_name: str) -> dict[str, str] | None:
    try:
        with LinuxSerial(port_name, 115200, timeout=1.0) as port:
            lines = port.command_lines("INFO", "INFO PRODUCT=", timeout=1.25)
    except (OSError, TimeoutError):
        return None
    pattern = re.compile(
        r'^INFO PRODUCT=(\S+) VERSION=(\S+) TAG=(\S+) HARDWARE="([^"]*)"$'
    )
    for line in lines:
        match = pattern.match(line.strip())
        if match:
            return {
                "product": match.group(1),
                "version": match.group(2),
                "tag": match.group(3),
                "hardware": match.group(4),
            }
    return None


def _looks_like_classic(device: dict[str, Any]) -> bool:
    identity = (device["vid"], device["pid"])
    product = device.get("product_name", "").lower()
    serial = device.get("serial", "").lower()
    manufacturer = device.get("manufacturer", "").lower()
    if identity == CLASSIC_VID_PID:
        return True
    if identity == LEGACY_RUNTIME_VID_PID:
        return (
            product.startswith("reflexps") or
            serial.startswith("reflexps") or
            "classic2usb" in product or
            "classic2usb" in serial
        )
    return manufacturer == "reflex adapt" and "xbox one controller" in product


def _serial_probe_candidate(device: dict[str, Any]) -> bool:
    identity = (device["vid"], device["pid"])
    if identity in {CLASSIC_VID_PID, LEGACY_RUNTIME_VID_PID}:
        return True
    identity_text = " ".join(
        str(device.get(key, "")).lower()
        for key in ("manufacturer", "product_name", "serial")
    )
    return "reflex adapt" in identity_text or "classic2usb" in identity_text


def discover_supported_devices(probe_serial: bool = True) -> list[dict[str, Any]]:
    devices: list[dict[str, Any]] = []
    for usb in usb_devices():
        info = None
        if probe_serial and _serial_probe_candidate(usb):
            for port_name in usb["ports"]:
                info = probe_classic_info(port_name)
                if info:
                    break
        identity = (usb["vid"], usb["pid"])
        if info and info.get("product") == "Classic2USB":
            product_id = "classic2usb"
            label = "Reflex Adapt Classic2USB"
        elif _looks_like_classic(usb):
            product_id = "classic2usb"
            label = "Reflex Adapt Classic2USB"
        elif identity == LEGACY_RUNTIME_VID_PID:
            product_id = "adapt-v1-legacy"
            label = "Adapt V1 / Legacy"
        elif identity == LEGACY_BOOT_VID_PID:
            product_id = "adapt-v1-legacy"
            label = "Adapt V1 / Legacy bootloader"
        else:
            continue
        devices.append({
            **usb,
            "product_id": product_id,
            "label": label,
            "firmware": info,
        })
    for boot in uf2_mount_details():
        devices.append({
            "product_id": "rp2040-uf2-unknown",
            "label": "Unidentified RP2040 BOOTSEL device",
            "vid": boot.get("vid", 0),
            "pid": boot.get("pid", 0),
            "bcd_device": None,
            "manufacturer": "Raspberry Pi",
            "product_name": "RPI-RP2",
            "serial": boot.get("serial", ""),
            "topology": boot.get("topology", ""),
            "ports": [],
            "mount": str(boot["mount"]),
            "board_id": boot.get("board_id", ""),
            "firmware": None,
        })
    return devices


class LinuxSerial:
    def __init__(self, port: str, baud: int, timeout: float = 2.0):
        if os.name != "posix" or termios is None:
            raise OSError("serial access is supported on MiSTer/Linux")
        self.fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        self.timeout = timeout
        attrs = termios.tcgetattr(self.fd)
        attrs[0] = 0
        attrs[1] = 0
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        attrs[3] = 0
        speed = {57600: termios.B57600, 115200: termios.B115200}[baud]
        attrs[4] = speed
        attrs[5] = speed
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 0
        termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
        os.set_blocking(self.fd, True)
        termios.tcflush(self.fd, termios.TCIOFLUSH)

    def close(self) -> None:
        if self.fd >= 0:
            os.close(self.fd)
            self.fd = -1

    def __enter__(self) -> "LinuxSerial":
        return self

    def __exit__(self, *_args: Any) -> None:
        self.close()

    def write(self, data: bytes) -> None:
        view = memoryview(data)
        while view:
            written = os.write(self.fd, view)
            view = view[written:]

    def read_exact(self, count: int) -> bytes:
        data = bytearray()
        deadline = time.monotonic() + self.timeout
        while len(data) < count:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError(f"serial timeout after {len(data)}/{count} bytes")
            ready, _, _ = select.select([self.fd], [], [], remaining)
            if not ready:
                continue
            chunk = os.read(self.fd, count - len(data))
            if chunk:
                data.extend(chunk)
        return bytes(data)

    def command_lines(self, command: str, terminator: str, timeout: float = 2.0) -> list[str]:
        termios.tcflush(self.fd, termios.TCIFLUSH)
        self.write(command.encode("ascii") + b"\n")
        data = bytearray()
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            ready, _, _ = select.select([self.fd], [], [], 0.1)
            if ready:
                chunk = os.read(self.fd, 4096)
                if chunk:
                    data.extend(chunk)
                    lines = data.decode("utf-8", "replace").splitlines()
                    if any(line.startswith(terminator) for line in lines):
                        return lines
        raise TimeoutError(f"no {terminator} response to {command}")


def parse_ihex(data: bytes, page_size: int = AVR109_PAGE_SIZE) -> dict[int, bytes]:
    memory: dict[int, int] = {}
    upper = 0
    for line_number, raw in enumerate(data.decode("ascii").splitlines(), 1):
        line = raw.strip()
        if not line:
            continue
        if not line.startswith(":"):
            raise ValueError(f"invalid Intel HEX line {line_number}")
        record = bytes.fromhex(line[1:])
        if len(record) < 5 or (sum(record) & 0xFF) != 0:
            raise ValueError(f"bad Intel HEX checksum on line {line_number}")
        length = record[0]
        address = (record[1] << 8) | record[2]
        kind = record[3]
        payload = record[4:4 + length]
        if kind == 0:
            absolute = upper + address
            for offset, value in enumerate(payload):
                memory[absolute + offset] = value
        elif kind == 1:
            break
        elif kind == 2:
            upper = int.from_bytes(payload, "big") << 4
        elif kind == 4:
            upper = int.from_bytes(payload, "big") << 16
    pages: dict[int, bytes] = {}
    for address in sorted({address - (address % page_size) for address in memory}):
        page = bytearray([0xFF] * page_size)
        for offset in range(page_size):
            page[offset] = memory.get(address + offset, 0xFF)
        pages[address] = bytes(page)
    if not pages:
        raise ValueError("Intel HEX contains no flash data")
    return pages


class Avr109:
    def __init__(self, port: LinuxSerial):
        self.port = port

    def command(self, command: bytes, response_count: int = 0) -> bytes:
        self.port.write(command)
        return self.port.read_exact(response_count) if response_count else b""

    def confirm(self) -> None:
        if self.port.read_exact(1) != b"\r":
            raise OSError("Caterina command was not acknowledged")

    def check(self) -> None:
        if self.command(b"S", 7) != AVR109_SOFTWARE_ID:
            raise OSError("device is not a Caterina bootloader")
        self.port.write(b"b")
        if self.port.read_exact(1) != b"Y":
            raise OSError("bootloader has no block-write support")
        block_size = int.from_bytes(self.port.read_exact(2), "big")
        if block_size != AVR109_PAGE_SIZE:
            raise OSError(f"unexpected Caterina page size {block_size}")

    def set_address(self, address: int) -> None:
        word_address = address >> 1
        self.port.write(b"A" + word_address.to_bytes(2, "big"))
        self.confirm()

    def write_page(self, address: int, data: bytes) -> None:
        self.set_address(address)
        self.port.write(b"B" + len(data).to_bytes(2, "big") + b"F" + data)
        self.confirm()

    def read_page(self, address: int, count: int) -> bytes:
        self.set_address(address)
        self.port.write(b"g" + count.to_bytes(2, "big") + b"F")
        return self.port.read_exact(count)

    def exit(self) -> None:
        self.port.write(b"E")
        self.confirm()


def flash_avr109(
    port_name: str,
    ihex: bytes,
    progress: Callable[[str, int, int], None] | None = None,
) -> None:
    pages = parse_ihex(ihex)
    with LinuxSerial(port_name, 57600, timeout=3.0) as serial:
        avr = Avr109(serial)
        avr.check()
        total = len(pages) * 2
        completed = 0
        for address, page in pages.items():
            avr.write_page(address, page)
            completed += 1
            if progress:
                progress("Flashing", completed, total)
        for address, page in pages.items():
            if avr.read_page(address, len(page)) != page:
                raise OSError(f"verification failed at flash address 0x{address:04x}")
            completed += 1
            if progress:
                progress("Verifying", completed, total)
        avr.exit()


def _decode_mount_field(value: str) -> str:
    return value.replace("\\040", " ").replace("\\011", "\t")


def uf2_mount_details() -> list[dict[str, Any]]:
    mounts: dict[Path, str] = {}
    try:
        for line in PROC_MOUNTS_PATH.read_text(encoding="utf-8").splitlines():
            fields = line.split()
            if len(fields) >= 2:
                mounts[Path(_decode_mount_field(fields[1]))] = _decode_mount_field(fields[0])
    except OSError:
        pass
    for root in (Path("/media"), Path("/mnt")):
        try:
            for path in root.iterdir():
                if path.is_dir():
                    mounts.setdefault(path, "")
        except OSError:
            pass
    uf2: list[dict[str, Any]] = []
    for path, source in mounts.items():
        try:
            info_path = path / "INFO_UF2.TXT"
            if not info_path.is_file():
                continue
            info = info_path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        match = re.search(r"^Board-ID:\s*(.+)$", info, re.MULTILINE | re.IGNORECASE)
        detail: dict[str, Any] = {
            "mount": path,
            "source": source,
            "board_id": match.group(1).strip() if match else "",
            "topology": "",
            "serial": "",
            "vid": 0,
            "pid": 0,
        }
        if source.startswith("/dev/"):
            parent = usb_parent_for(SYS_BLOCK_ROOT / Path(source).name / "device")
            if parent is not None:
                detail.update({
                    "topology": parent.name,
                    "serial": _read_text(parent / "serial"),
                    "vid": _read_hex(parent / "idVendor") or 0,
                    "pid": _read_hex(parent / "idProduct") or 0,
                })
        uf2.append(detail)
    return sorted(uf2, key=lambda item: str(item["mount"]))


def uf2_mounts() -> list[Path]:
    return [item["mount"] for item in uf2_mount_details()]


def wait_for_new_bootloader(
    before: set[str],
    timeout: float = 30.0,
    expected_topology: str = "",
) -> dict[str, Any]:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        ports = serial_ports(LEGACY_BOOT_VID_PID)
        matching_ports = [
            port for port in ports
            if not expected_topology or port["topology"] == expected_topology
        ]
        new_ports = [port for port in matching_ports if port["port"] not in before]
        if len(new_ports) == 1:
            return new_ports[0]
        if not before and len(matching_ports) == 1:
            return matching_ports[0]
        if len(new_ports) > 1 or (not before and len(matching_ports) > 1):
            raise OSError("multiple Caterina bootloaders found; disconnect unrelated devices")
        time.sleep(0.25)
    raise TimeoutError("Caterina bootloader did not appear")


def wait_for_uf2_target(
    before: set[Path],
    timeout: float = 30.0,
    expected_topology: str = "",
) -> dict[str, Any]:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        mounts = uf2_mount_details()
        new_mounts = [
            item for item in mounts
            if item["mount"] not in before and
            (not expected_topology or item.get("topology") == expected_topology)
        ]
        if len(new_mounts) == 1:
            return new_mounts[0]
        matching_mounts = [
            item for item in mounts
            if not expected_topology or item.get("topology") == expected_topology
        ]
        if not before and len(matching_mounts) == 1:
            return matching_mounts[0]
        if len(new_mounts) > 1 or (not before and len(matching_mounts) > 1):
            raise OSError("multiple UF2 devices found; disconnect unrelated BOOTSEL devices")
        time.sleep(0.25)
    raise TimeoutError("RP2040 UF2 volume did not appear")


def wait_for_uf2(before: set[Path], timeout: float = 30.0) -> Path:
    return wait_for_uf2_target(before, timeout)["mount"]


def download_artifact(artifact: dict[str, Any]) -> tuple[bytes, Path]:
    CACHE_ROOT.mkdir(parents=True, exist_ok=True)
    digest = artifact["sha256"]
    cached = CACHE_ROOT / f"{digest[:16]}-{safe_artifact_filename(artifact['filename'])}"
    if cached.is_file():
        data = cached.read_bytes()
        if sha256_bytes(data) == digest:
            return data, cached
        cached.unlink()
    request = urllib.request.Request(
        artifact["url"],
        headers={"User-Agent": f"Reflex-Adapt-Manager/{MANAGER_VERSION}"},
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        data = response.read()
    if sha256_bytes(data) != digest:
        raise OSError("downloaded firmware SHA256 does not match the catalog")
    if artifact.get("bytes") is not None and len(data) != artifact["bytes"]:
        raise OSError("downloaded firmware size does not match the catalog")
    cached.write_bytes(data)
    return data, cached


def validate_classic_uf2(data: bytes) -> tuple[int, int]:
    if not data or len(data) % 512:
        raise ValueError("Classic2USB firmware is not a whole-block UF2 file")
    blocks: dict[int, tuple[int, int]] = {}
    declared_count: int | None = None
    for offset in range(0, len(data), 512):
        magic0, magic1, flags, address, payload_size, block_number, block_count, family = (
            struct.unpack_from("<IIIIIIII", data, offset)
        )
        magic_end, = struct.unpack_from("<I", data, offset + 508)
        if (magic0, magic1, magic_end) != (UF2_MAGIC0, UF2_MAGIC1, UF2_MAGIC_END):
            raise ValueError("Classic2USB firmware has an invalid UF2 block")
        if flags & 0x1:
            raise ValueError("Classic2USB firmware contains a non-flash UF2 block")
        if not (flags & UF2_FLAG_FAMILY_ID) or family != RP2040_FAMILY_ID:
            raise ValueError("Classic2USB firmware is not tagged for the RP2040 family")
        if not 0 < payload_size <= 476:
            raise ValueError("Classic2USB firmware has an invalid UF2 payload size")
        if declared_count is None:
            declared_count = block_count
        if block_count != declared_count or block_number in blocks:
            raise ValueError("Classic2USB firmware has inconsistent UF2 block numbering")
        blocks[block_number] = (address, payload_size)
    if declared_count != len(blocks) or set(blocks) != set(range(len(blocks))):
        raise ValueError("Classic2USB firmware is missing UF2 blocks")
    start = previous_end = 0
    for block_number in range(len(blocks)):
        address, payload_size = blocks[block_number]
        if block_number == 0:
            start = address
        elif address != previous_end:
            raise ValueError("Classic2USB firmware UF2 addresses are not contiguous")
        previous_end = address + payload_size
    if start != RP2040_FLASH_BASE:
        raise ValueError(f"Classic2USB firmware starts at unexpected address {start:#010x}")
    if previous_end > CLASSIC_RESERVED_FLASH_BASE:
        raise ValueError(
            "Classic2USB firmware overlaps EEPROM or reserved auth/mapping storage "
            f"at {previous_end:#010x}"
        )
    return start, previous_end


def load_receipts() -> list[dict[str, Any]]:
    receipts = load_json(RECEIPTS_PATH, [])
    return receipts if isinstance(receipts, list) else []


def save_receipt(receipt: dict[str, Any]) -> None:
    STATE_ROOT.mkdir(parents=True, exist_ok=True)
    receipts = load_receipts()
    receipts.append(receipt)
    write_text_atomic(RECEIPTS_PATH, json.dumps(receipts[-100:], indent=2) + "\n")


def update_receipt(receipt_id: str, updates: dict[str, Any]) -> None:
    receipts = load_receipts()
    for receipt in reversed(receipts):
        if receipt.get("id") == receipt_id:
            receipt.update(updates)
            write_text_atomic(RECEIPTS_PATH, json.dumps(receipts[-100:], indent=2) + "\n")
            return
    raise OSError(f"installation receipt {receipt_id} was not found")


def backup_once(path: Path) -> Path:
    backup = path.with_name(f"{path.name}.reflex-adapt-manager.bak")
    if not backup.exists():
        shutil.copy2(path, backup)
    return backup


def write_text_atomic(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    mode = path.stat().st_mode if path.exists() else None
    with tempfile.NamedTemporaryFile(
        "w",
        encoding="utf-8",
        newline="\n",
        dir=path.parent,
        prefix=f".{path.name}.",
        delete=False,
    ) as handle:
        temporary = Path(handle.name)
        handle.write(text)
        handle.flush()
        os.fsync(handle.fileno())
    try:
        if mode is not None:
            os.chmod(temporary, mode)
        os.replace(temporary, path)
    finally:
        with contextlib.suppress(OSError):
            temporary.unlink()


def write_bytes_atomic(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    mode = path.stat().st_mode if path.exists() else None
    with tempfile.NamedTemporaryFile(
        "wb",
        dir=path.parent,
        prefix=f".{path.name}.",
        delete=False,
    ) as handle:
        temporary = Path(handle.name)
        handle.write(data)
        handle.flush()
        os.fsync(handle.fileno())
    try:
        if mode is not None:
            os.chmod(temporary, mode)
        os.replace(temporary, path)
    finally:
        with contextlib.suppress(OSError):
            temporary.unlink()


def disable_obsolete_downloader_sections(text: str) -> tuple[str, bool]:
    """Comment exact obsolete Reflex Adapt Downloader database sections."""
    lines = text.splitlines()
    section_pattern = re.compile(r"^\s*\[([^]]+)\]\s*$")
    url_pattern = re.compile(r"^\s*db_url\s*=\s*(\S+)\s*$", re.IGNORECASE)
    sections: list[tuple[int, int, str]] = []
    for index, line in enumerate(lines):
        match = section_pattern.match(line)
        if match:
            if sections:
                start, _end, name = sections[-1]
                sections[-1] = (start, index, name)
            sections.append((index, len(lines), match.group(1).strip().lower()))

    changed = False
    for start, end, name in reversed(sections):
        expected_url = OBSOLETE_DOWNLOADER_DATABASES.get(name)
        if expected_url is None:
            continue
        actual_url = next(
            (
                match.group(1)
                for line in lines[start + 1:end]
                if (match := url_pattern.match(line))
            ),
            None,
        )
        if actual_url != expected_url:
            continue
        disabled = [DOWNLOADER_DISABLED_MARKER]
        disabled.extend(f"; {line}" if line else ";" for line in lines[start:end])
        lines[start:end] = disabled
        changed = True

    suffix = "\n" if text.endswith(("\n", "\r")) else ""
    return "\n".join(lines) + suffix, changed


def restore_downloader_sections(text: str) -> tuple[str, bool]:
    """Re-enable maintained database sections this manager commented out.

    Only a block written by disable_obsolete_downloader_sections() is restored:
    the marker, one commented section header naming a maintained database, and
    its exact db_url. A block is left alone when the same section is already
    active, so a user's own re-enabled copy is never duplicated.
    """
    lines = text.splitlines()
    section_pattern = re.compile(r"^\s*\[([^]]+)\]\s*$")
    url_pattern = re.compile(r"^\s*db_url\s*=\s*(\S+)\s*$", re.IGNORECASE)
    active = {
        match.group(1).strip().lower()
        for line in lines
        if (match := section_pattern.match(line))
    }

    changed = False
    index = 0
    while index < len(lines):
        if lines[index] != DOWNLOADER_DISABLED_MARKER:
            index += 1
            continue
        end = index + 1
        while end < len(lines) and lines[end] != DOWNLOADER_DISABLED_MARKER and (
            lines[end] == ";" or lines[end].startswith("; ")
        ):
            end += 1
        block = ["" if line == ";" else line[2:] for line in lines[index + 1:end]]
        header = section_pattern.match(block[0]) if block else None
        name = header.group(1).strip().lower() if header else ""
        urls = [match.group(1) for line in block[1:] if (match := url_pattern.match(line))]
        restorable = (
            name in RESTORED_DOWNLOADER_DATABASES
            and name not in active
            and urls[:1] == [RESTORED_DOWNLOADER_DATABASES[name]]
            and not any(section_pattern.match(line) for line in block[1:])
        )
        if not restorable:
            index = end
            continue
        lines[index:end] = block
        active.add(name)
        index += len(block)
        changed = True

    suffix = "\n" if text.endswith(("\n", "\r")) else ""
    return "\n".join(lines) + suffix, changed


def update_downloader_entries(root: Path = MISTER_ROOT) -> list[Path]:
    """Disable obsolete and restore maintained Reflex Downloader sections."""
    changed_paths: list[Path] = []
    if not root.is_dir():
        return changed_paths
    candidates = sorted(
        path for path in root.iterdir()
        if path.is_file() and path.name.lower().endswith("downloader.ini")
    )
    for path in candidates:
        text = path.read_text(encoding="utf-8")
        updated, disabled = disable_obsolete_downloader_sections(text)
        updated, restored = restore_downloader_sections(updated)
        if not (disabled or restored):
            continue
        backup_once(path)
        write_text_atomic(path, updated)
        changed_paths.append(path)
    return changed_paths


def load_mister_map_bundle(path: Path) -> dict[str, bytes]:
    raw = path.read_bytes()
    try:
        decoded = zlib.decompress(raw)
    except zlib.error:
        decoded = zlib.decompress(base64.b64decode(b"".join(raw.split()), validate=True))
    bundle = json.loads(decoded.decode("utf-8"))
    if bundle.get("schema_version") != 1 or not isinstance(bundle.get("files"), dict):
        raise ValueError("unsupported MiSTer map bundle")

    files: dict[str, bytes] = {}
    valid_products = {"classic2usb", "adapt-v1-legacy"}
    for name, record in bundle["files"].items():
        parts = Path(name).parts
        if len(parts) != 2 or parts[0] not in valid_products or not parts[1].endswith(".map"):
            raise ValueError(f"invalid MiSTer map path: {name}")
        if not isinstance(record, dict):
            raise ValueError(f"invalid MiSTer map record: {name}")
        data = base64.b64decode(record.get("data", ""), validate=True)
        if len(data) != record.get("bytes") or sha256_bytes(data) != record.get("sha256"):
            raise ValueError(f"MiSTer map checksum mismatch: {name}")
        files[name] = data
    return files


def install_mister_maps(
    product_id: str,
    bundle_path: Path,
    root: Path = MISTER_ROOT,
    overwrite: bool = False,
) -> dict[str, int]:
    prefix = f"{product_id}/"
    maps = {
        name[len(prefix):]: data
        for name, data in load_mister_map_bundle(bundle_path).items()
        if name.startswith(prefix)
    }
    if not maps:
        raise ValueError(f"MiSTer map bundle has no files for {product_id}")

    result = {"installed": 0, "restored": 0, "current": 0, "preserved": 0}
    destination_root = root / "config" / "inputs"
    for filename, data in sorted(maps.items()):
        destination = destination_root / filename
        if not destination.exists():
            write_bytes_atomic(destination, data)
            result["installed"] += 1
        elif destination.read_bytes() == data:
            result["current"] += 1
        elif not overwrite:
            result["preserved"] += 1
        else:
            backup_once(destination)
            write_bytes_atomic(destination, data)
            result["restored"] += 1
    return result


# MiSTer names a Classic2USB map after the adapter's USB serial, which is the
# input mode (ReflexN64, ReflexSat, ...), with spaces as underscores. The same
# list is in the USB drive's remove_maps.sh; tools/test_adapt_manager.py keeps
# both equal to the serials the firmware sends.
CLASSIC2USB_MAP_SERIALS = (
    "ReflexClassic2USB", "Adapt_Classic2USB",
    "ReflexN64", "ReflexGC", "ReflexNES", "ReflexSNES", "ReflexVboy", "ReflexMD",
    "ReflexSat", "ReflexWii", "ReflexPCE", "ReflexPSDS1", "ReflexNeoGeo",
    "Reflex3DO", "ReflexJag", "ReflexDreamcast", "ReflexDriving", "ReflexSMS",
    "ReflexJPC", "ReflexIntv", "ReflexColeco",
    "ReflexPSGun", "ReflexPSWheel", "ReflexPSJogCon",
    "MiSTer-A1_Paddle", "MiSTer-A1_Spinner",
)


def classic2usb_map_files(root: Path = MISTER_ROOT) -> list[Path]:
    """Every map MiSTer keeps for the Classic2USB (USB ID 16d0:1460)."""
    inputs = root / "config" / "inputs"
    patterns: list[str] = []
    for serial in CLASSIC2USB_MAP_SERIALS:
        # Core and menu maps, modifier (_m), unique (_<hash>) and
        # joystick-to-keyboard (_jk) maps, keyboard maps and .bak copies.
        patterns += [f"*16d0_1460_{serial}_*", f"kbd_16d0_1460_{serial}.map*"]
    # MiSTer Main before May 2026 left the serial out of the name.
    patterns += [
        "*input_16d0_1460_v3.map*", "*input_16d0_1460_m_v3.map*",
        "*input_16d0_1460_jk.map*", "kbd_16d0_1460.map*",
    ]
    found: set[Path] = set()
    if inputs.is_dir():
        for pattern in patterns:
            found.update(path for path in inputs.glob(pattern) if path.is_file())
    return sorted(found)


def remove_classic2usb_maps(root: Path = MISTER_ROOT) -> tuple[int, Path | None]:
    """Move the Classic2USB maps into a dated folder beside them.

    MiSTer then uses its default mapping for the adapter. Nothing is deleted.
    Returns the number of files moved and the folder, or (0, None).
    """
    files = classic2usb_map_files(root)
    if not files:
        return 0, None
    destination = root / "config" / "inputs" / time.strftime("classic2usb_removed_%Y%m%d_%H%M%S")
    destination.mkdir(parents=True, exist_ok=True)
    for path in files:
        os.replace(path, destination / path.name)
    return len(files), destination


def map_remove_flow(ui: BlueUi) -> None:
    if not ui.confirm(
        "Remove all Classic2USB MiSTer maps?\n\n"
        "This includes maps you made with Define joystick buttons and the "
        "official ones. They move to config/inputs/classic2usb_removed_<date>, "
        "so MiSTer uses its default mapping. Maps for other controllers stay."
    ):
        return
    try:
        moved, destination = remove_classic2usb_maps()
        if moved:
            ui.status(f"Moved {moved} Classic2USB map file(s) to {destination}. Reload the core.")
        else:
            ui.status("No Classic2USB maps found.")
    except OSError as exc:
        log(f"Classic2USB map removal failed: {exc}")
        ui.error(str(exc))
    ui.pause()


def map_install_flow(ui: BlueUi, product_id: str, bundle_path: Path, overwrite: bool) -> None:
    product_name = "Classic2USB" if product_id == "classic2usb" else "Adapt V1 / Legacy"
    if overwrite and not ui.confirm(
        f"Restore all official {product_name} MiSTer maps?\n\n"
        "Changed official map filenames will be backed up before replacement."
    ):
        return
    try:
        result = install_mister_maps(product_id, bundle_path, overwrite=overwrite)
        ui.status(
            f"{product_name}: {result['installed']} installed, "
            f"{result['restored']} restored, {result['current']} current, "
            f"{result['preserved']} custom preserved."
        )
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        log(f"MiSTer map installation failed: {exc}")
        ui.error(str(exc))
    ui.pause()


def update_legacy_uboot(text: str) -> tuple[str, bool]:
    lines = text.splitlines()
    changed = False
    found = False
    for index, line in enumerate(lines):
        if not line.startswith("v="):
            continue
        found = True
        tokens = line[2:].split()
        for key in ("usbhid.jspoll", "xpad.cpoll"):
            prefix = f"{key}="
            positions = [position for position, token in enumerate(tokens) if token.startswith(prefix)]
            if not positions:
                tokens.append(f"{key}=1")
                changed = True
            elif tokens[positions[0]] != f"{key}=1":
                tokens[positions[0]] = f"{key}=1"
                changed = True
        quirk_prefix = "usbhid.quirks="
        quirk_position = next(
            (position for position, token in enumerate(tokens) if token.startswith(quirk_prefix)),
            None,
        )
        quirk_value = tokens[quirk_position][len(quirk_prefix):] if quirk_position is not None else ""
        quirks = [item for item in quirk_value.split(",") if item]
        if LEGACY_HID_QUIRK not in quirks:
            quirks.append(LEGACY_HID_QUIRK)
            token = quirk_prefix + ",".join(quirks)
            if quirk_position is None:
                tokens.append(token)
            else:
                tokens[quirk_position] = token
            changed = True
        if changed:
            lines[index] = "v=" + " ".join(tokens)
        break
    if not found:
        raise ValueError("u-boot.txt has no v= kernel command line")
    suffix = "\n" if text.endswith(("\n", "\r")) else ""
    return "\n".join(lines) + suffix, changed


def update_legacy_mister_ini(text: str) -> tuple[str, bool]:
    pattern = re.compile(r"^\s*no_merge_vidpid\s*=\s*([^;#\s]+)", re.IGNORECASE)
    for line in text.splitlines():
        match = pattern.match(line)
        if match and match.group(1).lower() == LEGACY_NO_MERGE_VID_PID:
            return text, False
    suffix = "" if not text or text.endswith(("\n", "\r")) else "\n"
    return f"{text}{suffix}no_merge_vidpid={LEGACY_NO_MERGE_VID_PID}\n", True


def legacy_mister_ini_paths(root: Path = MISTER_ROOT) -> list[Path]:
    return sorted(path for path in root.glob("MiSTer*.ini") if path.is_file())


def install_legacy_mister_support(root: Path = MISTER_ROOT) -> list[Path]:
    changed_paths: list[Path] = []
    uboot = root / "linux" / "u-boot.txt"
    if not uboot.is_file():
        raise OSError(f"MiSTer u-boot.txt not found: {uboot}")
    updated, changed = update_legacy_uboot(uboot.read_text(encoding="utf-8"))
    if changed:
        backup_once(uboot)
        write_text_atomic(uboot, updated)
        changed_paths.append(uboot)
    ini_paths = legacy_mister_ini_paths(root)
    if not ini_paths:
        raise OSError(f"No MiSTer*.ini files found in {root}")
    for path in ini_paths:
        updated, changed = update_legacy_mister_ini(path.read_text(encoding="utf-8"))
        if changed:
            backup_once(path)
            write_text_atomic(path, updated)
            changed_paths.append(path)
    return changed_paths


def legacy_support_flow(ui: BlueUi) -> None:
    detail = (
        "Enables 1 ms HID/XInput polling in linux/u-boot.txt.\n"
        "Adds the Adapt V1 composite HID quirk.\n"
        "Keeps both Adapt V1 players separate in every MiSTer*.ini file.\n\n"
        "Original files receive a .reflex-adapt-manager.bak backup.\n\n"
        "Apply these MiSTer settings?"
    )
    if not ui.confirm(detail):
        return
    try:
        changed = install_legacy_mister_support()
        if changed:
            ui.status(f"Updated {len(changed)} MiSTer configuration file(s). Reboot MiSTer.")
        else:
            ui.status("Adapt V1 / Legacy MiSTer support is already configured.")
    except (OSError, ValueError) as exc:
        log(f"Legacy MiSTer support configuration failed: {exc}")
        ui.error(str(exc))
    ui.pause()


def mister_support_flow(
    ui: BlueUi,
    product_ids: set[str],
    bundle_path: Path,
) -> None:
    actions: list[tuple[str, str]] = []
    if "classic2usb" in product_ids:
        actions.extend((
            ("Install missing Classic2USB mappings", "classic-install"),
            ("Restore official Classic2USB mappings", "classic-restore"),
            ("Remove all Classic2USB mappings", "classic-remove"),
        ))
    if "adapt-v1-legacy" in product_ids:
        actions.extend((
            ("Install missing Adapt V1 / Legacy mappings", "legacy-install"),
            ("Restore official Adapt V1 / Legacy mappings", "legacy-restore"),
            ("Configure Adapt V1 / Legacy USB support", "legacy-config"),
        ))
    while actions:
        selected = ui.choose("MiSTer setup / mappings", [label for label, _action in actions])
        if selected is None:
            return
        action = actions[selected][1]
        if action == "classic-install":
            map_install_flow(ui, "classic2usb", bundle_path, overwrite=False)
        elif action == "classic-restore":
            map_install_flow(ui, "classic2usb", bundle_path, overwrite=True)
        elif action == "classic-remove":
            map_remove_flow(ui)
        elif action == "legacy-install":
            map_install_flow(ui, "adapt-v1-legacy", bundle_path, overwrite=False)
        elif action == "legacy-restore":
            map_install_flow(ui, "adapt-v1-legacy", bundle_path, overwrite=True)
        elif action == "legacy-config":
            legacy_support_flow(ui)


def product_by_id(catalog: dict[str, Any], product_id: str) -> dict[str, Any]:
    return next(product for product in catalog["products"] if product["id"] == product_id)


def release_label(release: dict[str, Any]) -> str:
    label = f"v{release['version']}"
    if release.get("recommended"):
        label += "  [Recommended]"
    elif release.get("channel") == "experimental":
        label += "  [Experimental]"
    else:
        label += "  [Previous stable]"
    return label


def artifact_label(artifact: dict[str, Any]) -> str:
    return artifact.get("description") or artifact.get("display_name") or artifact["id"]


def progress_line(stage: str, completed: int, total: int) -> None:
    width = 30
    filled = int(width * completed / max(total, 1))
    print(f"\r{stage:<10} [{'#' * filled}{'.' * (width - filled)}] {completed}/{total}", end="", flush=True)
    if completed == total:
        print()


def classic_runtime_ports(devices: list[dict[str, Any]] | None = None) -> list[dict[str, Any]]:
    runtime: list[dict[str, Any]] = []
    for device in devices or discover_supported_devices():
        if device["product_id"] != "classic2usb":
            continue
        for port_name in device.get("ports", []):
            runtime.append({
                "port": port_name,
                "vid": device["vid"],
                "pid": device["pid"],
                "topology": device["topology"],
            })
    return runtime


def _choose_target(ui: BlueUi, title: str, items: list[dict[str, Any]], label: Callable[[dict[str, Any]], str]) -> dict[str, Any]:
    if len(items) == 1:
        return items[0]
    selected = ui.choose(title, [label(item) for item in items])
    if selected is None:
        raise OSError("firmware installation cancelled")
    return items[selected]


def _confirm_unknown_uf2_target(ui: BlueUi, target: dict[str, Any]) -> None:
    detail = (
        f"Mount: {target['mount']}\n"
        f"USB path: {target.get('topology') or 'unavailable'}\n"
        f"Board ID: {target.get('board_id') or 'RP2040 (unspecified)'}\n\n"
        "RP2040 BOOTSEL does not identify the attached product. Confirm that this "
        "specific device is the target Classic2USB adapter."
    )
    if not ui.confirm(detail):
        raise OSError("firmware installation cancelled; BOOTSEL target was not confirmed")


def _require_rp2040_bootsel(target: dict[str, Any]) -> None:
    board_id = str(target.get("board_id") or "").strip()
    if board_id != RP2040_BOOTSEL_BOARD_ID:
        raise OSError(
            f"UF2 volume {target.get('mount')} reports Board-ID {board_id or 'missing'!r}, "
            f"not {RP2040_BOOTSEL_BOARD_ID}; refusing to write RP2040 firmware"
        )
    vid, pid = target.get("vid") or 0, target.get("pid") or 0
    if (vid or pid) and (vid, pid) != RP2040_BOOTSEL_VID_PID:
        raise OSError(
            f"UF2 volume {target.get('mount')} is USB {vid:04x}:{pid:04x}, "
            "not the RP2040 boot ROM; refusing to write RP2040 firmware"
        )


def flash_classic(
    ui: BlueUi,
    artifact: dict[str, Any],
    *,
    recovery: bool = False,
) -> dict[str, Any]:
    data, _cache = download_artifact(artifact)
    validate_classic_uf2(data)
    existing_mounts = uf2_mount_details()
    before_mounts = {item["mount"] for item in existing_mounts}
    target: dict[str, Any] | None = None
    selected_runtime: dict[str, Any] | None = None
    runtime_ports = [] if recovery else classic_runtime_ports()
    if recovery:
        if not existing_mounts:
            raise OSError("the selected RP2040 BOOTSEL recovery device is no longer mounted")
        target = _choose_target(
            ui,
            "Select the RP2040 BOOTSEL recovery target",
            existing_mounts,
            lambda item: f"{item['mount']}  [{item.get('topology') or 'USB path unavailable'}]",
        )
        _confirm_unknown_uf2_target(ui, target)
    elif len(runtime_ports) == 1:
        selected_runtime = runtime_ports[0]
    elif len(runtime_ports) > 1:
        selected = ui.choose(
            "Select the Classic2USB device to update",
            [f"{port['port']}  [{port['topology']}]" for port in runtime_ports],
        )
        if selected is None:
            raise OSError("firmware installation cancelled")
        selected_runtime = runtime_ports[selected]
    if recovery:
        pass
    elif selected_runtime:
        ui.status(f"Requesting BOOTSEL on {selected_runtime['port']}...")
        try:
            with LinuxSerial(selected_runtime["port"], 115200) as port:
                port.command_lines("BOOTLOADER", "OK:BOOTLOADER", timeout=2.0)
        except (OSError, TimeoutError) as exc:
            log(f"Automatic RP2040 bootloader request failed: {exc}")
    elif not runtime_ports and existing_mounts:
        target = _choose_target(
            ui,
            "Select the RP2040 BOOTSEL target",
            existing_mounts,
            lambda item: f"{item['mount']}  [{item.get('topology') or 'USB path unavailable'}]",
        )
        _confirm_unknown_uf2_target(ui, target)
    else:
        ui.status("Put the target Adapt into BOOTSEL mode now.")
    if target is None:
        target = wait_for_uf2_target(
            before_mounts,
            expected_topology=selected_runtime["topology"] if selected_runtime else "",
        )
        if selected_runtime is None:
            _confirm_unknown_uf2_target(ui, target)
    _require_rp2040_bootsel(target)
    mount = target["mount"]
    destination = mount / safe_artifact_filename(artifact["filename"])
    with destination.open("wb") as handle:
        handle.write(data)
        handle.flush()
        os.fsync(handle.fileno())
    return {
        "topology": target.get("topology") or (selected_runtime or {}).get("topology", ""),
        "mount": str(mount),
        "runtime_port": (selected_runtime or {}).get("port", ""),
    }


def legacy_runtime_ports(devices: list[dict[str, Any]] | None = None) -> list[dict[str, Any]]:
    return [
        port
        for device in devices or discover_supported_devices()
        if device["product_id"] == "adapt-v1-legacy"
        for port in ({
            "port": name,
            "topology": device["topology"],
            "vid": device["vid"],
            "pid": device["pid"],
        } for name in device.get("ports", []))
        if (port["vid"], port["pid"]) == LEGACY_RUNTIME_VID_PID
    ]


def flash_legacy(ui: BlueUi, artifact: dict[str, Any]) -> str:
    data, _cache = download_artifact(artifact)
    existing = serial_ports(LEGACY_BOOT_VID_PID)
    if existing:
        boot = _choose_target(
            ui,
            "Select the Adapt V1 / Legacy bootloader",
            existing,
            lambda item: f"{item['port']}  [{item['topology']}]",
        )
        if not ui.confirm(
            f"Caterina bootloader {boot['port']} at USB path {boot['topology']} cannot "
            "identify the attached product. Confirm it is the selected Adapt V1 / Legacy family."
        ):
            raise OSError("firmware installation cancelled; bootloader target was not confirmed")
    else:
        runtimes = legacy_runtime_ports()
        runtime = _choose_target(
            ui,
            "Select the Adapt V1 / Legacy device to update",
            runtimes,
            lambda item: f"{item['port']}  [{item['topology']}]",
        ) if runtimes else None
        ui.status("Press RESET once on the selected Adapt V1 / Legacy device.")
        boot = wait_for_new_bootloader(
            set(),
            expected_topology=runtime["topology"] if runtime else "",
        )
        if runtime is None and not ui.confirm(
            f"Caterina bootloader {boot['port']} at USB path {boot['topology']} cannot "
            "identify the attached product. Confirm this is the target Adapt V1 / Legacy "
            "device and matches the firmware family selected above."
        ):
            raise OSError("firmware installation cancelled; bootloader target was not confirmed")
    flash_avr109(boot["port"], data, progress_line)
    return boot["topology"]


def verify_classic_install(
    topology: str,
    expected_version: str,
    expected_tag: str = "",
    timeout: float = 15.0,
) -> tuple[str, str]:
    if not topology:
        return "unverified", "USB topology was unavailable, so the rebooted adapter could not be correlated."
    deadline = time.monotonic() + timeout
    saw_runtime = False
    while time.monotonic() < deadline:
        for device in discover_supported_devices():
            if device.get("topology") != topology or device.get("product_id") != "classic2usb":
                continue
            saw_runtime = True
            info = device.get("firmware")
            if not info:
                continue
            if str(info.get("version")) != str(expected_version):
                return "failed", f"Adapter returned firmware v{info.get('version')}; expected v{expected_version}."
            if expected_tag and info.get("tag") not in {expected_tag, f"v{expected_tag}"}:
                return "failed", f"Adapter returned tag {info.get('tag')}; expected {expected_tag}."
            return "verified", f"Rebooted adapter at USB path {topology} reports firmware v{expected_version}."
        time.sleep(0.25)
    if saw_runtime:
        return "unverified", "The adapter rebooted, but this output mode has no usable management serial response."
    return "unverified", "The copied adapter did not reappear before the verification timeout."


def firmware_flow(
    ui: BlueUi,
    catalog: dict[str, Any],
    product_id: str,
    *,
    recovery: bool = False,
) -> None:
    product = product_by_id(catalog, product_id)
    releases = list(product.get("releases", []))
    if product.get("families"):
        family_index = ui.choose(
            "Select Adapt V1 / Legacy firmware family",
            [family["display_name"] for family in product["families"]],
        )
        if family_index is None:
            return
        family = product["families"][family_index]
        releases = [release for release in releases if release.get("family") == family["id"]]
    else:
        family = {"id": product_id, "display_name": product["display_name"]}
    advanced = ui.confirm("Show experimental firmware as well?")
    releases = [release for release in releases if advanced or release.get("channel") != "experimental"]
    if not releases:
        ui.error("No published firmware is available in the catalog.")
        ui.pause()
        return
    release_index = ui.choose("Select firmware release", [release_label(item) for item in releases])
    if release_index is None:
        return
    release = releases[release_index]
    artifacts = release["artifacts"]
    artifact_index = ui.choose(
        f"{family['display_name']} v{release['version']}: select firmware",
        [artifact_label(item) for item in artifacts],
    )
    if artifact_index is None:
        return
    artifact = artifacts[artifact_index]
    confirmation = [
        f"Product:  {product['display_name']}",
        f"Family:   {family['display_name']}",
        f"Version:  {release['version']}",
        f"Firmware: {artifact_label(artifact)}",
        f"SHA256:   {artifact['sha256']}",
    ]
    if product["backend"] == "avr109":
        confirmation.extend([
            "",
            "The Caterina bootloader cannot identify MPG versus Legacy hardware.",
            "Verify the selected family before continuing.",
            "EEPROM settings are preserved; downgrades may require a reset.",
        ])
        prior = next(
            (
                receipt for receipt in reversed(load_receipts())
                if receipt.get("product") == product_id and receipt.get("family") == family["id"] and
                receipt.get("status", "verified") == "verified"
            ),
            None,
        )
        if prior and version_key(release["version"]) < version_key(str(prior.get("version", "0"))):
            confirmation.append(
                f"WARNING: this rolls back from v{prior['version']} to v{release['version']}."
            )
    confirmation.extend(["", "Install this firmware?"])
    if not ui.confirm("\n".join(confirmation)):
        return
    try:
        flashed = (
            flash_classic(ui, artifact, recovery=recovery)
            if product["backend"] == "rp2040-uf2"
            else flash_legacy(ui, artifact)
        )
        topology = flashed.get("topology", "") if isinstance(flashed, dict) else flashed
        receipt_id = f"{time.time_ns()}-{artifact['sha256'][:12]}"
        receipt = {
            "id": receipt_id,
            "installed_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "manager_version": MANAGER_VERSION,
            "product": product_id,
            "family": family["id"],
            "version": release["version"],
            "artifact": artifact["id"],
            "filename": artifact["filename"],
            "sha256": artifact["sha256"],
            "topology": topology,
            "status": "verified" if product["backend"] == "avr109" else "pending",
        }
        if isinstance(flashed, dict):
            receipt["mount"] = flashed.get("mount", "")
        save_receipt(receipt)
        if product["backend"] == "avr109":
            ui.status("Firmware written and verified successfully.")
            if ui.confirm("Configure required Adapt V1 / Legacy MiSTer support now?"):
                try:
                    changed = install_legacy_mister_support()
                    if changed:
                        ui.status("MiSTer support configured. Reboot MiSTer before use.")
                    else:
                        ui.status("MiSTer support was already configured.")
                except (OSError, ValueError) as exc:
                    log(f"Firmware succeeded; Legacy MiSTer support failed: {exc}")
                    ui.error(f"Firmware succeeded, but MiSTer setup failed: {exc}")
        else:
            ui.status("Firmware file validated and copied. Waiting for the adapter to reboot for verification...")
            status, detail = verify_classic_install(
                topology,
                str(release["version"]),
                str(release.get("tag", "")),
            )
            update_receipt(receipt_id, {
                "status": status,
                "verification_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "verification_detail": detail,
            })
            if status == "verified":
                ui.status(f"Firmware copied and verified. {detail}")
            elif status == "failed":
                raise OSError(f"Firmware copy completed, but verification failed: {detail}")
            else:
                ui.status(f"Firmware copy completed; verification unavailable. {detail}")
    except (OSError, TimeoutError, ValueError, urllib.error.URLError) as exc:
        log(f"Firmware installation failed: {exc}")
        ui.error(str(exc))
    ui.pause()


def read_classic_status(port_name: str) -> dict[str, int]:
    with LinuxSerial(port_name, 115200) as port:
        lines = port.command_lines("STATUS", "STATUS CONFIG_NAME=", timeout=3.0)
    pattern = re.compile(
        r"^STATUS INPUT=(\d+) SAVED_INPUT=(\d+) CONFIG_OUT=(\d+) "
        r"RUNTIME_OUT=(\d+)"
    )
    for line in lines:
        match = pattern.match(line.strip())
        if match:
            return {
                "input": int(match.group(1)),
                "saved_input": int(match.group(2)),
                "configured_output": int(match.group(3)),
                "runtime_output": int(match.group(4)),
            }
    raise ValueError("Classic2USB did not report its active mode")


def read_classic_settings(port_name: str, mode: int | None = None) -> list[dict[str, Any]]:
    command = "SET LIST" if mode is None else f"SET LIST {mode}"
    with LinuxSerial(port_name, 115200) as port:
        lines = port.command_lines(command, "OK:SET_LIST", timeout=4.0)
    settings: list[dict[str, Any]] = []
    # Current firmware adds NAME, RUN and LIVE fields; older firmware omits them.
    pattern = re.compile(
        r"^SET ID=(\d+)(?: NAME=\S+)? S=(\w+) T=(\w+) MIN=(-?\d+) MAX=(-?\d+) "
        r"DEF=(-?\d+) VAL=(-?\d+)(?: RUN=-?\d+)?(?: LIVE=\w+)?(?: MODE=(\d+))?$"
    )
    for line in lines:
        match = pattern.match(line.strip())
        if not match:
            continue
        setting_id = int(match.group(1))
        settings.append({
            "id": setting_id,
            "name": SETTING_NAMES[setting_id] if setting_id < len(SETTING_NAMES) else f"Setting {setting_id}",
            "scope": match.group(2),
            "type": match.group(3),
            "min": int(match.group(4)),
            "max": int(match.group(5)),
            "default": int(match.group(6)),
            "value": int(match.group(7)),
            "mode": int(match.group(8)) if match.group(8) else None,
        })
    return settings


def read_classic_visible_setting_ids(port_name: str, mode: int) -> set[int]:
    with LinuxSerial(port_name, 115200) as port:
        lines = port.command_lines(
            f"SET VISIBLE {mode}",
            "OK:SET_VISIBLE",
            timeout=4.0,
        )
    visible: set[int] = set()
    for line in lines:
        match = re.match(r"^VIS SET=(\d+)", line.strip())
        if match:
            visible.add(int(match.group(1)))
    return visible


def read_classic_capabilities(port_name: str) -> set[str]:
    try:
        with LinuxSerial(port_name, 115200) as port:
            lines = port.command_lines("SET HELP", "CAP ", timeout=2.0)
    except (OSError, TimeoutError):
        return set()
    capabilities: set[str] = set()
    for line in lines:
        if line.startswith("CAP "):
            capabilities.update(
                name for name, enabled in re.findall(r"([A-Z_]+)=(\d+)", line)
                if int(enabled)
            )
    return capabilities


def read_psx_timing(port_name: str, mode: int = PLAYSTATION_INPUT_MODE) -> dict[str, int]:
    with LinuxSerial(port_name, 115200) as port:
        lines = port.command_lines(
            f"SET PSX_TIMING GET {mode}", "OK:PSX_TIMING_GET", timeout=3.0,
        )
    pattern = re.compile(
        r"^PSX_TIMING MODE=(\d+) RATE=(\d+) GAP=(\d+) CONFIGURED=(\d+) LIVE=(\d+)$"
    )
    for line in lines:
        match = pattern.match(line.strip())
        if match:
            return {
                "mode": int(match.group(1)), "rate": int(match.group(2)),
                "gap": int(match.group(3)), "configured": int(match.group(4)),
                "live": int(match.group(5)),
            }
    raise ValueError("Classic2USB did not return PSX timing settings")


def set_psx_timing(port_name: str, rate: int, gap: int, mode: int = PLAYSTATION_INPUT_MODE) -> None:
    with LinuxSerial(port_name, 115200) as port:
        port.command_lines(
            f"SET PSX_TIMING SET {rate} {gap} {mode}",
            "OK:PSX_TIMING_SET VERIFY=1",
            timeout=3.0,
        )


def read_rotary_profile(
    port_name: str,
    profile: str,
    mode: int = PLAYSTATION_INPUT_MODE,
) -> dict[str, Any]:
    profile = profile.upper()
    with LinuxSerial(port_name, 115200) as port:
        lines = port.command_lines(
            f"SET ROTARY GET {profile} {mode}", "OK:ROTARY_GET", timeout=3.0,
        )
    pattern = re.compile(
        r"^ROTARY MODE=(\d+) PROFILE=(GENERIC|JOGCON) GAIN=(\d+) "
        r"REVERSE=(\d+) CONFIGURED=(\d+) LIVE=(\d+)$"
    )
    for line in lines:
        match = pattern.match(line.strip())
        if match:
            return {
                "mode": int(match.group(1)), "profile": match.group(2),
                "gain": int(match.group(3)), "reverse": int(match.group(4)),
                "configured": int(match.group(5)), "live": int(match.group(6)),
            }
    raise ValueError("Classic2USB did not return rotary profile settings")


def set_rotary_profile(
    port_name: str,
    profile: str,
    gain: int,
    reverse: int,
    mode: int = PLAYSTATION_INPUT_MODE,
) -> None:
    profile = profile.upper()
    with LinuxSerial(port_name, 115200) as port:
        port.command_lines(
            f"SET ROTARY SET {profile} {gain} {reverse} {mode}",
            "OK:ROTARY_SET VERIFY=1",
            timeout=3.0,
        )


NEGCON_INPUT_NAMES = ("I", "II", "L")


def read_negcon_inputs(port_name: str, mode: int = PLAYSTATION_INPUT_MODE) -> dict[str, int]:
    with LinuxSerial(port_name, 115200) as port:
        lines = port.command_lines(
            f"SET NEGCON GET {mode}", "OK:NEGCON_GET", timeout=3.0,
        )
    pattern = re.compile(r"^NEGCON MODE=(\d+) I=([01]) II=([01]) L=([01]) LIVE=([01])$")
    for line in lines:
        match = pattern.match(line.strip())
        if match:
            return {
                "mode": int(match.group(1)), "I": int(match.group(2)),
                "II": int(match.group(3)), "L": int(match.group(4)),
                "live": int(match.group(5)),
            }
    raise ValueError("Classic2USB did not return neGcon I/II/L settings")


def set_negcon_inputs(
    port_name: str,
    digital: dict[str, int],
    mode: int = PLAYSTATION_INPUT_MODE,
) -> None:
    values = " ".join(str(1 if digital[name] else 0) for name in NEGCON_INPUT_NAMES)
    with LinuxSerial(port_name, 115200) as port:
        port.command_lines(
            f"SET NEGCON SET {values} {mode}",
            "OK:NEGCON_SET VERIFY=1",
            timeout=3.0,
        )


def set_classic_setting(port_name: str, setting: dict[str, Any], value: int) -> None:
    command = f"SET SET {setting['id']} {value}"
    if setting["mode"] is not None:
        command += f" {setting['mode']}"
    with LinuxSerial(port_name, 115200) as port:
        port.command_lines(command, "OK:SET")


def default_classic_setting(port_name: str, setting: dict[str, Any]) -> None:
    command = f"SET DEFAULT {setting['id']}"
    if setting["mode"] is not None:
        command += f" {setting['mode']}"
    with LinuxSerial(port_name, 115200) as port:
        port.command_lines(command, "OK:SET_DEFAULT")


def setting_name(setting_id: int, mode: int | None = None) -> str:
    names = {
        2: "Contrast",
        6: "Sound",
        7: "Sound Events",
        10: "Screensaver Mode",
        11: "Screensaver",
        12: "Quick Menu",
        13: "Down+Start",
        14: "Capture/TPad",
        15: "PC Mode",
        21: "Menu Hold",
        22: "Deadzone",
        24: "DPad",
        25: "Stick Invert",
        26: "Rumble",
        27: "Triggers",
        28: "Spin Speed",
        29: "Button Map",
        30: "GC Z Button" if mode == GAMECUBE_INPUT_MODE else "N64 Z Button",
        31: "Right DPad" if mode == VIRTUAL_BOY_INPUT_MODE else "N64 C Buttons",
        32: "Stick Range",
        33: "Stick Range",
        34: "Device",
        35: "NES Device",
        36: "GunCon Align X",
        37: "GunCon Align Y",
        39: "L Shoulder",
        42: "JogCon Mode",
        43: "Force",
        44: "Wheel Sensitivity",
        45: "Digital Output",
        46: "Wheel Axis",
        56: "2P Merge",
        57: "System Menu",
        58: "Kiosk",
        59: "Pointer Speed",
        60: "Transport",
        61: "BT Output Persona",
        62: "MiSTer Mode",
        64: "USB Drive",
        66: "Rotary Gain",
        67: "Reverse",
        68: "Ends",
        69: "Inactive",
        70: "Wake",
    }
    if setting_id in names:
        return names[setting_id]
    if 0 <= setting_id < len(SETTING_NAMES):
        return SETTING_NAMES[setting_id]
    return f"Setting {setting_id}"


def setting_choices(setting: dict[str, Any]) -> tuple[tuple[int, str], ...]:
    setting_id = setting["id"]
    mode = setting.get("mode")
    if setting_id == 30 and mode == GAMECUBE_INPUT_MODE:
        return ((0, "R1"), (1, "Back"))
    if setting_id == 34:
        fixed = NSO_DEVICE_BY_MODE.get(mode)
        return ((0, "Switch Pro"), (1, fixed)) if fixed else tuple(enumerate(SWITCH_DEVICE_NAMES))
    return SETTING_CHOICES.get(setting_id, ())


def setting_value_label(setting: dict[str, Any]) -> str:
    setting_id = setting["id"]
    value = setting["value"]
    if setting_id == 13:
        return "OSD" if value else "Off"
    if setting_id in BOOLEAN_SETTING_IDS:
        return "On" if value else "Off"
    if setting_id == 7:
        known_mask = sum(mask for mask, _name in SOUND_EVENTS)
        enabled = sum(1 for mask, _name in SOUND_EVENTS if value & mask)
        return "All on" if (value & known_mask) == known_mask else f"{enabled} of {len(SOUND_EVENTS)} on"
    for option_value, label in setting_choices(setting):
        if value == option_value:
            return label
    if setting_id == 2:
        return f"{round(value * 100 / 255)}%"
    return str(value)


def screensaver_mode_timer(mode: int) -> int:
    return (mode - 1) % 3 + 1 if 1 <= mode <= 6 else 0


def screensaver_mode_dims(mode: int) -> bool:
    return 1 <= mode <= 3


def encode_screensaver_mode(timer: int, dim: bool) -> int:
    """An Off timer stores no Dim choice, matching the firmware."""
    return (timer if dim else timer + 3) if 1 <= timer <= 3 else 0


def settings_by_id(settings: list[dict[str, Any]]) -> dict[int, dict[str, Any]]:
    return {setting["id"]: setting for setting in settings}


def choose_setting_value(ui: BlueUi, port_name: str, setting: dict[str, Any]) -> None:
    choices = setting_choices(setting)
    if not choices:
        return
    current_index = next(
        (index for index, (value, _label) in enumerate(choices) if value == setting["value"]),
        0,
    )
    selected = ui.choose(
        setting_name(setting["id"], setting.get("mode")),
        [label for _value, label in choices],
        initial=current_index,
    )
    if selected is not None:
        set_classic_setting(port_name, setting, choices[selected][0])


def choose_numeric_setting(ui: BlueUi, port_name: str, setting: dict[str, Any]) -> None:
    while True:
        raw_value = ui.input_value(
            setting_name(setting["id"], setting.get("mode")),
            f"Enter a value from {setting['min']} to {setting['max']}.",
            str(setting["value"]),
        )
        if raw_value is None:
            return
        try:
            value = int(raw_value, 10)
        except ValueError:
            ui.message("Invalid Value", "Enter a whole number.")
            continue
        if value < setting["min"] or value > setting["max"]:
            ui.message(
                "Invalid Value",
                f"Value must be from {setting['min']} to {setting['max']}.",
            )
            continue
        set_classic_setting(port_name, setting, value)
        return


def edit_setting(ui: BlueUi, port_name: str, setting: dict[str, Any]) -> None:
    if setting["id"] in BOOLEAN_SETTING_IDS:
        toggle_setting(port_name, setting)
    elif setting_choices(setting):
        choose_setting_value(ui, port_name, setting)
    else:
        choose_numeric_setting(ui, port_name, setting)


def toggle_setting(port_name: str, setting: dict[str, Any]) -> None:
    set_classic_setting(port_name, setting, 0 if setting["value"] else 1)


def sound_events_flow(ui: BlueUi, port_name: str, mode: int) -> None:
    while True:
        setting = settings_by_id(read_classic_settings(port_name, mode))[7]
        selected = ui.choose(
            "Sound Events",
            [f"{name:<24} {'On' if setting['value'] & mask else 'Off'}" for mask, name in SOUND_EVENTS],
        )
        if selected is None:
            return
        mask = SOUND_EVENTS[selected][0]
        set_classic_setting(port_name, setting, setting["value"] ^ mask)


def sound_flow(ui: BlueUi, port_name: str, mode: int) -> None:
    while True:
        current = settings_by_id(read_classic_settings(port_name, mode))
        options = [f"{'Mode':<28} {setting_value_label(current[6])}"]
        if 7 in current:
            options.append(f"{'Sound Events':<28} {setting_value_label(current[7])}")
        selected = ui.choose("Sound", options)
        if selected is None:
            return
        if selected == 0:
            choose_setting_value(ui, port_name, current[6])
        else:
            sound_events_flow(ui, port_name, mode)


def screensaver_choice(current: dict[int, dict[str, Any]]) -> int:
    if screensaver_mode_dims(current[10]["value"]):
        return SCREENSAVER_DIM
    return current[11]["value"] if 11 in current else 0


def oled_rows(current: dict[int, dict[str, Any]]) -> list[tuple[str, str]]:
    """The OLED page: Inactive, Inactive timer, Wake and, for Screensaver, the picker."""
    rows = []
    inactive = current.get(69)
    if inactive is not None:
        rows.append((f"{'Inactive':<28} {setting_value_label(inactive)}", "inactive"))
    timer = screensaver_mode_timer(current[10]["value"])
    rows.append((f"{'Inactive timer':<28} {OLED_TIMER_NAMES[timer]}", "timer"))
    wake = current.get(70)
    if wake is not None:
        rows.append((f"{'Wake':<28} {setting_value_label(wake)}", "wake"))
    if inactive is None or inactive["value"] == 0:
        choice = screensaver_choice(current)
        label = "Dim Display" if choice == SCREENSAVER_DIM else dict(SCREENSAVER_ANIMATIONS).get(choice, "Bouncing Logo")
        rows.append((f"{'Screensaver':<28} {label}", "screensaver"))
    return rows


def oled_flow(ui: BlueUi, port_name: str, mode: int) -> None:
    while True:
        current = settings_by_id(read_classic_settings(port_name, mode))
        if 10 not in current:
            return
        rows = oled_rows(current)
        selected = ui.choose("OLED", [label for label, _action in rows])
        if selected is None:
            return
        action = rows[selected][1]
        saver_mode = current[10]
        timer = screensaver_mode_timer(saver_mode["value"])
        dims = screensaver_mode_dims(saver_mode["value"])
        if action == "inactive":
            toggle_setting(port_name, current[69])
        elif action == "wake":
            toggle_setting(port_name, current[70])
        elif action == "timer":
            picked = ui.choose("Inactive timer", list(OLED_TIMER_NAMES), initial=timer)
            if picked is not None:
                set_classic_setting(port_name, saver_mode, encode_screensaver_mode(picked, dims))
        else:
            choices = ((SCREENSAVER_DIM, "Dim Display"), *SCREENSAVER_ANIMATIONS)
            chosen = screensaver_choice(current)
            initial = next((i for i, (value, _label) in enumerate(choices) if value == chosen), 0)
            picked = ui.choose("Screensaver", [label for _value, label in choices], initial=initial)
            if picked is None:
                continue
            value = choices[picked][0]
            if value != SCREENSAVER_DIM and 11 in current:
                set_classic_setting(port_name, current[11], value)
            set_classic_setting(port_name, saver_mode,
                                encode_screensaver_mode(timer, value == SCREENSAVER_DIM))


def hotkeys_flow(ui: BlueUi, port_name: str, mode: int) -> None:
    ordered_ids = (21, 13)
    while True:
        current = settings_by_id(read_classic_settings(port_name, mode))
        selected = ui.choose("Hotkey", [
            f"{setting_name(setting_id):<28} {setting_value_label(current[setting_id])}"
            for setting_id in ordered_ids
        ])
        if selected is None:
            return
        edit_setting(ui, port_name, current[ordered_ids[selected]])


def calibration_flow(ui: BlueUi, port_name: str, mode: int) -> None:
    while True:
        current = settings_by_id(read_classic_settings(port_name, mode))
        summary = (
            f"Left stick   X {current[47]['value']}..{current[48]['value']}  "
            f"Y {current[49]['value']}..{current[50]['value']}\n"
            f"Right stick  X {current[51]['value']}..{current[52]['value']}  "
            f"Y {current[53]['value']}..{current[54]['value']}"
        )
        enabled = setting_value_label(current[55])
        selected = ui.choose("Analog Calibration", [
            f"Automatic calibration          {enabled}",
            f"View learned ranges: {summary.replace(chr(10), ' | ')}",
            "Reset learned ranges",
        ])
        if selected is None:
            return
        if selected == 0:
            toggle_setting(port_name, current[55])
        elif selected == 1:
            ui.message("Analog Calibration", summary)
        elif ui.confirm("Reset the learned analog ranges for this input mode?"):
            for setting_id in range(47, 55):
                default_classic_setting(port_name, current[setting_id])


def manager_input_setting_ids(mode: int, visible: set[int]) -> set[int]:
    exposed = visible & INPUT_SETTING_IDS
    if mode in {GAMECUBE_INPUT_MODE, WII_INPUT_MODE}:
        exposed.discard(32)
    if mode != GAMECUBE_INPUT_MODE:
        exposed.discard(39)
    return exposed


def input_settings_flow(
    ui: BlueUi,
    port_name: str,
    mode: int,
    *,
    title: str | None = None,
    setting_ids: set[int] | None = None,
    exclude_ids: set[int] | None = None,
) -> None:
    current = settings_by_id(read_classic_settings(port_name, mode))
    if setting_ids is None:
        visible = manager_input_setting_ids(
            mode,
            read_classic_visible_setting_ids(port_name, mode),
        )
    else:
        visible = manager_input_setting_ids(mode, setting_ids & current.keys())
    if exclude_ids:
        visible -= exclude_ids
    exposed = sorted(visible - CALIBRATION_SETTING_IDS)
    show_calibration = bool(visible & CALIBRATION_SETTING_IDS)
    if not exposed and not show_calibration:
        ui.message(
            title or "Input Settings",
            "No configurable settings are available for this input mode.",
        )
        return
    while True:
        current = settings_by_id(read_classic_settings(port_name, mode))
        options = [
            f"{setting_name(setting_id, mode):<28} {setting_value_label(current[setting_id])}"
            for setting_id in exposed
        ]
        if show_calibration:
            options.append("Analog Calibration")
        selected = ui.choose(
            title or f"{CLASSIC_INPUT_MODE_NAMES.get(mode, 'Active')} Input Settings",
            options,
        )
        if selected is None:
            return
        if show_calibration and selected == len(exposed):
            calibration_flow(ui, port_name, mode)
            continue
        setting = current[exposed[selected]]
        edit_setting(ui, port_name, setting)


PSX_RATE_LABELS = ("Auto", "1000Hz", "500Hz", "250Hz", "125Hz", "60Hz")
PSX_GAP_LABELS = ("Auto", "0.25ms", "0.5ms", "1ms", "2ms", "3ms", "4ms", "8ms", "16ms")


def psx_timing_flow(ui: BlueUi, port_name: str) -> None:
    while True:
        timing = read_psx_timing(port_name)
        rows = (
            f"{'Poll Rate':<28} {PSX_RATE_LABELS[timing['rate']]}",
            f"{'Command Gap':<28} {PSX_GAP_LABELS[timing['gap']]}",
        )
        selected = ui.choose("PlayStation Poll Timing", list(rows))
        if selected is None:
            return
        labels = PSX_RATE_LABELS if selected == 0 else PSX_GAP_LABELS
        current = timing["rate"] if selected == 0 else timing["gap"]
        picked = ui.choose(rows[selected].split()[0], list(labels), initial=current)
        if picked is None:
            continue
        rate = picked if selected == 0 else timing["rate"]
        gap = picked if selected == 1 else timing["gap"]
        set_psx_timing(port_name, rate, gap)


def rotary_profile_flow(ui: BlueUi, port_name: str, profile: str) -> None:
    profile = profile.upper()
    title = "Generic Rotary Stored Profile" if profile == "GENERIC" else "JogCon Stored Profile"
    gain_choices = SETTING_CHOICES[66]
    gain_labels = dict(gain_choices)
    while True:
        value = read_rotary_profile(port_name, profile)
        gain_label = gain_labels.get(value["gain"], "Choose Gain first")
        rows = (
            f"{'Rotary Gain':<28} {gain_label}",
            f"{'Reverse':<28} {'On' if value['reverse'] else 'Off'}",
        )
        selected = ui.choose(title, list(rows))
        if selected is None:
            return
        if selected == 0:
            current = next((i for i, (gain, _label) in enumerate(gain_choices) if gain == value["gain"]), 0)
            picked = ui.choose("Rotary Gain", [label for _gain, label in gain_choices], initial=current)
            if picked is not None:
                set_rotary_profile(port_name, profile, gain_choices[picked][0], value["reverse"])
        else:
            if value["gain"] == 0:
                ui.message(
                    title,
                    "This stored profile has legacy tuning that has not been converted. "
                    "Choose Rotary Gain first; that explicit choice creates a valid profile "
                    "without silently changing the legacy setting.",
                )
                continue
            picked = ui.choose("Reverse", ["Off", "On"], initial=value["reverse"])
            if picked is not None:
                set_rotary_profile(port_name, profile, value["gain"], picked)


def negcon_inputs_flow(ui: BlueUi, port_name: str) -> None:
    labels = ("Analog", "Digital")
    while True:
        value = read_negcon_inputs(port_name)
        rows = [
            f"{name + ' Button':<28} {labels[value[name]]}" for name in NEGCON_INPUT_NAMES
        ]
        selected = ui.choose("neGcon I / II / L", rows)
        if selected is None:
            return
        name = NEGCON_INPUT_NAMES[selected]
        picked = ui.choose(f"{name} Button", list(labels), initial=value[name])
        if picked is None:
            continue
        set_negcon_inputs(port_name, {**value, name: picked})


def atari_input_settings_flow(ui: BlueUi, port_name: str) -> None:
    labels = ("Atari / C64 / SMS", "Atari Driving Controller")
    modes = (ATARI_INPUT_MODE, ATARI_DRIVING_INPUT_MODE)
    while True:
        selected = ui.choose("Atari / C64 / SMS Settings", list(labels))
        if selected is None:
            return
        input_settings_flow(ui, port_name, modes[selected], title=f"{labels[selected]} Settings")


def playstation_input_settings_flow(ui: BlueUi, port_name: str) -> None:
    capabilities = read_classic_capabilities(port_name)
    rows: list[tuple[str, str]] = [
        ("Gamepads / neGcon", "gamepads"), ("GunCon", "guncon"), ("JogCon", "jogcon"),
    ]
    if "ROTARY_PROFILE" in capabilities:
        rows.extend((
            ("Generic Rotary Stored Profile", "rotary-generic"),
            ("JogCon Stored Profile", "rotary-jogcon"),
        ))
    if "NEGCON_INPUTS" in capabilities:
        rows.append(("neGcon I / II / L", "negcon-inputs"))
    if "PSX_TIMING" in capabilities:
        rows.append(("Poll Rate / Command Gap", "timing"))
    while True:
        selected = ui.choose("PlayStation Settings", [label for label, _action in rows])
        if selected is None:
            return
        action = rows[selected][1]
        if action == "gamepads":
            input_settings_flow(
                ui,
                port_name,
                PLAYSTATION_INPUT_MODE,
                title="PlayStation / neGcon Settings",
                exclude_ids=GUNCON_SETTING_IDS | JOGCON_SETTING_IDS,
            )
        elif action == "guncon":
            input_settings_flow(
                ui,
                port_name,
                PLAYSTATION_INPUT_MODE,
                title="GunCon Settings",
                setting_ids=GUNCON_SETTING_IDS,
            )
        elif action == "jogcon":
            input_settings_flow(
                ui,
                port_name,
                JOGCON_INPUT_MODE,
                title="JogCon Settings",
                setting_ids=JOGCON_SETTING_IDS,
            )
        elif action == "rotary-generic":
            rotary_profile_flow(ui, port_name, "GENERIC")
        elif action == "rotary-jogcon":
            rotary_profile_flow(ui, port_name, "JOGCON")
        elif action == "negcon-inputs":
            negcon_inputs_flow(ui, port_name)
        else:
            psx_timing_flow(ui, port_name)


def grouped_input_mode_is_active(mode: int, active_mode: int) -> bool:
    if mode == ATARI_INPUT_MODE:
        return active_mode in {ATARI_INPUT_MODE, ATARI_DRIVING_INPUT_MODE}
    if mode == PLAYSTATION_INPUT_MODE:
        return active_mode in {PLAYSTATION_INPUT_MODE, JOGCON_INPUT_MODE}
    return mode == active_mode


def input_mode_settings_flow(
    ui: BlueUi,
    port_name: str,
    active_mode: int,
) -> None:
    while True:
        selected = ui.choose(
            "Input Mode Settings",
            [
                f"{CLASSIC_INPUT_MODE_NAMES[mode]}"
                f"{' (active)' if grouped_input_mode_is_active(mode, active_mode) else ''}"
                for mode in CONFIGURABLE_INPUT_MODES
            ],
        )
        if selected is None:
            return
        mode = CONFIGURABLE_INPUT_MODES[selected]
        if mode == ATARI_INPUT_MODE:
            atari_input_settings_flow(ui, port_name)
        elif mode == PLAYSTATION_INPUT_MODE:
            playstation_input_settings_flow(ui, port_name)
        else:
            input_settings_flow(ui, port_name, mode)


def system_setting_rows(current: dict[int, dict[str, Any]]) -> list[tuple[str, int | str]]:
    """System Settings rows in OLED order; an int action edits that setting id."""
    rows: list[tuple[str, int | str]] = []
    for setting_id in (15, 62, 64, 2, 58):
        if setting_id in current:
            rows.append((f"{setting_name(setting_id):<28} {setting_value_label(current[setting_id])}", setting_id))
    if 71 in current:
        rows.append((f"{setting_name(71):<28} {setting_value_label(current[71])}", 71))
    if 13 in current and 21 not in current:
        rows.append((f"{'Hotkeys':<28} {setting_value_label(current[13])}", 13))
    else:
        rows.append(("Hotkey", "hotkeys"))
    if 6 in current:
        rows.append(("Sound", "sound"))
    if 10 in current:
        rows.append(("OLED", "oled"))
    rows += [("Input Mode Settings", "input"), ("Factory reset all settings", "factory-reset"),
             ("Reboot device", "reboot")]
    return rows


def settings_flow(ui: BlueUi) -> None:
    ports = classic_runtime_ports()
    if len(ports) != 1:
        ui.error(f"Expected one connected Classic2USB device; found {len(ports)}.")
        ui.pause()
        return
    port_name = ports[0]["port"]
    flows = {"hotkeys": hotkeys_flow, "sound": sound_flow, "oled": oled_flow,
             "input": input_mode_settings_flow}
    try:
        while True:
            status = read_classic_status(port_name)
            mode = status["input"]
            current = settings_by_id(read_classic_settings(port_name, mode))
            rows = system_setting_rows(current)
            selected = ui.choose("Classic2USB Settings", [label for label, _action in rows])
            if selected is None:
                return
            action = rows[selected][1]
            if isinstance(action, int):
                edit_setting(ui, port_name, current[action])
            elif action in flows:
                flows[action](ui, port_name, mode)
            elif action == "factory-reset":
                if ui.confirm("Factory reset settings, remaps, turbo, and hotkeys?"):
                    with LinuxSerial(port_name, 115200) as port:
                        port.command_lines("SET FACTORY_RESET CONFIRM", "OK:SET_FACTORY_RESET")
            elif action == "reboot":
                try:
                    with LinuxSerial(port_name, 115200) as port:
                        port.write(b"REBOOT\n")
                        time.sleep(0.15)
                except OSError:
                    pass
                ui.status("Reboot requested.")
                return
    except (OSError, TimeoutError, ValueError, KeyError) as exc:
        ui.error(str(exc))
        ui.pause()


def latest_release(catalog: dict[str, Any], product_id: str, family: str | None = None) -> dict[str, Any] | None:
    product = product_by_id(catalog, product_id)
    releases = [
        release for release in product.get("releases", [])
        if release.get("channel") == "stable" and
        (family is None or release.get("family") == family)
    ]
    if not releases:
        return None
    return max(releases, key=lambda release: version_key(str(release["version"])))


def classic_update_status(catalog: dict[str, Any], devices: list[dict[str, Any]]) -> str:
    update = catalog.get("_update_check", {})
    error_suffix = ""
    if update.get("attempted") and not update.get("online"):
        error_suffix = " (online check failed; cached catalog)" if update.get("cached") else " (online check failed)"
    latest = latest_release(catalog, "classic2usb")
    if latest is None:
        return "No cached Classic2USB release" + error_suffix
    current_versions = [
        device["firmware"]["version"]
        for device in devices
        if device["product_id"] == "classic2usb" and device.get("firmware")
    ]
    if not current_versions:
        return f"Latest v{latest['version']} (connect in a management mode to compare){error_suffix}"
    outdated = [
        version for version in current_versions
        if version_key(str(latest["version"])) > version_key(str(version))
    ]
    if outdated:
        oldest = min(outdated, key=version_key)
        return (
            f"Update available for {len(outdated)} of {len(current_versions)} adapter(s): "
            f"v{oldest} -> v{latest['version']}{error_suffix}"
        )
    current = max(current_versions, key=version_key)
    return f"Current v{current}; latest v{latest['version']}{error_suffix}"


def device_summary(
    devices: list[dict[str, Any]] | None = None,
    catalog: dict[str, Any] | None = None,
) -> list[str]:
    detected = devices if devices is not None else discover_supported_devices()
    lines: list[str] = []
    for device in detected:
        if device.get("mount"):
            lines.append(f"{device['label']}: {device['mount']}")
            lines.append(
                f"  USB {device.get('topology') or 'unknown'}  |  "
                f"Board ID {device.get('board_id') or 'unknown'}"
            )
            continue
        usb_id = f"{device['vid']:04x}:{device['pid']:04x}"
        mode = device.get("product_name") or "USB device"
        ports = ", ".join(device.get("ports", [])) or "no CDC serial"
        lines.append(f"{device['label']}: {usb_id}  {mode}")
        lines.append(f"  USB {device['topology']}  |  {ports}")
        if device.get("firmware"):
            info = device["firmware"]
            lines.append(f"  Firmware v{info['version']}  |  {info['hardware']}")
        elif device["product_id"] == "classic2usb":
            lines.append("  Firmware version unavailable in this output mode.")
    if catalog is not None and any(device["product_id"] == "classic2usb" for device in detected):
        lines.append(f"Update check: {classic_update_status(catalog, detected)}")
    return lines or ["No supported Reflex Adapt devices found."]


def history_flow(ui: BlueUi) -> None:
    receipts = load_receipts()
    lines = ["No manager installation receipts have been recorded."] if not receipts else [
        f"{receipt.get('installed_utc', '?')}  {receipt.get('family', '?')} "
        f"v{receipt.get('version', '?')}  {receipt.get('artifact', '?')}  "
        f"[{receipt.get('status', 'historical')}]"
        for receipt in reversed(receipts[-20:])
    ]
    ui.message("Firmware installation history", "\n".join(lines))


def main_actions(catalog: dict[str, Any], devices: list[dict[str, Any]]) -> list[tuple[str, str]]:
    product_ids = {device["product_id"] for device in devices}
    classic_present = "classic2usb" in product_ids
    legacy_present = "adapt-v1-legacy" in product_ids
    unknown_uf2_present = "rp2040-uf2-unknown" in product_ids
    classic_cdc = bool(classic_runtime_ports(devices))
    actions: list[tuple[str, str]] = []
    if devices:
        actions.append(("Connected device details", "devices"))
    if classic_present and classic_cdc:
        actions.append((
            f"Classic2USB firmware / rollback  [{classic_update_status(catalog, devices)}]",
            "classic-firmware",
        ))
    if unknown_uf2_present:
        actions.append((
            "Classic2USB recovery firmware  [requires target confirmation]",
            "classic-recovery",
        ))
    if classic_cdc:
        actions.append(("Classic2USB settings", "classic-settings"))
    if legacy_present:
        actions.append(("Adapt V1 / Legacy firmware / rollback", "legacy-firmware"))
    if classic_present or legacy_present:
        actions.append(("MiSTer setup / mappings", "mister-support"))
    update = catalog.get("_update_check", {})
    rescan_label = "Rescan devices and firmware updates"
    if update.get("attempted") and not update.get("online"):
        rescan_label += "  [last online check failed]"
    actions.extend([
        ("Installation history", "history"),
        (rescan_label, "rescan"),
        ("Exit", "exit"),
    ])
    return actions


def run_interactive(catalog: dict[str, Any], catalog_path: Path, map_bundle_path: Path) -> int:
    STATE_ROOT.mkdir(parents=True, exist_ok=True)
    updated_databases = update_downloader_entries()
    if updated_databases:
        log(
            "Updated Reflex Adapt Downloader database entries in: "
            + ", ".join(str(path) for path in updated_databases)
        )
    with BlueUi() as ui:
        ui.info("Scanning connected devices and checking firmware updates...")
        devices = discover_supported_devices()
        while True:
            product_ids = {device["product_id"] for device in devices}
            classic_present = "classic2usb" in product_ids
            unknown_uf2_present = "rp2040-uf2-unknown" in product_ids
            actions = main_actions(catalog, devices)
            device_text = f"{len(devices)} device{'s' if len(devices) != 1 else ''} connected"
            if classic_present:
                if classic_runtime_ports(devices):
                    device_text += f"  |  {classic_update_status(catalog, devices)}"
                else:
                    device_text += "  |  Classic2USB: DInput required"
            elif unknown_uf2_present:
                device_text += "  |  Unidentified RP2040 BOOTSEL"
            selected = ui.choose(
                f"v{MANAGER_VERSION}  |  {device_text}",
                [label for label, _action in actions],
                allow_back=False,
            )
            if selected is None:
                return 0
            action = actions[selected][1]
            if action == "devices":
                ui.message(
                    "Connected Reflex Adapt devices",
                    "\n".join(device_summary(devices, catalog)),
                )
            elif action == "classic-firmware":
                firmware_flow(ui, catalog, "classic2usb")
            elif action == "classic-recovery":
                firmware_flow(ui, catalog, "classic2usb", recovery=True)
            elif action == "legacy-firmware":
                firmware_flow(ui, catalog, "adapt-v1-legacy")
            elif action == "classic-settings":
                settings_flow(ui)
            elif action == "mister-support":
                mister_support_flow(ui, product_ids, map_bundle_path)
            elif action == "history":
                history_flow(ui)
            elif action == "rescan":
                ui.info("Scanning connected devices and checking firmware updates...")
                catalog = load_catalog(catalog_path, refresh=True)
                devices = discover_supported_devices()
            elif action == "exit":
                return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Reflex Adapt Manager")
    parser.add_argument("--catalog", type=Path, required=True)
    parser.add_argument("--map-bundle", type=Path, default=DEFAULT_MAP_BUNDLE)
    parser.add_argument("--offline", action="store_true", help="Use only the embedded catalog")
    parser.add_argument("--validate-catalog", action="store_true")
    parser.add_argument("--list-firmware", action="store_true")
    parser.add_argument("--status", action="store_true", help="Print detected devices and update state")
    args = parser.parse_args(argv)
    interactive = not (args.validate_catalog or args.list_firmware or args.status)
    if interactive and sys.stdout.isatty():
        ui = BlueUi()
        if ui.dialog:
            ui.info("Checking connected devices and published firmware...")
    try:
        catalog = load_catalog(args.catalog, refresh=not args.offline)
    except (OSError, ValueError) as exc:
        print(f"Catalog error: {exc}", file=sys.stderr)
        return 2
    if args.validate_catalog:
        print("OK: Adapt Manager catalog is valid")
        return 0
    if args.list_firmware:
        for product in catalog["products"]:
            for release in product.get("releases", []):
                for artifact in release["artifacts"]:
                    print(
                        f"{product['id']}\t{release.get('family', product['id'])}\t"
                        f"{release['version']}\t{artifact['id']}\t{artifact['sha256']}"
                    )
        return 0
    if args.status:
        devices = discover_supported_devices()
        for line in device_summary(devices, catalog):
            print(line)
        return 0 if devices else 1
    return run_interactive(catalog, args.catalog, args.map_bundle)


if __name__ == "__main__":
    raise SystemExit(main())
