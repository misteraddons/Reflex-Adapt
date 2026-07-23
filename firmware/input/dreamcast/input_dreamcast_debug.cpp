#include "Input_Dreamcast.h"

#ifdef ENABLE_INPUT_DREAMCAST

namespace {
constexpr uint32_t kDreamcastCapB = DC_BTN_B;
constexpr uint32_t kDreamcastCapA = DC_BTN_A;
constexpr uint32_t kDreamcastCapStart = DC_BTN_START;
constexpr uint32_t kDreamcastCapDpadUp = DC_BTN_UP;
constexpr uint32_t kDreamcastCapDpadDown = DC_BTN_DOWN;
constexpr uint32_t kDreamcastCapDpadLeft = DC_BTN_LEFT;
constexpr uint32_t kDreamcastCapDpadRight = DC_BTN_RIGHT;
constexpr uint32_t kDreamcastCapY = DC_BTN_Y;
constexpr uint32_t kDreamcastCapX = DC_BTN_X;
constexpr uint32_t kDreamcastCapDpad2Up = 1UL << 12;
constexpr uint32_t kDreamcastCapDpad2Down = 1UL << 13;
constexpr uint32_t kDreamcastCapDpad2Left = 1UL << 14;
constexpr uint32_t kDreamcastCapDpad2Right = 1UL << 15;
constexpr uint32_t kDreamcastCapRTrigger = 1UL << 16;
constexpr uint32_t kDreamcastCapLTrigger = 1UL << 17;
constexpr uint32_t kDreamcastCapAnalogX = 1UL << 18;
constexpr uint32_t kDreamcastCapAnalogY = 1UL << 19;

constexpr uint32_t kDreamcastCapsStandardButtons =
  kDreamcastCapA | kDreamcastCapB | kDreamcastCapX | kDreamcastCapY | kDreamcastCapStart;
constexpr uint32_t kDreamcastCapsDpad =
  kDreamcastCapDpadUp | kDreamcastCapDpadDown | kDreamcastCapDpadLeft | kDreamcastCapDpadRight;
constexpr uint32_t kDreamcastCapsDualDpad =
  kDreamcastCapsDpad | kDreamcastCapDpad2Up | kDreamcastCapDpad2Down |
  kDreamcastCapDpad2Left | kDreamcastCapDpad2Right;
constexpr uint32_t kDreamcastCapsTriggers = kDreamcastCapLTrigger | kDreamcastCapRTrigger;
constexpr uint32_t kDreamcastCapsAnalog = kDreamcastCapAnalogX | kDreamcastCapAnalogY;
constexpr uint32_t kDreamcastTypeAsciiMissionStick =
  kDreamcastCapsStandardButtons | kDreamcastCapsDualDpad |
  kDreamcastCapsTriggers | kDreamcastCapsAnalog;

bool dreamcastDeviceHasCapability(const MapleDeviceInfo& info, uint32_t capability) {
  return (info.func_data[0] & capability) != 0;
}

bool dreamcastDeviceHasCapabilities(const MapleDeviceInfo& info, uint32_t capabilities) {
  return (info.func_data[0] & capabilities) == capabilities;
}

char dreamcastUpperAscii(char c) {
  if (c >= 'a' && c <= 'z') return (char)(c - 'a' + 'A');
  return c;
}

bool dreamcastDeviceHasProductToken(const MapleDeviceInfo& info, const char* token) {
  if (token == nullptr || token[0] == '\0') return false;
  for (const char* p = info.product_name; *p != '\0'; ++p) {
    const char* haystack = p;
    const char* needle = token;
    while (*haystack != '\0' && *needle != '\0' &&
           dreamcastUpperAscii(*haystack) == dreamcastUpperAscii(*needle)) {
      ++haystack;
      ++needle;
    }
    if (*needle == '\0') return true;
  }
  return false;
}

bool dreamcastDeviceIsWheelController(const MapleDeviceInfo& info) {
  return dreamcastDeviceHasCapability(info, kDreamcastCapAnalogX) &&
         !dreamcastDeviceHasCapability(info, kDreamcastCapAnalogY) &&
         dreamcastDeviceHasCapability(info, DC_BTN_LEFT) &&
         dreamcastDeviceHasCapability(info, DC_BTN_RIGHT);
}

bool dreamcastDeviceIsMissionStickController(const MapleDeviceInfo& info) {
  if (dreamcastDeviceHasProductToken(info, "ASCII ANALOG STICK") ||
      dreamcastDeviceHasProductToken(info, "MISSION STICK")) {
    return true;
  }
  return dreamcastDeviceHasCapabilities(info, kDreamcastTypeAsciiMissionStick);
}
}  // namespace

