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
    /// A reading at or above this value switches the light to "on".
    uint16_t lightOnThreshold = 1800;

    /// Once the light is on, a reading at or below this value switches it back to "off".
    /// Readings between the two thresholds keep the previous state (hysteresis).
    uint16_t lightOffThreshold = 1200;

    /// How long the light must stay continuously on before the coffee is considered ready.
    uint32_t readyConfirmationMs = 3000;

    /// How long the light must stay continuously off before the detector re-arms.
    uint32_t rearmConfirmationMs = 3000;

    /// Largest value the sensor can legitimately produce. Anything above is treated
    /// as an invalid reading.
    uint16_t sensorMaxValue = 4095;

    /// What to do when the light is already on at the very first valid reading:
    ///   false -> assume the coffee was ready before we started watching; go straight
    ///            to NOTIFIED and wait for the light to go off (no alert).
    ///   true  -> treat it like any other detection and alert after confirmation.
    bool alertIfLightOnAtBoot = false;

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
