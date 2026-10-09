#include "core/BrewSession.h"

namespace coffee {

BrewSession::BrewSession(const CoffeeConfig& config) : config_(config) {}

bool BrewSession::HoldTimer::update(bool condition, uint32_t nowMs, uint32_t durationMs) {
    if (!condition) {
        holding_ = false;
        return false;
    }
    if (!holding_) {
        holding_ = true;
        sinceMs_ = nowMs;
        return false;
    }
    // Unsigned subtraction keeps this correct across the uint32_t roll-over.
    return (nowMs - sinceMs_) >= durationMs;
}

EventType BrewSession::start(uint32_t nowMs) {
    if (isActive()) {
        return EventType::StartIgnored;
    }
    transitionTo(BrewState::Starting, nowMs);
    return EventType::BrewStarted;
}

EventType BrewSession::update(bool lightOn, bool sensorHealthy, uint32_t nowMs) {
    if (!sensorHealthy) {
        // No information about the light: hold the session where it is.
        frozen_ = true;
        return EventType::None;
    }
    if (frozen_) {
        // Observation resumed after a gap: whatever was being confirmed has to
        // be confirmed again from scratch.
        frozen_ = false;
        hold_.reset();
        return EventType::None;
    }

    switch (state_) {
        case BrewState::Starting:
            if (hold_.update(!lightOn, nowMs, config_.cycleStartConfirmationMs)) {
                transitionTo(BrewState::Brewing, nowMs);
                return EventType::BrewArmed;
            }
            break;

        case BrewState::Brewing:
            if (hold_.update(lightOn, nowMs, config_.readyConfirmationMs)) {
                transitionTo(BrewState::Ready, nowMs);
                return EventType::CoffeeReady;
            }
            break;

        case BrewState::Idle:
        case BrewState::Ready:
        case BrewState::Completed:
            // The light carries no meaning outside an armed session.
            break;
    }
    return EventType::None;
}

EventType BrewSession::complete(uint32_t nowMs) {
    if (state_ != BrewState::Ready) {
        return EventType::None;
    }
    transitionTo(BrewState::Completed, nowMs);
    return EventType::BrewCompleted;
}

bool BrewSession::isActive() const {
    return state_ == BrewState::Starting || state_ == BrewState::Brewing || state_ == BrewState::Ready;
}

void BrewSession::transitionTo(BrewState next, uint32_t nowMs) {
    state_ = next;
    stateEnteredMs_ = nowMs;
    hold_.reset();
}

}  // namespace coffee
