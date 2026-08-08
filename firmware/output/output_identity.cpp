#include "../product_config.h"

#include "../core/device_runtime_state.h"
#include "runtime/output_boot_bridge.h"
#include "output_capabilities.h"

extern const uint16_t RZORD1_VID = 0x16D0;
extern const uint16_t RZORD1_PID = 0x1460;
// Keep the legacy Reflex PID only for MiSTer PSX special modes so existing
// MiSTer main quirk handling continues to match those descriptors.
extern const uint16_t RZORD1_MISTER_PID = 0x127E;
extern const uint16_t RZORD1_VERSION = 0x0100;
extern const char* const RZORD1_VENDOR = "MiSTerAddons";
extern const char* const RZORD1_PPRODUCT = "Adapt " PRODUCT_SHORT_NAME;
extern const uint8_t bcd_input_revision = 1;

const char* get_reflex_input_product_name() {
  if (output_effective_mode_is(OUTPUT_MISTER_GUNCON)) return "ReflexPSGun";
  if (output_effective_mode_is(OUTPUT_MISTER_NEGCON)) return "ReflexPSWheel";
  if (output_effective_mode_is(OUTPUT_MISTER_JOGCON)) return "ReflexPSJogCon";

  switch (deviceMode) {
#ifdef ENABLE_INPUT_N64
    case RZORD_N64: return "ReflexN64";
#endif
#ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE: return "ReflexGC";
#endif
#ifdef ENABLE_INPUT_GBA
    case RZORD_GBA: return "ReflexGBA";
#endif
#ifdef ENABLE_INPUT_NES
    case RZORD_NES: return "ReflexNES";
#endif
#ifdef ENABLE_INPUT_SNES
    case RZORD_SNES: return "ReflexSNES";
#endif
#ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY: return "ReflexVboy";
#endif
#ifdef ENABLE_INPUT_MEGADRIVE
    case RZORD_MEGADRIVE: return "ReflexMD";
#endif
#ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN: return "ReflexSat";
#endif
#ifdef ENABLE_INPUT_WII
    case RZORD_WII: return "ReflexWii";
#endif
#ifdef ENABLE_INPUT_PCE
    case RZORD_PCE: return "ReflexPCE";
#endif
#ifdef ENABLE_INPUT_PSX
    case RZORD_PSX: return "ReflexPSDS1";
#endif
#ifdef ENABLE_INPUT_PSX_DANCE
    case RZORD_PSX_DANCE: return "ReflexPSDance";
#endif
#ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO: return "ReflexNeoGeo";
#endif
#ifdef ENABLE_INPUT_3DO
    case RZORD_3DO: return "Reflex3DO";
#endif
#ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR: return "ReflexJag";
#endif
#ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST: return "ReflexDreamcast";
#endif
#ifdef ENABLE_INPUT_INTV
    case RZORD_INTV: return "ReflexIntv";
#endif
#ifdef ENABLE_INPUT_PADDLE
    case RZORD_PADDLE: return "ReflexPaddle";
#endif
#ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING: return "ReflexDriving";
#endif
#ifdef ENABLE_INPUT_GAMEPORT
    case RZORD_GAMEPORT: return "ReflexGameport";
#endif
#ifdef ENABLE_INPUT_MEMCARD
    case RZORD_MEMCARD: return "ReflexMemCard";
#endif
#ifdef ENABLE_INPUT_SMS
    case RZORD_SMS: return "ReflexSMS";
#endif
#ifdef ENABLE_INPUT_JPC
    case RZORD_JPC: return "ReflexJPC";
#endif
#ifdef ENABLE_INPUT_JVS
    case RZORD_JVS: return "ReflexJVS";
#endif
#ifdef ENABLE_INPUT_USB
    case RZORD_USB: return "ReflexUSB";
#endif
    default: return RZORD1_PPRODUCT;
  }
}

const char* get_reflex_input_usb_serial_descriptor() {
  // MiSTer copies the USB unique/serial string into the input name for
  // Reflex VID/PID devices. Use the exact input-mode identity here; shared
  // input modules like NES/SNES/VB must not collapse to one module USB ID.
  return get_reflex_input_product_name();
}
