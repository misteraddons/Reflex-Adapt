#include "../../../product_config.h"

#include <hardware/watchdog.h>

#include "../../../core/analog_calibration_state.h"
#include "../../../core/classic_dual_merge_config.h"
#include "../../../core/controller_frame_state.h"
#include "../../../core/controller_settings_state.h"
#include "../../../core/device_runtime_state.h"
#include "../../../core/settings_store.h"
#include "../../../core/turbo.h"
#include "../../../input/psx/input_psx_guncon_runtime_state.h"
#include "../../../menu/menu_display_state.h"
#include "../../../menu/menu_mode_state.h"
#include "../../../menu/menu_runtime_state.h"
#include "../../../menu/menu_working_state.h"
#include "../../output_runtime_state.h"
#include "../../../platform/buzzer.h"
#include "../../../platform/display_runtime_state.h"
#include "../../../platform/latency_test.h"
#include "webhid_input_modes.h"
#include "webhid_protocol.h"
#include "webhid_settings_reports.h"

namespace {

uint8_t getCurrentBuzzerMode() {
  return (uint8_t)loadSettingValue(SettingId::BuzzerMode, deviceMode);
}

void applyBuzzerModeRuntime(uint8_t mode) {
  buzzer_enabled = mode > 0;
  buzzer.setEnabled(buzzer_enabled);
  buzzer.setVolume((mode == 1) ? 10 : 100);
  menu_buzzer_mode = mode;
}

void requestOledHomeClearRedraw() {
#ifndef REFLEX_HOST_TEST_EEPROM_AUTH
  needsU8g2Clear = true;
  padDisplayNeedsRedraw = true;
#endif
}

}  // namespace

