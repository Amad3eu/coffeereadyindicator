#pragma once

#include <cstdint>

#include "core/CoffeeConfig.h"
#include "core/Event.h"
#include "core/Hysteresis.h"
#include "core/LightReading.h"

namespace coffee {

/// Turns raw sensor readings into a debounced light state plus sensor health.
///
/// It knows nothing about coffee: "on" simply means the reading crossed the
/// hysteresis thresholds (a green READY LED after calibration). Invalid
/// readings (sensor fault or out-of-range value) raise SensorFailure once and
/// leave the light state untouched until valid readings return.
class LightMonitor {
   public:
    explicit LightMonitor(const CoffeeConfig& config);

    /// Processes one sample. Returns SensorFailure / SensorRecovered on health
    /// transitions, EventType::None otherwise.
    EventType update(LightReading reading);

    bool isOn() const { return light_.isOn(); }
    bool isHealthy() const { return !sensorFailed_; }

   private:
    uint16_t sensorMaxValue_;
    Hysteresis light_;
    bool sensorFailed_ = false;
};

}  // namespace coffee
