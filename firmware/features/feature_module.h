#pragma once

#include <Arduino.h>

#include "../menu/menu_action.h"
#include "../menu/menu_item_defs.h"

struct FeatureModule {
  const char* id;
  bool (*handleMenuItem)(menu_item_enum item, menu_item_action action);
  bool (*shouldHideMenuItem)(menu_item_enum item, bool* hidden);
  bool (*handleSerialCommand)(const char* command, Print& out);
  void (*appendSerialHelp)(Print& out);
  void (*appendSerialState)(Print& out);
  bool (*renderOledTransient)();
};

bool featureModulesHandleMenuItem(menu_item_enum item, menu_item_action action);
bool featureModulesShouldHideMenuItem(menu_item_enum item, bool* hidden);
bool featureModulesHandleSerialCommand(const char* command, Print& out);
void featureModulesAppendSerialHelp(Print& out);
void featureModulesAppendSerialState(Print& out);
bool featureModulesRenderOledTransient();