void RZInputDreamcast::storeWebHidDebugFrame() {
#if DREAMCAST_WEBHID_DEBUG
  uint8_t raw[32] = {0};
  raw[0] = 0xDC;
  raw[1] = 0x02;

  for (uint8_t i = 0; i < input_ports && i < 4; ++i) {
    if (initSuccess[i]) raw[2] |= (1u << i);
    if (maple[i]->isConnected()) raw[2] |= (1u << (i + 4));
  }

  raw[3] = input_ports;
  raw[4] = maple_init_fail_reason;
  raw[5] = maple_pio_out_block;
  raw[6] = maple_pio_in_block;
  raw[7] = maple_pio0_free;
  raw[8] = maple_pio1_free;
  raw[9] = maple_write_ok & 0xFF;
  raw[10] = (maple_write_ok >> 8) & 0xFF;
  raw[11] = maple_write_fail & 0xFF;
  raw[12] = (maple_write_fail >> 8) & 0xFF;
  raw[13] = maple_read_ok & 0xFF;
  raw[14] = (maple_read_ok >> 8) & 0xFF;
  raw[15] = maple_read_timeout & 0xFF;
  raw[16] = (maple_read_timeout >> 8) & 0xFF;
  raw[17] = maple_bus_busy & 0xFF;
  raw[18] = (maple_bus_busy >> 8) & 0xFF;
  raw[19] = maple_gpio_activity & 0xFF;
  raw[20] = (maple_gpio_activity >> 8) & 0xFF;
  raw[21] = maple_dma_words & 0xFF;
  raw[22] = (maple_dma_words >> 8) & 0xFF;

  if (input_ports > 0) {
    raw[23] = (uint8_t)maple[0]->getStatus();
    raw[24] = maple[0]->getConsecutiveFailCount();
    raw[25] = maple[0]->getLastResponseCmd();
    raw[26] = (maple[0]->getPinA() ? 0x01 : 0x00) | (maple[0]->getPinB() ? 0x02 : 0x00);
  }
  if (input_ports > 1) {
    raw[27] = (uint8_t)maple[1]->getStatus();
    raw[28] = maple[1]->getConsecutiveFailCount();
    raw[29] = maple[1]->getLastResponseCmd();
    raw[30] = (maple[1]->getPinA() ? 0x01 : 0x00) | (maple[1]->getPinB() ? 0x02 : 0x00);
  }
  raw[31] = maple_update_calls & 0xFF;

  webhid_store_raw_data(raw, sizeof(raw));
#endif
}

