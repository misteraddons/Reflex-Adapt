
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

#ifndef Bugtton_h
#define Bugtton_h

#include "Arduino.h"

//No idea how to know if building for leonardo...
#if NUM_DIGITAL_PINS == 31 && NUM_ANALOG_INPUTS == 12
  #define BUGTTON_IS_ATMEGA_32U4
#endif

// Read .cpp for comments
class Bugtton {
  
    private:

        uint8_t _maskD;
        uint8_t _maskB;
        uint8_t _maskC;
        uint8_t _maskE;
        uint8_t _idleD;
        uint8_t _idleB;
        uint8_t _idleC;
        uint8_t _idleE;
        uint8_t _count;
        uint8_t _debounceTime;
        bool _allStable;
        bool _flag1;
        
        uint8_t *_pins;
        uint8_t *_bits;
        uint32_t *_stateStarted;
        uint32_t *_ticksStarted;
        uint32_t _allUpStarted;
        
        void makeMasks();
        void printBIN(uint8_t b);
        
        void currentBit(uint8_t i, bool a);
        bool currentBit(uint8_t i);
        void stableBit(uint8_t i, bool a);
        bool stableBit(uint8_t i);
        void oldBit(uint8_t i, bool a);
        bool oldBit(uint8_t i);
        void changedBit(uint8_t i, bool a);
        bool changedBit(uint8_t i);
        void heldUntilUsed(uint8_t i, bool a);
        bool heldUntilUsed(uint8_t i);
        void stateStarted(uint8_t i, uint32_t a);
        uint32_t stateStarted(uint8_t i);
        void tickBit(uint8_t i, bool a);
        bool tickBit(uint8_t i);
        void flippedBit(uint8_t i, bool a);
        bool flippedBit(uint8_t i);
        

    public:

        Bugtton(const uint8_t a, const uint8_t *b, uint8_t dt);
        void debounceTime(uint16_t a);        
        void update();
        void setMode(uint8_t i, uint8_t mode);
        uint32_t duration(uint8_t);
        bool fell(uint8_t i);
        bool rose(uint8_t i);
        bool up(uint8_t i);
        bool held(uint8_t i);
        bool heldUntil(uint8_t i, uint16_t t);
        bool upUntil(uint8_t i, uint16_t t);
        bool intervalTick(uint8_t i, uint32_t t);
        
};


#endif


