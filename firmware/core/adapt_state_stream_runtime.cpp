#include "adapt_state_stream_runtime.h"

#include <Adafruit_TinyUSB.h>
#include <string.h>

#include "../firmware_platform_config.h"
#include "../menu/menu_runtime_state.h"
#include "../menu/menu_ui_state.h"
#include "../menu/quick_config.h"
#include "../output/output_runtime_state.h"
#include "../output/usb/webhid/webhid_input_modes.h"
#include "controller_output_cache_state.h"
#include "controller_frame_state.h"
#include "device_runtime_state.h"
#include "neutral_frame_packet.h"

namespace {

constexpr int kAdaptStateCdcChunkMax = 64;
constexpr uint8_t kAdaptStateMagic0 = 'A';
constexpr uint8_t kAdaptStateMagic1 = 'D';
constexpr uint8_t kAdaptStateMagic2 = 'S';
constexpr uint8_t kAdaptStateMagic3 = 'T';
constexpr uint8_t kAdaptStateMaxPlayers = MAX_USB_OUT;

static_assert(kAdaptStateMaxPlayers <= 8, "ADAPTSTATE connected mask is one byte");

enum AdaptStateScreen : uint8_t {
  ADAPT_SCREEN_HOME = 0,
  ADAPT_SCREEN_QUICK_CONFIG = 1,
  ADAPT_SCREEN_SYSTEM_MENU = 2,
  ADAPT_SCREEN_PAD_TEST = 3,
  ADAPT_SCREEN_ANALOG_TEST = 4,
  ADAPT_SCREEN_GAME = 5,
  ADAPT_SCREEN_OTHER = 255,
};

typedef struct __attribute((packed, aligned(1))) {
  uint8_t magic[4];
  uint8_t version;
  uint8_t header_size;
  uint16_t packet_size;
  uint32_t sequence;
  uint8_t screen;
  uint8_t input_mode;
  uint8_t output_mode;
  uint8_t windows_output;
  uint8_t player_count;
  uint8_t max_players;
  uint8_t connected_mask;
  uint8_t menu_state;
  uint8_t menu_cursor;
  uint8_t menu_top;
  uint8_t menu_item;
  uint8_t menu_flags;
  uint8_t reserved;
} AdaptStateHeader;

typedef struct __attribute((packed, aligned(1))) {
  AdaptStateHeader header;
  NeutralFramePacket players[kAdaptStateMaxPlayers];
  uint32_t output_buttons[kAdaptStateMaxPlayers];
} AdaptStatePacket;

bool adaptStateEnabled = false;
uint16_t adaptStateRate = ADAPT_STATE_DEFAULT_RATE_HZ;
uint32_t adaptStateLastWriteMs = 0;
uint32_t adaptStateFrames = 0;
AdaptStatePacket pendingPacket{};
uint16_t pendingOffset = 0;
bool pendingActive = false;

uint16_t clampRate(uint16_t rateHz) {
  if (rateHz < ADAPT_STATE_MIN_RATE_HZ) {
    return ADAPT_STATE_MIN_RATE_HZ;
  }
  if (rateHz > ADAPT_STATE_MAX_RATE_HZ) {
    return ADAPT_STATE_MAX_RATE_HZ;
  }
  return rateHz;
}

uint16_t intervalForRate(uint16_t rateHz) {
  const uint16_t clamped = clampRate(rateHz);
  const uint16_t interval = (uint16_t)(1000u / clamped);
  return interval == 0 ? 1 : interval;
}

bool dueForPacket() {
  if (!adaptStateEnabled) {
    return false;
  }

  const uint32_t now = millis();
  const uint16_t intervalMs = intervalForRate(adaptStateRate);
  if (adaptStateLastWriteMs != 0 &&
      (uint32_t)(now - adaptStateLastWriteMs) < intervalMs) {
    return false;
  }
  adaptStateLastWriteMs = now;
  return true;
}

uint8_t activeScreen() {
  if (quickConfig.isOpen()) {
    return ADAPT_SCREEN_QUICK_CONFIG;
  }
  if (padTestActive) {
    return ADAPT_SCREEN_PAD_TEST;
  }
  if (analogTestActive) {
    return ADAPT_SCREEN_ANALOG_TEST;
  }
  if (games_submenu_active) {
    return ADAPT_SCREEN_GAME;
  }
  if (isMenuOpen) {
    return ADAPT_SCREEN_SYSTEM_MENU;
  }
  return ADAPT_SCREEN_HOME;
}

void fillMenuState(AdaptStateHeader& header) {
  header.menu_state = 0;
  header.menu_cursor = 0;
  header.menu_top = 0;
  header.menu_item = 0xFF;
  header.menu_flags = 0;

  if (quickConfig.isOpen()) {
    QuickConfigSnapshot snapshot{};
    quickConfig.getSnapshot(snapshot);
    header.menu_state = snapshot.state;
    header.menu_cursor = snapshot.cursor;
    header.menu_top = snapshot.top;
    header.menu_item = snapshot.item;
    header.menu_flags = snapshot.flags;
    return;
  }

  if (isMenuOpen) {
    header.menu_state = menu_navigation_state;
    header.menu_cursor = menu_selected_visible;
    header.menu_top = menu_scroll_offset;
    header.menu_item = menu_selected_visible;
    header.menu_flags = menu_bottom_right_selected ? 0x01 : 0x00;
  }
}

void beginPendingPacket() {
  memset(&pendingPacket, 0, sizeof(pendingPacket));
  pendingPacket.header.magic[0] = kAdaptStateMagic0;
  pendingPacket.header.magic[1] = kAdaptStateMagic1;
  pendingPacket.header.magic[2] = kAdaptStateMagic2;
  pendingPacket.header.magic[3] = kAdaptStateMagic3;
  pendingPacket.header.version = NEUTRAL_FRAME_PACKET_VERSION;
  pendingPacket.header.header_size = sizeof(AdaptStateHeader);
  pendingPacket.header.packet_size = sizeof(AdaptStatePacket);
  pendingPacket.header.sequence = adaptStateFrames;
  pendingPacket.header.screen = activeScreen();
  pendingPacket.header.input_mode = webhid_input_mode_from_device(deviceMode);
  pendingPacket.header.output_mode = (uint8_t)outputMode;
  pendingPacket.header.windows_output = menu_win_output;
  pendingPacket.header.max_players = kAdaptStateMaxPlayers;
  pendingPacket.header.player_count = max_devices > kAdaptStateMaxPlayers
    ? kAdaptStateMaxPlayers
    : max_devices;

  fillMenuState(pendingPacket.header);

  for (uint8_t player = 0; player < kAdaptStateMaxPlayers; ++player) {
    const controller_state_t& frame = controllerFrameConst(player);
    packNeutralFramePacket(frame, pendingPacket.players[player]);
    pendingPacket.output_buttons[player] = post_remap_buttons[player];
    if (frame.connected) {
      pendingPacket.header.connected_mask |= (uint8_t)((uint8_t)1u << player);
    }
  }

  pendingOffset = 0;
  pendingActive = true;
}

void servicePendingPacketCdc(Adafruit_USBD_CDC& stream) {
  if (!pendingActive) {
    return;
  }

  const int available = stream.availableForWrite();
  if (available <= 0) {
    return;
  }

  const uint8_t* data = reinterpret_cast<const uint8_t*>(&pendingPacket);
  const uint16_t length = sizeof(pendingPacket);
  if (pendingOffset >= length) {
    pendingActive = false;
    ++adaptStateFrames;
    return;
  }

  uint16_t remaining = (uint16_t)(length - pendingOffset);
  uint16_t chunk = (remaining > (uint16_t)available) ? (uint16_t)available : remaining;
  if (chunk > kAdaptStateCdcChunkMax) {
    chunk = kAdaptStateCdcChunkMax;
  }

  const size_t written = stream.write(data + pendingOffset, chunk);
  if (written == 0) {
    return;
  }
  pendingOffset += (uint16_t)written;
  if (pendingOffset >= length) {
    pendingActive = false;
    ++adaptStateFrames;
  }
}

}  // namespace

