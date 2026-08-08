#include "auth_status.h"

#include "../output_runtime_state.h"
#include "auth_storage.h"
#include "ps_auth_dongle_runtime.h"
#include "xbone_auth_passthrough.h"

namespace {

constexpr uint8_t kPs4AuthStatusMask = 0x01;

}  // namespace

bool authOutputModeRequiresAuth(outputMode_t mode) {
  switch (canonicalizeOutputMode(mode)) {
    case OUTPUT_XINPUT:
      // Xbox 360 is the only XInput-shaped mode that requires console auth.
      // Windows XInputW and MiSTer/Linux XInput2P must remain absent from this
      // switch so host-preferred XInput cannot be blocked by XSM3 state.
    case OUTPUT_PS4:
    case OUTPUT_PS5:
    case OUTPUT_XBOXONE:
      return true;
    default:
      return false;
  }
}

bool authOutputModeHasProvider(outputMode_t mode) {
  switch (canonicalizeOutputMode(mode)) {
    case OUTPUT_XINPUT:
      // XSM3 is internal and always available for the dedicated Xbox 360 path.
    case OUTPUT_XINPUT2P:
    case OUTPUT_XINPUTW:
      // Host XInput modes are auth-free but return true here so generic
      // "can run?" checks do not treat them as locked console outputs.
      return true;
    case OUTPUT_PS4:
      return ((authStorageKeyStatus() & kPs4AuthStatusMask) != 0) ||
             ps_auth_dongle_has_provider_for_mode(OUTPUT_PS4);
    case OUTPUT_PS5:
      #if defined(ENABLE_OUTPUT_PS5)
      return ps_auth_dongle_has_provider_for_mode(OUTPUT_PS5);
      #else
      return false;
      #endif
    case OUTPUT_XBOXONE:
      #if defined(PRODUCT_CLASSIC2USB)
      return false;
      #else
      #ifdef ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT
      return xbone_auth_passthrough_ready();
      #else
      return false;
      #endif
      #endif
    default:
      return true;
  }
}

bool authOutputModeCanRun(outputMode_t mode) {
  mode = canonicalizeOutputMode(mode);
  if (mode == OUTPUT_PS4) {
    // PS4 accepts unauthenticated controllers for the timeout grace window.
    return true;
  }
  if (mode == OUTPUT_PS5) {
    #if defined(ENABLE_OUTPUT_PS5)
    // PS5 output is part of the shared USB output matrix. Builds without an
    // auth provider can still enumerate locked so the UI can explain why
    // controls will not pass through.
    return true;
    #else
    return false;
    #endif
  }
  if (mode == OUTPUT_XBOXONE) {
    #if defined(PRODUCT_CLASSIC2USB)
    return false;
    #else
    #ifdef ENABLE_EXPERIMENTAL_XBOXONE_OUTPUT
    return true;
    #else
    return false;
    #endif
    #endif
  }
  return !authOutputModeRequiresAuth(mode) || authOutputModeHasProvider(mode);
}

bool authOutputModeShouldShowLock(outputMode_t mode) {
  return authOutputModeRequiresAuth(mode) && !authOutputModeHasProvider(mode);
}

bool authOutputModeShouldShowKey(outputMode_t mode) {
  return authOutputModeRequiresAuth(mode) && authOutputModeHasProvider(mode);
}
