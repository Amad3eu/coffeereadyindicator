#include "core/CoffeeDetector.h"

namespace coffee {

CoffeeDetector::CoffeeDetector(const CoffeeConfig& config)
    : config_(config), light_(config.lightOnThreshold, config.lightOffThreshold) {}

EventType CoffeeDetector::update(LightReading reading, uint32_t nowMs) {
    const bool valid = reading.valid && reading.value <= config_.sensorMaxValue;
    if (!valid) {
        return handleInvalidReading();
    }
    return handleValidReading(reading.value, nowMs);
}

void CoffeeDetector::markNotified(uint32_t nowMs) {
    if (state_ == CoffeeState::Ready) {
        transitionTo(CoffeeState::Notified, nowMs);
    }
}

EventType CoffeeDetector::handleInvalidReading() {
    // The state machine is frozen while the sensor is unhealthy: an invalid
    // sample carries no information about the light.
    if (sensorFailed_) {
        return EventType::None;
    }
    sensorFailed_ = true;
    return EventType::SensorFailure;
}

EventType CoffeeDetector::handleValidReading(uint16_t value, uint32_t nowMs) {
    light_.update(value);

    if (sensorFailed_) {
        // We lost sight of the light for a while, so "continuously on/off" can
        // no longer be claimed: restart the confirmation window of the current
        // state and resume the state machine on the next sample.
        sensorFailed_ = false;
        stateEnteredMs_ = nowMs;
        return EventType::SensorRecovered;
    }

    return runStateMachine(nowMs);
}

EventType CoffeeDetector::runStateMachine(uint32_t nowMs) {
    const bool lightOn = light_.isOn();
    const bool isFirstObservation = firstObservation_;
    firstObservation_ = false;

    switch (state_) {
        case CoffeeState::Idle:
            if (lightOn) {
                if (isFirstObservation && !config_.alertIfLightOnAtBoot) {
                    // The light was already on when we started watching: the
                    // coffee was ready before boot. Don't alert, wait for the
                    // light to go off.
                    transitionTo(CoffeeState::Notified, nowMs);
                } else {
                    transitionTo(CoffeeState::Detecting, nowMs);
                }
            }
            break;

        case CoffeeState::Detecting:
            if (!lightOn) {
                transitionTo(CoffeeState::Idle, nowMs);
            } else if (stateDurationReached(nowMs, config_.readyConfirmationMs)) {
                transitionTo(CoffeeState::Ready, nowMs);
                return EventType::CoffeeReady;
            }
            break;

        case CoffeeState::Ready:
            // Waiting for markNotified(); the light is deliberately ignored here.
            break;

        case CoffeeState::Notified:
            if (!lightOn) {
                transitionTo(CoffeeState::Rearming, nowMs);
            }
            break;

        case CoffeeState::Rearming:
            if (lightOn) {
                // The light came back before the re-arm window elapsed: still the
                // same coffee, keep waiting without alerting again.
                transitionTo(CoffeeState::Notified, nowMs);
            } else if (stateDurationReached(nowMs, config_.rearmConfirmationMs)) {
                transitionTo(CoffeeState::Idle, nowMs);
                return EventType::CoffeeReset;
            }
            break;
    }
    return EventType::None;
}

void CoffeeDetector::transitionTo(CoffeeState next, uint32_t nowMs) {
    state_ = next;
    stateEnteredMs_ = nowMs;
}

bool CoffeeDetector::stateDurationReached(uint32_t nowMs, uint32_t durationMs) const {
    // Unsigned subtraction keeps this correct across the uint32_t roll-over.
    return (nowMs - stateEnteredMs_) >= durationMs;
}

}  // namespace coffee
