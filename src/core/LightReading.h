#pragma once

#include <cstdint>

namespace coffee {

/// A single sample from the light sensor.
///
/// `valid == false` means the sensor could not produce a trustworthy value
/// (missing hardware, pin not wired to an ADC channel, driver error). The
/// detector treats invalid readings as "no information" and never lets them
/// move the state machine.
struct LightReading {
    uint16_t value = 0;
    bool valid = false;

    static constexpr LightReading of(uint16_t value) { return LightReading{value, true}; }
    static constexpr LightReading invalid() { return LightReading{}; }
};

}  // namespace coffee
