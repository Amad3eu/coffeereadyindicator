#pragma once

#include <cstdint>

#include "hardware/LightSensor.h"

namespace coffee {
namespace test {

/// Sensor whose next reading is set by the test.
class MockLightSensor : public LightSensor {
   public:
    LightReading read() override {
        ++reads;
        return reading_;
    }

    void setValue(uint16_t value) { reading_ = LightReading::of(value); }
    void setInvalid() { reading_ = LightReading::invalid(); }
    void setReading(LightReading reading) { reading_ = reading; }

    uint32_t reads = 0;

   private:
    LightReading reading_ = LightReading::of(0);
};

}  // namespace test
}  // namespace coffee
