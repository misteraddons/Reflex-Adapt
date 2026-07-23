#include "../product_config.h"

#include "menu_screensaver_simulations.h"

#include <Arduino.h>
#include <string.h>

#include "../platform/display_runtime_state.h"

#ifndef USE_I2C_DISPLAY

void renderGameOfLife() {}
void renderMaze() {}
void renderTetris() {}
void renderRain() {}
void resetSimulationScreensaverState() {}

#else

namespace {

constexpr uint32_t AMBIENT_FRAME_MS = 33;

bool golInitialized = false;
uint8_t golGrid[16][8];
uint8_t golNextGrid[16][8];

bool mazeInitialized = false;
uint8_t mazeGrid[32][16];
int8_t mazeStack[256][2];
uint8_t mazeStackTop = 0;

bool tetrisInitialized = false;
uint16_t tetrisFieldRows[10];
int8_t tetrisPieceX = 0;
int8_t tetrisPieceY = 0;
uint8_t tetrisPieceType = 0;
uint8_t tetrisPieceRot = 0;
const uint16_t tetrisPieces[7] = {0x4444, 0x0660, 0x0E40, 0x0C60, 0x06C0, 0x0E20, 0x0E80};

bool rainInitialized = false;
struct RainDrop {
  int8_t x;
  int8_t y;
  int8_t radius;
  int8_t maxRadius;
};
RainDrop rainDrops[8];

void initGameOfLife() {
  for (int x = 0; x < 16; x++) {
    for (int y = 0; y < 8; y++) {
      golGrid[x][y] = (micros() >> (x + y)) & 0xFF;
    }
  }
  golInitialized = true;
}

void initMaze() {
  memset(mazeGrid, 0xFF, sizeof(mazeGrid));
  mazeStackTop = 0;
  mazeStack[mazeStackTop][0] = 1;
  mazeStack[mazeStackTop][1] = 1;
  mazeStackTop++;
  mazeGrid[1][1] = 0;
  mazeInitialized = true;
}

void initTetris() {
  for (int i = 0; i < 10; i++) {
    tetrisFieldRows[i] = 0;
  }
  tetrisPieceX = 3;
  tetrisPieceY = 0;
  tetrisPieceType = micros() % 7;
  tetrisPieceRot = 0;
  tetrisInitialized = true;
}

void initRain() {
  for (int i = 0; i < 8; i++) {
    rainDrops[i].radius = 0;
    rainDrops[i].maxRadius = 0;
  }
  rainInitialized = true;
}

}  // namespace

void renderGameOfLife() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();
  if (!golInitialized) {
    initGameOfLife();
    lastFrame = now;
  }

  if (now - lastFrame < 150) {
    return;
  }
  lastFrame = now;

  for (int x = 0; x < 16; x++) {
    for (int y = 0; y < 8; y++) {
      golNextGrid[x][y] = 0;
      for (int bit = 0; bit < 8; bit++) {
        int cx = x * 8 + bit;
        int cy = y * 8;
        for (int subY = 0; subY < 8; subY++) {
          int neighbors = 0;
          for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
              if (dx == 0 && dy == 0) {
                continue;
              }
              int nx = (cx + dx + 128) % 128;
              int ny = (cy + subY + dy + 64) % 64;
              int gx = nx / 8;
              int gb = nx % 8;
              int gy = ny / 8;
              if ((golGrid[gx][gy] >> (7 - gb)) & 1) {
                neighbors++;
              }
            }
          }
          bool alive = (golGrid[x][y] >> (7 - bit)) & 1;
          bool next = alive ? (neighbors == 2 || neighbors == 3) : (neighbors == 3);
          if (next) {
            golNextGrid[x][y] |= (1 << (7 - bit));
          }
        }
      }
    }
  }
  memcpy(golGrid, golNextGrid, sizeof(golGrid));

  u8g2.clearBuffer();
  for (int x = 0; x < 16; x++) {
    for (int y = 0; y < 8; y++) {
      for (int bit = 0; bit < 8; bit++) {
        if ((golGrid[x][y] >> (7 - bit)) & 1) {
          u8g2.drawPixel(x * 8 + bit, y * 8);
        }
      }
    }
  }
  u8g2.sendBuffer();
}

