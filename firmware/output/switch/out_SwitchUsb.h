/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 */

#ifndef SwitchUsb_h
#define SwitchUsb_h

#include "out_SwitchCommon.h"
#include "out_SwitchConsts.h"

class SwitchUsb : public SwitchCommon {
 public:
  void init();
  uint8_t *generate_usb_report();
};

#endif
