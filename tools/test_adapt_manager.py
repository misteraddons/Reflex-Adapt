from __future__ import annotations

import importlib.util
import json
import subprocess
import tempfile
from pathlib import Path
from zipfile import ZipFile


ROOT = Path(__file__).resolve().parents[1]
MANAGER_DIR = ROOT / "tools" / "adapt_manager"


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise AssertionError(f"cannot import {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def ihex_record(address: int, kind: int, payload: bytes) -> str:
    body = bytes([len(payload), address >> 8, address & 0xFF, kind]) + payload
    checksum = (-sum(body)) & 0xFF
    return ":" + (body + bytes([checksum])).hex().upper()


class PublicMapRemovalFixture:
    """File set for trees without the USB-drive generator (public release).

    REMOVED mirrors tools/test_usb_drive_image.py, which development checks
    use directly together with the firmware's serial names; KEPT holds the
    same kinds of non-Classic2USB names.
    """

    REMOVED = [
        "N64_input_16d0_1460_ReflexN64_v3.map",
        "input_16d0_1460_ReflexSNES_v3.map",
        "PSX_input_16d0_1460_ReflexPSDS1_m_v3.map",
        "N64_input_16d0_1460_ReflexN64_v3.map.bak",
        "Saturn_input_16d0_1460_ReflexSat_1a2b3c4d_v3.map",
        "N64_input_16d0_1460_ReflexN64_jk.map",
        "kbd_16d0_1460_ReflexJag.map",
        "NES_input_16d0_1460_ReflexClassic2USB_v3.map",
        "input_16d0_1460_Adapt_Classic2USB_v3.map",
        "PSX_input_16d0_1460_MiSTer-A1_Paddle_v3.map",
        "input_16d0_1460_v3.map",
        "N64_input_16d0_1460_m_v3.map",
    ]
    KEPT = [
        "input_16d0_1460_ReflexOther_v3.map",
        "input_16d0_127e_ReflexN64_v3.map",
        "N64_input_16d0_1460_ReflexN64x_v3.map",
        "input_16c0_0480_MiSTer-A1_Paddle_v3.map",
        "input_045e_028e_v3.map",
    ]


def check_classic2usb_map_removal(manager) -> None:
    """Remove all Classic2USB mappings matches the drive's remove_maps.sh."""
    serials = manager.CLASSIC2USB_MAP_SERIALS
    assert len(serials) == len(set(serials))
    drive_test_path = ROOT / "tools" / "test_usb_drive_image.py"
    if drive_test_path.is_file():
        drive_test = load_module("test_usb_drive_image", drive_test_path)
        assert set(serials) == drive_test.classic2usb_serials(), (
            sorted(set(serials) ^ drive_test.classic2usb_serials()))
        assert PublicMapRemovalFixture.REMOVED == drive_test.REMOVED
        assert len(PublicMapRemovalFixture.KEPT) == len(drive_test.KEPT)
    else:
        drive_test = PublicMapRemovalFixture
        classic_maps = (ROOT / "tools" / "release_assets" / "classic2usb" / "mister" /
                        "config" / "inputs").glob("*.map")
        published = {path.name.split("_16d0_1460_", 1)[1].rsplit("_v3.map", 1)[0]
                     for path in classic_maps}
        assert published and published <= set(serials), sorted(published - set(serials))
        print("SKIP: USB-drive serial comparison; no tools/test_usb_drive_image.py")

    def run(files_module) -> tuple[list[str], list[str], int, int]:
        with tempfile.TemporaryDirectory() as root_text:
            root = Path(root_text)
            inputs = root / "config" / "inputs"
            inputs.mkdir(parents=True)
            for name in drive_test.REMOVED + drive_test.KEPT + [
                    "N64_input_16d0_1460_ReflexN64_v3.map.reflex-adapt-manager.bak"]:
                (inputs / name).write_text(name, encoding="utf-8")
            moved, destination = files_module(root)
            again, _ = files_module(root)
            moved_names = sorted(path.name for path in destination.iterdir()) if destination else []
            left = sorted(path.name for path in inputs.iterdir() if path.is_file())
            return moved_names, left, moved, again

    expected = sorted(drive_test.REMOVED + ["N64_input_16d0_1460_ReflexN64_v3.map.reflex-adapt-manager.bak"])
    moved_names, left, moved, again = run(manager.remove_classic2usb_maps)
    assert moved_names == expected, sorted(set(moved_names) ^ set(expected))
    assert left == sorted(drive_test.KEPT), sorted(set(left) ^ set(drive_test.KEPT))
    assert moved == len(expected) and again == 0
    with tempfile.TemporaryDirectory() as empty_text:
        assert manager.remove_classic2usb_maps(Path(empty_text)) == (0, None)

    # Negative control: without the separator after the serial, a longer
    # serial that starts with a Classic2USB one is moved too.
    original = manager.CLASSIC2USB_MAP_SERIALS
    manager.CLASSIC2USB_MAP_SERIALS = tuple(f"{serial}*" for serial in original)
    try:
        broken_moved, _left, _moved, _again = run(manager.remove_classic2usb_maps)
    finally:
        manager.CLASSIC2USB_MAP_SERIALS = original
    assert broken_moved != expected, "negative control passed: serial separator is not checked"
    print(f"OK: Remove all Classic2USB mappings moves {len(expected)} files, keeps "
          f"{len(drive_test.KEPT)}; negative control rejected")


def main() -> int:
    manager = load_module("reflex_adapt_manager", MANAGER_DIR / "reflex_adapt_manager.py")
    renderer = load_module("render_manager", MANAGER_DIR / "render_manager.py")
    map_bundle_generator = load_module(
        "generate_map_bundle",
        MANAGER_DIR / "generate_map_bundle.py",
    )
    descriptor_module = load_module(
        "generate_release_descriptor",
        MANAGER_DIR / "generate_release_descriptor.py",
    )
    downloader = load_module(
        "generate_downloader_db",
        MANAGER_DIR / "generate_downloader_db.py",
    )

    catalog_path = MANAGER_DIR / "catalog.json"
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    manager.validate_catalog(catalog)
    ui = manager.BlueUi()
    ui.dialog = "/usr/bin/dialog"
    dialog_calls = []

    def fake_dialog(args, capture=False):
        dialog_calls.append((args, capture))
        return subprocess.CompletedProcess(args, 0, stdout="", stderr="1")

    ui._run = fake_dialog
    assert ui.choose("Test menu", ["One", "Two"], allow_back=False) == 1
    assert "--menu" in dialog_calls[0][0]
    assert "--nocancel" in dialog_calls[0][0]
    assert ui.confirm("Continue?")
    assert "--yesno" in dialog_calls[1][0]
    products = {product["id"]: product for product in catalog["products"]}
    assert set(products) == {"classic2usb", "adapt-v1-legacy"}
    legacy = products["adapt-v1-legacy"]
    assert [family["id"] for family in legacy["families"]] == ["mpg", "legacy"]
    assert any(release["family"] == "mpg" and release["recommended"] for release in legacy["releases"])
    assert any(release["family"] == "legacy" and release["recommended"] for release in legacy["releases"])
    assert not list(MANAGER_DIR.rglob("*.hex")), "Legacy binaries must remain in Reflex-Adapt-Legacy"

    # Names and values follow the OLED menus (see test_config_tool_settings_parity.py).
    assert manager.setting_name(43) == "Force"
    assert manager.setting_name(36) == "GunCon Align X"
    assert manager.setting_name(37) == "GunCon Align Y"
    assert manager.setting_name(44) == "Wheel Sensitivity"
    assert manager.setting_name(45) == "Digital Output"
    assert manager.setting_name(30, 9) == "N64 Z Button"
    assert manager.setting_name(30, 10) == "GC Z Button"
    assert manager.setting_name(39, 10) == "L Shoulder"
    assert manager.setting_name(62) == "MiSTer Mode"
    assert manager.setting_name(69) == "Inactive"
    assert manager.setting_name(99) == "Setting 99", "unknown ids must not raise"
    assert manager.setting_choices({"id": 30, "mode": 9}) == ((0, "L1"), (1, "L2"))
    assert manager.setting_choices({"id": 30, "mode": 10}) == ((0, "R1"), (1, "Back"))
    assert manager.setting_choices({"id": 39, "mode": 10}) == ((0, "ZL"), (1, "L"))
    assert manager.setting_choices({"id": 34, "mode": 9}) == ((0, "Switch Pro"), (1, "N64 NSO"))
    assert manager.setting_value_label({"id": 2, "value": 192}) == "75%"
    assert manager.setting_value_label({"id": 2, "value": 24}) == "9%"
    assert manager.setting_value_label({"id": 6, "value": 2}) == "High"
    assert manager.setting_value_label({"id": 13, "value": 1}) == "OSD"
    assert manager.setting_value_label({"id": 11, "value": 9}) == "Breakout AI"
    assert manager.setting_value_label({"id": 22, "value": 30}) == "30%"
    assert manager.setting_value_label({"id": 43, "value": 15}) == "High"
    assert manager.setting_value_label({"id": 45, "value": 0}) == "Left/Right"
    assert manager.setting_value_label({"id": 55, "value": 1}) == "On"
    assert all(value not in {5, 7, 11} for value, _name in manager.SCREENSAVER_ANIMATIONS), (
        "disabled animations fall back to the logo and must not be offered")
    for mode in range(7):
        assert manager.encode_screensaver_mode(
            manager.screensaver_mode_timer(mode), manager.screensaver_mode_dims(mode)) == mode
    assert manager.encode_screensaver_mode(0, True) == 0
    assert {0, 1, 3, 4, 5, 8, 9, 15, 16, 17, 18, 19, 20, 23, 38, 40, 41}.isdisjoint(
        manager.INPUT_SETTING_IDS | manager.GLOBAL_DISPLAY_SETTING_IDS |
        manager.GLOBAL_OLED_SETTING_IDS | manager.GLOBAL_HOTKEY_SETTING_IDS
    ), "Manager must not expose mode selectors, diagnostics, LEDs, JVS, or retired settings"
    assert manager.GLOBAL_HOTKEY_SETTING_IDS == {13, 21}, "Classic2USB Hotkey page rows"
    assert len(manager.SOUND_EVENTS) == 11
    assert (1 << 6, "Turbo") in manager.SOUND_EVENTS
    assert (1 << 7, "Mode Change") in manager.SOUND_EVENTS
    assert all(name != "Coin" for _mask, name in manager.SOUND_EVENTS)
    # SET LIST lines as current firmware prints them, plus the older format.
    set_list_lines = [
        "SET ID=2 NAME=DISPLAY_CONTRAST S=G T=U8 MIN=1 MAX=254 DEF=128 VAL=192 RUN=192 LIVE=1",
        "SET ID=22 NAME=DEADZONE S=M T=U8 MIN=0 MAX=30 DEF=0 VAL=10 RUN=10 LIVE=1 MODE=9",
        "SET ID=68 NAME=DRIVING_WRAP S=M T=U8 MIN=0 MAX=1 DEF=0 VAL=1 LIVE=NA MODE=3",
        "SET ID=10 S=G T=U8 MIN=0 MAX=6 DEF=4 VAL=5",
        "OK:SET_LIST",
    ]

    class FakeSerial:
        def __init__(self, *_args):
            pass

        def __enter__(self):
            return self

        def __exit__(self, *_args):
            return False

        def command_lines(self, _command, _terminator, timeout=2.0):
            return set_list_lines

    original_serial = manager.LinuxSerial
    manager.LinuxSerial = FakeSerial
    try:
        parsed = {item["id"]: item for item in manager.read_classic_settings("/dev/ttyACM0")}
    finally:
        manager.LinuxSerial = original_serial
    assert parsed[2]["value"] == 192 and parsed[2]["mode"] is None
    assert parsed[22]["value"] == 10 and parsed[22]["mode"] == 9
    assert parsed[68]["value"] == 1 and parsed[68]["mode"] == 3
    assert parsed[10]["value"] == 5
    assert manager.CONFIGURABLE_INPUT_MODES == (
        2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 18, 19, 20, 21,
    )
    assert manager.CLASSIC_INPUT_MODE_NAMES[9] == "N64"
    assert 32 not in manager.manager_input_setting_ids(10, {30, 32, 55})
    assert 32 not in manager.manager_input_setting_ids(11, {32, 55})
    assert manager.manager_input_setting_ids(9, {30, 33}) == {30, 33}
    assert manager.manager_input_setting_ids(10, {30, 39}) == {30, 39}
    assert manager.manager_input_setting_ids(9, {30, 39}) == {30}
    assert manager.manager_input_setting_ids(14, {39, 59, 66, 67}) == {59, 66, 67}
    assert manager.CLASSIC_INPUT_MODE_NAMES[20] == "Intellivision"
    assert manager.CLASSIC_INPUT_MODE_NAMES[21] == "ColecoVision"
    assert manager.grouped_input_mode_is_active(2, 3)
    assert manager.grouped_input_mode_is_active(14, 15)
    assert "management mode" in manager.classic_update_status(
        {"products": [{"id": "classic2usb", "releases": [{
            "version": "1.0.0", "channel": "stable",
        }]}]},
        [{"product_id": "classic2usb", "firmware": None}],
    )

    class ModePicker:
        def __init__(self):
            self.results = iter((manager.CONFIGURABLE_INPUT_MODES.index(9), None))

        def choose(self, _title, _options):
            return next(self.results)

    selected_modes = []
    original_input_settings_flow = manager.input_settings_flow
    manager.input_settings_flow = (
        lambda _ui, _port_name, selected_mode: selected_modes.append(selected_mode)
    )
    try:
        manager.input_mode_settings_flow(ModePicker(), "/dev/ttyACM0", 6)
    finally:
        manager.input_settings_flow = original_input_settings_flow
    assert selected_modes == [9], "Dreamcast users should be able to select N64 settings"

    class GroupPicker:
        def __init__(self, *results):
            self.results = iter(results)

        def choose(self, _title, _options):
            return next(self.results)

    grouped_calls = []
    original_input_settings_flow = manager.input_settings_flow
    manager.input_settings_flow = lambda *args, **kwargs: grouped_calls.append((args, kwargs))
    try:
        manager.atari_input_settings_flow(GroupPicker(1, None), "/dev/ttyACM0")
        manager.playstation_input_settings_flow(GroupPicker(0, 1, 2, None), "/dev/ttyACM0")
    finally:
        manager.input_settings_flow = original_input_settings_flow
    assert grouped_calls[0][0][2] == 3, "Atari Driving must live under Atari/C64/SMS"
    assert grouped_calls[1][0][2] == 14
    assert grouped_calls[1][1]["exclude_ids"] == manager.GUNCON_SETTING_IDS | manager.JOGCON_SETTING_IDS
    assert grouped_calls[2][0][2] == 14
    assert grouped_calls[2][1]["setting_ids"] == manager.GUNCON_SETTING_IDS
    assert grouped_calls[3][0][2] == 15
    assert grouped_calls[3][1]["setting_ids"] == manager.JOGCON_SETTING_IDS

    def setting(setting_id, value):
        return {"id": setting_id, "value": value, "mode": None}

    system = {item["id"]: item for item in (
        setting(15, 0), setting(62, 0), setting(64, 0), setting(2, 128), setting(58, 0),
        setting(6, 1), setting(7, 0x7EF), setting(10, 4), setting(11, 1), setting(69, 0),
    )}
    rows = manager.system_setting_rows(system)
    assert [action for _label, action in rows] == [
        15, 62, 64, 2, 58, "hotkeys", "sound", "oled", "input", "factory-reset", "reboot",
    ], "System Settings rows follow the OLED System Settings order"
    assert rows[0][0].startswith("PC Mode") and rows[1][0].startswith("MiSTer Mode")
    fixed_system = {**system, 13: setting(13, 1)}
    fixed_rows = manager.system_setting_rows(fixed_system)
    assert fixed_rows[5][1] == 13 and fixed_rows[5][0].startswith("Hotkeys"), \
        "Fixed firmware exposes the master switch directly"
    assert "hotkeys" not in [action for _label, action in fixed_rows]
    with_ok_back = {**fixed_system, 71: setting(71, 2)}
    ok_back_rows = manager.system_setting_rows(with_ok_back)
    assert any(action == 71 and "South" in label for label, action in ok_back_rows)
    fixed_system[21] = setting(21, 1)
    assert manager.system_setting_rows(fixed_system)[5][1] == "hotkeys", \
        "Older firmware keeps its original Hotkey page"
    assert [action for _label, action in manager.oled_rows(system)] == [
        "inactive", "timer", "screensaver"]
    assert "Starfield" in manager.oled_rows(system)[2][0]
    system[69] = setting(69, 1)
    assert [action for _label, action in manager.oled_rows(system)] == ["inactive", "timer"], (
        "Blank OLED hides the Screensaver picker")
    del system[69]
    assert [action for _label, action in manager.oled_rows(system)] == ["timer", "screensaver"], (
        "firmware without OLED Inactive keeps the timer and picker")

    class OledPicker:
        def __init__(self, *results):
            self.results = iter(results)

        def choose(self, _title, _options, **_kwargs):
            return next(self.results)

    oled_state = {10: setting(10, 5), 11: setting(11, 1), 69: setting(69, 0)}
    writes = []
    original_read_classic_settings = manager.read_classic_settings
    original_set_classic_setting = manager.set_classic_setting
    manager.read_classic_settings = lambda _port_name, _mode: list(oled_state.values())

    def record_write(_port_name, item, value):
        writes.append((item["id"], value))
        oled_state[item["id"]] = setting(item["id"], value)

    manager.set_classic_setting = record_write
    try:
        # Inactive timer 5m -> 10m keeps the animation, then Dim Display.
        manager.oled_flow(OledPicker(1, 3, 2, 0, None), "/dev/ttyACM0", 9)
    finally:
        manager.read_classic_settings = original_read_classic_settings
        manager.set_classic_setting = original_set_classic_setting
    assert writes == [(10, 6), (10, 3)], writes

    with tempfile.TemporaryDirectory() as sysfs_text:
        sysfs = Path(sysfs_text)
        usb_root = sysfs / "usb"
        tty_root = sysfs / "tty"
        guncon = usb_root / "1-1.4"
        guncon.mkdir(parents=True)
        tty_root.mkdir()
        (guncon / "idVendor").write_text("16d0\n", encoding="ascii")
        (guncon / "idProduct").write_text("127e\n", encoding="ascii")
        (guncon / "bcdDevice").write_text("0103\n", encoding="ascii")
        (guncon / "manufacturer").write_text("MiSTerAddons\n", encoding="ascii")
        (guncon / "product").write_text("ReflexPSGun\n", encoding="ascii")
        (guncon / "serial").write_text("ReflexPSGun\n", encoding="ascii")
        old_usb, old_tty = manager.SYS_USB_ROOT, manager.SYS_TTY_ROOT
        manager.SYS_USB_ROOT, manager.SYS_TTY_ROOT = usb_root, tty_root
        try:
            detected = manager.discover_supported_devices(probe_serial=False)
        finally:
            manager.SYS_USB_ROOT, manager.SYS_TTY_ROOT = old_usb, old_tty
        assert len(detected) == 1
        assert detected[0]["product_id"] == "classic2usb"
        actions = {action for _label, action in manager.main_actions(catalog, detected)}
        assert "classic-firmware" not in actions
        assert "classic-settings" not in actions
        assert "legacy-firmware" not in actions
        assert "mister-support" in actions
        detected[0]["ports"] = ["/dev/ttyACM0"]
        actions = {action for _label, action in manager.main_actions(catalog, detected)}
        assert "classic-firmware" in actions
        assert "classic-settings" in actions

    empty_actions = {action for _label, action in manager.main_actions(catalog, [])}
    assert "classic-firmware" not in empty_actions
    assert "legacy-firmware" not in empty_actions
    assert "mister-support" not in empty_actions
    assert {"history", "rescan", "exit"}.issubset(empty_actions)

    # Reflex-Adapt-Legacy is the maintained Adapt V1 feed: it stays active, and
    # the exact block that Manager 0.4.0 commented out is restored.
    legacy_feed = (
        "[misteraddons/reflex-adapt-legacy]\n"
        "db_url = https://raw.githubusercontent.com/misteraddons/"
        "Reflex-Adapt-Legacy/main/reflex-adapt-legacy.json.zip\n"
    )
    obsolete_reflexadapt = (
        "[misteraddons/reflexadapt]\n"
        "db_url = https://github.com/misteraddons/Reflex-Adapt/"
        "raw/main/reflexadapt.json.zip\n"
    )
    active_manager = (
        "[misteraddons/reflex-adapt-manager]\n"
        "db_url = https://github.com/misteraddons/Reflex-Adapt/"
        "raw/main/reflex-adapt-manager.json.zip\n"
    )

    def disabled_by_manager(section: str) -> str:
        # The exact text the earlier Manager wrote for a disabled section.
        return "".join(
            [manager.DOWNLOADER_DISABLED_MARKER + "\n"]
            + [f"; {line}\n" if line else ";\n" for line in section.splitlines()]
        )

    downloader_text = legacy_feed + "\n" + active_manager + "\n" + obsolete_reflexadapt
    updated_downloader, changed = manager.disable_obsolete_downloader_sections(downloader_text)
    assert changed
    assert legacy_feed + "\n" + active_manager in updated_downloader
    assert "; [misteraddons/reflex-adapt-legacy]" not in updated_downloader
    assert "; [misteraddons/reflexadapt]" in updated_downloader
    assert manager.disable_obsolete_downloader_sections(updated_downloader) == (
        updated_downloader,
        False,
    )
    wrong_url = obsolete_reflexadapt.replace("main/", "old/")
    assert manager.disable_obsolete_downloader_sections(wrong_url) == (wrong_url, False)

    previously_disabled = (
        disabled_by_manager(legacy_feed + "\n") + active_manager + "\n"
        + disabled_by_manager(obsolete_reflexadapt)
    )
    original = legacy_feed + "\n" + active_manager + "\n" + disabled_by_manager(obsolete_reflexadapt)
    assert manager.restore_downloader_sections(previously_disabled) == (original, True)
    assert manager.restore_downloader_sections(original) == (original, False)
    assert manager.disable_obsolete_downloader_sections(original) == (original, False)
    # Negative controls: a user's re-enabled copy, a changed URL, a plain
    # comment, and the retired database are never un-commented.
    reenabled = legacy_feed + previously_disabled
    assert manager.restore_downloader_sections(reenabled) == (reenabled, False)
    moved = disabled_by_manager(legacy_feed.replace("main/", "old/"))
    assert manager.restore_downloader_sections(moved) == (moved, False)
    user_comment = "".join(f"; {line}\n" for line in legacy_feed.splitlines())
    assert manager.restore_downloader_sections(user_comment) == (user_comment, False)
    retired = disabled_by_manager(obsolete_reflexadapt)
    assert manager.restore_downloader_sections(retired) == (retired, False)

    with tempfile.TemporaryDirectory() as downloader_root_text:
        downloader_root = Path(downloader_root_text)
        primary = downloader_root / "downloader.ini"
        alternate = downloader_root / "reflex_downloader.ini"
        current = downloader_root / "update_all_downloader.ini"
        unrelated = downloader_root / "other.ini"
        primary.write_text(previously_disabled.replace(
            disabled_by_manager(obsolete_reflexadapt), obsolete_reflexadapt), encoding="utf-8")
        alternate.write_text(disabled_by_manager(legacy_feed), encoding="utf-8")
        current.write_text(legacy_feed + "\n" + active_manager, encoding="utf-8")
        unrelated.write_text(obsolete_reflexadapt, encoding="utf-8")
        changed_paths = manager.update_downloader_entries(downloader_root)
        assert changed_paths == [primary, alternate]
        assert primary.read_text(encoding="utf-8") == original
        assert alternate.read_text(encoding="utf-8") == legacy_feed
        assert primary.with_name("downloader.ini.reflex-adapt-manager.bak").is_file()
        assert alternate.with_name("reflex_downloader.ini.reflex-adapt-manager.bak").is_file()
        assert current.read_text(encoding="utf-8") == legacy_feed + "\n" + active_manager
        assert not current.with_name("update_all_downloader.ini.reflex-adapt-manager.bak").exists()
        assert unrelated.read_text(encoding="utf-8") == obsolete_reflexadapt
        assert manager.update_downloader_entries(downloader_root) == []

    map_bundle_path = MANAGER_DIR / "mister_maps.bin"
    bundled_maps = manager.load_mister_map_bundle(map_bundle_path)
    classic_map_root = ROOT / "tools" / "release_assets" / "classic2usb" / "mister" / "config" / "inputs"
    classic_map_count = len(list(classic_map_root.glob("*.map")))
    assert len([name for name in bundled_maps if name.startswith("classic2usb/")]) == classic_map_count
    assert len([name for name in bundled_maps if name.startswith("adapt-v1-legacy/")]) == 346
    # The legacy maps live in a Reflex-Adapt-Legacy checkout (sibling, or
    # REFLEX_ADAPT_LEGACY_REPO where the generator honors it). The rebuild
    # comparison runs only where that checkout exists.
    legacy_map_root = map_bundle_generator.DEFAULT_LEGACY_REPO / "mister" / "config" / "inputs"
    with tempfile.TemporaryDirectory() as bundle_text:
        rebuilt_bundle = Path(bundle_text) / "mister_maps.bin"
        if legacy_map_root.is_dir():
            manifest = map_bundle_generator.build_bundle(
                classic_map_root,
                legacy_map_root,
                rebuilt_bundle,
            )
            assert len(manifest["files"]) == classic_map_count + 346
            assert rebuilt_bundle.read_bytes() == map_bundle_path.read_bytes(), "map bundle is stale"
        else:
            print(f"SKIP: map bundle rebuild check; no legacy checkout at {legacy_map_root}")
            rebuilt_bundle.write_bytes(map_bundle_path.read_bytes())

        mister = Path(bundle_text) / "mister"
        custom_name = next(
            name.split("/", 1)[1]
            for name in bundled_maps
            if name.startswith("classic2usb/")
        )
        custom_path = mister / "config" / "inputs" / custom_name
        custom_path.parent.mkdir(parents=True)
        custom_path.write_bytes(b"custom map")
        installed = manager.install_mister_maps("classic2usb", rebuilt_bundle, mister)
        assert installed == {"installed": classic_map_count - 1, "restored": 0, "current": 0, "preserved": 1}
        assert custom_path.read_bytes() == b"custom map"
        restored = manager.install_mister_maps(
            "classic2usb", rebuilt_bundle, mister, overwrite=True
        )
        assert restored == {"installed": 0, "restored": 1, "current": classic_map_count - 1, "preserved": 0}
        assert custom_path.read_bytes() == bundled_maps[f"classic2usb/{custom_name}"]
        assert custom_path.with_name(f"{custom_name}.reflex-adapt-manager.bak").read_bytes() == b"custom map"

    check_classic2usb_map_removal(manager)

    uboot = (
        "v=loglevel=4 quiet usbhid.jspoll=0 xpad.cpoll=0 "
        "usbhid.quirks=0x1234:0xabcd:0x040\n"
    )
    updated, changed = manager.update_legacy_uboot(uboot)
    assert changed
    assert "usbhid.jspoll=1" in updated
    assert "xpad.cpoll=1" in updated
    assert " quiet " in updated
    assert "usbhid.quirks=0x1234:0xabcd:0x040,0x16d0:0x127e:0x040" in updated
    assert manager.update_legacy_uboot(updated) == (updated, False)

    ini = "[MiSTer]\nkey=value\n"
    updated_ini, changed = manager.update_legacy_mister_ini(ini)
    assert changed
    assert updated_ini.endswith("no_merge_vidpid=0x16d0127e\n")
    assert manager.update_legacy_mister_ini(updated_ini) == (updated_ini, False)

    with tempfile.TemporaryDirectory() as mister_text:
        mister = Path(mister_text)
        (mister / "linux").mkdir()
        (mister / "linux" / "u-boot.txt").write_text(uboot, encoding="utf-8")
        (mister / "MiSTer.ini").write_text(ini, encoding="utf-8")
        changed_paths = manager.install_legacy_mister_support(mister)
        assert len(changed_paths) == 2
        assert (mister / "linux" / "u-boot.txt.reflex-adapt-manager.bak").is_file()
        assert (mister / "MiSTer.ini.reflex-adapt-manager.bak").is_file()
        assert manager.install_legacy_mister_support(mister) == []

    source = (
        ihex_record(0, 0, bytes(range(16))) + "\n" +
        ihex_record(0, 1, b"") + "\n"
    ).encode("ascii")
    pages = manager.parse_ihex(source)
    assert list(pages) == [0]
    assert pages[0][:16] == bytes(range(16))
    assert pages[0][16:] == bytes([0xFF]) * 112

    with tempfile.TemporaryDirectory() as temp_text:
        temp = Path(temp_text)
        rendered = temp / "reflex_adapt_manager.sh"
        renderer.render(
            MANAGER_DIR / "reflex_adapt_manager.sh.in",
            MANAGER_DIR / "reflex_adapt_manager.py",
            catalog_path,
            rendered,
        )
        committed = (
            ROOT / "tools" / "release_assets" / "adapt-manager" / "mister" /
            "Scripts" / "reflex_adapt_manager.sh"
        )
        assert (
            committed.read_text(encoding="utf-8") == rendered.read_text(encoding="utf-8")
        ), "rendered manager script is stale"
        text = rendered.read_text(encoding="utf-8")
        assert "{{REFLEX_ADAPT_MANAGER" not in text
        assert "__REFLEX_MANAGER_PYTHON__" in text
        assert "__REFLEX_MANAGER_CATALOG__" in text
        assert "__REFLEX_MANAGER_MAPS__" in text
        embedded_maps = text.split("__REFLEX_MANAGER_MAPS__\n", 1)[1].split(
            "\n__REFLEX_MANAGER_END__", 1
        )[0]
        embedded_map_path = temp / "embedded_maps.b64"
        embedded_map_path.write_text(embedded_maps, encoding="ascii")
        assert manager.load_mister_map_bundle(embedded_map_path) == bundled_maps
        assert "--backtitle" in text and "--menu" in text
        assert "--validate-catalog|--list-firmware|--status) CLEAR_SCREEN=0" in text
        assert "printf '\\033[2J\\033[H'" in text
        assert "REFLEX ADAPT MANAGER                                 |" not in text

        uf2 = temp / "ReflexClassic2USB_v1.2.3.uf2"
        uf2.write_bytes(b"UF2-test-data")
        release_descriptor = descriptor_module.descriptor(uf2, "1.2.3")
        manager.validate_catalog(release_descriptor)
        assert release_descriptor["manager_min_version"] == manager.MANAGER_VERSION
        release = release_descriptor["products"][0]["releases"][0]
        assert release["hardware_compatibility"] == {
            "policy": "all-published-revisions",
            "product_family": "Classic2USB",
            "mcu": "RP2040",
            "flash": "2 MB",
        }
        artifact = release["artifacts"][0]
        assert artifact["sha256"] == manager.sha256_bytes(uf2.read_bytes())
        assert artifact["url"].endswith("/v1.2.3/ReflexClassic2USB_v1.2.3.uf2")

    database = downloader.build_database()
    assert database["db_id"] == "misteraddons/reflex-adapt-manager"
    assert "Scripts/reflex_adapt_manager.sh" in database["files"]
    assert len(database["files"]) == 1
    assert all(record["overwrite"] for record in database["files"].values())
    assert not any(path.endswith(".map") for path in database["files"])
    assert not any("ReflexPSGun" in path for path in database["files"])
    assert not (ROOT / downloader.DB_NAME).exists()
    with ZipFile(ROOT / f"{downloader.DB_NAME}.zip") as archive:
        published_database = json.loads(archive.read(downloader.DB_NAME))
    assert published_database["db_id"] == database["db_id"]
    assert published_database["files"] == database["files"]
    assert published_database["folders"] == database["folders"]

    print("OK: Adapt Manager catalog, flasher parser, package, and Downloader checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