void RZInputDreamcast::printSerialDebug(uint32_t now) {
#if DREAMCAST_SERIAL_DEBUG
  if (now - lastSerialDebugTime < DREAMCAST_SERIAL_DEBUG_INTERVAL_MS) return;
  lastSerialDebugTime = now;

  #if defined(ADAPT_OUTPUT_USB_DEVICE)
  if (!dreamcastDebugCdcEnabled || !dreamcastDebugCdc) return;
  auto& dcdbg = dreamcastDebugCdc;
  #else
  if (!Serial) return;
  auto& dcdbg = Serial;
  #endif

  dcdbg.print(F("[DC] "));
  for (uint8_t i = 0; i < input_ports; ++i) {
    dcdbg.print(F("P"));
    dcdbg.print(i + 1);
    dcdbg.print(F(":I"));
    dcdbg.print(initSuccess[i] ? 1 : 0);
    dcdbg.print(F(" C"));
    dcdbg.print(maple[i]->isConnected() ? 1 : 0);
    dcdbg.print(F(" S"));
    dcdbg.print((uint8_t)maple[i]->getStatus());
    dcdbg.print(F(" F"));
    dcdbg.print(maple[i]->getConsecutiveFailCount());
    dcdbg.print(F(" R0x"));
    if (maple[i]->getLastResponseCmd() < 16) dcdbg.print('0');
    dcdbg.print(maple[i]->getLastResponseCmd(), HEX);
    dcdbg.print(F(" DA0x"));
    if (maple[i]->getDeviceAddress() < 16) dcdbg.print('0');
    dcdbg.print(maple[i]->getDeviceAddress(), HEX);
    dcdbg.print(F(" AF0x"));
    uint32_t acc_func = maple[i]->getAccessoryFunctionMask();
    if (acc_func < 0x10000000UL) dcdbg.print('0');
    dcdbg.print(acc_func, HEX);
    if (i + 1 < input_ports) dcdbg.print(F(" | "));
  }

  dcdbg.print(F(" PR0x"));
  if (maple_last_probe_recipient < 16) dcdbg.print('0');
  dcdbg.print(maple_last_probe_recipient, HEX);
  dcdbg.print(F(" CL"));
  dcdbg.print(maple_last_condition_layout);
  dcdbg.print(F(" TM"));
  dcdbg.print(trigger_mode);
  dcdbg.print(F(" OM"));
  dcdbg.print(output_runtime_mode_value());
  dcdbg.print(F(" || W "));
  dcdbg.print(maple_write_ok);
  dcdbg.print(F("/"));
  dcdbg.print(maple_write_fail);
  dcdbg.print(F(" R "));
  dcdbg.print(maple_read_ok);
  dcdbg.print(F("/"));
  dcdbg.print(maple_read_timeout);
  dcdbg.print(F(" BF "));
  dcdbg.print(maple_bad_frame);
  dcdbg.print(F(" B "));
  dcdbg.print(maple_bus_busy);
  dcdbg.print(F(" G "));
  dcdbg.print(maple_gpio_activity);
  dcdbg.print(F(" A "));
  dcdbg.print(maple_gpio_activity_a);
  dcdbg.print(F(" Bb "));
  dcdbg.print(maple_gpio_activity_b);
  dcdbg.print(F(" Ea "));
  dcdbg.print(maple_posttx_edges_a);
  dcdbg.print(F(" Eb "));
  dcdbg.print(maple_posttx_edges_b);
  dcdbg.print(F(" Ta "));
  dcdbg.print(maple_tx_low_seen_a);
  dcdbg.print(F(" Tb "));
  dcdbg.print(maple_tx_low_seen_b);
  dcdbg.print(F(" Wa "));
  dcdbg.print(maple_wait_low_a);
  dcdbg.print(F(" Wb "));
  dcdbg.print(maple_wait_low_b);
  dcdbg.print(F(" SYS "));
  dcdbg.print(maple_sys_hz / 1000000UL);
  dcdbg.print(F(" D "));
  dcdbg.print(maple_dma_words);
  dcdbg.print(F(" IRQ "));
  dcdbg.print(maple_rx_irq_hits);
  dcdbg.print(F(" F0 0x"));
  if (maple_last_rx_word0_swapped < 0x10000000UL) dcdbg.print('0');
  dcdbg.print(maple_last_rx_word0_swapped, HEX);
  dcdbg.print(F(" RAW 0x"));
  if (maple_last_rx_word0_raw < 0x10000000UL) dcdbg.print('0');
  dcdbg.print(maple_last_rx_word0_raw, HEX);
  dcdbg.print(F(" L "));
  dcdbg.print(maple_last_rx_len);
  dcdbg.print(F(" DI S0x"));
  if (maple_last_devinfo_sender < 16) dcdbg.print('0');
  dcdbg.print(maple_last_devinfo_sender, HEX);
  dcdbg.print(F(" R0x"));
  if (maple_last_devinfo_recipient < 16) dcdbg.print('0');
  dcdbg.print(maple_last_devinfo_recipient, HEX);
  dcdbg.print(F(" DL "));
  dcdbg.print(maple_last_devinfo_len);
  dcdbg.print(F(" FN 0x"));
  if (maple_last_devinfo_func_native < 0x10000000UL) dcdbg.print('0');
  dcdbg.print(maple_last_devinfo_func_native, HEX);
  dcdbg.print(F(" FS 0x"));
  if (maple_last_devinfo_func_swapped < 0x10000000UL) dcdbg.print('0');
  dcdbg.print(maple_last_devinfo_func_swapped, HEX);
  dcdbg.print(F(" TL "));
  const DreamcastControllerCondition& d0 = maple[0]->getController();
  dcdbg.print(d0.ltrigger);
  dcdbg.print(F("/"));
  dcdbg.print(d0.rtrigger);
  dcdbg.println();
#else
  (void)now;
#endif
}

