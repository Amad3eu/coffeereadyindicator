#pragma once

#include <cstdint>

#include "core/BrewState.h"
#include "core/CoffeeConfig.h"
#include "core/Event.h"

namespace coffee {

/// The brew session state machine: the rule that decides when a green light
/// actually means "coffee ready".
///
/// A session only exists between a START press and the delivery of its single
/// alert. The light state (from LightMonitor) is interpreted in exactly two
/// states:
///
///   STARTING  the light must stay OFF for cycleStartConfirmationMs. A START
///             pressed while the machine is still showing green therefore
///             never alerts: the session waits for the machine to leave that
///             state first.
///   BREWING   the light must stay ON for readyConfirmationMs, then
///             CoffeeReady fires once.
///
/// Everywhere else the light is ignored. Invalid sensor readings freeze the
/// session; when valid readings return, the confirmation window in progress
/// restarts, because "continuously on/off" cannot be claimed across a gap.
///
/// Timing uses `uint32_t` milliseconds and unsigned subtraction, so it keeps
/// working across the 49.7-day `millis()` roll-over.
class BrewSession {
   public:
    explicit BrewSession(const CoffeeConfig& config);

    /// START pressed. Returns BrewStarted (IDLE or COMPLETED) or StartIgnored
    /// while a session is active.
    EventType start(uint32_t nowMs);

    /// Processes one light sample. Returns BrewArmed, CoffeeReady or None.
    EventType update(bool lightOn, bool sensorHealthy, uint32_t nowMs);

    /// The alert for this session has been delivered. READY -> COMPLETED and
    /// BrewCompleted; None in every other state.
    EventType complete(uint32_t nowMs);

    BrewState state() const { return state_; }
    bool isActive() const;

    /// Milliseconds spent in the current state.
    uint32_t timeInStateMs(uint32_t nowMs) const { return nowMs - stateEnteredMs_; }

   private:
    /// Measures how long a condition has held continuously.
    class HoldTimer {
       public:
        void reset() { holding_ = false; }
        /// Feeds the condition; true once it has held for `durationMs`.
        bool update(bool condition, uint32_t nowMs, uint32_t durationMs);

       private:
        bool holding_ = false;
        uint32_t sinceMs_ = 0;
    };

    void transitionTo(BrewState next, uint32_t nowMs);

    CoffeeConfig config_;
    BrewState state_ = BrewState::Idle;
    uint32_t stateEnteredMs_ = 0;
    HoldTimer hold_;
    bool frozen_ = false;
};

}  // namespace coffee
