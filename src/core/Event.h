#pragma once

#include <cstdint>

namespace coffee {

/// Domain events. The detector produces the coffee/sensor events; the
/// application produces the connectivity events. Consumers (buzzer, notifier,
/// logger) only ever see this enum, so the detection logic never learns how
/// Alexa, HTTP or Wi-Fi work.
enum class EventType : uint8_t {
    None,             ///< Nothing happened.
    CoffeeReady,      ///< Light confirmed on: the coffee is ready.
    CoffeeReset,      ///< Light confirmed off: the detector re-armed for the next brew.
    SensorFailure,    ///< The sensor started producing invalid readings.
    SensorRecovered,  ///< The sensor produces valid readings again.
    WiFiConnected,    ///< Network link established.
    WiFiDisconnected  ///< Network link lost.
};

/// Identifier used in logs, e.g. "CoffeeReady".
const char* toString(EventType event);

/// Machine-friendly identifier used in payloads, e.g. "coffee_ready".
const char* toSlug(EventType event);

/// Human-friendly sentence used by notifiers, e.g. "Coffee is ready".
const char* toMessage(EventType event);

}  // namespace coffee
