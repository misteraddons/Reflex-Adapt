Import("env")

import os
import shutil
import subprocess


PROJECT_DIR = env.subst("$PROJECT_DIR")
COMPILE_DB_PATH = os.path.join(PROJECT_DIR, "build", "vscode", "compile_commands.json")
UF2_EXPORT_DIR = os.path.join(PROJECT_DIR, "dist")
UF2_EXPORT_PATH = os.path.join(UF2_EXPORT_DIR, f"{env['PIOENV']}.uf2")
PICOTOOL_DIR = env.PioPlatform().get_package_dir("tool-picotool-rp2040-earlephilhower")
PICOTOOL_EXE = os.path.join(PICOTOOL_DIR, "picotool.exe" if os.name == "nt" else "picotool")


os.makedirs(os.path.dirname(COMPILE_DB_PATH), exist_ok=True)
env.Replace(COMPILATIONDB_PATH=COMPILE_DB_PATH)


def write_generated_tinyusb_source(env, node, patch_group, patched_source):
    source_path = node.srcnode().get_abspath()
    relative_dir = os.path.relpath(os.path.dirname(source_path), PROJECT_DIR)
    include_paths = [os.path.dirname(source_path)]
    normalized_source = source_path.replace("\\", "/")
    tinyusb_marker = "/Adafruit_TinyUSB_Arduino/src/"
    marker_index = normalized_source.find(tinyusb_marker)
    if marker_index >= 0:
        tinyusb_src = source_path[:marker_index + len("/Adafruit_TinyUSB_Arduino/src")].replace("/", os.sep)
        relative_dir = os.path.join(
            "tinyusb",
            os.path.relpath(os.path.dirname(source_path), tinyusb_src),
        )
        include_paths.extend([
            tinyusb_src,
            os.path.join(tinyusb_src, "arduino"),
            os.path.join(tinyusb_src, "class"),
            os.path.join(tinyusb_src, "common"),
            os.path.join(tinyusb_src, "device"),
            os.path.join(tinyusb_src, "host"),
        ])
    env.Prepend(CPPPATH=include_paths)
    patched_dir = os.path.join(
        PROJECT_DIR,
        "build",
        "patched_sources",
        env["PIOENV"],
        patch_group,
        relative_dir,
    )
    os.makedirs(patched_dir, exist_ok=True)
    patched_path = os.path.join(patched_dir, os.path.basename(source_path))
    with open(patched_path, "w", encoding="utf-8", newline="") as handle:
        handle.write(patched_source)
    print(f"Using generated TinyUSB patch source: {patched_path}")
    return env.File(patched_path)


def patch_tinyusb_device_descriptor_hooks(env, node):
    # Keep the tracked intent in tools/tinyusb_patches/xinput-autodetect-descriptor-hooks.patch.
    source_path = node.srcnode().get_abspath()
    with open(source_path, "r", encoding="utf-8") as handle:
        source = handle.read()

    replacements = [
        (
            """// TinyUSB stack callbacks
//--------------------------------------------------------------------+

extern "C" {
""",
            """// TinyUSB stack callbacks
//--------------------------------------------------------------------+

// Hook function pointers for descriptor overrides (used by XInput/AUTO detect).
uint8_t const* (*tud_descriptor_device_hook)(void) = nullptr;
uint8_t const* (*tud_descriptor_configuration_hook)(uint8_t index) = nullptr;
uint16_t const* (*tud_descriptor_string_hook)(uint8_t index, uint16_t langid) = nullptr;

extern "C" {
""",
        ),
        (
            """uint8_t const *tud_descriptor_device_cb(void) {
  return (uint8_t const *)&TinyUSBDevice._desc_device;
}
""",
            """uint8_t const *tud_descriptor_device_cb(void) {
  if (tud_descriptor_device_hook) {
    return tud_descriptor_device_hook();
  }
  return (uint8_t const *)&TinyUSBDevice._desc_device;
}
""",
        ),
        (
            """uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    return TinyUSBDevice._desc_cfg;
}
""",
            """uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    if (tud_descriptor_configuration_hook) {
      return tud_descriptor_configuration_hook(index);
    }
    return TinyUSBDevice._desc_cfg;
}
""",
        ),
        (
            """uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
   if (index == 0xEE)
     return get_ms_os_10_string_ptr();
  return TinyUSBDevice.descriptor_string_cb(index, langid);
}
""",
            """uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  if (index == STRID_MANUFACTURER) {
    string_manufacturer_read_cb();
  }

  if (tud_descriptor_string_hook) {
    return tud_descriptor_string_hook(index, langid);
  }

  if (index == 0xEE) {
    return get_ms_os_10_string_ptr();
  }

  return TinyUSBDevice.descriptor_string_cb(index, langid);
}
""",
        ),
    ]

    patched = source
    for patch_index, (needle, replacement) in enumerate(replacements):
        if needle not in patched:
            raise Exception(f"TinyUSB device descriptor hook patch anchor {patch_index} not found in {source_path}")
        patched = patched.replace(needle, replacement, 1)
    return write_generated_tinyusb_source(env, node, "xinput-autodetect-descriptor-hooks", patched)