uint16_t webhid_get_settings_report(uint8_t* buffer) {
  // Return current device settings
  // Format:
  // Byte 0: magic (0xAD)
  // Byte 1: output mode
  // Byte 2: stable WebHID input mode ID
  // Byte 3: saved input mode
  // Byte 4: deadzone percent
  // Byte 5: rumble level
  // Byte 6: trigger mode
  // Byte 7: buzzer enabled
  // Byte 8: LED mode
  // Byte 9: LED brightness
  // Byte 10: stick invert
  // Byte 11: dpad as buttons
  // Byte 12: NSO special
  // Byte 13: SOCD mode
  // Byte 14: screensaver mode
  // Byte 15: OLED contrast
  // Byte 16: max players (max_devices)
  const uint8_t buzzerMode = getCurrentBuzzerMode();

  buffer[0] = WEBHID_MAGIC_BYTE;
  buffer[1] = outputMode;
  buffer[2] = webhid_input_mode_from_device(deviceMode);
  buffer[3] = webhid_input_mode_from_device(savedDeviceMode);
  buffer[4] = deadzone_percent;
  buffer[5] = rumble_level;
  buffer[6] = trigger_mode;
  buffer[7] = buzzerMode;
  buffer[8] = led_mode;
  buffer[9] = led_brightness;
  buffer[10] = stick_invert;
  buffer[11] = dpad_mode;
  buffer[12] = nso_special ? 1 : 0;
#ifdef PRODUCT_CLASSIC2USB
  buffer[13] = 0;
#else
  buffer[13] = socdMode;
#endif
  buffer[14] = screensaver_mode;
  buffer[15] = display_contrast;
  buffer[16] = max_devices;

  buffer[17] = analogCalibration.enabled ? 1 : 0;
  buffer[18] = (uint8_t)analogCalibration.lx.raw_min;
  buffer[19] = (uint8_t)analogCalibration.lx.raw_max;
  buffer[20] = (uint8_t)analogCalibration.ly.raw_min;
  buffer[21] = (uint8_t)analogCalibration.ly.raw_max;
  buffer[22] = (uint8_t)analogCalibration.rx.raw_min;
  buffer[23] = (uint8_t)analogCalibration.rx.raw_max;
  buffer[24] = (uint8_t)analogCalibration.ry.raw_min;
  buffer[25] = (uint8_t)analogCalibration.ry.raw_max;

  buffer[26] = button_map_mode;
  buffer[27] = n64_z_mode;
  buffer[28] = wii_analog_range;
  buffer[29] = n64_analog_range;
  buffer[30] = 3;
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  buffer[31] = latencyTest.isEnabled() ? 1 : 0;
  buffer[32] = latencyTest.isControllerInLoop() ? 1 : 0;
  buffer[33] = latencyTest.getHostType();
  buffer[34] = latencyTest.isBenchRunning() ? 1 : 0;
  buffer[35] = latencyTest.isCurrentModeReady() ? 1 : 0;
  buffer[36] = latencyTest.getPulsePin();
  buffer[37] = latencyTest.getTriggerPin();
  buffer[38] = latencyTest.getReturnPin();
  buffer[39] = menu_latency_test;
  buffer[40] = menu_latency_controller_in_loop;
  buffer[41] = menu_latency_host_type;
#else
  memset(&buffer[31], 0, 11);
#endif
  buffer[42] = menu_win_output;
  buffer[43] = menu_home_screen_debug;
  buffer[WEBHID_SETTINGS_HOTKEY_FLAGS_INDEX] =
      (menu_menu_hotkey ? 0x01 : 0) |
      (menu_system_menu_hotkey ? 0x02 : 0) |
      (menu_home_hotkey ? 0x04 : 0) |
      (menu_capture_hotkey ? 0x08 : 0) |
      (menu_kiosk_mode ? 0x10 : 0);
  buffer[45] = menu_home_jvs_view;
  buffer[46] = menu_hotkey_hold_time;
  buffer[47] = n64_cstick_mode;
  buffer[48] = menu_powerpad_mode;
  buffer[49] = classicDualMergeEnabledForMode(deviceMode) ? 1 : 0;
  const uint32_t mergeP2Mask = storedClassicDualMergeP2Mask(deviceMode);
  buffer[50] = mergeP2Mask & 0xFF;
  buffer[51] = (mergeP2Mask >> 8) & 0xFF;
  buffer[52] = (mergeP2Mask >> 16) & 0xFF;
  buffer[53] = (mergeP2Mask >> 24) & 0xFF;
  buffer[54] = getClassicDualMergeButtonCount(deviceMode);
#if defined(ADAPT_ENABLE_LATENCY_TEST)
  const uint32_t latencyCompletedSamples = latencyTest.getCompletedRunSamples();
  const uint32_t latencyTargetSamples = latencyTest.getRunTargetSamples();
#else
  const uint32_t latencyCompletedSamples = 0;
  const uint32_t latencyTargetSamples = 0;
#endif
  buffer[55] = latencyCompletedSamples & 0xFF;
  buffer[56] = (latencyCompletedSamples >> 8) & 0xFF;
  buffer[57] = (latencyCompletedSamples >> 16) & 0xFF;
  buffer[58] = (latencyCompletedSamples >> 24) & 0xFF;
  buffer[59] = latencyTargetSamples & 0xFF;
  buffer[60] = (latencyTargetSamples >> 8) & 0xFF;
  buffer[WEBHID_SETTINGS_GUNCON_X_INDEX] = (uint8_t)menu_guncon_offset_x;
  buffer[WEBHID_SETTINGS_GUNCON_Y_INDEX] = (uint8_t)menu_guncon_offset_y;
  return WEBHID_REPORT_SIZE;
}

