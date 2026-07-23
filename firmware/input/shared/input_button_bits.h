#pragma once

// Shared logical input button bit layout used across runtime helpers,
// display overlays, and feature modules.
#ifndef INPUT_A
  #define INPUT_A        (1UL <<  0)
  #define INPUT_B        (1UL <<  1)
  #define INPUT_X        (1UL <<  2)
  #define INPUT_Y        (1UL <<  3)
  #define INPUT_L1       (1UL <<  4)
  #define INPUT_R1       (1UL <<  5)
  #define INPUT_L2       (1UL <<  6)
  #define INPUT_R2       (1UL <<  7)
  #define INPUT_L3       (1UL <<  8)
  #define INPUT_R3       (1UL <<  9)
  #define INPUT_START    (1UL << 10)
  #define INPUT_SELECT   (1UL << 11)
  #define INPUT_HOME     (1UL << 12)
  #define INPUT_CAPTURE  (1UL << 13)
  #define INPUT_EXTRA0   (1UL << 14)
  #define INPUT_EXTRA1   (1UL << 15)
  #define INPUT_EXTRA2   (1UL << 16)
  #define INPUT_EXTRA3   (1UL << 17)
  #define INPUT_EXTRA4   (1UL << 18)
  #define INPUT_EXTRA5   (1UL << 19)
  #define INPUT_EXTRA6   (1UL << 20)
  #define INPUT_EXTRA7   (1UL << 21)
  #define INPUT_EXTRA8   (1UL << 22)
  #define INPUT_EXTRA9   (1UL << 23)
  #define INPUT_PAD_U    (1UL << 28)
  #define INPUT_PAD_D    (1UL << 29)
  #define INPUT_PAD_L    (1UL << 30)
  #define INPUT_PAD_R    (1UL << 31)
#endif
