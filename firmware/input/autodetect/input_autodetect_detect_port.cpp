#include "../../product_config.h"
#include "../../firmware_platform_config.h"

#include <Arduino.h>

#include "../../platform/button_handler.h"
#include "../../platform/buzzer.h"
#include "Input_AutoDetect.h"

#ifdef ENABLE_INPUT_AUTODETECT
#ifdef AUTODETECT_DEBUG
namespace {

void recordAutoDetectProbeMs(uint8_t port, uint8_t probe_index, uint32_t started_ms) {
  if (port >= kAutoDetectDebugPortCount || probe_index >= ADDBG_PROBE_COUNT) {
    return;
  }

  const uint32_t elapsed = millis() - started_ms;
  lastDebug[port].probe_ms[probe_index] =
      (elapsed > 65535u) ? 65535u : (uint16_t)elapsed;
}

}  // namespace
#endif

AutoDetectResult AutoDetector::detectPortPsxOnly(uint8_t port, bool is_hotswap) {
  (void)is_hotswap;
  if (port >= kAutoDetectPortCount) {
    return AUTODETECT_NONE;
  }

  #ifdef AUTODETECT_DEBUG
  memset(&lastDebug[port], 0, sizeof(AutoDetectDebug));
  lastDebug[port].final_result = AUTODETECT_NONE;
  lastDebug[port].probes_run |= ADDBG_PROBE_PSX;
  uint32_t psx_probe_started = millis();
  auto finish = [&](AutoDetectResult result) -> AutoDetectResult {
    recordAutoDetectProbeMs(port, ADDBG_PROBE_INDEX_PSX, psx_probe_started);
    lastDebug[port].final_result = (uint8_t)result;
    return result;
  };
  #else
  auto finish = [&](AutoDetectResult result) -> AutoDetectResult { return result; };
  #endif

  #ifdef ENABLE_INPUT_PSX
  const autodetect_pins_t& pins = autodetect_pins[port];
  AutoDetectResult psx = probePSX(pins, port);
  if (psx != AUTODETECT_NONE) {
    return finish(psx);
  }
  #else
  #endif

  return finish(AUTODETECT_NONE);
}

AutoDetectResult AutoDetector::detectPortDreamcastOnly(uint8_t port, bool is_hotswap) {
  if (port >= kAutoDetectPortCount) {
    return AUTODETECT_NONE;
  }

  #ifdef AUTODETECT_DEBUG
  memset(&lastDebug[port], 0, sizeof(AutoDetectDebug));
  lastDebug[port].final_result = AUTODETECT_NONE;
  lastDebug[port].probes_run |= ADDBG_PROBE_DREAMCAST;
  uint32_t dreamcast_probe_started = millis();
  auto finish = [&](AutoDetectResult result) -> AutoDetectResult {
    recordAutoDetectProbeMs(port, ADDBG_PROBE_INDEX_DREAMCAST, dreamcast_probe_started);
    lastDebug[port].final_result = (uint8_t)result;
    return result;
  };
  #else
  auto finish = [&](AutoDetectResult result) -> AutoDetectResult { return result; };
  #endif

  #ifdef ENABLE_INPUT_DREAMCAST
  const autodetect_pins_t& pins = autodetect_pins[port];
  AutoDetectResult dreamcast = probeDreamcast(pins, is_hotswap);
  if (dreamcast != AUTODETECT_NONE) {
    return finish(dreamcast);
  }
  #else
  (void)is_hotswap;
  #endif

  return finish(AUTODETECT_NONE);
}

