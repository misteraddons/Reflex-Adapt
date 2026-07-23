#include "../../product_config.h"

#include "boot_storage_runtime.h"

#include <EEPROM.h>

#include "../../menu/menu_runtime_state.h"
#include "../../menu/menu_ui_state.h"
#include "../../menu/menu_working_state.h"
#include "../../output/auth/auth_storage.h"
#include "../../output/runtime/input_runtime_output_bridge.h"
#include "../../output/output_runtime_state.h"
#include "../../platform/buzzer.h"
#include "../../platform/boot/boot_ui_runtime.h"
#include "../../platform/latency_trace_gpio.h"
#include "../../platform/latency_test.h"
#include "../../platform/rgb_led.h"
#include "../analog_calibration_state.h"
#include "../button_chord_remap.h"
#include "../classic_dual_merge_config.h"
#include "../controller_settings_state.h"
#include "../firmware_support.h"
#include "../hotkey_combo.h"
#include "../settings_layout.h"
#include "../settings_registry.h"
#include "../settings_store.h"

namespace {

void applyGlobalBootConfiguration(const GlobalSettingsRecord& settings) {
  savedDeviceMode = (DeviceEnum)settings.persisted_input_mode;
  if (savedDeviceMode > RZORD_NONE && savedDeviceMode < RZORD_LAST) {
    deviceMode = savedDeviceMode;
  } else {
    deviceMode = DEFAULT_INPUT_MODE;
    savedDeviceMode = deviceMode;
  }

  configuredOutputMode = canonicalizeOutputMode((outputMode_t)settings.configured_output_mode);
#if defined(PRODUCT_CLASSIC2USB) && defined(FORCE_CLASSIC_XINPUT2P_OUTPUT) && \
    defined(ENABLE_EXPERIMENTAL_XINPUT2P_OUTPUT)
  configuredOutputMode = OUTPUT_XINPUT2P;
#endif
  outputMode = sanitizeRuntimeOutputMode(configuredOutputMode);
}

void initializeCoreRuntimeSettingsFromStorage(const GlobalSettingsRecord& globalSettings,
                                              const PerModeSettingsRecord& perModeSettings) {
  display_contrast = globalSettings.display_contrast;
  snes_rumbletech_enabled = 1;
  stick_invert = perModeSettings.stick_invert;
  rumble_level = perModeSettings.rumble_level;
  trigger_mode = perModeSettings.trigger_mode;
  spinner_speed = perModeSettings.spinner_speed;
  button_map_mode = perModeSettings.button_map_mode;
  n64_z_mode = perModeSettings.n64_z_mode;
  n64_cstick_mode = perModeSettings.n64_cstick_mode;
  wii_analog_range = perModeSettings.wii_analog_range;
  n64_analog_range = perModeSettings.n64_analog_range;
  menu_latency_test = globalSettings.latency_test;
  menu_latency_controller_in_loop = globalSettings.latency_controller_in_loop;
  menu_latency_host_type = globalSettings.latency_host_type;

  HotkeyBindingSet hotkeys{};
  readHotkeyBindings(hotkeys);
  menu_hotkey_combo = hotkeys.menu;
  menu_system_hotkey_combo = hotkeys.system_menu;
  menu_home_hotkey_combo = hotkeys.home;
  menu_capture_hotkey_combo = hotkeys.capture;
  menu_menu_hotkey = (uint8_t)loadSettingValue(SettingId::MenuHotkey, RZORD_NONE);
  menu_system_menu_hotkey = (uint8_t)loadSettingValue(SettingId::SystemMenuHotkey, RZORD_NONE);
  menu_home_hotkey = (uint8_t)loadSettingValue(SettingId::HomeHotkey, RZORD_NONE);
  menu_capture_hotkey = (uint8_t)loadSettingValue(SettingId::CaptureHotkey, RZORD_NONE);
  menu_kiosk_mode = (uint8_t)loadSettingValue(SettingId::KioskMode, RZORD_NONE);
  loadButtonChordRemaps();
}

void initializeBuzzerAndLatencySettingsFromStorage(const GlobalSettingsRecord& globalSettings) {
  const uint8_t buzzerMode = globalSettings.buzzer_mode;
  buzzer_enabled = buzzerMode > 0;
  const uint8_t buzzerVolume = (buzzerMode == 1) ? 10 : 100;
  buzzer.begin();
  buzzer.setEnabled(buzzer_enabled);
  buzzer.setVolume(buzzerVolume);
  buzzer.setEventMask(globalSettings.sound_events);

  latencyTest.begin();
  latencyTraceGpioBegin();
  latencyTest.setEnabled(globalSettings.latency_test != 0);
  latencyTest.setControllerInLoop(globalSettings.latency_controller_in_loop != 0);
  latencyTest.setHostType(globalSettings.latency_host_type);
}

void initializeLedAndScreensaverSettingsFromStorage(const GlobalSettingsRecord& globalSettings) {
  led_mode = globalSettings.led_mode;
  led_brightness = globalSettings.led_brightness;
  #ifdef USE_WS2812
  rgbLed.begin();
  rgbLed.setMode((led_mode_t)led_mode);
  rgbLed.setBrightness(led_brightness);
  #endif

  screensaver_mode = globalSettings.screensaver_mode;
  screensaver_animation = globalSettings.screensaver_animation;
  menu_screensaver = screensaver_mode;
  menu_screensaver_anim = screensaver_animation;
}

void initializeMenuRuntimeSettingsFromStorage(const GlobalSettingsRecord& globalSettings,
                                              const PerModeSettingsRecord& perModeSettings) {
  menu_powerpad_mode = perModeSettings.powerpad_mode;
  menu_jogcon_mode = perModeSettings.jogcon_mode;
  menu_jogcon_force = perModeSettings.jogcon_force;
  menu_wheel_sensitivity = perModeSettings.wheel_sensitivity;
  menu_jogcon_digital_map = perModeSettings.jogcon_digital_map;
  menu_jogcon_wheel_axis = perModeSettings.jogcon_wheel_axis;
  menu_win_output = globalSettings.win_output;
  menu_psx_periph = globalSettings.psx_periph;
  menu_snes_rumbletech = 1;
  menu_home_screen_debug = 0;
  menu_home_button_labels = 0;
  menu_home_jvs_view = (globalSettings.home_screen_debug & HOME_JVS_VIEW_MASK) >> HOME_JVS_VIEW_SHIFT;
  menu_hotkey_hold_time = (globalSettings.home_screen_debug & HOTKEY_HOLD_TIME_MASK) >> HOTKEY_HOLD_TIME_SHIFT;
  menu_classic_dual_merge = classicDualMergeEnabledForMode(deviceMode) ? 1 : 0;
  classic_dual_merge_enabled = menu_classic_dual_merge;
  menu_wii_analog_range = perModeSettings.wii_analog_range;
  menu_guncon_offset_x = perModeSettings.guncon_offset_x;
  menu_guncon_offset_y = perModeSettings.guncon_offset_y;
}

void initializeSoundAndUsbIdentitySettingsFromStorage() {
  ps4_type = PS4_DEVICE_TYPE_GAMEPAD;
  bcd_device_version.revision = bcd_input_revision;
}

}  // namespace

