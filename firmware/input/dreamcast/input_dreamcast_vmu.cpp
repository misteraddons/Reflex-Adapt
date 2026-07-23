#include "Input_Dreamcast.h"

namespace {
DreamcastVmuSerialStats g_vmuSerialStats;

void resetDreamcastVmuPins(uint8_t pinA, uint8_t pinB) {
  gpio_set_oeover(pinA, GPIO_OVERRIDE_NORMAL);
  gpio_set_outover(pinA, GPIO_OVERRIDE_NORMAL);
  gpio_set_function(pinA, GPIO_FUNC_SIO);
  gpio_init(pinA);
  gpio_set_dir(pinA, GPIO_IN);
  gpio_pull_up(pinA);

  gpio_set_oeover(pinB, GPIO_OVERRIDE_NORMAL);
  gpio_set_outover(pinB, GPIO_OVERRIDE_NORMAL);
  gpio_set_function(pinB, GPIO_FUNC_SIO);
  gpio_init(pinB);
  gpio_set_dir(pinB, GPIO_IN);
  gpio_pull_up(pinB);
}
}

const DreamcastVmuSerialStats& dreamcastVmuSerialStats() {
  return g_vmuSerialStats;
}

bool RZInputDreamcast::refreshVmuInfo(uint8_t port, uint8_t slot, VMUInfo* info) {
  if (port >= input_ports || slot > 1 || maple[port] == nullptr || !maple[port]->isConnected()) {
    if (info != nullptr) {
      *info = VMUInfo{};
    }
    return false;
  }

  quietVmuPolling(port, 180);

  // SCAN must prove the VMU is still answering. MaplePort preserves cached VMU
  // info through transient timeouts for controller stability, so clear the slot
  // here before probing or Adapt.html can see a stale "present" VMU that times
  // out on the first block read.
  maple[port]->clearVMUInfo(slot);
  const bool present = maple[port]->queryVMU(slot) && maple[port]->getVMUMemoryInfo(slot);
  quietVmuPolling(port);
  if (info != nullptr) {
    *info = present ? maple[port]->getVMUInfo(slot) : VMUInfo{};
  }
  return present;
}

bool RZInputDreamcast::getVmuInfo(uint8_t port, uint8_t slot, VMUInfo* info) const {
  if (port >= input_ports || slot > 1 || maple[port] == nullptr) {
    if (info != nullptr) {
      *info = VMUInfo{};
    }
    return false;
  }

  const VMUInfo& current = maple[port]->getVMUInfo(slot);
  if (info != nullptr) {
    *info = current;
  }
  return current.present;
}

bool RZInputDreamcast::recoverVmuPort(uint8_t port) {
  if (port >= input_ports || maple[port] == nullptr) {
    return false;
  }

  g_vmuSerialStats.recovery_count++;
  const uint8_t pinA = input_dreamcast_config[port].pinA;
  const uint8_t pinB = input_dreamcast_config[port].pinB;

  quietVmuPolling(port, 350);
  maple[port]->end();
  resetDreamcastVmuPins(pinA, pinB);
  delay(10);
  initSuccess[port] = (pinB == pinA + 1) && maple[port]->begin(pinA);
  if (!initSuccess[port]) {
    return false;
  }

  const uint32_t start = millis();
  while ((millis() - start) < 550) {
    maple[port]->update();
    if (maple[port]->isConnected()) {
      if (port < MAX_USB_OUT) {
        controller_state_t& frame = inputFrame(port);
        setInputFrameConnected(frame, true);
        setInputFrameTypeName(frame, getControllerDeviceLabel(maple[port]->getDeviceInfo(), maple[port]->getAccessoryFunctionMask()));
        setUpdated(port);
      }
      g_vmuSerialStats.recovery_success_count++;
      quietVmuPolling(port, 160);
      return true;
    }
    delay(4);
  }

  if (port < MAX_USB_OUT) {
    resetState(port);
    setInputFrameConnected(inputFrame(port), false);
    setUpdated(port);
  }
  return false;
}

