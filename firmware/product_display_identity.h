#pragma once

#include "product_config.h"

inline const char* getProductBootDisplayTitle() {
  return PRODUCT_OLED_BOOT_TITLE;
}

inline const char* getProductHomeDisplayTitle() {
  return PRODUCT_OLED_HOME_TITLE;
}

inline const char* getProductScreensaverTitle() {
  return "REFLEX";
}

inline const char* getProductAboutDisplayTitle() {
  return PRODUCT_OLED_ABOUT_TITLE;
}

inline const char* getProductAboutRepoPath() {
  return PRODUCT_OLED_ABOUT_LINK;
}
