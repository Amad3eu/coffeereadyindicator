#include "core/Calibration.h"

namespace coffee {

void SampleStats::add(uint16_t value) {
    if (count_ == 0) {
        min_ = value;
        max_ = value;
    } else {
        if (value < min_) {
            min_ = value;
        }
        if (value > max_) {
            max_ = value;
        }
    }
    sum_ += value;
    ++count_;
}

void SampleStats::reset() {
    count_ = 0;
    sum_ = 0;
    min_ = 0;
    max_ = 0;
}

uint16_t SampleStats::mean() const {
    if (count_ == 0) {
        return 0;
    }
    return static_cast<uint16_t>(sum_ / count_);
}

ThresholdSuggestion suggestThresholds(uint16_t ledOffValue, uint16_t ledOnValue) {
    ThresholdSuggestion suggestion;
    if (ledOnValue <= ledOffValue) {
        return suggestion;
    }
    const uint32_t span = static_cast<uint32_t>(ledOnValue) - ledOffValue;
    if (span < kMinimumCalibrationSpan) {
        return suggestion;
    }
    suggestion.lightOnThreshold = static_cast<uint16_t>(ledOffValue + (span * 6) / 10);
    suggestion.lightOffThreshold = static_cast<uint16_t>(ledOffValue + (span * 3) / 10);
    suggestion.valid = true;
    return suggestion;
}

}  // namespace coffee
