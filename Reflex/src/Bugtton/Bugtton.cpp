
// Bugtton - button debounce library for ATmega168/328P

// Fast button debounce library for ATmega328P. Uses registers instead of digitalRead.
// It's fast and I want it faster.

// Created by Sami Kaukasalo / sakabug, July 20, 2021.

// MIT License

// Copyright (c) 2021 Sami Kaukasalo / sakabug

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "Bugtton.h"

// Bugtton buttons(buttonCount, buttonPins(array), INPUT/INPUT_PULLUP, debounce time)
Bugtton::Bugtton(const uint8_t a, const uint8_t *b, uint8_t dt){
    // Init values
    _maskD = B00000000;
    _maskB = B00000000;
    _maskC = B00000000;
    _maskE = B00000000;
    _idleD = B00000000;
    _idleB = B00000000;
    _idleC = B00000000;
    _idleE = B00000000;
    _count = a;
    _debounceTime = dt;
    _allStable = false;
    _flag1 = false;
    
    // Create buttons
    _pins = new uint8_t[_count];
    _bits = new uint8_t[_count];
    _stateStarted = new uint32_t[_count];
    _ticksStarted = new uint32_t[_count];
    // Init button data
    for(uint8_t i=0; i<_count; i++){
        // If pin # negative, it's inverted pin
        int8_t pin = b[i];

        //remap pins for leonardo board
        //output mode is not implemented
        #ifdef BUGTTON_IS_ATMEGA_32U4
        switch(pin) {
          case 0: pin = 2; break;//D2
          case 1: pin = 3; break;//D3
          case 2: pin = 1; break;//D1
          case 3: pin = 0; break;//D0
          //case 4: pin = 4; break;//D4 no need to remap
          case 5: pin = 22; break;//C6
          case 6: pin = 7; break;//D7
          case 7: pin = 30; break;//E6 no idea... 30?
          case 8: pin = 12; break;//B4
          case 9: pin = 13; break;//B5
          case 10: pin = 20; break;//B6
          case 11: pin = 21; break;//B7
          case 12: pin = 6; break;//D6
          case 13: pin = 23; break;//C7 no idea...23?
          case 14: pin = 11; break;//B3
          case 15: pin = 9; break;//B1
          case 16: pin = 10; break;//B2

          //case 17: pin = ?; break;//B0
          //case 18: pin = ?; break;//F7
          //case 19: pin = ?; break;//F6
          //case 20: pin = ?; break;//F5
        }
        #endif
        
        if(pin<0){
            pin *= -1;
            _bits[i] = B11100001;
            setMode(pin, INPUT);
        }
        else{
            _bits[i] = B11100000;
            setMode(pin, INPUT_PULLUP);
        }
        _pins[i] = pin;
        _stateStarted[i] = 0;
        _ticksStarted[i] = 0;
    }
    
    // Make bitmasks
    makeMasks();
}

// Bitmask for registers, formed from pin array, mark active registers
void Bugtton::makeMasks(){
    for (uint8_t i=0; i<_count; i++){
        if (_pins[i] < 8) {
            // Write to maskD   (active buttons)
            bitWrite(_maskD, _pins[i], 1);
            // Write to idleD  (button idle state)
            if (flippedBit(i)) bitWrite(_idleD, _pins[i], 0);
            else bitWrite(_idleD, _pins[i], 1);
        }
        else if (_pins[i] >= 8 && _pins[i] < 14) {
            // Write to maskB (active buttons)
            bitWrite(_maskB, (_pins[i]-8), 1);
            // Write to idleB  (button idle state)
            if (flippedBit(i)) bitWrite(_idleB, (_pins[i]-8), 0);
            else bitWrite(_idleB, (_pins[i]-8), 1);
        }
        else if (_pins[i] >= 14 && _pins[i] < 20) {
            // Write to maskC (active buttons)
            bitWrite(_maskC, (_pins[i]-14), 1);
            // Write to idleC  (button idle state)
            if (flippedBit(i)) bitWrite(_idleC, (_pins[i]-14), 0);
            else bitWrite(_idleC, (_pins[i]-14), 1);
        }
        else if (_pins[i] >= 20 && _pins[i] < 22) {
          // Write to maskB (active buttons)
          bitWrite(_maskB, (_pins[i]-14), 1);
          // Write to idleB  (button idle state)
          if (flippedBit(i)) bitWrite(_idleB, (_pins[i]-14), 0);
          else bitWrite(_idleB, (_pins[i]-14), 1);
        }
        else if (_pins[i] >= 22 && _pins[i] < 24) { //C6, C7
          // Write to maskC (active buttons)
          bitWrite(_maskC, (_pins[i]-16), 1);
          // Write to idleC  (button idle state)
          if (flippedBit(i)) bitWrite(_idleC, (_pins[i]-16), 0);
          else bitWrite(_idleC, (_pins[i]-16), 1);
        }
        else if (_pins[i] == 30) { //E6
          // Write to maskE (active buttons)
          bitWrite(_maskE, (_pins[i]-24), 1);
          // Write to idleE  (button idle state)
          if (flippedBit(i)) bitWrite(_idleE, (_pins[i]-24), 0);
          else bitWrite(_idleE, (_pins[i]-24), 1);
        }
    }
}

