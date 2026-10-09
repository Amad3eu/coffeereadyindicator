#pragma once

#include <cstdint>

#include "core/CoffeeConfig.h"

namespace coffee {

/// Application-level settings: detection parameters plus the scheduling knobs
/// of the main loop.
struct AppConfig {
    CoffeeConfig detection;

    /// How often the sensor is sampled and fed to the brew session.
    uint32_t sampleIntervalMs = 50;

    /// How often the raw sensor value is logged at DEBUG level.
    uint32_t sensorLogIntervalMs = 5000;

    /// Settling time of the START button contacts.
    uint32_t buttonDebounceMs = 50;
};

}  // namespace coffee
