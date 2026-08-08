#pragma once

#include <stdint.h>

class SSD1306AsciiWire;

#include "menu_action.h"
#include "menu_item_defs.h"

void printOledMarqueeText(SSD1306AsciiWire& target, const char* text,
                          uint8_t max_chars, bool selected,
                          uint32_t marquee_start_ms = 0);

bool should_hide_menu_item(menu_item_enum item);
uint8_t getVisibleItemCount();
uint8_t getActualIndex(uint8_t visible_index);
uint8_t getVisibleIndex(uint8_t actual_index);
void clearRow(uint8_t row);
bool is_mode_specific_setting(menu_item_enum item);
bool is_per_input_quick_setting(menu_item_enum item);
bool is_system_setting(menu_item_enum item);
void printMenuItem(uint8_t actual_index, uint8_t row, bool selected);
void renderBottomRow(bool on_bottom_row, bool right_selected);
void renderScrollIndicator(uint8_t scroll_offset);
void renderMenu(uint8_t selected_visible, uint8_t scroll_offset, bool bottom_right_selected);
void menuResetAllItems();
void menuSaveAllItems();
void handleMenuItemAction(menu_item_enum item, menu_item_action action);