AutoDetectResult AutoDetector::detectPortShiftRegisterOnly(uint8_t port, bool is_hotswap) {
  (void)is_hotswap;
  if (port >= kAutoDetectPortCount) {
    return AUTODETECT_NONE;
  }

  #ifdef AUTODETECT_DEBUG
  memset(&lastDebug[port], 0, sizeof(AutoDetectDebug));
  lastDebug[port].final_result = AUTODETECT_NONE;
  lastDebug[port].probes_run |= ADDBG_PROBE_SNES;
  uint32_t snes_probe_started = millis();
  auto finish = [&](AutoDetectResult result) -> AutoDetectResult {
    recordAutoDetectProbeMs(port, ADDBG_PROBE_INDEX_SNES, snes_probe_started);
    lastDebug[port].final_result = (uint8_t)result;
    return result;
  };
  #else
  auto finish = [&](AutoDetectResult result) -> AutoDetectResult { return result; };
  #endif

  const autodetect_pins_t& pins = autodetect_pins[port];
  return finish(probeSNES(pins, port));
}

AutoDetectResult AutoDetector::detectPort(uint8_t port, bool is_hotswap) {
  if (port >= kAutoDetectPortCount) {
    return AUTODETECT_NONE;
  }

  #ifdef AUTODETECT_DEBUG
  memset(&lastDebug[port], 0, sizeof(AutoDetectDebug));
  lastDebug[port].final_result = AUTODETECT_NONE;
  auto finish = [&](AutoDetectResult result) -> AutoDetectResult {
    lastDebug[port].final_result = (uint8_t)result;
    return result;
  };
  #else
  auto finish = [&](AutoDetectResult result) -> AutoDetectResult { return result; };
  #endif

  const autodetect_pins_t& pins = autodetect_pins[port];
  bool pceCandidate = false;

  #ifdef ENABLE_INPUT_PSX
  AutoDetectResult psx = AUTODETECT_NONE;
  bool psxProbeAlreadyRun = false;
  if (is_hotswap) {
    // PSX replies are strict and quick; checking it first avoids making PSX
    // hotplug wait behind the slower Joybus/Saturn/Dreamcast miss paths.
    #ifdef AUTODETECT_DEBUG
    lastDebug[port].probes_run |= ADDBG_PROBE_PSX;
    uint32_t psx_probe_started = millis();
    #endif
    psx = probePSX(pins, port);
    psxProbeAlreadyRun = true;
    #ifdef AUTODETECT_DEBUG
    recordAutoDetectProbeMs(port, ADDBG_PROBE_INDEX_PSX, psx_probe_started);
    #endif
    if (psx != AUTODETECT_NONE) {
      return finish(psx);
    }
    resetSharedPins(pins, is_hotswap);
  }
  #endif

  #if defined(ENABLE_INPUT_N64) || defined(ENABLE_INPUT_GAMECUBE) || defined(ENABLE_INPUT_GBA)
  #ifdef AUTODETECT_DEBUG
  lastDebug[port].probes_run |= ADDBG_PROBE_JOYBUS;
  uint32_t joybus_probe_started = millis();
  #endif
  AutoDetectResult joybus = probeJoybus(pins, port, is_hotswap);
  #ifdef AUTODETECT_DEBUG
  recordAutoDetectProbeMs(port, ADDBG_PROBE_INDEX_JOYBUS, joybus_probe_started);
  #endif
  if (joybus != AUTODETECT_NONE) {
    return finish(joybus);
  }
  #endif

  resetSharedPins(pins, is_hotswap);

  #ifdef AUTODETECT_DEBUG
  lastDebug[port].probes_run |= ADDBG_PROBE_PCE;
  uint32_t pce_probe_started = millis();
  #endif
  AutoDetectResult pce = probePCE(pins, port);
  bool pceConfirmed = false;
  bool pceMultitapPresent = false;
  #ifdef ENABLE_INPUT_PCE
  resetSharedPins(pins, is_hotswap);
  pceConfirmed = confirmPCEViaPceLib(pins, is_hotswap, port, pce != AUTODETECT_NONE, &pceMultitapPresent);
  #endif
  #ifdef AUTODETECT_DEBUG
  lastDebug[port].pce_lib_confirmed = pceConfirmed;
  recordAutoDetectProbeMs(port, ADDBG_PROBE_INDEX_PCE, pce_probe_started);
  #endif
  if (pceMultitapPresent) {
    pce = AUTODETECT_NONE;
  }
  if (pce != AUTODETECT_NONE || pceConfirmed) {

    if (pceConfirmed) {
      pceCandidate = true;
    } else {
      resetSharedPins(pins, is_hotswap);
      AutoDetectResult pceSaturn = probeSaturn(pins, port, nullptr, is_hotswap);
      if (pceSaturn == AUTODETECT_SATURN) {
        return finish(pceSaturn);
      }
      #if defined(ENABLE_INPUT_SATURN) || defined(ENABLE_INPUT_MEGADRIVE)
      resetSharedPins(pins, is_hotswap);
      if (confirmSaturnViaSaturnLib(pins, port, is_hotswap)) {
        return finish(AUTODETECT_SATURN);
      }
      #endif
    }

    resetSharedPins(pins, is_hotswap);
  }

  resetSharedPins(pins, is_hotswap);

  #ifdef AUTODETECT_DEBUG
  lastDebug[port].probes_run |= ADDBG_PROBE_SNES;
  uint32_t snes_probe_started = millis();
  #endif
  AutoDetectResult snes = probeSNES(pins, port);
  #ifdef AUTODETECT_DEBUG
  recordAutoDetectProbeMs(port, ADDBG_PROBE_INDEX_SNES, snes_probe_started);
  #endif
  if (snes != AUTODETECT_NONE) {
    return finish(snes);
  }

  resetSharedPins(pins, is_hotswap);

  #ifdef AUTODETECT_DEBUG
  lastDebug[port].probes_run |= ADDBG_PROBE_SATURN;
  uint32_t saturn_probe_started = millis();
  #endif
  bool saturnBusActive = false;
  AutoDetectResult saturn = probeSaturn(pins, port, &saturnBusActive, is_hotswap);
  #ifdef AUTODETECT_DEBUG
  recordAutoDetectProbeMs(port, ADDBG_PROBE_INDEX_SATURN, saturn_probe_started);
  lastDebug[port].saturn_bus_active = saturnBusActive;
  #endif
  if (saturn == AUTODETECT_MEGADRIVE) {
    #if defined(ENABLE_INPUT_SATURN) || defined(ENABLE_INPUT_MEGADRIVE)
    resetSharedPins(pins, is_hotswap);
    if (confirmSaturnViaSaturnLib(pins, port, is_hotswap)) {
      return finish(AUTODETECT_SATURN);
    }
    #endif
    if (pceCandidate) {
      saturn = AUTODETECT_NONE;
    }
  }
  if (saturn == AUTODETECT_SATURN && pceCandidate) {
    saturn = AUTODETECT_NONE;
  }
  if (saturn != AUTODETECT_NONE) {
    return finish(saturn);
  }

  if (pceCandidate) {
    return finish(AUTODETECT_PCE);
  }

  if (pce != AUTODETECT_NONE) {
    resetSharedPins(pins, is_hotswap);
    if (confirmPCEViaRawProbe(pins, is_hotswap)) {
      return finish(AUTODETECT_PCE);
    }
  }

  resetSharedPins(pins, is_hotswap);

  #ifdef ENABLE_INPUT_DRIVING
  // Saturn Mission Stick/wheel analog activity can look like encoder movement;
  // keep Atari Driving as a fallback after Saturn/PCE have first refusal.
  #ifdef AUTODETECT_DEBUG
  lastDebug[port].probes_run |= ADDBG_PROBE_ATARI_DRIVING;
  uint32_t driving_probe_started = millis();
  #endif
  AutoDetectResult driving = probeAtariDriving(pins, is_hotswap);
  #ifdef AUTODETECT_DEBUG
  recordAutoDetectProbeMs(port, ADDBG_PROBE_INDEX_ATARI_DRIVING, driving_probe_started);
  #endif
  if (driving != AUTODETECT_NONE) {
    return finish(driving);
  }
  #endif

  resetSharedPins(pins, is_hotswap);

  #ifdef ENABLE_INPUT_3DO
  bool allow3doHotswap = !is_hotswap || (AUTODETECT_ENABLE_3DO_HOTSWAP != 0);
  if (!saturnBusActive && allow3doHotswap) {
    #ifdef AUTODETECT_DEBUG
    lastDebug[port].probes_run |= ADDBG_PROBE_3DO;
    uint32_t tdo_probe_started = millis();
    #endif
    AutoDetectResult tdo = probe3DO(pins, is_hotswap);
    #ifdef AUTODETECT_DEBUG
    recordAutoDetectProbeMs(port, ADDBG_PROBE_INDEX_3DO, tdo_probe_started);
    #endif
    if (tdo != AUTODETECT_NONE) {
      return finish(tdo);
    }
  }
  #endif

  resetSharedPins(pins, is_hotswap);

  #ifdef ENABLE_INPUT_PSX
  if (!psxProbeAlreadyRun) {
    #ifdef AUTODETECT_DEBUG
    lastDebug[port].probes_run |= ADDBG_PROBE_PSX;
    uint32_t psx_probe_started = millis();
    #endif
    psx = probePSX(pins, port);
    #ifdef AUTODETECT_DEBUG
    recordAutoDetectProbeMs(port, ADDBG_PROBE_INDEX_PSX, psx_probe_started);
    #endif
  }
  if (psx != AUTODETECT_NONE) {
    return finish(psx);
  }
  #endif

  resetSharedPins(pins, is_hotswap);

  #ifdef ENABLE_INPUT_WII
  #ifdef AUTODETECT_DEBUG
  lastDebug[port].probes_run |= ADDBG_PROBE_WII;
  uint32_t wii_probe_started = millis();
  #endif
  AutoDetectResult wii = probeWii(pins, port);
  #ifdef AUTODETECT_DEBUG
  recordAutoDetectProbeMs(port, ADDBG_PROBE_INDEX_WII, wii_probe_started);
  #endif
  if (wii != AUTODETECT_NONE) {
    return finish(wii);
  }
  #endif

  resetSharedPins(pins, is_hotswap);

  #ifdef ENABLE_INPUT_DREAMCAST
  #ifdef AUTODETECT_DEBUG
  lastDebug[port].probes_run |= ADDBG_PROBE_DREAMCAST;
  uint32_t dreamcast_probe_started = millis();
  #endif
  AutoDetectResult dreamcast = probeDreamcast(pins, is_hotswap);
  #ifdef AUTODETECT_DEBUG
  recordAutoDetectProbeMs(port, ADDBG_PROBE_INDEX_DREAMCAST, dreamcast_probe_started);
  #endif
  if (dreamcast != AUTODETECT_NONE) {
    return finish(dreamcast);
  }
  #endif

  resetSharedPins(pins, is_hotswap);

  #ifdef ENABLE_INPUT_NEOGEO
  #ifdef AUTODETECT_DEBUG
  lastDebug[port].probes_run |= ADDBG_PROBE_NEOGEO;
  uint32_t neogeo_probe_started = millis();
  #endif
  AutoDetectResult neogeo = probeNeoGeo(pins);
  #ifdef AUTODETECT_DEBUG
  recordAutoDetectProbeMs(port, ADDBG_PROBE_INDEX_NEOGEO, neogeo_probe_started);
  #endif
  if (neogeo != AUTODETECT_NONE) {
    return finish(neogeo);
  }
  #endif

  resetSharedPins(pins, is_hotswap);

  #ifdef ENABLE_INPUT_PADDLE
  #ifdef AUTODETECT_DEBUG
  lastDebug[port].probes_run |= ADDBG_PROBE_ATARI_PADDLE;
  uint32_t paddle_probe_started = millis();
  #endif
  AutoDetectResult paddle = probeAtariPaddle(pins, port);
  #ifdef AUTODETECT_DEBUG
  recordAutoDetectProbeMs(port, ADDBG_PROBE_INDEX_ATARI_PADDLE, paddle_probe_started);
  #endif
  if (paddle != AUTODETECT_NONE) {
    return finish(paddle);
  }
  #endif

  return finish(AUTODETECT_NONE);
}

#endif
