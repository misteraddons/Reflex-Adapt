#include "input_wii_runtime_state.h"

namespace {
constexpr uint8_t kWiiPorts = 2;
}

uint32_t wii_last_digital_state[kWiiPorts] = { 0 };
uint64_t wii_last_analog_sticks_state[kWiiPorts] = { 0 };
uint64_t wii_last_analog_buttons_state[kWiiPorts] = { 0 };


namespace {
struct WiiAutodetectIdentity {
  bool valid;
  uint8_t pinPair;
  uint8_t identity[6];
};
WiiAutodetectIdentity wiiAutodetectIdentity[kWiiPorts] = {};
}  // namespace

void cacheWiiAutodetectIdentity(uint8_t port, uint8_t pinPair, const uint8_t identity[6]) {
  if (port >= kWiiPorts || identity == nullptr) return;
  wiiAutodetectIdentity[port].valid = true;
  wiiAutodetectIdentity[port].pinPair = pinPair;
  memcpy(wiiAutodetectIdentity[port].identity, identity, 6);
}

bool peekWiiAutodetectIdentity(uint8_t port, uint8_t* pinPair, uint8_t identity[6]) {
  if (port >= kWiiPorts || pinPair == nullptr || identity == nullptr ||
      !wiiAutodetectIdentity[port].valid) return false;
  *pinPair = wiiAutodetectIdentity[port].pinPair;
  memcpy(identity, wiiAutodetectIdentity[port].identity, 6);
  return true;
}

void clearWiiAutodetectIdentity(uint8_t port) {
  if (port < kWiiPorts) wiiAutodetectIdentity[port].valid = false;
}