const char* RZInputDreamcast::getDebugStatus(uint8_t port) {
  static char failStr[24];
  if (port >= input_ports) return "Invalid";
  if (!initSuccess[port]) {
    snprintf(failStr, sizeof(failStr), "F:%d 0:%d 1:%d",
      maple_init_fail_reason, maple_pio0_free, maple_pio1_free);
    return failStr;
  }
  if (!maple[port]->isInitialized()) return "PIO FAIL";
  if (maple[port]->isConnected()) return "Connected";
  snprintf(failStr, sizeof(failStr), "W%d G%d R%d",
    maple_write_ok % 100, maple_gpio_activity, maple_read_ok);
  return failStr;
}

const char* RZInputDreamcast::getDeviceLabel(uint32_t func) {
  bool hasController = (func & MAPLE_FUNC_CONTROLLER) != 0;
  bool hasVMU = (func & (MAPLE_FUNC_MEMORY | MAPLE_FUNC_LCD | MAPLE_FUNC_CLOCK)) != 0;
  bool hasRumble = (func & MAPLE_FUNC_PURUPURU) != 0;

  if (hasController) {
    if (hasVMU && hasRumble) return "Pad + Both";
    if (hasVMU) return "Pad + VMU";
    if (hasRumble) return "Pad + Rmb";
    return "Pad";
  }

  if (func & MAPLE_FUNC_MOUSE) return "DC Mouse ns";
  if (func & MAPLE_FUNC_KEYBOARD) return "DC Keybd ns";
  if (func & MAPLE_FUNC_MICROPHONE) return "DC Mic ns";
  if (func & (MAPLE_FUNC_AR_GUN | MAPLE_FUNC_LIGHT_GUN)) return "DC Gun ns";
  if (hasVMU) return "DC VMU ns";
  return "DC Dev ns";
}

const char* RZInputDreamcast::getControllerDeviceLabel(const MapleDeviceInfo& info, uint32_t accessoryMask) {
  const uint32_t func = info.func | accessoryMask;
  const bool hasController = (func & MAPLE_FUNC_CONTROLLER) != 0;
  const bool hasVMU = (func & (MAPLE_FUNC_MEMORY | MAPLE_FUNC_LCD | MAPLE_FUNC_CLOCK)) != 0;
  const bool hasRumble = (func & MAPLE_FUNC_PURUPURU) != 0;

  if (hasController && dreamcastDeviceIsWheelController(info)) {
    if (hasVMU && hasRumble) return "Whl+Both";
    if (hasVMU) return "Whl+VMU";
    if (hasRumble) return "Whl+Rmb";
    return "Wheel";
  }

  if (hasController && dreamcastDeviceIsMissionStickController(info)) {
    if (hasVMU) return "Mission+VMU";
    return "Mission";
  }

  return getDeviceLabel(func);
}

bool RZInputDreamcast::getPinState(uint8_t port, bool pinB) {
  if (port >= input_ports) return false;
  uint8_t pin = pinB ? input_dreamcast_config[port].pinB : input_dreamcast_config[port].pinA;
  return gpio_get(pin);
}

