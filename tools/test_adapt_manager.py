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

    assert manager.setting_name(43) == "JogCon Force"
    assert manager.setting_name(36) == "GunCon X Alignment"
    assert manager.setting_name(37) == "GunCon Y Alignment"
    assert manager.setting_name(44) == "JogCon Range"
    assert manager.setting_name(45) == "JogCon Digital Output"
    assert manager.setting_name(30, 9) == "N64 Z Output"
    assert manager.setting_name(30, 10) == "GameCube Z Output"
    assert manager.setting_choices({"id": 30, "mode": 9}) == ((0, "L1"), (1, "L2"))
    assert manager.setting_choices({"id": 30, "mode": 10}) == ((0, "R1"), (1, "Back"))
    assert manager.setting_value_label({"id": 2, "value": 24}) == "9%"
    assert manager.setting_value_label({"id": 6, "value": 1}) == "On"
    assert manager.setting_value_label({"id": 10, "value": 5}) == "Animate after 5 minutes"
    assert manager.setting_value_label({"id": 11, "value": 9}) == "Breakout AI"
    assert manager.setting_value_label({"id": 43, "value": 15}) == "High"
    assert manager.setting_value_label({"id": 45, "value": 2}) == "L1 / R1"
    assert manager.setting_value_label({"id": 55, "value": 1}) == "On"
    assert {0, 1, 3, 4, 5, 8, 9, 15, 16, 17, 18, 19, 20, 23, 38, 39, 40, 41, 42}.isdisjoint(
        manager.INPUT_SETTING_IDS | manager.GLOBAL_DISPLAY_SETTING_IDS |
        manager.GLOBAL_SCREENSAVER_SETTING_IDS | manager.GLOBAL_HOTKEY_SETTING_IDS
    ), "Manager must not expose mode selectors, diagnostics, LEDs, JVS, or retired settings"
    assert 57 in manager.GLOBAL_HOTKEY_SETTING_IDS
    assert len(manager.SOUND_EVENTS) == 10
    assert ((1 << 6) | (1 << 7), "Setting changed") in manager.SOUND_EVENTS
    assert all(name != "Coin" for _mask, name in manager.SOUND_EVENTS)
    assert manager.CONFIGURABLE_INPUT_MODES == (
        2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 18, 19,
    )
    assert manager.CLASSIC_INPUT_MODE_NAMES[9] == "N64"
    assert 32 not in manager.manager_input_setting_ids(10, {30, 32, 55})
    assert 32 not in manager.manager_input_setting_ids(11, {32, 55})
    assert manager.manager_input_setting_ids(9, {30, 33}) == {30, 33}
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

    obsolete_legacy = (
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
    downloader_text = obsolete_legacy + "\n" + active_manager + "\n" + obsolete_reflexadapt
    updated_downloader, changed = manager.disable_obsolete_downloader_sections(downloader_text)
    assert changed
    assert "; [misteraddons/reflex-adapt-legacy]" in updated_downloader
    assert "; [misteraddons/reflexadapt]" in updated_downloader
    assert active_manager.strip() in updated_downloader
    assert manager.disable_obsolete_downloader_sections(updated_downloader) == (
        updated_downloader,
        False,
    )
    wrong_url = obsolete_legacy.replace("main/", "old/")
    assert manager.disable_obsolete_downloader_sections(wrong_url) == (wrong_url, False)

    with tempfile.TemporaryDirectory() as downloader_root_text:
        downloader_root = Path(downloader_root_text)
        primary = downloader_root / "downloader.ini"
        alternate = downloader_root / "reflex_downloader.ini"
        unrelated = downloader_root / "other.ini"
        primary.write_text(downloader_text, encoding="utf-8")
        alternate.write_text(obsolete_legacy, encoding="utf-8")
        unrelated.write_text(obsolete_reflexadapt, encoding="utf-8")
        changed_paths = manager.disable_obsolete_downloader_entries(downloader_root)
        assert changed_paths == [primary, alternate]
        assert primary.with_name("downloader.ini.reflex-adapt-manager.bak").is_file()
        assert alternate.with_name("reflex_downloader.ini.reflex-adapt-manager.bak").is_file()
        assert unrelated.read_text(encoding="utf-8") == obsolete_reflexadapt
        assert manager.disable_obsolete_downloader_entries(downloader_root) == []

    map_bundle_path = MANAGER_DIR / "mister_maps.bin"
    bundled_maps = manager.load_mister_map_bundle(map_bundle_path)
    assert len([name for name in bundled_maps if name.startswith("classic2usb/")]) == 4
    assert len([name for name in bundled_maps if name.startswith("adapt-v1-legacy/")]) == 346
    with tempfile.TemporaryDirectory() as bundle_text:
        rebuilt_bundle = Path(bundle_text) / "mister_maps.bin"
        manifest = map_bundle_generator.build_bundle(
            ROOT / "tools" / "release_assets" / "classic2usb" / "mister" / "config" / "inputs",
            map_bundle_generator.DEFAULT_LEGACY_REPO / "mister" / "config" / "inputs",
            rebuilt_bundle,
        )
        assert len(manifest["files"]) == 350
        assert rebuilt_bundle.read_bytes() == map_bundle_path.read_bytes(), "map bundle is stale"

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
        assert installed == {"installed": 3, "restored": 0, "current": 0, "preserved": 1}
        assert custom_path.read_bytes() == b"custom map"
        restored = manager.install_mister_maps(
            "classic2usb", rebuilt_bundle, mister, overwrite=True
        )
        assert restored == {"installed": 0, "restored": 1, "current": 3, "preserved": 0}
        assert custom_path.read_bytes() == bundled_maps[f"classic2usb/{custom_name}"]
        assert custom_path.with_name(f"{custom_name}.reflex-adapt-manager.bak").read_bytes() == b"custom map"

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
