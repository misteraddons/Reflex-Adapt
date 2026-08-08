#include "../../product_config.h"

#include "auth_storage.h"
#include "../runtime/output_boot_bridge.h"
#include "webhid_auth_runtime.h"

uint8_t auth_key_status = 0;  // Bit 0: PS4 loaded, bit 1 reserved
PS4Auth ps4Auth;

extern "C" void webhid_check_auth_keys() {
  updateAuthKeyStatus();
}