// For debugging purposes
void Bugtton::printBIN(uint8_t b){
  for(int i = 7; i >= 0; i--)
    Serial.print(bitRead(b,i));
  Serial.println();  
}

// If you need set debounce time with code, THIS IS set at constructor
void Bugtton::debounceTime(uint16_t a){ _debounceTime = a; }

// Updates all buttons at once, needs to run only once in loop
void Bugtton::update(){
    if (_allStable){
        // Buttons unpressed?
        if ((_idleD == (PIND&_maskD)) &&
            (_idleB == (PINB&_maskB)) &&
            (_idleB == (PINC&_maskC)) &&
            (_idleC == (PINE&_maskE)) ){
            // Let function run once, then keep skipping until changes in registers
            if (_flag1) {
                return;
            }
            _flag1 = true;
        }
    }
    //Update bits
    for (uint8_t i=0; i<_count; i++){
        // Reset changedBit
        changedBit(i, 0);
        // Active low (pull up)
        if ((_bits[i]&B00000001) == B00000000){
            if (_pins[i] < 8)       currentBit(i, PIND&(1<<_pins[i]) );
            else if (_pins[i] < 14) currentBit(i, PINB&(1<<(_pins[i]-8) ));
            else if (_pins[i] < 20) currentBit(i, PINC&(1<<(_pins[i]-14) ));
            else if (_pins[i] < 22) currentBit(i, PINB&(1<<(_pins[i]-14) ));
            else if (_pins[i] < 24) currentBit(i, PINC&(1<<(_pins[i]-16) ));
            else if (_pins[i] == 30) currentBit(i, PINE&(1<<(_pins[i]-24) ));
        }
        // Active high (pull down)
        else{
            if (_pins[i] < 8)       currentBit(i, !(PIND&(1<<_pins[i])) );
            else if (_pins[i] < 14) currentBit(i, !(PINB&(1<<(_pins[i]-8)) ));
            else if (_pins[i] < 20) currentBit(i, !(PINC&(1<<(_pins[i]-14)) ));
            else if (_pins[i] < 22) currentBit(i, !(PINB&(1<<(_pins[i]-14)) ));
            else if (_pins[i] < 24) currentBit(i, !(PINC&(1<<(_pins[i]-16)) ));
            else if (_pins[i] == 30) currentBit(i, !(PINE&(1<<(_pins[i]-24)) ));
        }
        //No change in button state
        if ( currentBit(i) == oldBit(i)){
          stableBit(i, 1);
          _allStable = true;
        }
        //Change in button state
        else {
            _allStable = false;
            _flag1 = false;
            // Unstable, wait for debounce time
            if (stableBit(i)){
                stableBit(i, 0);
                stateStarted(i, millis());
            }
            // Stable, debounce finished, state will change
            else if (millis() - stateStarted(i) >= _debounceTime ){
                stableBit(i, 1);
                oldBit(i, currentBit(i));
                changedBit(i, 1);
                stateStarted(i, millis());
                _ticksStarted[i] = millis();
                heldUntilUsed(i,0);
            }
        }
    }
}

// Manipulating single button byte
void Bugtton::currentBit(uint8_t i, bool a) { bitWrite(_bits[i], 7, a); }
bool Bugtton::currentBit(uint8_t i)         { return bitRead(_bits[i], 7); }
void Bugtton::stableBit(uint8_t i, bool a)  { bitWrite(_bits[i], 6, a); }
bool Bugtton::stableBit(uint8_t i)          { return bitRead(_bits[i], 6); }      
void Bugtton::oldBit(uint8_t i, bool a)     { bitWrite(_bits[i], 5, a); }
bool Bugtton::oldBit(uint8_t i)             { return bitRead(_bits[i], 5); }
void Bugtton::changedBit(uint8_t i, bool a) { bitWrite(_bits[i], 4, a); }
bool Bugtton::changedBit(uint8_t i)         { return bitRead(_bits[i], 4); }
void Bugtton::heldUntilUsed(uint8_t i, bool a) { bitWrite(_bits[i], 3, a); }
bool Bugtton::heldUntilUsed(uint8_t i)      { return bitRead(_bits[i], 3); }
void Bugtton::tickBit(uint8_t i, bool a)    { bitWrite(_bits[i], 2, a); }
bool Bugtton::tickBit(uint8_t i)            { return bitRead(_bits[i], 2); }
void Bugtton::flippedBit(uint8_t i, bool a) { bitWrite(_bits[i], 0, a); }
bool Bugtton::flippedBit(uint8_t i)         { return bitRead(_bits[i], 0); }

