#pragma once

#include "hardware/Buzzer.h"
#include "infrastructure/esp32/GpioOutput.h"

namespace coffee {

/// Active buzzer on a GPIO (it makes sound whenever the pin is high).
class Esp32Buzzer : public Buzzer {
   public:
    explicit Esp32Buzzer(uint8_t pin, bool activeHigh = true) : gpio_(pin, activeHigh) {}

    void begin() { gpio_.begin(); }
    void set(bool on) override { gpio_.write(on); }

   private:
    GpioOutput gpio_;
};

}  // namespace coffee
