#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#if defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)

#include "output_autodetect_rp2040_usb_debug.h"

#include <Adafruit_TinyUSB.h>

#include "../output_runtime_state.h"

#include "hardware/irq.h"
#include "hardware/regs/usb.h"
#include "hardware/structs/usb.h"

#ifndef PICO_SHARED_IRQ_HANDLER_LOWEST_ORDER_PRIORITY
#define PICO_SHARED_IRQ_HANDLER_LOWEST_ORDER_PRIORITY 0x00
#endif

namespace {

uint8_t attach_retry_stage = 0;
uint32_t attach_retry_reconnect_at_ms = 0;
uint8_t pullup_pulse_stage = 0;
uint32_t pullup_pulse_reconnect_at_ms = 0;
uint32_t pullup_pulse_next_at_ms = 0;

void rp2040_usb_debug_irq() {
  uint32_t status = usb_hw->ints;
  if (!status)
    return;

  output_autodetect_usb_irq_observed |= 0x80;
  if (status & USB_INTS_BUS_RESET_BITS)
    output_autodetect_usb_irq_observed |= 0x10;
  if (status & USB_INTS_SETUP_REQ_BITS)
    output_autodetect_usb_irq_observed |= 0x20;
  if (status & USB_INTS_BUFF_STATUS_BITS)
    output_autodetect_usb_irq_observed |= 0x40;
}

uint8_t get_rp2040_usb_hw_flags() {
  uint8_t flags = 0;

  if (tud_inited())
    flags |= 0x01;
  if (irq_is_enabled(USBCTRL_IRQ))
    flags |= 0x02;
  if (usb_hw->main_ctrl & USB_MAIN_CTRL_CONTROLLER_EN_BITS)
    flags |= 0x04;
  if (usb_hw->sie_ctrl & USB_SIE_CTRL_PULLUP_EN_BITS)
    flags |= 0x08;

  const uint32_t required_muxing = USB_USB_MUXING_TO_PHY_BITS | USB_USB_MUXING_SOFTCON_BITS;
  if ((usb_hw->muxing & required_muxing) == required_muxing)
    flags |= 0x10;
  if (usb_hw->inte & USB_INTS_SETUP_REQ_BITS)
    flags |= 0x20;
  if (usb_hw->sie_status & USB_SIE_STATUS_CONNECTED_BITS)
    flags |= 0x40;
  if (usb_hw->sie_status & USB_SIE_STATUS_VBUS_DETECTED_BITS)
    flags |= 0x80;

  return flags;
}

void rp2040_usb_force_attach() {
  usb_hw->muxing = USB_USB_MUXING_TO_PHY_BITS | USB_USB_MUXING_SOFTCON_BITS;
  usb_hw->pwr = USB_USB_PWR_VBUS_DETECT_BITS | USB_USB_PWR_VBUS_DETECT_OVERRIDE_EN_BITS;
  usb_hw->main_ctrl = USB_MAIN_CTRL_CONTROLLER_EN_BITS;
  usb_hw->sie_ctrl |= USB_SIE_CTRL_EP0_INT_1BUF_BITS;
  usb_hw->inte |= USB_INTS_BUFF_STATUS_BITS | USB_INTS_BUS_RESET_BITS | USB_INTS_SETUP_REQ_BITS |
                  USB_INTS_DEV_SUSPEND_BITS | USB_INTS_DEV_RESUME_FROM_HOST_BITS;
  irq_set_enabled(USBCTRL_IRQ, true);
  tud_connect();
  usb_hw->sie_ctrl |= USB_SIE_CTRL_PULLUP_EN_BITS;
}

}  // namespace

extern "C" void rp2040_usb_install_debug_irq() {
  static bool installed = false;
  if (installed)
    return;

  irq_add_shared_handler(USBCTRL_IRQ, rp2040_usb_debug_irq,
                         PICO_SHARED_IRQ_HANDLER_LOWEST_ORDER_PRIORITY);
  installed = true;
}

bool output_autodetect_usb_attach_retry_attempted() {
#if defined(AUTODETECT_DISABLE_RP2040_ATTACH_RETRY)
  return false;
#else
  return attach_retry_stage >= 2;
#endif
}

void update_output_autodetect_rp2040_usb_debug(uint32_t ms_now, uint32_t auto_detect_elapsed_ms) {
#if defined(AUTODETECT_DISABLE_RP2040_ATTACH_RETRY)
  (void)ms_now;
#else
  if (!tud_connected()) {
    if (attach_retry_stage == 0 && auto_detect_elapsed_ms >= 750) {
      tud_disconnect();
      attach_retry_stage = 1;
      attach_retry_reconnect_at_ms = ms_now + 50;
    } else if (attach_retry_stage == 1 && (int32_t)(ms_now - attach_retry_reconnect_at_ms) >= 0) {
      rp2040_usb_force_attach();
      attach_retry_stage = 2;
    }
  }
#endif

#if defined(AUTODETECT_PULSE_PULLUP_AFTER_VBUS)
  const bool vbusDetected =
    (usb_hw->sie_status & USB_SIE_STATUS_VBUS_DETECTED_BITS) != 0;
  if (vbusDetected && !tud_connected()) {
    if (pullup_pulse_stage == 0 &&
        auto_detect_elapsed_ms >= 1000 &&
        (pullup_pulse_next_at_ms == 0 ||
         (int32_t)(ms_now - pullup_pulse_next_at_ms) >= 0)) {
      tud_disconnect();
      usb_hw->sie_ctrl &= ~USB_SIE_CTRL_PULLUP_EN_BITS;
      pullup_pulse_stage = 1;
      pullup_pulse_reconnect_at_ms = ms_now + 250;
      output_autodetect_transition_reason = AUTO_TRANSITION_DIAG_PULLUP_PULSE;
    } else if (pullup_pulse_stage == 1 &&
               (int32_t)(ms_now - pullup_pulse_reconnect_at_ms) >= 0) {
      rp2040_usb_force_attach();
      pullup_pulse_stage = 0;
      pullup_pulse_next_at_ms = ms_now + 2000;
    }
  } else if (tud_connected()) {
    pullup_pulse_stage = 0;
  }
#endif

  output_autodetect_usb_hw_flags = get_rp2040_usb_hw_flags();
  output_autodetect_usb_irq_flags = 0;
  if (usb_hw->inte & USB_INTS_BUS_RESET_BITS)
    output_autodetect_usb_irq_flags |= 0x01;
  if (usb_hw->inte & USB_INTS_SETUP_REQ_BITS)
    output_autodetect_usb_irq_flags |= 0x02;
  if (tud_task_event_ready())
    output_autodetect_usb_irq_flags |= 0x04;
  if (output_autodetect_usb_irq_observed)
    output_autodetect_usb_irq_flags |= 0x08;
  if (auto_detect_elapsed_ms >= 750)
    output_autodetect_usb_irq_flags |= 0x10;
  if (attach_retry_stage == 1)
    output_autodetect_usb_irq_flags |= 0x20;
  if (attach_retry_stage >= 2)
    output_autodetect_usb_irq_flags |= 0x40;
  output_autodetect_usb_irq_flags |=
    (uint8_t)(((usb_hw->sie_status & USB_SIE_STATUS_LINE_STATE_BITS) >>
               USB_SIE_STATUS_LINE_STATE_LSB) << 5);
  if (usb_hw->ints & (USB_INTS_BUS_RESET_BITS | USB_INTS_SETUP_REQ_BITS | USB_INTS_BUFF_STATUS_BITS))
    output_autodetect_usb_irq_flags |= 0x80;
}

#endif

#endif  // ADAPT_OUTPUT_USB_DEVICE