// Timestamps for debounce, and duration function
void Bugtton::stateStarted(uint8_t i, uint32_t a){ _stateStarted[i] = a; }
uint32_t Bugtton::stateStarted(uint8_t i) { return _stateStarted[i]; }
uint32_t Bugtton::duration(uint8_t i) { return millis() - _stateStarted[i]; }

// Set pin mode here
void Bugtton::setMode(uint8_t i, uint8_t mode){
    if (mode == OUTPUT){
        // Set DDR bit -> OUTPUT
        if (i < 8) DDRD|=(1<<(i));
        else if (i < 14) DDRB|=(1<<(i-8));
        else if (i < 20) DDRC|=(1<<(i-14));
        // Defaults to LOW if coming from INPUT mode (HIGH if from INPUT_PULLUP)
    }
    else{
        // Clear DDR bit -> INPUT
        if (i < 8) DDRD&=~(1<<(i));
        else if (i < 14) DDRB&=~(1<<(i-8));
        else if (i < 20) DDRC&=~(1<<(i-14));
        else if (i < 22) DDRB&=~(1<<(i-14));
        else if (i < 24) DDRC&=~(1<<(i-16));
        else if (i == 30) DDRE&=~(1<<(i-24));
        
        if (mode == INPUT_PULLUP){
            // Set PORT bit
            if (i < 8) PORTD|=(1<<(i));
            else if (i < 14) PORTB|=(1<<(i-8));
            else if (i < 20) PORTC|=(1<<(i-14));
            else if (i < 22) PORTB|=(1<<(i-14));
            else if (i < 24) PORTC|=(1<<(i-16));
            else if (i == 30) PORTE|=(1<<(i-24));
        }
        else{
            // Clear PORT bit
            if (i < 8) PORTD&=~(1<<(i));
            else if (i < 14) PORTB&=~(1<<(i-8));
            else if (i < 20) PORTC&=~(1<<(i-14));
            else if (i < 22) PORTB&=~(1<<(i-14));
            else if (i < 24) PORTC&=~(1<<(i-16));
            else if (i == 30) PORTE&=~(1<<(i-24));
        }
    }
}

// Button state has changed to unpressed to pressed
bool Bugtton::fell(uint8_t i){
	if ( (_bits[i]&B11110000) == B01010000 ) {
        return true;
    }
	return false;
}

// Button state has changed to pressed to unpressed
bool Bugtton::rose(uint8_t i){
	if ( (_bits[i]&B11110000) == B11110000 ) {
        return true;
    }
	return false;
}

// Button is unpressed
bool Bugtton::up(uint8_t i){
    if ( (_bits[i]&B11110000) == B11100000 ) {
        return true;
    }
    return false;
}

// Button is being held down (pressed)
bool Bugtton::held(uint8_t i){
	if ( (_bits[i]&B11110000) == B01000000 ) {
        return true;
    }
	return false;
}

// Returns true once when <time> ms reached while button pressed state
bool Bugtton::heldUntil(uint8_t i, uint16_t t){
    //printBIN(_bits[i]);
	if ( (_bits[i]&B11111000) == B01000000 && duration(i) >= t) {
        bitWrite(_bits[i], 3, 1);
        return true;
    }
	return false;
}

// Returns true once when <time> ms reached while button unpressed state
bool Bugtton::upUntil(uint8_t i, uint16_t t){
	if ( (_bits[i]&B11111000) == B11100000 && duration(i) >= t) {
        bitWrite(_bits[i], 3, 1);
        return true;
    }
	return false;
}

// Returns true once every <time> ms
bool Bugtton::intervalTick(uint8_t i, uint32_t t){
    if ( (_bits[i]&B11110000) == B01000000){
        if ( (millis() - _ticksStarted[i]) >= t && !tickBit(i) ){
            tickBit(i, 1);
            _ticksStarted[i] += t;
            return true;
        }
        else if ( (millis() - _ticksStarted[i]) < t && tickBit(i) ){
            tickBit(i, 0);
        }
    }
    return false;
}
