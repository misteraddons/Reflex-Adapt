#include "../product_config.h"

#include "menu_screensaver_dispatch.h"

#include "menu_idle_runtime.h"
#include "menu_screensaver_action_games.h"
#include "menu_screensaver_auto_snake.h"
#include "menu_screensaver_bounce.h"
#include "menu_screensaver_simulations.h"
#include "screensaver_animation.h"
#include "menu_screensaver_starfield_matrix.h"
#include "menu_screensaver_visuals.h"
#include "menu_ui_state.h"
#include "../core/controller_settings_state.h"
#include "../platform/display_runtime_state.h"

void renderSelectedAnimation() {
  const bool enteringIdleAnimation = !idleAnimationActive;
  idleAnimationActive = true;
  screensaver_animation = sanitizeScreensaverAnimation(screensaver_animation);

#ifdef USE_I2C_DISPLAY
  if (enteringIdleAnimation) {
    beginDisplayWire();
    u8g2.begin();
    u8g2.setI2CAddress(I2C_ADDRESS * 2);
    restoreOledPanelOrientation();
  }
  u8g2.setDrawColor(1);
  display.invertDisplay(false);
  display.setInvertMode(false);
  display.setContrast(display_contrast);
#endif

  switch (screensaver_animation) {
    case 0: renderIdleAnimation(); break;
    case 1: renderStarfield(); break;
    case 2: renderMatrixRain(); break;
    case 3: renderAutoSnake(); break;
    case 4: renderPong(); break;
    case 5: renderGameOfLife(); break;
    case 6: renderFireworks(); break;
    case 7: renderClock(); break;
    case 8: renderMaze(); break;
    case 9: renderBreakout(); break;
    case 10: renderPlasma(); break;
    case 11: renderTetris(); break;
    case 12: renderRain(); break;
    case 13: renderSpirograph(); break;
    default: renderIdleAnimation(); break;
  }
}

void resetAnimationState() {
  idleAnimationActive = false;
#ifdef USE_I2C_DISPLAY
  restoreOledPanelOrientation();
  u8g2.setDrawColor(1);
  display.invertDisplay(false);
  display.setInvertMode(false);
#endif
  resetBounceScreensaverState();
  resetStarfieldMatrixScreensaverState();
  resetAutoSnakeScreensaverState();
  resetActionGameScreensaverState();
  resetSimulationScreensaverState();
  resetVisualScreensaverState();
}
