#pragma once

#include <cstdint>

namespace coffee {

/// Two-threshold comparator (a software Schmitt trigger).
///
/// The output turns on when the input reaches `onThreshold` and only turns off
/// again once the input drops to `offThreshold`. Values in between keep the
/// previous output, which stops a signal hovering around a single threshold
/// from flickering.
class Hysteresis {
   public:
    Hysteresis(uint16_t onThreshold, uint16_t offThreshold);

    /// Feeds a new input value and returns the resulting output.
    bool update(uint16_t value);

    bool isOn() const { return on_; }
    void reset() { on_ = false; }

    uint16_t onThreshold() const { return onThreshold_; }
    uint16_t offThreshold() const { return offThreshold_; }

   private:
    uint16_t onThreshold_;
    uint16_t offThreshold_;
    bool on_ = false;
};

}  // namespace coffee
