#pragma once

#ifdef ADAPT_HAS_USB_HOST_STACK
  // Boards with any USB-host hardware path need a concrete D+ pin.
  #ifndef PIN_USB_HOST_DP
    #ifdef PIN_USB_HOST_DP_DEFAULT
      #define PIN_USB_HOST_DP PIN_USB_HOST_DP_DEFAULT
    #endif
  #endif
#endif
