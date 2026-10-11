#include "application/Calibrator.h"

namespace coffee {

namespace {
constexpr const char* kTag = "CALIBRATION";
}

const char* toString(CalibrationPhase phase) {
    switch (phase) {
        case CalibrationPhase::Ambient:
            return "AMBIENT";
        case CalibrationPhase::LedOff:
            return "LED OFF";
        case CalibrationPhase::LedOn:
            return "LED ON";
        case CalibrationPhase::Done:
            return "DONE";
    }
    return "?";
}

Calibrator::Calibrator(LightSensor& sensor, const Clock& clock, Logger& logger,
                       const CalibrationConfig& config)
    : sensor_(sensor), clock_(clock), logger_(logger), config_(config) {}

void Calibrator::begin() {
    phase_ = CalibrationPhase::Ambient;
    sampling_ = false;
    result_ = CalibrationResult{};
    lastLiveLogMs_ = clock_.nowMs() - config_.liveLogIntervalMs;
    logger_.info(kTag, "Starting calibration (3 phases). Values are shown live; press ENTER when ready.");
    prompt();
}

void Calibrator::confirm() {
    if (isDone() || sampling_) {
        return;
    }
    const uint32_t now = clock_.nowMs();
    stats_.reset();
    sampling_ = true;
    windowStartedMs_ = now;
    lastSampleMs_ = now - config_.sampleIntervalMs;
    logger_.info(kTag, "Sampling %s for %lu ms...", toString(phase_),
                 static_cast<unsigned long>(config_.sampleWindowMs));
}

void Calibrator::update() {
    if (isDone()) {
        return;
    }
    const uint32_t now = clock_.nowMs();
    if (!sampling_) {
        logLiveValue(now);
        return;
    }
    if (now - lastSampleMs_ >= config_.sampleIntervalMs) {
        takeSample(now);
    }
    if (now - windowStartedMs_ >= config_.sampleWindowMs) {
        finishPhase();
    }
}

void Calibrator::prompt() {
    switch (phase_) {
        case CalibrationPhase::Ambient:
            logger_.info(kTag,
                         "Phase 1/3 AMBIENT: keep the sensor away from the machine LED, then press ENTER");
            break;
        case CalibrationPhase::LedOff:
            logger_.info(kTag,
                         "Phase 2/3 LED OFF: mount the sensor on the machine while it is NOT green (off or "
                         "red), then press ENTER");
            break;
        case CalibrationPhase::LedOn:
            logger_.info(kTag, "Phase 3/3 LED ON: wait for the ready LED to turn on, then press ENTER");
            break;
        case CalibrationPhase::Done:
            logger_.info(kTag, "Calibration complete; reset the board to run it again");
            break;
    }
}

void Calibrator::takeSample(uint32_t nowMs) {
    lastSampleMs_ = nowMs;
    const LightReading reading = sensor_.read();
    if (reading.valid) {
        stats_.add(reading.value);
    }
}

void Calibrator::finishPhase() {
    sampling_ = false;
    if (stats_.count() == 0) {
        logger_.warn(kTag, "No valid samples for %s; check the sensor wiring and press ENTER to retry",
                     toString(phase_));
        return;
    }
    const uint16_t mean = stats_.mean();
    logger_.info(kTag, "%s: %u (min=%u max=%u samples=%lu)", toString(phase_), static_cast<unsigned>(mean),
                 static_cast<unsigned>(stats_.minimum()), static_cast<unsigned>(stats_.maximum()),
                 static_cast<unsigned long>(stats_.count()));

    // Sampling can only be started in the three measuring phases, so the
    // final branch is necessarily LED ON.
    if (phase_ == CalibrationPhase::Ambient) {
        result_.ambient = mean;
        phase_ = CalibrationPhase::LedOff;
    } else if (phase_ == CalibrationPhase::LedOff) {
        result_.ledOff = mean;
        phase_ = CalibrationPhase::LedOn;
    } else {
        result_.ledOn = mean;
        result_.suggestion = suggestThresholds(result_.ledOff, result_.ledOn);
        phase_ = CalibrationPhase::Done;
        logSummary();
    }
    prompt();
}

void Calibrator::logLiveValue(uint32_t nowMs) {
    if (nowMs - lastLiveLogMs_ < config_.liveLogIntervalMs) {
        return;
    }
    lastLiveLogMs_ = nowMs;
    const LightReading reading = sensor_.read();
    if (reading.valid) {
        logger_.info(kTag, "live value=%u", static_cast<unsigned>(reading.value));
    } else {
        logger_.warn(kTag, "live value=invalid");
    }
}

void Calibrator::logSummary() {
    logger_.info(kTag, "Ambient: %u", static_cast<unsigned>(result_.ambient));
    logger_.info(kTag, "LED OFF: %u", static_cast<unsigned>(result_.ledOff));
    logger_.info(kTag, "LED ON: %u", static_cast<unsigned>(result_.ledOn));
    if (result_.suggestion.valid) {
        logger_.info(kTag, "Suggested thresholds: lightOnThreshold=%u lightOffThreshold=%u",
                     static_cast<unsigned>(result_.suggestion.lightOnThreshold),
                     static_cast<unsigned>(result_.suggestion.lightOffThreshold));
        logger_.info(kTag, "Copy them into AppConfig (src/main.cpp) or wait for NVS support (see roadmap)");
    } else {
        logger_.warn(kTag, "LED ON (%u) is too close to LED OFF (%u); reposition the sensor and run again",
                     static_cast<unsigned>(result_.ledOn), static_cast<unsigned>(result_.ledOff));
    }
}

}  // namespace coffee
