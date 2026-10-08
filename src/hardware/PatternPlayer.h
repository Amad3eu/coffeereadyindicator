#pragma once

#include <cstdint>

#include "hardware/DigitalOutput.h"
#include "hardware/Pattern.h"

namespace coffee {

/// Plays an on/off pattern on a DigitalOutput without blocking.
///
/// Call `update()` from the main loop. Each step lasts at least its duration;
/// if the loop stalls, the pattern is delayed rather than skipped.
class PatternPlayer {
   public:
    explicit PatternPlayer(DigitalOutput& output);

    void play(const Pattern& pattern, uint32_t nowMs);
    void stop();
    void update(uint32_t nowMs);

    bool isPlaying() const { return playing_; }

   private:
    void applyCurrentStep(uint32_t nowMs);

    DigitalOutput& output_;
    Pattern pattern_{nullptr, 0, false};
    uint8_t stepIndex_ = 0;
    uint32_t stepStartedMs_ = 0;
    bool playing_ = false;
};

}  // namespace coffee
