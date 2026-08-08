////enum inputDeviceType : uint8_t {
////  DEVICE_TYPE_NONE = 0,
////  DEVICE_TYPE_MOUSE,        // buttons, x, y, wheel
////  DEVICE_TYPE_DIGITAL,      // digital only
////  DEVICE_TYPE_DUALSTICK,    // xbox, ps, switch
////  DEVICE_TYPE_JOYSTICK,     // joystick, hotas
////  DEVICE_TYPE_WHEEL,        // racing wheel with pedals
////  DEVICE_TYPE_DDGOCLASSIC,  // densha de go classic. encoded 16bits on DDGO report.
////  DEVICE_TYPE_OTHER,        // 
////};
//
//enum analog_stick_precision {
//  ANALOG_STICK_PRECISION_8 = 0, // common
//  ANALOG_STICK_PRECISION_12,    // switch pro
//  ANALOG_STICK_PRECISION_16,    // xbox
//};
//
//typedef struct __attribute((packed, aligned(1))) {
//  union {
//    struct __attribute((packed, aligned(1))) {
//      //config
//      uint8_t HAS_BTN_HOME: 1;
//      uint8_t HAS_BTN_SELECT : 1;
//      uint8_t HAS_BTN_START : 1;
//      uint8_t HAS_ANALOG_STICK_MAIN : 1;
//      uint8_t HAS_ANALOG_STICK_AUX : 1;
//      uint8_t HAS_ANALOG_TRIGGERS : 1;      //contains analog L2 and R2
//      uint8_t HAS_ANALOG_MAIN_BUTTONS : 1;  //contains analog buttons (except L2 and R2)
//      uint8_t HAS_ANALOG_DPAD : 1;          //contains analog DPAD
//    };
//    uint8_t config;
//  };
//
//  uint8_t sticks_precision_bits;
//
//
//  //digital data. buttons with "xbox label"
//  union {
//    struct __attribute((packed, aligned(1))) {
//      uint8_t PAD_U : 1;
//      uint8_t PAD_D : 1;
//      uint8_t PAD_L : 1;
//      uint8_t PAD_R : 1;
//      uint8_t A : 1;
//      uint8_t B : 1;
//      uint8_t X : 1;
//      uint8_t Y : 1;
//    
//      uint8_t L1 : 1; // sega Z, ogXbox white
//      uint8_t R1 : 1; // sega C, ogXbox black
//      uint8_t L2 : 1;
//      uint8_t R2 : 1;
//      uint8_t L3 : 1;
//      uint8_t R3 : 1;
//      
//      uint8_t START : 1;
//      uint8_t SELECT : 1;
//      uint8_t HOME : 1;
//
////      uint8_t BTN_13 : 1;
////      uint8_t BTN_14 : 1;
////      uint8_t BTN_15 : 1;
////      uint8_t BTN_16 : 1;
////      uint8_t BTN_17 : 1;
////      uint8_t BTN_18 : 1;
////      uint8_t BTN_19 : 1;
//      //uint8_t : 7; // padding
//
//      uint16_t EXTRA : 15; //
//    };
//    struct __attribute((packed, aligned(1))) {
//      uint32_t digital_buttons;
//    };
//  };
//
//  union {
//    struct __attribute((packed, aligned(1))) {
//      //analog axis
//      int16_t LX;
//      int16_t LY;
//      int16_t RX;
//      int16_t RY;
//    };
//    uint64_t analog_sticks;
//  };
//
//  union {
//    struct __attribute((packed, aligned(1))) {
//      //analog buttons
//      uint8_t ANALOG_L2;
//      uint8_t ANALOG_R2;
//      uint8_t ANALOG_A;
//      uint8_t ANALOG_B;
//      uint8_t ANALOG_X;
//      uint8_t ANALOG_Y;
//      uint8_t ANALOG_L1;
//      uint8_t ANALOG_R1;
//    };
//    uint64_t analog_buttons;
//  };
//
//  union {
//    struct __attribute((packed, aligned(1))) {
//      //analog dpad
//      uint8_t ANALOG_PAD_U;
//      uint8_t ANALOG_PAD_D;
//      uint8_t ANALOG_PAD_L;
//      uint8_t ANALOG_PAD_R;
//    };
//    uint32_t analog_pad;
//  };
//
//  //device_config_t CONFIG;
//  //inputDriver INPUT_DRIVER;
//} usb_input_report_t;