void webhid_write_settings_report(const uint8_t* buffer, uint16_t bufsize) {
  // Write device settings
  // Format: magic(1), setting_id(1), value(1)
  // setting_id: 0=output_mode, 1=deadzone, 2=rumble_level, 3=trigger_mode,
  //             4=buzzer, 5=led_mode, 6=led_brightness, 7=stick_invert,
  //             8=dpad_as_buttons, 9=nso_special, 10=socd_mode,
  //             11=screensaver, 12=display_contrast, 13=input_mode,
  //             14-22=analog calibration, 23=button_map_mode, 24=n64_z_mode,
  //             25=wii_analog_range, 26=n64_analog_range, 27=latency enable,
  //             28=latency controller source, 29=latency host type,
  //             30=start latency run, 31=stop latency run, 32=clear latency samples,
  //             33=Windows output preference, 34=home screen, 35=home button labels,
  //             36=JVS 1P view, 37=hotkey hold time, 38=N64 C-stick, 39=Power Pad,
  //             40=Classic dual-controller merge, 41=Classic merge P2 mask,
  //             42=Quick Menu hotkey, 43=System Menu hotkey,
  //             44=Home hotkey, 45=Capture hotkey, 46=Kiosk,
  //             47=GunCon X alignment, 48=GunCon Y alignment,
  //             0xFE=save calibration, 0xFF=mode change (will reboot)
  if (bufsize < 3 || buffer[0] != WEBHID_MAGIC_BYTE) return;

  uint8_t setting_id = buffer[1];
  uint8_t value = buffer[2];

  switch (setting_id) {
    case 0:
      saveSettingValue(SettingId::ConfiguredOutputMode, value, deviceMode);
      configuredOutputMode = canonicalizeOutputMode((outputMode_t)loadSettingValue(SettingId::ConfiguredOutputMode, deviceMode));
      outputMode = sanitizeRuntimeOutputMode(configuredOutputMode);
      menu_output = configuredOutputMode;
      autoDetectState = AUTO_STATE_IDLE;
      auto_detect_clear_scratch_state();
      break;
    case 1:
      saveSettingValue(SettingId::Deadzone, value, deviceMode);
      deadzone_percent = (uint8_t)loadSettingValue(SettingId::Deadzone, deviceMode);
      menu_deadzone_percent = deadzone_percent;
      break;
    case 2:
      saveSettingValue(SettingId::RumbleLevel, value, deviceMode);
      rumble_level = (uint8_t)loadSettingValue(SettingId::RumbleLevel, deviceMode);
      menu_rumble_level = rumble_level;
      break;
    case 3:
      saveSettingValue(SettingId::TriggerMode, value, deviceMode);
      trigger_mode = (uint8_t)loadSettingValue(SettingId::TriggerMode, deviceMode);
      menu_trigger_mode = trigger_mode;
      requestOledHomeClearRedraw();
      break;
    case 4:
      saveSettingValue(SettingId::BuzzerMode, value, deviceMode);
      applyBuzzerModeRuntime((uint8_t)loadSettingValue(SettingId::BuzzerMode, deviceMode));
      break;
    case 5:
      saveSettingValue(SettingId::LedMode, value, deviceMode);
      led_mode = (uint8_t)loadSettingValue(SettingId::LedMode, deviceMode);
      break;
    case 6:
      saveSettingValue(SettingId::LedBrightness, value, deviceMode);
      led_brightness = (uint8_t)loadSettingValue(SettingId::LedBrightness, deviceMode);
      break;
    case 7:
      saveSettingValue(SettingId::StickInvert, value, deviceMode);
      stick_invert = (uint8_t)loadSettingValue(SettingId::StickInvert, deviceMode);
      menu_stick_invert = stick_invert;
      break;
    case 8:
      saveSettingValue(SettingId::DpadMode, value, deviceMode);
      dpad_mode = (uint8_t)loadSettingValue(SettingId::DpadMode, deviceMode);
      menu_dpad_mode = dpad_mode;
      break;
    case 9:
      saveSettingValue(SettingId::NsoSpecial, value, deviceMode);
      nso_special = loadSettingValue(SettingId::NsoSpecial, deviceMode) != 0;
      menu_nso_special = nso_special;
      break;
    case 10:
#ifdef PRODUCT_CLASSIC2USB
      value = 0;
#endif
      saveSettingValue(SettingId::Socd, value, deviceMode);
      socdMode = (socdMode_t)loadSettingValue(SettingId::Socd, deviceMode);
      menu_socdMode = socdMode;
      break;
    case 11:
      saveSettingValue(SettingId::ScreensaverMode, value, deviceMode);
      screensaver_mode = (uint8_t)loadSettingValue(SettingId::ScreensaverMode, deviceMode);
      menu_screensaver = screensaver_mode;
      break;
    case 12:
      saveSettingValue(SettingId::DisplayContrast, value, deviceMode);
      display_contrast = (uint8_t)loadSettingValue(SettingId::DisplayContrast, deviceMode);
      menu_display_contrast = display_contrast;
      break;
    case 13: {
      DeviceEnum nextMode = webhid_input_mode_to_device(value, savedDeviceMode);
      if (nextMode != RZORD_NONE) {
        savedDeviceMode = nextMode;
      }
      break;
    }
    case 14: analogCalibration.enabled = (value != 0); break;
    case 15: analogCalibration.lx.raw_min = (int8_t)value; break;
    case 16: analogCalibration.lx.raw_max = (int8_t)value; break;
    case 17: analogCalibration.ly.raw_min = (int8_t)value; break;
    case 18: analogCalibration.ly.raw_max = (int8_t)value; break;
    case 19: analogCalibration.rx.raw_min = (int8_t)value; break;
    case 20: analogCalibration.rx.raw_max = (int8_t)value; break;
    case 21: analogCalibration.ry.raw_min = (int8_t)value; break;
    case 22: analogCalibration.ry.raw_max = (int8_t)value; break;
    case 23:
      saveSettingValue(SettingId::ButtonMapMode, value, deviceMode);
      button_map_mode = (uint8_t)loadSettingValue(SettingId::ButtonMapMode, deviceMode);
      menu_button_map = button_map_mode;
      break;
    case 24:
      saveSettingValue(SettingId::N64ZMode, value, deviceMode);
      n64_z_mode = (uint8_t)loadSettingValue(SettingId::N64ZMode, deviceMode);
      menu_n64_z_mode = n64_z_mode;
      break;
    case 25:
      saveSettingValue(SettingId::WiiAnalogRange, value, deviceMode);
      wii_analog_range = (uint8_t)loadSettingValue(SettingId::WiiAnalogRange, deviceMode);
      menu_wii_analog_range = wii_analog_range;
      break;
    case 26:
      saveSettingValue(SettingId::N64AnalogRange, value, deviceMode);
      n64_analog_range = (uint8_t)loadSettingValue(SettingId::N64AnalogRange, deviceMode);
      menu_n64_analog_range = n64_analog_range;
      break;
    case 27:
      saveSystemSettingByte(SettingId::LatencyTest, value ? 1 : 0);
      menu_latency_test = (uint8_t)loadSettingValue(SettingId::LatencyTest, deviceMode);
      latencyTest.setEnabled(menu_latency_test != 0);
      break;
    case 28:
      saveSystemSettingByte(SettingId::LatencyControllerInLoop, value ? 1 : 0);
      menu_latency_controller_in_loop = (uint8_t)loadSettingValue(SettingId::LatencyControllerInLoop, deviceMode);
      latencyTest.setControllerInLoop(menu_latency_controller_in_loop != 0);
      break;
    case 29:
      saveSystemSettingByte(SettingId::LatencyHostType, value);
      menu_latency_host_type = (uint8_t)loadSettingValue(SettingId::LatencyHostType, deviceMode);
      latencyTest.setHostType(menu_latency_host_type);
      break;
    case 30: {
      uint32_t sampleCount = LATENCY_DEFAULT_SAMPLE_COUNT;
      if (bufsize >= 6) {
        sampleCount = ((uint32_t)buffer[2]) |
                      ((uint32_t)buffer[3] << 8) |
                      ((uint32_t)buffer[4] << 16) |
                      ((uint32_t)buffer[5] << 24);
      }
      latencyTest.startBench(sampleCount);
      break;
    }
    case 31:
      latencyTest.stopBench();
      break;
    case 32:
      latencyTest.clearSamples();
      break;
    case 33:
      if (value > 2) value = 0;
      {
        const bool windowsOutputDescriptorChanged =
          value != menu_win_output &&
          configuredOutputMode == OUTPUT_AUTO &&
          autoDetectState == AUTO_STATE_WINDOWS;
      if (value != menu_win_output) {
        auto_detect_clear_scratch_state();
      }
      saveSystemSettingByte(SettingId::WinOutput, value);
      menu_win_output = value;
      if (windowsOutputDescriptorChanged) {
        watchdog_enable(100, false);
        while (1) {}
      }
      }
      break;
    case 34:
      saveSystemSettingByte(SettingId::HomeScreenDebug, 0);
      menu_home_screen_debug = 0;
      break;
    case 35:
      saveSystemSettingByte(SettingId::HomeButtonLabels, 0);
      menu_home_button_labels = 0;
      break;
    case 36:
      if (value > 1) value = 0;
      saveSystemSettingByte(SettingId::HomeJvsView, value);
      menu_home_jvs_view = value;
      break;
    case 37:
      if (value > 6) value = 1;
      saveSystemSettingByte(SettingId::HotkeyHoldTime, value);
      menu_hotkey_hold_time = value;
      break;
    case 38:
      if (value > 2) value = 0;
      saveSettingValue(SettingId::N64CStickMode, value, deviceMode);
      n64_cstick_mode = (uint8_t)loadSettingValue(SettingId::N64CStickMode, deviceMode);
      menu_n64_cstick_mode = n64_cstick_mode;
      break;
    case 39:
      saveSettingValue(SettingId::PowerpadMode, value ? 1 : 0, deviceMode);
      menu_powerpad_mode = (uint8_t)loadSettingValue(SettingId::PowerpadMode, deviceMode);
      break;
    case 40:
    {
      const uint8_t newValue = (value != 0) ? 1 : 0;
      const bool descriptorChanged = newValue != menu_classic_dual_merge;
      saveClassicDualMergeEnabledForMode(deviceMode, newValue != 0);
      if (descriptorChanged) {
        menu_usb_descriptor_reboot_required = 1;
        watchdog_enable(100, false);
        while (1) {}
      }
      menu_classic_dual_merge = newValue;
      classic_dual_merge_enabled = menu_classic_dual_merge != 0;
      break;
    }
    case 41:
    {
      if (bufsize < 6) break;
      const uint32_t mask = ((uint32_t)buffer[2]) |
                            ((uint32_t)buffer[3] << 8) |
                            ((uint32_t)buffer[4] << 16) |
                            ((uint32_t)buffer[5] << 24);
      saveClassicDualMergeP2MaskForMode(deviceMode, mask);
      classic_dual_merge_p2_mask = sanitizeClassicDualMergeP2Mask(deviceMode, mask);
      break;
    }
    case 42:
      saveSystemSettingByte(SettingId::MenuHotkey, value ? 1 : 0);
      menu_menu_hotkey = (uint8_t)loadSettingValue(SettingId::MenuHotkey, deviceMode);
      break;
    case 43:
      saveSystemSettingByte(SettingId::SystemMenuHotkey, value ? 1 : 0);
      menu_system_menu_hotkey = (uint8_t)loadSettingValue(SettingId::SystemMenuHotkey, deviceMode);
      break;
    case 44:
      saveSystemSettingByte(SettingId::HomeHotkey, value ? 1 : 0);
      menu_home_hotkey = (uint8_t)loadSettingValue(SettingId::HomeHotkey, deviceMode);
      break;
    case 45:
      saveSystemSettingByte(SettingId::CaptureHotkey, value ? 1 : 0);
      menu_capture_hotkey = (uint8_t)loadSettingValue(SettingId::CaptureHotkey, deviceMode);
      break;
    case 46:
      saveSystemSettingByte(SettingId::KioskMode, value ? 1 : 0);
      menu_kiosk_mode = (uint8_t)loadSettingValue(SettingId::KioskMode, deviceMode);
      break;
    case 47:
    case 48:
    {
      const int8_t alignment = normalizeGunconAlignmentOffset((int8_t)value);
      if (setting_id == 47) {
        menu_guncon_offset_x = alignment;
        offsetX = alignment;
      } else {
        menu_guncon_offset_y = alignment;
        offsetY = alignment;
      }
      saveGunconOffsets(menu_guncon_offset_x, menu_guncon_offset_y);
      break;
    }
    case 0xFE: {
      saveAnalogCalibration();
      break;
    }
    case 0xFF: {
      if (value >= OUTPUT_LAST) break;
      outputMode_t newOutputMode = canonicalizeOutputMode((outputMode_t)value);
      DeviceEnum newInputMode = savedDeviceMode;
      if (bufsize >= 4) {
        DeviceEnum convertedMode = webhid_input_mode_to_device(buffer[3], savedDeviceMode);
        if (convertedMode != RZORD_NONE) {
          newInputMode = convertedMode;
        }
      }

      GlobalSettingsRecord globalSettings{};
      readGlobalSettings(globalSettings);
      globalSettings.persisted_input_mode = (uint8_t)newInputMode;
      globalSettings.configured_output_mode = (uint8_t)newOutputMode;
      writeGlobalSettings(globalSettings);

      savedDeviceMode = newInputMode;
      configuredOutputMode = newOutputMode;
      outputMode = sanitizeRuntimeOutputMode(configuredOutputMode);
      autoDetectState = AUTO_STATE_IDLE;
      auto_detect_clear_scratch_state();

      watchdog_enable(100, false);
      while (1) {}
      break;
    }
  }
}
