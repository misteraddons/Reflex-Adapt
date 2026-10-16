#!/usr/bin/env python3
"""Fault-oriented host checks for Reflex Adapt Manager update/recovery paths."""

from __future__ import annotations

import importlib.util
import json
import os
import struct
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "tools" / "adapt_manager" / "reflex_adapt_manager.py"
SPEC = importlib.util.spec_from_file_location("reflex_adapt_manager_under_test", SOURCE)
assert SPEC and SPEC.loader
manager = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(manager)


def catalog(version: str = "1.2.3") -> dict:
    return {
        "schema_version": 1,
        "manager_min_version": "0.5.0",
        "products": [{
            "id": "classic2usb", "display_name": "Classic2USB", "backend": "rp2040-uf2",
            "releases": [{
                "version": version, "channel": "stable", "artifacts": [{
                    "id": "classic", "filename": "classic.uf2", "url": "https://example/firmware",
                    "sha256": "0" * 64,
                }],
            }],
        }],
    }


def uf2(blocks: int = 2, family: int = manager.RP2040_FAMILY_ID) -> bytes:
    output = bytearray()
    for number in range(blocks):
        block = bytearray(512)
        struct.pack_into(
            "<IIIIIIII", block, 0,
            manager.UF2_MAGIC0, manager.UF2_MAGIC1, manager.UF2_FLAG_FAMILY_ID,
            manager.RP2040_FLASH_BASE + number * 256, 256, number, blocks, family,
        )
        struct.pack_into("<I", block, 508, manager.UF2_MAGIC_END)
        output.extend(block)
    return bytes(output)


class Ui:
    def __init__(self, confirms: list[bool] | None = None, choices: list[int | None] | None = None):
        self.confirms = iter(confirms or [])
        self.choices = iter(choices or [])
        self.messages: list[str] = []

    def confirm(self, prompt: str) -> bool:
        self.messages.append(prompt)
        return next(self.confirms)

    def choose(self, title: str, _options: list[str], initial: int = 0) -> int | None:
        self.messages.append(title)
        return next(self.choices)

    def status(self, message: str) -> None:
        self.messages.append(message)

    def message(self, title: str, message: str) -> None:
        self.messages.append(f"{title}: {message}")


