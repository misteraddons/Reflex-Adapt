/* ZordButton by sonik-br
 *  
 * Button class:
 * MUST call begin() one time.
 * Debounces a button state.
 * 
 * ArcadePad class:
 * MUST call begin() one time.
 * Can instantiate and handle multiple Button instances.
 * 
 * Arduino's digitalRead() seems to be fast enough for button reading.
 * It's also possible to modify the code and use direct port access.
 * Tested with DigitalIO and FastGPIO libs.
 * 
 * Credits:
 * Debounce and held/timertick from
 * https://github.com/sakabug/Bugtton
 * 
*/

#ifndef ZORDBUTTON_H
#define ZORDBUTTON_H

class Button {
  private:
    const uint8_t pin_n;

    bool finalState;

    uint32_t updateMillis;

    //debounce
    uint8_t debounceTime;
    bool currentState;
    bool debouncedState;
    bool stateChanged;
    uint32_t buttonMillis;
    
    //stateDuration()
    uint32_t debouncedAt;

    void configurePin() const { pinMode(pin_n, INPUT_PULLUP); }
    bool getPinState() const { return digitalRead(pin_n); }

    bool debounce() {
      if((debouncedState != currentState) && (updateMillis - buttonMillis) > debounceTime) {
        debouncedState = currentState;
        debouncedAt = updateMillis;
        buttonMillis = updateMillis;
        return true;
      }
      return false;
    }

  public:
    Button(const uint8_t pin) : pin_n(pin) { };

    void begin(const uint8_t time) {
      configurePin(); //configure pin as INPUT_PULLUP

      updateMillis = millis();
      
      finalState = HIGH;

      //debounce
      stateChanged = false;
      currentState = HIGH;
      debouncedState = HIGH;
      
      debounceTime = time;

      buttonMillis = updateMillis;

      //stateDuration()
      debouncedAt = updateMillis;
    }

    bool update() {
      stateChanged = false;

      updateMillis =  millis();

      currentState = getPinState();
      
      if(debounce() && finalState != currentState) {
        stateChanged = true;
        finalState = currentState;
        debouncedAt = updateMillis;
      }
      return stateChanged;
    }

    bool state() const { return finalState; }
    bool fell() const { return stateChanged && !finalState; }
    bool rose() const { return stateChanged && finalState; }

    uint32_t stateDuration() const { return millis() - debouncedAt; }
}; //end class Buton


class ArcadePad {
  private:
    Button** _pins;
    const uint8_t _count;
    const uint8_t _interval;

  public:
    ArcadePad(const uint8_t count, const uint8_t* buttons, const uint8_t interval) :
        _pins(new Button*[count]),
        _count(count),
        _interval(interval)
      {

      for(uint8_t i = 0; i < _count; ++i)
        _pins[i] = new Button(buttons[i]);
    }

    void begin() const {
      for(uint8_t i = 0; i < _count; ++i)
        _pins[i]->begin(_interval);
    }

    bool update() {
      bool stateChanged = false;
      //const bool turboTick = isTurboTick();
      for(uint8_t i = 0; i < _count; ++i) {
        if(_pins[i]->update())
          stateChanged = true;
      }
      return stateChanged;
    }

    bool state(const uint8_t i) const { return _pins[i]->state(); }
    bool fell(const uint8_t i) const { return _pins[i]->fell(); }
    bool rose(const uint8_t i) const { return _pins[i]->rose(); }

}; //end class ArcadePad
#endif

    //direct pin access using DigitalIO
    //#include <DigitalIO.h>
    //DigitalPin<tpin> pin;
    //virtual void configurePin() override { pin.config (INPUT, HIGH); }
    //virtual bool getPinState() override { return pin == HIGH; }

    //direct pin access using https://github.com/pololu/fastgpio-arduino
    //#include <FastGPIO.h>
    //FastGPIO::Pin<tpin> pin;
    //virtual void configurePin() override { pin.setInputPulledUp(); }
    //virtual bool getPinState() override { return pin.isInputHigh(); }

  //protected:
  //  virtual void configurePin() = 0;
  //  virtual bool getPinState() = 0;
