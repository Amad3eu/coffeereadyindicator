#pragma once

#include <Arduino.h>

#include <cstdint>

namespace coffee {

/// A GPIO driven as a digital output. Shared by the buzzer and LED adapters.
class GpioOutput {
   public:
    explicit GpioOutput(uint8_t pin, bool activeHigh = true) : pin_(pin), activeHigh_(activeHigh) {}

    void begin() {
        pinMode(pin_, OUTPUT);
        write(false);
    }

    void write(bool on) { digitalWrite(pin_, (on == activeHigh_) ? HIGH : LOW); }

   private:
    uint8_t pin_;
    bool activeHigh_;
};

}  // namespace coffee
