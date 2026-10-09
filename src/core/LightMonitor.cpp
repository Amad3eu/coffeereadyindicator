#include "core/LightMonitor.h"

namespace coffee {

LightMonitor::LightMonitor(const CoffeeConfig& config)
    : sensorMaxValue_(config.sensorMaxValue), light_(config.lightOnThreshold, config.lightOffThreshold) {}

EventType LightMonitor::update(LightReading reading) {
    const bool valid = reading.valid && reading.value <= sensorMaxValue_;
    if (!valid) {
        if (sensorFailed_) {
            return EventType::None;
        }
        sensorFailed_ = true;
        return EventType::SensorFailure;
    }

    light_.update(reading.value);
    if (sensorFailed_) {
        sensorFailed_ = false;
        return EventType::SensorRecovered;
    }
    return EventType::None;
}

}  // namespace coffee