def main() -> int:
    too_new = catalog()
    too_new["manager_min_version"] = "99.0.0"
    try:
        manager.validate_catalog(too_new)
        raise AssertionError("newer manager requirement was accepted")
    except ValueError as exc:
        assert "requires" in str(exc)

    for unsafe in ("../classic.uf2", "sub/classic.uf2", "..\\classic.uf2", ".hidden.uf2", "/abs.uf2"):
        hostile = catalog()
        hostile["products"][0]["releases"][0]["artifacts"][0]["filename"] = unsafe
        try:
            manager.validate_catalog(hostile)
            raise AssertionError(f"unsafe catalog filename {unsafe!r} was accepted")
        except ValueError as exc:
            assert "unsafe artifact filename" in str(exc)
    assert manager.safe_artifact_filename("ReflexClassic2USB_v1.2.3.uf2") == "ReflexClassic2USB_v1.2.3.uf2"

    for wrong in ({"mount": Path("/media/X"), "board_id": "RP2350"},
                  {"mount": Path("/media/X"), "board_id": ""},
                  {"mount": Path("/media/X"), "board_id": "RPI-RP2", "vid": 0x239A, "pid": 0x0001}):
        try:
            manager._require_rp2040_bootsel(wrong)
            raise AssertionError(f"non-RP2040 UF2 volume {wrong} was accepted")
        except OSError as exc:
            assert "refusing" in str(exc)
    manager._require_rp2040_bootsel({"mount": Path("/media/RPI-RP2"), "board_id": "RPI-RP2",
                                     "vid": 0x2E8A, "pid": 0x0003})
    malformed = catalog()
    malformed["manager_min_version"] = "next"
    try:
        manager.validate_catalog(malformed)
        raise AssertionError("malformed manager_min_version was accepted")
    except ValueError as exc:
        assert "invalid manager_min_version" in str(exc)
    assert manager.version_key("0.5") == manager.version_key("0.5.0")

    old_fetch = manager.fetch_json
    pages: list[str] = []
    release_descriptor = catalog("2.0.0")
    first_page = [{"draft": False, "prerelease": False, "assets": []} for _ in range(100)]
    first_page[0]["assets"] = [{
        "name": "adapt-manager-release.json", "browser_download_url": "https://example/descriptor",
    }]
    def paged_fetch(url: str):
        pages.append(url)
        if url == "https://example/descriptor":
            return release_descriptor
        return first_page if "&page=1" in url else []
    manager.fetch_json = paged_fetch
    try:
        descriptors, errors = manager.discover_classic_release_descriptors()
        assert descriptors == [release_descriptor] and not errors
        assert any("page=2" in url for url in pages), "release discovery did not paginate"
        manager.fetch_json = lambda _url: (_ for _ in ()).throw(OSError("network down"))
        assert manager.discover_classic_release_descriptors()[1]
    finally:
        manager.fetch_json = old_fetch

    valid = uf2()
    assert manager.validate_classic_uf2(valid) == (manager.RP2040_FLASH_BASE, manager.RP2040_FLASH_BASE + 512)
    corrupt = bytearray(valid)
    corrupt[0] ^= 1
    wrong_family = uf2(family=0x12345678)
    missing = valid[:512]
    for payload, message in ((bytes(corrupt), "invalid UF2"), (wrong_family, "RP2040"), (missing, "missing")):
        try:
            manager.validate_classic_uf2(payload)
            raise AssertionError(f"bad UF2 accepted: {message}")
        except ValueError as exc:
            assert message in str(exc), str(exc)
    protected_blocks = ((manager.CLASSIC_RESERVED_FLASH_BASE - manager.RP2040_FLASH_BASE) // 256) + 1
    try:
        manager.validate_classic_uf2(uf2(protected_blocks))
        raise AssertionError("UF2 overlapping preserved flash was accepted")
    except ValueError as exc:
        assert "overlaps" in str(exc)

    old_details = manager.uf2_mount_details
    manager.uf2_mount_details = lambda: [
        {"mount": Path("/media/a"), "topology": "1-1"},
        {"mount": Path("/media/b"), "topology": "1-2"},
    ]
    try:
        selected = manager.wait_for_uf2_target(set(), timeout=0.1, expected_topology="1-2")
        assert selected["mount"] == Path("/media/b")
    finally:
        manager.uf2_mount_details = old_details

    with tempfile.TemporaryDirectory() as temp_text:
        temp = Path(temp_text)
        usb = temp / "sys" / "devices" / "platform" / "xhci" / "usb1" / "1-2"
        partition = usb / "interface1" / "host0" / "target0" / "lun0" / "block" / "sda" / "sda1"
        partition.mkdir(parents=True)
        (usb / "idVendor").write_text("2e8a\n", encoding="ascii")
        (usb / "idProduct").write_text("0003\n", encoding="ascii")
        (usb / "serial").write_text("E661-test\n", encoding="ascii")
        class_block = temp / "sys" / "class" / "block"
        class_block.mkdir(parents=True)
        os.symlink(partition, class_block / "sda1", target_is_directory=True)
        mount = temp / "media" / "RPI-RP2"
        mount.mkdir(parents=True)
        (mount / "INFO_UF2.TXT").write_text("Board-ID: RPI-RP2\n", encoding="ascii")
        proc_mounts = temp / "proc_mounts"
        proc_mounts.write_text(f"/dev/sda1 {mount} vfat rw 0 0\n", encoding="utf-8")
        old_block, old_proc = manager.SYS_BLOCK_ROOT, manager.PROC_MOUNTS_PATH
        manager.SYS_BLOCK_ROOT, manager.PROC_MOUNTS_PATH = class_block, proc_mounts
        try:
            details = manager.uf2_mount_details()
            assert details[0]["topology"] == "1-2"
            assert details[0]["serial"] == "E661-test"
            assert details[0]["mount"] == mount
        finally:
            manager.SYS_BLOCK_ROOT, manager.PROC_MOUNTS_PATH = old_block, old_proc

    old_discover = manager.discover_supported_devices
    manager.discover_supported_devices = lambda: [{
        "product_id": "classic2usb", "topology": "1-2",
        "firmware": {"version": "1.2.3", "tag": "v1.2.3"},
    }]
    try:
        assert manager.verify_classic_install("1-2", "1.2.3", "v1.2.3", timeout=0.1)[0] == "verified"
        assert manager.verify_classic_install("1-2", "9.9.9", timeout=0.1)[0] == "failed"
        assert manager.verify_classic_install("", "1.2.3", timeout=0.1)[0] == "unverified"
    finally:
        manager.discover_supported_devices = old_discover

    unknown = {"mount": Path("/media/RPI-RP2"), "topology": "1-3", "board_id": "RPI-RP2"}
    try:
        manager._confirm_unknown_uf2_target(Ui([False]), unknown)
        raise AssertionError("unconfirmed generic BOOTSEL target was accepted")
    except OSError as exc:
        assert "not confirmed" in str(exc)

    with tempfile.TemporaryDirectory() as mount_text:
        mount = Path(mount_text)
        recovery_target = {"mount": mount, "topology": "4-2", "board_id": "RPI-RP2"}
        old_download, old_mounts, old_runtime, old_serial = (
            manager.download_artifact, manager.uf2_mount_details,
            manager.classic_runtime_ports, manager.LinuxSerial,
        )
        manager.download_artifact = lambda _artifact: (valid, Path("cache"))
        manager.uf2_mount_details = lambda: [recovery_target]
        manager.classic_runtime_ports = lambda: (_ for _ in ()).throw(
            AssertionError("recovery must not select or reboot a healthy runtime")
        )
        manager.LinuxSerial = lambda *_args, **_kwargs: (_ for _ in ()).throw(
            AssertionError("recovery must not send BOOTLOADER to a runtime")
        )
        try:
            result = manager.flash_classic(Ui([True]), {"filename": "recovery.uf2"}, recovery=True)
            assert result["topology"] == "4-2"
            assert (mount / "recovery.uf2").read_bytes() == valid
        finally:
            manager.download_artifact, manager.uf2_mount_details = old_download, old_mounts
            manager.classic_runtime_ports, manager.LinuxSerial = old_runtime, old_serial

    old_usb, old_mounts, old_probe = manager.usb_devices, manager.uf2_mount_details, manager.probe_classic_info
    probes: list[str] = []
    manager.usb_devices = lambda: [{
        "vid": 0x9999, "pid": 1, "bcd_device": 1, "manufacturer": "Other",
        "product_name": "Serial Gadget", "serial": "x", "topology": "2-1", "ports": ["/dev/ttyACM9"],
    }]
    manager.uf2_mount_details = lambda: []
    manager.probe_classic_info = lambda port: probes.append(port) or None
    try:
        assert manager.discover_supported_devices() == []
        assert probes == [], "unrecognized serial device received an INFO probe"
    finally:
        manager.usb_devices, manager.uf2_mount_details, manager.probe_classic_info = old_usb, old_mounts, old_probe

    with tempfile.TemporaryDirectory() as temp_text:
        temp = Path(temp_text)
        embedded = temp / "embedded.json"
        embedded.write_text(json.dumps(catalog("1.0.0")), encoding="utf-8")
        old_cache, old_fetch, old_discover = manager.CATALOG_CACHE_PATH, manager.fetch_json, manager.discover_classic_release_descriptors
        manager.CATALOG_CACHE_PATH = temp / "state" / "catalog.json"
        manager.fetch_json = lambda _url: catalog("1.1.0")
        manager.discover_classic_release_descriptors = lambda: ([], [])
        try:
            online = manager.load_catalog(embedded, refresh=True)
            assert online["_update_check"]["online"]
            assert manager.CATALOG_CACHE_PATH.is_file()
            manager.fetch_json = lambda _url: (_ for _ in ()).throw(OSError("offline"))
            offline = manager.load_catalog(embedded, refresh=False)
            assert manager.latest_release(offline, "classic2usb")["version"] == "1.1.0"
            failed = manager.load_catalog(embedded, refresh=True)
            assert not failed["_update_check"]["online"]
            assert manager.latest_release(failed, "classic2usb")["version"] == "1.1.0"
            assert "online check failed" in manager.classic_update_status(failed, [])
            assert any(
                "online check failed" in label
                for label, action in manager.main_actions(failed, []) if action == "rescan"
            )
            manager.fetch_json = lambda _url: catalog("1.0.0")
            recovered = manager.load_catalog(embedded, refresh=True)
            assert recovered["_update_check"]["online"]
            assert manager.latest_release(recovered, "classic2usb")["version"] == "1.0.0", (
                "successful online refresh retained a withdrawn cached release"
            )
        finally:
            manager.CATALOG_CACHE_PATH, manager.fetch_json, manager.discover_classic_release_descriptors = old_cache, old_fetch, old_discover

    with tempfile.TemporaryDirectory() as temp_text:
        old_state, old_receipts = manager.STATE_ROOT, manager.RECEIPTS_PATH
        manager.STATE_ROOT = Path(temp_text)
        manager.RECEIPTS_PATH = Path(temp_text) / "receipts.json"
        try:
            manager.save_receipt({"id": "one", "status": "pending"})
            manager.update_receipt("one", {"status": "verified"})
            assert manager.load_receipts()[0]["status"] == "verified"
        finally:
            manager.STATE_ROOT, manager.RECEIPTS_PATH = old_state, old_receipts

    with tempfile.TemporaryDirectory() as temp_text:
        target = Path(temp_text) / "catalog.json"
        target.write_text("old\n", encoding="utf-8")
        old_replace = manager.os.replace
        manager.os.replace = lambda *_args: (_ for _ in ()).throw(OSError("injected replace failure"))
        try:
            try:
                manager.write_text_atomic(target, "new\n")
                raise AssertionError("injected atomic-write failure was ignored")
            except OSError:
                pass
            assert target.read_text(encoding="utf-8") == "old\n"
            assert not list(target.parent.glob(".catalog.json.*"))
        finally:
            manager.os.replace = old_replace

    old_download, old_serial, old_flash = manager.download_artifact, manager.serial_ports, manager.flash_avr109
    manager.download_artifact = lambda _artifact: (b":00000001FF\n", Path("cache"))
    manager.serial_ports = lambda identity=None: ([{"port": "/dev/ttyACM0", "topology": "3-1"}] if identity == manager.LEGACY_BOOT_VID_PID else [])
    manager.flash_avr109 = lambda *_args: (_ for _ in ()).throw(AssertionError("flash must not run"))
    try:
        try:
            manager.flash_legacy(Ui([False]), {"sha256": "0" * 64})
            raise AssertionError("unconfirmed Caterina target was accepted")
        except OSError as exc:
            assert "not confirmed" in str(exc)
    finally:
        manager.download_artifact, manager.serial_ports, manager.flash_avr109 = old_download, old_serial, old_flash

    old_download, old_serial = manager.download_artifact, manager.serial_ports
    old_runtime, old_wait, old_flash = manager.legacy_runtime_ports, manager.wait_for_new_bootloader, manager.flash_avr109
    manager.download_artifact = lambda _artifact: (b":00000001FF\n", Path("cache"))
    manager.serial_ports = lambda _identity=None: []
    manager.legacy_runtime_ports = lambda: []
    manager.wait_for_new_bootloader = lambda *_args, **_kwargs: {
        "port": "/dev/ttyACM1", "topology": "3-2",
    }
    manager.flash_avr109 = lambda *_args: (_ for _ in ()).throw(AssertionError("flash must not run"))
    try:
        try:
            manager.flash_legacy(Ui([False]), {"sha256": "0" * 64})
            raise AssertionError("unidentified new Caterina target was accepted")
        except OSError as exc:
            assert "not confirmed" in str(exc)
    finally:
        manager.download_artifact, manager.serial_ports = old_download, old_serial
        manager.legacy_runtime_ports, manager.wait_for_new_bootloader = old_runtime, old_wait
        manager.flash_avr109 = old_flash

    actions = {action for _label, action in manager.main_actions(catalog(), [{
        "product_id": "rp2040-uf2-unknown", "mount": "/media/RPI-RP2",
    }])}
    assert "classic-recovery" in actions and "classic-firmware" not in actions
    assert manager.setting_name(71) == "OK / Back"
    assert {20, 21}.issubset(manager.CONFIGURABLE_INPUT_MODES)
    assert manager.manager_input_setting_ids(14, {39, 59, 66, 67}) == {59, 66, 67}
    assert dict(manager.SETTING_CHOICES[66])[22] == "16x"
    assert {66, 67}.issubset(manager.JOGCON_SETTING_IDS)

    old_serial_class = manager.LinuxSerial
    commands: list[str] = []
    class SemanticSerial:
        def __init__(self, *_args, **_kwargs):
            pass
        def __enter__(self):
            return self
        def __exit__(self, *_args):
            return False
        def command_lines(self, command: str, _terminator: str, timeout: float = 2.0):
            commands.append(command)
            if command == "SET HELP":
                return ["CAP PSX_TIMING=1 ROTARY_PROFILE=1 NEGCON_INPUTS=1"]
            if command.startswith("SET NEGCON GET"):
                return ["NEGCON MODE=14 I=0 II=1 L=0 LIVE=1", "OK:NEGCON_GET"]
            if command.startswith("SET NEGCON SET"):
                return ["OK:NEGCON_SET VERIFY=1"]
            if command.startswith("SET PSX_TIMING GET"):
                return ["PSX_TIMING MODE=14 RATE=2 GAP=4 CONFIGURED=1 LIVE=0", "OK:PSX_TIMING_GET"]
            if command.startswith("SET ROTARY GET"):
                return [
                    "ROTARY MODE=14 PROFILE=JOGCON GAIN=22 REVERSE=1 CONFIGURED=1 LIVE=0",
                    "OK:ROTARY_GET",
                ]
            return ["OK:PSX_TIMING_SET VERIFY=1"]
    manager.LinuxSerial = SemanticSerial
    try:
        assert manager.read_classic_capabilities("port") == {
            "PSX_TIMING", "ROTARY_PROFILE", "NEGCON_INPUTS",
        }
        assert manager.read_negcon_inputs("port") == {
            "mode": 14, "I": 0, "II": 1, "L": 0, "live": 1,
        }
        # Picking L, then Digital, keeps II Digital and I Analog.
        manager.negcon_inputs_flow(Ui(choices=[2, 1, None]), "port")
        assert commands[-2] == "SET NEGCON SET 0 1 1 14"
        assert manager.read_psx_timing("port") == {
            "mode": 14, "rate": 2, "gap": 4, "configured": 1, "live": 0,
        }
        manager.set_psx_timing("port", 3, 5)
        assert commands[-1] == "SET PSX_TIMING SET 3 5 14"
        assert manager.read_rotary_profile("port", "JOGCON")["gain"] == 22
        manager.set_rotary_profile("port", "JOGCON", 20, 1)
        assert commands[-1] == "SET ROTARY SET JOGCON 20 1 14"
    finally:
        manager.LinuxSerial = old_serial_class

    old_read_rotary, old_set_rotary = manager.read_rotary_profile, manager.set_rotary_profile
    manager.read_rotary_profile = lambda *_args: {
        "mode": 14, "profile": "GENERIC", "gain": 0, "reverse": 0,
        "configured": 0, "live": 0,
    }
    manager.set_rotary_profile = lambda *_args: (_ for _ in ()).throw(
        AssertionError("Reverse must not write an unconfigured gain-0 profile")
    )
    try:
        rotary_ui = Ui(choices=[1, None])
        manager.rotary_profile_flow(rotary_ui, "port", "GENERIC")
        assert any("Choose Rotary Gain first" in message for message in rotary_ui.messages)
    finally:
        manager.read_rotary_profile, manager.set_rotary_profile = old_read_rotary, old_set_rotary

    mixed = [
        {"product_id": "classic2usb", "firmware": {"version": "1.0.0"}},
        {"product_id": "classic2usb", "firmware": {"version": "1.2.3"}},
    ]
    mixed_status = manager.classic_update_status(catalog("1.2.3"), mixed)
    assert "1 of 2" in mixed_status and "1.0.0 -> v1.2.3" in mixed_status

    print("OK: Adapt Manager updater fault and recovery checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
