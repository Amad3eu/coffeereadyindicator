#pragma once

#include <Arduino.h>

#include "hardware/Clock.h"

namespace coffee {

/// Clock backed by Arduino's millis().
class ArduinoClock : public Clock {
   public:
    uint32_t nowMs() const override { return millis(); }
};

}  // namespace coffee
