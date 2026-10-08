#pragma once

#include <cstdint>

namespace coffee {

/// Running min / max / mean over a stream of sensor samples.
class SampleStats {
   public:
    void add(uint16_t value);
    void reset();

    uint32_t count() const { return count_; }
    uint16_t minimum() const { return min_; }
    uint16_t maximum() const { return max_; }
    uint16_t mean() const;

   private:
    uint32_t count_ = 0;
    uint32_t sum_ = 0;
    uint16_t min_ = 0;
    uint16_t max_ = 0;
};

/// Thresholds derived from a calibration run.
struct ThresholdSuggestion {
    uint16_t lightOnThreshold = 0;
    uint16_t lightOffThreshold = 0;
    bool valid = false;
};

/// Minimum difference between the LED-off and LED-on readings for the
/// suggestion to be considered trustworthy.
constexpr uint16_t kMinimumCalibrationSpan = 100;

/// Places the "on" threshold at 60 % and the "off" threshold at 30 % of the
/// distance between the LED-off and LED-on readings. The wide band between
/// them is what gives the hysteresis its noise immunity.
ThresholdSuggestion suggestThresholds(uint16_t ledOffValue, uint16_t ledOnValue);

}  // namespace coffee
