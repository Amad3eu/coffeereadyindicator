#pragma once

#include <cstdint>

namespace coffee {

/// Non-blocking debouncer with press-edge detection.
///
/// Feed the raw level on every loop iteration. The debounced level only
/// changes once the raw level has been stable for `debounceMs`, and a press
/// is reported exactly once per debounced inactive -> active transition.
/// A button already held when the first sample arrives does not count as a
/// press.
class Debouncer {
   public:
    explicit Debouncer(uint32_t debounceMs);

    void update(bool rawPressed, uint32_t nowMs);

    /// True once for every debounced press since the last call.
    bool consumePress();

    bool isPressed() const { return stable_; }

   private:
    uint32_t debounceMs_;
    bool initialised_ = false;
    bool lastRaw_ = false;
    uint32_t lastChangeMs_ = 0;
    bool stable_ = false;
    bool pressPending_ = false;
};

}  // namespace coffee
