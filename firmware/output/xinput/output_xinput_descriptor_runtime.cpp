#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#include <Arduino.h>
#include <device/usbd_pvt.h>
#include <tusb_option.h>

#include "../output_runtime_state.h"
#include "out_xinput_multi.h"
#include "out_xinput.h"
#include "output_xinput_auth_runtime_state.h"
#include "output_xinput_autodetect_descriptor_data_runtime.h"
#include "output_xinput_descriptor_data_runtime.h"
#include "output_xinput_descriptor_state.h"

namespace {

constexpr uint16_t kMsOs20SetHeaderLen = 0x0A;
constexpr uint16_t kMsOs20ConfigSubsetHeaderLen = 0x08;
constexpr uint16_t kMsOs20ConfigSubsetOffset = kMsOs20SetHeaderLen;
constexpr uint16_t kMsOs20FunctionSubsetOffset =
    kMsOs20SetHeaderLen + kMsOs20ConfigSubsetHeaderLen;
constexpr uint16_t kMsOs20FunctionFirstInterfaceOffset = 4;
constexpr uint16_t kMsOs20TotalLengthOffset = 8;
constexpr uint16_t kMsOs20ConfigSubsetTotalLengthOffset =
    kMsOs20ConfigSubsetOffset + 6;

void write_le16(uint8_t* dst, uint16_t value) {
    dst[0] = value & 0xFF;
    dst[1] = (value >> 8) & 0xFF;
}

void build_xinput_multi_ms_os_descriptor(uint8_t first_itf) {
    memcpy(desc_ms_os_20_multi,
           desc_ms_os_20,
           kMsOs20FunctionSubsetOffset);

    write_le16(&desc_ms_os_20_multi[kMsOs20TotalLengthOffset],
               MS_OS_20_MULTI_DESC_LEN);
    write_le16(&desc_ms_os_20_multi[kMsOs20ConfigSubsetTotalLengthOffset],
               MS_OS_20_MULTI_DESC_LEN - kMsOs20SetHeaderLen);

    memcpy(&desc_ms_os_20_multi[kMsOs20FunctionSubsetOffset],
           &desc_ms_os_20[kMsOs20FunctionSubsetOffset],
           MS_OS_20_FUNCTION_SUBSET_LEN);
    memcpy(&desc_ms_os_20_multi[kMsOs20FunctionSubsetOffset +
                                MS_OS_20_FUNCTION_SUBSET_LEN],
           &desc_ms_os_20[kMsOs20FunctionSubsetOffset],
           MS_OS_20_FUNCTION_SUBSET_LEN);

    desc_ms_os_20_multi[kMsOs20FunctionSubsetOffset +
                        kMsOs20FunctionFirstInterfaceOffset] = first_itf;
    desc_ms_os_20_multi[kMsOs20FunctionSubsetOffset +
                        MS_OS_20_FUNCTION_SUBSET_LEN +
                        kMsOs20FunctionFirstInterfaceOffset] =
        first_itf + XINPUT_MULTI_INTERFACES_PER_CONTROLLER;
}

bool use_xinput_multi_ms_os_descriptor() {
    return outputMode == OUTPUT_XINPUT2P &&
           xinput_multi_controller_count() > 1;
}

}  // namespace

void xinput_register_descriptor_hooks(void) {
    // The donor TinyUSB library is used unchanged and does not expose the
    // descriptor hook globals from our experimental third_party copy.
    debug_hooks_registered = false;
}

void autodetect_register_descriptor_hooks(uint16_t hid_report_desc_len) {
    xinput_init_autodetect_config(hid_report_desc_len);
    debug_hooks_registered = false;
}

void xinput_fill_descriptor_debug_info(XInputDebugInfo* info) {
    if (!info) return;
    info->raw_descriptors = xinput_use_raw_descriptors;
    info->hooks_registered = debug_hooks_registered;
    info->desc_device_calls = debug_desc_device_calls;
    info->desc_config_calls = debug_desc_config_calls;
}

bool xinput_handle_ms_os_descriptor_request(
    uint8_t rhport,
    const tusb_control_request_t *request
) {
    const uint8_t* descriptor = desc_ms_os_20;
    if (use_xinput_multi_ms_os_descriptor()) {
        build_xinput_multi_ms_os_descriptor(desc_ms_os_20[kMsOs20FunctionSubsetOffset +
                                                          kMsOs20FunctionFirstInterfaceOffset]);
        descriptor = desc_ms_os_20_multi;
    }

    uint16_t total_len;
    memcpy(&total_len, descriptor + kMsOs20TotalLengthOffset, 2);
    return tud_control_xfer(rhport, request, (void *)descriptor, total_len);
}

extern "C" bool xinput_handle_ms_os_descriptor_request_c(
    uint8_t rhport,
    const tusb_control_request_t *request
) {
    return xinput_handle_ms_os_descriptor_request(rhport, request);
}

void xinput_set_ms_os_first_interface(uint8_t itfnum) {
    desc_ms_os_20[kMsOs20FunctionSubsetOffset +
                  kMsOs20FunctionFirstInterfaceOffset] = itfnum;
    build_xinput_multi_ms_os_descriptor(itfnum);
}

extern "C" {

const uint8_t *tud_descriptor_bos_cb(void) {
    if (use_xinput_multi_ms_os_descriptor()) {
        return desc_bos_multi;
    }
    return desc_bos;
}

}  // extern "C"

#endif  // ADAPT_OUTPUT_USB_DEVICE
