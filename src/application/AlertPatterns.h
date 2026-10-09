#pragma once

#include "hardware/Pattern.h"

namespace coffee {
namespace patterns {

// --- Buzzer -----------------------------------------------------------------

/// One short blip: START accepted, a brew session began.
constexpr PatternStep kBrewStartedSteps[] = {
    {true, 80},
};
constexpr Pattern kBrewStarted = makePattern(kBrewStartedSteps);

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

/// Fast blink while waiting for the machine to leave its green state (STARTING).
constexpr PatternStep kStartingBlinkSteps[] = {
    {true, 100},
    {false, 150},
};
constexpr Pattern kStartingBlink = makePattern(kStartingBlinkSteps, true);

/// Slow heartbeat while armed and watching for the ready light (BREWING).
constexpr PatternStep kBrewingBlinkSteps[] = {
    {true, 60},
    {false, 940},
};
constexpr Pattern kBrewingBlink = makePattern(kBrewingBlinkSteps, true);

/// Solid on while the coffee is ready (READY, COMPLETED).
constexpr PatternStep kSolidOnSteps[] = {
    {true, 1000},
};
constexpr Pattern kSolidOn = makePattern(kSolidOnSteps, true);

}  // namespace patterns
}  // namespace coffee