void beginPersistentStorage() {
  uint16_t storageEnd = max((uint16_t)SETTINGS_STORAGE_REGION_SIZE, hotkeyStorageRequiredEnd());
  storageEnd = max(storageEnd, buttonChordStorageRequiredEnd());
#ifdef PRODUCT_CLASSIC2USB
  storageEnd = max(storageEnd, classicDualMergeStorageRequiredEnd());
#endif
  #ifdef ADAPT_OUTPUT_USB_DEVICE
  storageEnd = max(storageEnd, authStorageRequiredEnd());
  #endif
  #ifdef ENABLE_BLUETOOTH_PAIRING
  storageEnd = max(storageEnd, bluetoothBondStorageRequiredEnd());
  #endif
  EEPROM.begin(storageEnd);

  GlobalSettingsRecord globalSettings{};
  if (!readGlobalSettings(globalSettings)) {
    factoryResetSettings();
  }

  authStorageInitialize();
  updateAuthKeyStatus();
}

bool loadPersistedBootConfiguration() {
  GlobalSettingsRecord globalSettings{};
  const bool repaired = !readGlobalSettings(globalSettings);
  if (repaired) {
    writeGlobalSettings(globalSettings);
  }

  applyGlobalBootConfiguration(globalSettings);
  return repaired;
}

void initializeRuntimeSettingsFromStorage() {
  GlobalSettingsRecord globalSettings{};
  readGlobalSettings(globalSettings);

  PerModeSettingsRecord perModeSettings{};
  readPerModeSettings(deviceMode, perModeSettings);

  initializeCoreRuntimeSettingsFromStorage(globalSettings, perModeSettings);
  initializeBuzzerAndLatencySettingsFromStorage(globalSettings);
  initializeLedAndScreensaverSettingsFromStorage(globalSettings);
  initializeMenuRuntimeSettingsFromStorage(globalSettings, perModeSettings);
  initializeSoundAndUsbIdentitySettingsFromStorage();
}
