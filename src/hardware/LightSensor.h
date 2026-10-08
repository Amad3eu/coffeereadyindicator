#pragma once

#include "core/LightReading.h"

namespace coffee {

/// Source of light readings. Higher values mean more light.
class LightSensor {
   public:
    virtual ~LightSensor() = default;
    virtual LightReading read() = 0;
};

}  // namespace coffee