VMUBlockResult RZInputDreamcast::readVmuBlock(uint8_t port, uint8_t slot, uint16_t block, uint8_t* buffer) {
  const uint32_t startUs = micros();
  g_vmuSerialStats.read_count++;
  g_vmuSerialStats.last_port = port;
  g_vmuSerialStats.last_slot = slot;
  g_vmuSerialStats.last_block = block;

  auto finish = [&](VMUBlockResult result) {
    const uint32_t elapsedUs = micros() - startUs;
    quietVmuPolling(port);
    g_vmuSerialStats.last_read_us = elapsedUs;
    if (elapsedUs > g_vmuSerialStats.max_read_us) {
      g_vmuSerialStats.max_read_us = elapsedUs;
    }
    g_vmuSerialStats.last_result = result;
    if (result == VMUBlockResult::SUCCESS) {
      g_vmuSerialStats.success_count++;
    } else if (result == VMUBlockResult::TIMEOUT) {
      g_vmuSerialStats.timeout_count++;
    }
    return result;
  };

  if (port >= input_ports || slot > 1 || maple[port] == nullptr || buffer == nullptr) {
    return finish(VMUBlockResult::NOT_PRESENT);
  }

  VMUInfo info{};
  if (!getVmuInfo(port, slot, &info) && !refreshVmuInfo(port, slot, &info)) {
    return finish(VMUBlockResult::NOT_PRESENT);
  }

  VMUBlockResult result = VMUBlockResult::READ_ERROR;
  quietVmuPolling(port, 120);
  for (uint8_t attempt = 0; attempt < 5; ++attempt) {
    result = maple[port]->readVMUBlock(slot, block, buffer);
    if (result == VMUBlockResult::SUCCESS) {
      break;
    }
    if (result != VMUBlockResult::TIMEOUT) {
      break;
    }
    g_vmuSerialStats.timeout_attempt_count++;
    if (attempt + 1 >= 5) {
      break;
    }
    delay(60);
    recoverVmuPort(port);
    VMUInfo refreshed{};
    if (!refreshVmuInfo(port, slot, &refreshed)) {
      result = VMUBlockResult::NOT_PRESENT;
      break;
    }
    quietVmuPolling(port, 120);
    g_vmuSerialStats.retry_count++;
  }

  return finish(result);
}

VMUBlockResult RZInputDreamcast::writeVmuBlock(uint8_t port, uint8_t slot, uint16_t block, const uint8_t* buffer) {
  const uint32_t startUs = micros();
  g_vmuSerialStats.write_count++;
  g_vmuSerialStats.last_port = port;
  g_vmuSerialStats.last_slot = slot;
  g_vmuSerialStats.last_block = block;

  auto finish = [&](VMUBlockResult result) {
    const uint32_t elapsedUs = micros() - startUs;
    quietVmuPolling(port, 140);
    g_vmuSerialStats.last_read_us = elapsedUs;
    if (elapsedUs > g_vmuSerialStats.max_read_us) {
      g_vmuSerialStats.max_read_us = elapsedUs;
    }
    g_vmuSerialStats.last_result = result;
    if (result == VMUBlockResult::SUCCESS) {
      g_vmuSerialStats.write_success_count++;
    } else if (result == VMUBlockResult::TIMEOUT) {
      g_vmuSerialStats.timeout_count++;
    }
    return result;
  };

  if (port >= input_ports || slot > 1 || maple[port] == nullptr || buffer == nullptr) {
    return finish(VMUBlockResult::NOT_PRESENT);
  }

  VMUInfo info{};
  if (!getVmuInfo(port, slot, &info) && !refreshVmuInfo(port, slot, &info)) {
    return finish(VMUBlockResult::NOT_PRESENT);
  }

  VMUBlockResult result = VMUBlockResult::WRITE_ERROR;
  quietVmuPolling(port, 650);
  for (uint8_t attempt = 0; attempt < 3; ++attempt) {
    result = maple[port]->writeVMUBlock(slot, block, buffer);
    if (result == VMUBlockResult::SUCCESS) {
      break;
    }
    if (result != VMUBlockResult::TIMEOUT) {
      break;
    }
    g_vmuSerialStats.timeout_attempt_count++;
    if (attempt + 1 >= 3) {
      break;
    }
    delay(60);
    recoverVmuPort(port);
    VMUInfo refreshed{};
    if (!refreshVmuInfo(port, slot, &refreshed)) {
      result = VMUBlockResult::NOT_PRESENT;
      break;
    }
    g_vmuSerialStats.retry_count++;
  }

  return finish(result);
}
