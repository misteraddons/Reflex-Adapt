#pragma once

// Reflex Adapt 2 board, Classic2USB release profile.

// Mode button.
#define PIN_MODE_BTN      18

// OLED I2C.
#define PIN_OLED_SDA      0
#define PIN_OLED_SCL      1

// Buzzer / WS2812B shared GPIO.
#define PIN_BUZZER_LED    8

// HDMI Connector 1.
#define PIN_HDMI1_01      11
#define PIN_HDMI1_02      10
#define PIN_HDMI1_03      26
#define PIN_HDMI1_04      27
#define PIN_HDMI1_05       9
#define PIN_HDMI1_06      12
#define PIN_HDMI1_07      13
#define PIN_HDMI1_08       5
#define PIN_HDMI1_09       6
#define PIN_HDMI1_10       2
#define PIN_HDMI1_11       7
#define PIN_HDMI1_12       4
#define PIN_HDMI1_13       3

// HDMI Connector 2.
#define PIN_HDMI2_01      23
#define PIN_HDMI2_02      22
#define PIN_HDMI2_03      28
#define PIN_HDMI2_04      29
#define PIN_HDMI2_05      20
#define PIN_HDMI2_06      21
#define PIN_HDMI2_07      19
#define PIN_HDMI2_08      16
#define PIN_HDMI2_09      17
#define PIN_HDMI2_10      14
#define PIN_HDMI2_11      15
#define PIN_HDMI2_12      24
#define PIN_HDMI2_13      25

// Latency fixture wiring for controller-in-loop tests.
// Trigger: player 2 HDMI pin 1. Return: player 1 HDMI pin 13.
#define PIN_LATENCY_TRIGGER PIN_HDMI2_01
#define PIN_LATENCY_RETURN  PIN_HDMI1_13

// PIO-USB host path used by shared USB host services.
#define PIN_USB_HOST_DP_DEFAULT   6
