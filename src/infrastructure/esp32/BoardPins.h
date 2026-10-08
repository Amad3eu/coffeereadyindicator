#pragma once

#include <cstdint>

// GPIO assignments for the ESP32 DevKit V1. See docs/hardware/wiring.md.
namespace coffee {
namespace pins {

/// Phototransistor output. GPIO34 is ADC1_CH6: input-only, no internal
/// pull-ups, and on ADC1, which keeps working while Wi-Fi is active (ADC2
/// does not).
constexpr uint8_t kLightSensor = 34;

/// Active buzzer (through an NPN driver if the buzzer draws more than ~12 mA).
constexpr uint8_t kBuzzer = 25;

/// On-board blue LED of the DevKit V1.
constexpr uint8_t kStatusLed = 2;

}  // namespace pins
}  // namespace coffee