void RZInputDreamcast::printDiagnostics(Print& out) {
  out.print(F("DCSTAT PORTS="));
  out.print((int)input_ports);
  out.print(F(" INIT_FAIL="));
  out.print((int)maple_init_fail_reason);
  out.print(F(" PIO="));
  out.print((int)maple_pio_out_block);
  out.print('/');
  out.print((int)maple_pio_in_block);
  out.print(F(" FREE="));
  out.print((int)maple_pio0_free);
  out.print('/');
  out.println((int)maple_pio1_free);

  for (uint8_t i = 0; i < input_ports; ++i) {
    out.print(F("DCSTAT P="));
    out.print((int)i);
    out.print(F(" INIT="));
    out.print(initSuccess[i] ? 1 : 0);
    out.print(F(" INZD="));
    out.print(maple[i]->isInitialized() ? 1 : 0);
    out.print(F(" CONN="));
    out.print(maple[i]->isConnected() ? 1 : 0);
    out.print(F(" ST="));
    out.print((int)maple[i]->getStatus());
    out.print(F(" FAIL="));
    out.print((int)maple[i]->getConsecutiveFailCount());
    out.print(F(" CMD=0x"));
    if (maple[i]->getLastResponseCmd() < 16) out.print('0');
    out.print((int)maple[i]->getLastResponseCmd(), HEX);
    out.print(F(" ADDR=0x"));
    if (maple[i]->getDeviceAddress() < 16) out.print('0');
    out.print((int)maple[i]->getDeviceAddress(), HEX);
    out.print(F(" FUNC=0x"));
    out.print(maple[i]->getLastSeenFunction(), HEX);
    out.print(F(" ACC=0x"));
    out.print(maple[i]->getAccessoryFunctionMask(), HEX);
    out.print(F(" PINS="));
    out.print(getPinState(i, false) ? 1 : 0);
    out.print('/');
    out.print(getPinState(i, true) ? 1 : 0);
    const DreamcastControllerCondition& dc = maple[i]->getController();
    out.print(F(" BTN=0x"));
    out.print((int)dc.buttons, HEX);
    out.print(F(" TRIG="));
    out.print((int)dc.ltrigger);
    out.print('/');
    out.print((int)dc.rtrigger);
    out.print(F(" AX="));
    out.print((int)dc.joyx);
    out.print('/');
    out.print((int)dc.joyy);
    out.print(F(" AX2="));
    out.print((int)dc.joyx2);
    out.print('/');
    out.print((int)dc.joyy2);
    out.print(F(" FD="));
    const MapleDeviceInfo& info = maple[i]->getDeviceInfo();
    out.print(info.func_data[0], HEX);
    out.print('/');
    out.print(info.func_data[1], HEX);
    out.print('/');
    out.print(info.func_data[2], HEX);
    out.print(F(" PROD=\""));
    out.print(info.product_name);
    out.print(F("\" LIC=\""));
    out.print(info.license);
    out.print(F("\" PWR="));
    out.print((int)info.standby_power);
    out.print('/');
    out.print((int)info.max_power);
    out.print(F(" LABEL="));
    out.println(getDebugStatus(i));
  }

  out.print(F("DCSTAT BUS W="));
  out.print(maple_write_ok);
  out.print('/');
  out.print(maple_write_fail);
  out.print(F(" R="));
  out.print(maple_read_ok);
  out.print('/');
  out.print(maple_read_timeout);
  out.print(F(" BAD="));
  out.print(maple_bad_frame);
  out.print(F(" BUSY="));
  out.print(maple_bus_busy);
  out.print(F(" GPIO="));
  out.print(maple_gpio_activity);
  out.print(F(" EDGE="));
  out.print(maple_posttx_edges_a);
  out.print('/');
  out.print(maple_posttx_edges_b);
  out.print(F(" WAIT="));
  out.print(maple_wait_low_a);
  out.print('/');
  out.print(maple_wait_low_b);
  out.print(F(" DMA="));
  out.print(maple_dma_words);
  out.print(F(" IRQ="));
  out.print(maple_rx_irq_hits);
  out.print(F(" F0=0x"));
  out.print(maple_last_rx_word0_swapped, HEX);
  out.print(F(" RAW=0x"));
  out.print(maple_last_rx_word0_raw, HEX);
  out.print(F(" LEN="));
  out.println((int)maple_last_rx_len);

  out.print(F("DCSTAT DEVINFO PR=0x"));
  out.print((int)maple_last_probe_recipient, HEX);
  out.print(F(" DI_R=0x"));
  out.print((int)maple_last_devinfo_recipient, HEX);
  out.print(F(" DI_S=0x"));
  out.print((int)maple_last_devinfo_sender, HEX);
  out.print(F(" DI_LEN="));
  out.print((int)maple_last_devinfo_len);
  out.print(F(" FN=0x"));
  out.print(maple_last_devinfo_func_native, HEX);
  out.print(F(" FS=0x"));
  out.print(maple_last_devinfo_func_swapped, HEX);
  out.print(F(" LAYOUT="));
  out.println((int)maple_last_condition_layout);

  out.print(F("DCSTAT VMU ADDR=0x"));
  out.print((int)maple_last_vmu_addr, HEX);
  out.print(F(" PH="));
  out.print((int)maple_last_vmu_phase);
  out.print(F(" B="));
  out.print((int)maple_last_vmu_block);
  out.print(F(" CMD=0x"));
  out.print((int)maple_last_vmu_cmd, HEX);
  out.print(F(" LEN="));
  out.print((int)maple_last_vmu_len);
  out.print(F(" W0=0x"));
  out.print(maple_last_vmu_word0, HEX);
  out.print(F(" W1=0x"));
  out.print(maple_last_vmu_word1, HEX);
  out.print(F(" W2=0x"));
  out.print(maple_last_vmu_word2, HEX);
  out.print(F(" WC="));
  out.print((int)maple_last_vmu_write_count);
  out.print(F(" PB="));
  out.print((int)maple_last_vmu_phase_bytes);
  out.print(F(" STAGE="));
  out.println((int)maple_last_vmu_stage);
  out.println(F("OK:DCSTAT"));
}

#endif
