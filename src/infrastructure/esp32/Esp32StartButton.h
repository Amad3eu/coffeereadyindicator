#pragma once

#include <Arduino.h>

#include <cstdint>

#include "hardware/StartButton.h"

namespace coffee {

/// Momentary push button on a GPIO. Wired between the pin and GND with the
/// internal pull-up (active low), which needs no external resistor.
class Esp32StartButton : public StartButton {
   public:
    explicit Esp32StartButton(uint8_t pin, bool activeLow = true) : pin_(pin), activeLow_(activeLow) {}

    void begin() { pinMode(pin_, activeLow_ ? INPUT_PULLUP : INPUT); }

    bool isPressed() override { return (digitalRead(pin_) == LOW) == activeLow_; }

   private:
    uint8_t pin_;
    bool activeLow_;
};

}  // namespace coffee
