#include "../../product_config.h"
#ifdef ADAPT_OUTPUT_USB_DEVICE

#include <Arduino.h>

#include "output_xinput_blob_identity.h"
#include "../auth/xb360_key_blob.h"

extern "C" {
#include <libxsm3/xsm3.h>
}

namespace {

bool xinputBlobHasSerialOverride(const uint8_t* blob) {
  const uint8_t* serial = blob + XB360_KEY_BLOB_SERIAL_OFFSET;
  for (uint8_t i = 0; i < XB360_KEY_BLOB_SERIAL_SIZE; i++) {
    if (serial[i] != 0x00 && serial[i] != 0xFF) {
      return true;
    }
  }
  return false;
}

}

void xinputApplyBlobIdentity() {
  memcpy(
      xsm3_id_data_ms_controller,
      xb360_key_blob_template + XB360_KEY_BLOB_ID_DATA_OFFSET,
      XB360_KEY_BLOB_ID_DATA_SIZE);

  uint8_t serial[0x0C];
  if (xinputBlobHasSerialOverride(xb360_key_blob_template)) {
    memcpy(
        serial,
        xb360_key_blob_template + XB360_KEY_BLOB_SERIAL_OFFSET,
        XB360_KEY_BLOB_SERIAL_SIZE);
  } else {
    memcpy(serial, xsm3_id_data_ms_controller + 6, XB360_KEY_BLOB_SERIAL_SIZE);
  }
  xsm3_set_vid_pid(serial, 0x045E, 0x028E);
}

#endif  // ADAPT_OUTPUT_USB_DEVICE
