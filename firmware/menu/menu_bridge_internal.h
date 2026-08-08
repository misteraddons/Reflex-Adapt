#pragma once

#include <stdint.h>

#include "menu_input.h"

bool handleMenuToolScreens(bool btnChangeJustPressed, bool btnNavigateJustPressed,
                           const MenuControllerInput& ctrl,
                           uint8_t selected_visible, uint8_t scroll_offset, bool bottom_right);
bool handleMenuSubmenus(bool btnChangeJustPressed, bool btnNavigateJustPressed,
                        const MenuControllerInput& ctrl,
                        uint8_t selected_visible, uint8_t scroll_offset, bool bottom_right);