def patch_tinyusb_usbd_ms_os_request_hook(env, node):
    # Keep the tracked intent in tools/tinyusb_patches/xinput-autodetect-descriptor-hooks.patch.
    source_path = node.srcnode().get_abspath()
    with open(source_path, "r", encoding="utf-8") as handle:
        source = handle.read()

    needle = """      TU_LOG_USBD(" String[%u]\\r\\n", desc_index);

      // String Descriptor always uses the desc set from user
      uint8_t const* desc_str = (uint8_t const*) tud_descriptor_string_cb(desc_index, tu_le16toh(p_request->wIndex));
"""
    replacement = """      TU_LOG_USBD(" String[%u]\\r\\n", desc_index);

      if (desc_index == 0xEE) {
        extern void tud_string_0xee_requested_cb(void) TU_ATTR_WEAK;
        if (tud_string_0xee_requested_cb) {
          tud_string_0xee_requested_cb();
        }
      }

      // String Descriptor always uses the desc set from user
      uint8_t const* desc_str = (uint8_t const*) tud_descriptor_string_cb(desc_index, tu_le16toh(p_request->wIndex));
"""
    if needle not in source:
        raise Exception(f"TinyUSB USBD MS OS request hook patch anchor not found in {source_path}")
    patched = source.replace(needle, replacement, 1)

    xfer_needle = """        if (0 == epnum) {
          usbd_control_xfer_cb(event.rhport, ep_addr, (xfer_result_t) event.xfer_complete.result,
                               event.xfer_complete.len);
        } else {
          usbd_class_driver_t const* driver = get_driver(_usbd_dev.ep2drv[epnum][ep_dir]);
          TU_ASSERT(driver,);

          TU_LOG_USBD("  %s xfer callback\\r\\n", driver->name);
          driver->xfer_cb(event.rhport, ep_addr, (xfer_result_t) event.xfer_complete.result, event.xfer_complete.len);
        }
"""
    xfer_replacement = """        if (0 == epnum) {
          usbd_control_xfer_cb(event.rhport, ep_addr, (xfer_result_t) event.xfer_complete.result,
                               event.xfer_complete.len);
        } else {
          uint8_t const driver_id = _usbd_dev.ep2drv[epnum][ep_dir];
          extern bool tud_xfer_complete_override_cb(uint8_t rhport,
                                                    uint8_t ep_addr,
                                                    uint8_t driver_id,
                                                    xfer_result_t result,
                                                    uint32_t xferred_bytes) TU_ATTR_WEAK;
          if (tud_xfer_complete_override_cb &&
              tud_xfer_complete_override_cb(event.rhport,
                                            ep_addr,
                                            driver_id,
                                            (xfer_result_t) event.xfer_complete.result,
                                            event.xfer_complete.len)) {
            break;
          }

          usbd_class_driver_t const* driver = get_driver(driver_id);
          TU_ASSERT(driver,);

          TU_LOG_USBD("  %s xfer callback\\r\\n", driver->name);
          driver->xfer_cb(event.rhport, ep_addr, (xfer_result_t) event.xfer_complete.result, event.xfer_complete.len);
        }
"""
    if xfer_needle not in patched:
        raise Exception(f"TinyUSB xfer complete override hook anchor not found in {source_path}")
    patched = patched.replace(xfer_needle, xfer_replacement, 1)
    return write_generated_tinyusb_source(env, node, "xinput-autodetect-descriptor-hooks", patched)


env.AddBuildMiddleware(
    patch_tinyusb_device_descriptor_hooks,
    "*Adafruit_TinyUSB_Arduino*arduino*Adafruit_USBD_Device.cpp",
)
env.AddBuildMiddleware(
    patch_tinyusb_usbd_ms_os_request_hook,
    "*Adafruit_TinyUSB_Arduino*device*usbd.c",
)


def export_uf2(target=None, source=None, env=None, **kwargs):
    os.makedirs(UF2_EXPORT_DIR, exist_ok=True)
    result = subprocess.run(
        [PICOTOOL_EXE, "uf2", "convert", str(source[0]), UF2_EXPORT_PATH],
        cwd=PROJECT_DIR,
    )
    if result.returncode != 0:
        raise Exception(f"picotool uf2 convert failed with exit code {result.returncode}")
    print(f"Exported UF2: {UF2_EXPORT_PATH}")


env.AddCustomTarget(
    "uf2",
    "$BUILD_DIR/firmware.elf",
    env.VerboseAction(export_uf2, "Exporting UF2 to $PROJECT_DIR/dist/"),
    title="Build UF2",
    description="Build firmware and export dist/<env>.uf2"
)
