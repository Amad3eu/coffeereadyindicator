#pragma once

#include <cstdint>

#include "application/CoffeeSentinelApp.h"

#include "mocks/MockBuzzer.h"
#include "mocks/MockClock.h"
#include "mocks/MockLightSensor.h"
#include "mocks/MockLogger.h"
#include "mocks/MockNetworkLink.h"
#include "mocks/MockNotifier.h"
#include "mocks/MockStartButton.h"
#include "mocks/MockStatusLed.h"

namespace coffee {
namespace test {

/// A fully mocked CoffeeSentinelApp plus helpers to run it through time.
///
/// The constructor boots the app and lets it run briefly with the light off,
/// so every test starts from a settled IDLE device.
struct AppHarness {
    static constexpr uint32_t kLoopStepMs = 10;
    static constexpr uint32_t kSettleMs = 100;

    explicit AppHarness(const AppConfig& config = AppConfig{})
        : app(config, sensor, button, clock, buzzer, led, notifier, network, logger) {
        app.begin();
        sensor.setValue(0);
        runFor(kSettleMs);
    }

    /// Runs the main loop for `durationMs`, calling loop() every kLoopStepMs.
    void runFor(uint32_t durationMs) {
        for (uint32_t elapsed = 0; elapsed < durationMs; elapsed += kLoopStepMs) {
            app.loop();
            clock.advance(kLoopStepMs);
        }
    }

    /// Sets the sensor value and runs for `durationMs`.
    void lightAt(uint16_t value, uint32_t durationMs) {
        sensor.setValue(value);
        runFor(durationMs);
    }

    /// A clean press of the START button, longer than the debounce window.
    void pressStart() {
        const uint32_t hold = app.config().buttonDebounceMs * 3;
        button.pressed = true;
        runFor(hold);
        button.pressed = false;
        runFor(hold);
    }

    MockClock clock;
    MockLightSensor sensor;
    MockStartButton button;
    MockBuzzer buzzer;
    MockStatusLed led;
    MockNotifier notifier;
    MockNetworkLink network;
    MockLogger logger;
    CoffeeSentinelApp app;
};

}  // namespace test
}  // namespace coffee
