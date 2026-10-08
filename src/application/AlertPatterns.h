#pragma once

#include "hardware/Pattern.h"

namespace coffee {
namespace patterns {

// --- Buzzer -----------------------------------------------------------------

/// BEEP, pause, BEEP: the coffee is ready.
constexpr PatternStep kCoffeeReadySteps[] = {
    {true, 150},
    {false, 120},
    {true, 150},
};
constexpr Pattern kCoffeeReady = makePattern(kCoffeeReadySteps);

/// Three short chirps: the light sensor stopped producing valid readings.
constexpr PatternStep kSensorFailureSteps[] = {
    {true, 60}, {false, 60}, {true, 60}, {false, 60}, {true, 60},
};
constexpr Pattern kSensorFailure = makePattern(kSensorFailureSteps);

// --- Status LED -------------------------------------------------------------

/// Fast blink while the light is being confirmed (DETECTING).
constexpr PatternStep kDetectingBlinkSteps[] = {
    {true, 100},
    {false, 150},
};
constexpr Pattern kDetectingBlink = makePattern(kDetectingBlinkSteps, true);

/// Solid on while the coffee is ready (READY, NOTIFIED).
constexpr PatternStep kSolidOnSteps[] = {
    {true, 1000},
};
constexpr Pattern kSolidOn = makePattern(kSolidOnSteps, true);

/// Slow heartbeat while waiting for the machine to settle (REARMING).
constexpr PatternStep kRearmingBlinkSteps[] = {
    {true, 60},
    {false, 940},
};
constexpr Pattern kRearmingBlink = makePattern(kRearmingBlinkSteps, true);

}  // namespace patterns
}  // namespace coffee
