#pragma once

#include <cstdint>

#include "hardware/LightSensor.h"

namespace coffee {

/// Reads a phototransistor (or any light-dependent voltage) on an ADC pin.
///
/// Readings are the average of `samplesPerRead` conversions, which smooths
/// the ESP32's fairly noisy ADC. Use an ADC1 pin (GPIO32-39) so readings keep
/// working while Wi-Fi is on.
class Esp32AnalogLightSensor : public LightSensor {
   public:
    explicit Esp32AnalogLightSensor(uint8_t pin, uint8_t samplesPerRead = 8);

    /// Configures the ADC. Must be called from setup().
    void begin();

    LightReading read() override;

   private:
    uint8_t pin_;
    uint8_t samplesPerRead_;
    bool available_ = false;
};

}  // namespace coffee
