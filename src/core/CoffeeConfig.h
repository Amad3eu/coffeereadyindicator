#pragma once

#include <cstdint>

namespace coffee {

/// Detection parameters. Every tunable number of the detection logic lives here
/// so there are no magic numbers spread across the code base.
///
/// The defaults assume a 12-bit ADC (0..4095) and a TEPT4400 phototransistor
/// with a 10 kΩ load resistor. Run the calibration mode to obtain values for
/// your own machine (see docs/calibration.md).
struct CoffeeConfig {
    /// A reading at or above this value switches the light to "on" (green).
    uint16_t lightOnThreshold = 1800;

    /// Once the light is on, a reading at or below this value switches it back to "off".
    /// Readings between the two thresholds keep the previous state (hysteresis).
    uint16_t lightOffThreshold = 1200;

    /// After START, the light must stay continuously off for this long before the
    /// brew session is armed. This is what makes a START pressed while the light
    /// is already green harmless.
    uint32_t cycleStartConfirmationMs = 3000;

    /// Once armed, the light must stay continuously on for this long before the
    /// coffee is considered ready. Raise it above the longest transient green
    /// phase of your machine.
    uint32_t readyConfirmationMs = 3000;

    /// Largest value the sensor can legitimately produce. Anything above is treated
    /// as an invalid reading.
    uint16_t sensorMaxValue = 4095;

    enum class Error : uint8_t {
        None,
        ThresholdsInverted,       ///< lightOffThreshold >= lightOnThreshold
        ThresholdAboveSensorMax,  ///< lightOnThreshold > sensorMaxValue
        ZeroConfirmation,         ///< a confirmation window is 0 ms
    };

    Error validate() const;
    bool isValid() const { return validate() == Error::None; }
};

const char* toString(CoffeeConfig::Error error);

}  // namespace coffee
