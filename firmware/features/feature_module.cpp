#include "../product_config.h"

#include "feature_module.h"

#ifdef ENABLE_TTY2OLED_SERIAL
#include "tty2oled/tty2oled_feature.h"
#endif

namespace {

const FeatureModule* const kFeatureModules[] = {
#ifdef ENABLE_TTY2OLED_SERIAL
  &kTty2OledFeatureModule,
#endif
  nullptr,
};

template <typename Callback>
bool visitModules(Callback callback) {
  for (const FeatureModule* module : kFeatureModules) {
    if (module && callback(*module)) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool featureModulesHandleMenuItem(menu_item_enum item, menu_item_action action) {
  return visitModules([&](const FeatureModule& module) {
    return module.handleMenuItem && module.handleMenuItem(item, action);
  });
}

bool featureModulesShouldHideMenuItem(menu_item_enum item, bool* hidden) {
  return visitModules([&](const FeatureModule& module) {
    return module.shouldHideMenuItem && module.shouldHideMenuItem(item, hidden);
  });
}

bool featureModulesHandleSerialCommand(const char* command, Print& out) {
  return visitModules([&](const FeatureModule& module) {
    return module.handleSerialCommand && module.handleSerialCommand(command, out);
  });
}

void featureModulesAppendSerialHelp(Print& out) {
  for (const FeatureModule* module : kFeatureModules) {
    if (module && module->appendSerialHelp) {
      module->appendSerialHelp(out);
    }
  }
}

void featureModulesAppendSerialState(Print& out) {
  for (const FeatureModule* module : kFeatureModules) {
    if (module && module->appendSerialState) {
      module->appendSerialState(out);
    }
  }
}

bool featureModulesRenderOledTransient() {
  return visitModules([](const FeatureModule& module) {
    return module.renderOledTransient && module.renderOledTransient();
  });
}
