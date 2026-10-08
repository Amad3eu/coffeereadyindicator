#pragma once

#include <cstdint>

#include "core/CoffeeConfig.h"
#include "core/CoffeeState.h"
#include "core/Event.h"
#include "core/Hysteresis.h"
#include "core/LightReading.h"

namespace coffee {

/// Coffee detection state machine.
///
/// Feed it sensor readings together with the current time and it tells you
/// what happened. It has no notion of hardware, logging or networking: it is
/// plain logic, which is what makes it testable on a desktop.
///
/// Timing uses `uint32_t` milliseconds and unsigned subtraction, so it keeps
/// working across the 49.7-day `millis()` roll-over.
class CoffeeDetector {
   public:
    explicit CoffeeDetector(const CoffeeConfig& config);

    /// Processes one sensor sample. Returns the event produced by this sample,
    /// or `EventType::None`. At most one event is produced per call.
    EventType update(LightReading reading, uint32_t nowMs);

    /// Tells the detector the alert for the current coffee has been delivered.
    /// Moves READY -> NOTIFIED; a no-op in every other state.
    void markNotified(uint32_t nowMs);

    CoffeeState state() const { return state_; }
    bool isLightOn() const { return light_.isOn(); }
    bool isSensorHealthy() const { return !sensorFailed_; }
    const CoffeeConfig& config() const { return config_; }

    /// Milliseconds spent in the current state.
    uint32_t timeInStateMs(uint32_t nowMs) const { return nowMs - stateEnteredMs_; }

   private:
    EventType handleInvalidReading();
    EventType handleValidReading(uint16_t value, uint32_t nowMs);
    EventType runStateMachine(uint32_t nowMs);
    void transitionTo(CoffeeState next, uint32_t nowMs);
    bool stateDurationReached(uint32_t nowMs, uint32_t durationMs) const;

    CoffeeConfig config_;
    Hysteresis light_;
    CoffeeState state_ = CoffeeState::Idle;
    uint32_t stateEnteredMs_ = 0;
    bool sensorFailed_ = false;
    bool firstObservation_ = true;
};

}  // namespace coffee