void renderMaze() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();
  if (!mazeInitialized) {
    initMaze();
    lastFrame = now;
  }

  if (now - lastFrame < 20) {
    return;
  }
  lastFrame = now;

  if (mazeStackTop > 0) {
    int8_t cx = mazeStack[mazeStackTop - 1][0];
    int8_t cy = mazeStack[mazeStackTop - 1][1];

    int8_t dirs[4][2] = {{2, 0}, {-2, 0}, {0, 2}, {0, -2}};
    int8_t validDirs[4][2];
    uint8_t validCount = 0;

    for (int i = 0; i < 4; i++) {
      int8_t nx = cx + dirs[i][0];
      int8_t ny = cy + dirs[i][1];
      if (nx > 0 && nx < 31 && ny > 0 && ny < 15 && mazeGrid[nx][ny] == 0xFF) {
        validDirs[validCount][0] = dirs[i][0];
        validDirs[validCount][1] = dirs[i][1];
        validCount++;
      }
    }

    if (validCount > 0) {
      int choice = micros() % validCount;
      int8_t dx = validDirs[choice][0];
      int8_t dy = validDirs[choice][1];
      mazeGrid[cx + dx / 2][cy + dy / 2] = 0;
      mazeGrid[cx + dx][cy + dy] = 0;
      mazeStack[mazeStackTop][0] = cx + dx;
      mazeStack[mazeStackTop][1] = cy + dy;
      mazeStackTop++;
    } else {
      mazeStackTop--;
    }
  } else {
    static uint32_t completeTime = 0;
    if (completeTime == 0) {
      completeTime = now;
    }
    if (now - completeTime > 3000) {
      completeTime = 0;
      mazeInitialized = false;
    }
  }

  u8g2.clearBuffer();
  u8g2.drawBox(0, 0, 128, 64);
  u8g2.setDrawColor(0);
  for (int x = 0; x < 32; x++) {
    for (int y = 0; y < 16; y++) {
      if (mazeGrid[x][y]) {
        u8g2.drawBox(x * 4, y * 4, 4, 4);
      }
    }
  }
  if (mazeStackTop > 0) {
    u8g2.drawFrame(mazeStack[mazeStackTop - 1][0] * 4, mazeStack[mazeStackTop - 1][1] * 4, 4, 4);
  }
  u8g2.setDrawColor(1);
  u8g2.sendBuffer();
}

void renderTetris() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();
  if (!tetrisInitialized) {
    initTetris();
    lastFrame = now;
  }

  if (now - lastFrame < 200) {
    return;
  }
  lastFrame = now;

  tetrisPieceY++;
  bool collision = tetrisPieceY > 12;

  if (collision) {
    for (int i = 0; i < 4; i++) {
      int col = (tetrisPieceX + i) % 10;
      tetrisFieldRows[col] |= (1 << (15 - tetrisPieceY));
    }
    tetrisPieceX = 3;
    tetrisPieceY = 0;
    tetrisPieceType = micros() % 7;

    bool gameOver = false;
    for (int i = 0; i < 10; i++) {
      if (tetrisFieldRows[i] & 0xF000) {
        gameOver = true;
      }
    }
    if (gameOver) {
      initTetris();
    }
  }

  u8g2.clearBuffer();
  u8g2.drawFrame(34, 0, 62, 64);
  for (int col = 0; col < 10; col++) {
    for (int row = 0; row < 16; row++) {
      if (tetrisFieldRows[col] & (1 << (15 - row))) {
        u8g2.drawBox(36 + col * 6, row * 4, 5, 3);
      }
    }
  }
  u8g2.drawBox(36 + tetrisPieceX * 6, tetrisPieceY * 4, 23, 3);
  (void)tetrisPieceType;
  (void)tetrisPieceRot;
  (void)tetrisPieces;
  u8g2.sendBuffer();
}

void renderRain() {
  static uint32_t lastFrame = 0;
  static uint32_t lastDrop = 0;
  uint32_t now = millis();
  if (!rainInitialized) {
    initRain();
    lastFrame = now;
    lastDrop = now;
  }

  if (now - lastFrame < AMBIENT_FRAME_MS) {
    return;
  }
  lastFrame = now;

  if (now - lastDrop > 400) {
    lastDrop = now;
    for (int i = 0; i < 8; i++) {
      if (rainDrops[i].radius >= rainDrops[i].maxRadius) {
        rainDrops[i].x = 10 + (micros() % 108);
        rainDrops[i].y = 10 + (micros() % 44);
        rainDrops[i].radius = 1;
        rainDrops[i].maxRadius = 15 + (micros() % 15);
        break;
      }
    }
  }

  for (int i = 0; i < 8; i++) {
    if (rainDrops[i].radius < rainDrops[i].maxRadius) {
      rainDrops[i].radius++;
    }
  }

  u8g2.clearBuffer();
  for (int i = 0; i < 8; i++) {
    if (rainDrops[i].radius > 0 && rainDrops[i].radius < rainDrops[i].maxRadius) {
      u8g2.drawCircle(rainDrops[i].x, rainDrops[i].y, rainDrops[i].radius);
    }
  }
  u8g2.sendBuffer();
}

void resetSimulationScreensaverState() {
  golInitialized = false;
  mazeInitialized = false;
  tetrisInitialized = false;
  rainInitialized = false;
}

#endif
