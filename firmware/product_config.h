#pragma once

#ifndef PRODUCT_CONFIG_H
#define PRODUCT_CONFIG_H

#include "config/product_select.h"

#if defined(PRODUCT_CLASSIC2USB)
  #include "config/classic2usb/product_config.h"
#else
  #error "No supported Adapt product configuration selected"
#endif

#include "config/usb_config.h"

#endif // PRODUCT_CONFIG_H
