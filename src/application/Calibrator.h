#pragma once

#include <cstdint>

#include "core/Calibration.h"
#include "hardware/Clock.h"
#include "hardware/LightSensor.h"
#include "hardware/Logger.h"

namespace coffee {

struct CalibrationConfig {
    /// Length of the sampling window of each phase.
    uint32_t sampleWindowMs = 2000;
    /// Interval between samples inside a window.
    uint32_t sampleIntervalMs = 20;
    /// Interval between "live value" log lines while waiting for the user.
    uint32_t liveLogIntervalMs = 500;
};

enum class CalibrationPhase : uint8_t { Ambient, LedOff, LedOn, Done };

const char* toString(CalibrationPhase phase);

struct CalibrationResult {
    uint16_t ambient = 0;
    uint16_t ledOff = 0;
    uint16_t ledOn = 0;
    ThresholdSuggestion suggestion;
};

/// Interactive three-phase calibration:
///
///   1. ambient  - sensor away from the machine
///   2. LED off  - sensor mounted, ready LED off
///   3. LED on   - sensor mounted, ready LED on
///
/// Each phase waits for `confirm()` (a key press on the serial monitor), then
/// samples for `sampleWindowMs` and logs the mean. The final phase also logs
/// suggested thresholds. Fully non-blocking and hardware independent.
class Calibrator {
   public:
    Calibrator(LightSensor& sensor, const Clock& clock, Logger& logger,
               const CalibrationConfig& config = CalibrationConfig{});

    void begin();
    void confirm();
    void update();

    CalibrationPhase phase() const { return phase_; }
    bool isSampling() const { return sampling_; }
    bool isDone() const { return phase_ == CalibrationPhase::Done; }
    const CalibrationResult& result() const { return result_; }

   private:
    void prompt();
    void takeSample(uint32_t nowMs);
    void finishPhase();
    void logLiveValue(uint32_t nowMs);
    void logSummary();

    LightSensor& sensor_;
    const Clock& clock_;
    Logger& logger_;
    CalibrationConfig config_;

    CalibrationPhase phase_ = CalibrationPhase::Ambient;
    CalibrationResult result_;
    SampleStats stats_;
    bool sampling_ = false;
    uint32_t windowStartedMs_ = 0;
    uint32_t lastSampleMs_ = 0;
    uint32_t lastLiveLogMs_ = 0;
};

}  // namespace coffee
