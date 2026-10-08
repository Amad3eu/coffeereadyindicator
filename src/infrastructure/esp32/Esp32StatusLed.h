#pragma once

#include "hardware/StatusLed.h"
#include "infrastructure/esp32/GpioOutput.h"

namespace coffee {

/// Status LED on a GPIO (on-board LED or an external one with a series resistor).
class Esp32StatusLed : public StatusLed {
   public:
    explicit Esp32StatusLed(uint8_t pin, bool activeHigh = true) : gpio_(pin, activeHigh) {}

    void begin() { gpio_.begin(); }
    void set(bool on) override { gpio_.write(on); }

   private:
    GpioOutput gpio_;
};

}  // namespace coffee
