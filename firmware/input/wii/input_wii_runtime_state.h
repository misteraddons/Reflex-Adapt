#pragma once

#include <Arduino.h>

extern uint32_t wii_last_digital_state[];
extern uint64_t wii_last_analog_sticks_state[];
extern uint64_t wii_last_analog_buttons_state[];


void cacheWiiAutodetectIdentity(uint8_t port, uint8_t pinPair, const uint8_t identity[6]);
bool peekWiiAutodetectIdentity(uint8_t port, uint8_t* pinPair, uint8_t identity[6]);
void clearWiiAutodetectIdentity(uint8_t port);