void adaptStateSetEnabled(bool enabled) {
  adaptStateEnabled = enabled;
  adaptStateLastWriteMs = 0;
  pendingActive = false;
  pendingOffset = 0;
}

bool adaptStateIsEnabled() {
  return adaptStateEnabled;
}

void adaptStateSetRateHz(uint16_t rateHz) {
  adaptStateRate = clampRate(rateHz);
}

uint16_t adaptStateRateHz() {
  return adaptStateRate;
}

uint32_t adaptStateFrameCount() {
  return adaptStateFrames;
}

void adaptStateWriteStatus(Print& stream) {
  stream.print(F("ADAPTSTATE ENABLED="));
  stream.print(adaptStateEnabled ? 1 : 0);
  stream.print(F(" RATE_HZ="));
  stream.print((int)adaptStateRate);
  stream.print(F(" PLAYERS="));
  stream.print((int)max_devices);
  stream.print(F(" MAX_PLAYERS="));
  stream.print((int)kAdaptStateMaxPlayers);
  stream.print(F(" PACKET_BYTES="));
  stream.print((int)sizeof(AdaptStatePacket));
  stream.print(F(" FRAMES="));
  stream.println(adaptStateFrames);
  stream.print(F("ADAPTSTATE BUTTONS="));
  stream.println(kNeutralFrameButtonOrder);
  stream.print(F("ADAPTSTATE ARCADE="));
  stream.println(kNeutralFrameArcadeOverlayOrder);
}

void adaptStateTask(Adafruit_USBD_CDC& stream) {
  if (!adaptStateEnabled) {
    pendingActive = false;
    pendingOffset = 0;
    return;
  }

  if (!pendingActive && dueForPacket()) {
    beginPendingPacket();
  }
  servicePendingPacketCdc(stream);
}
