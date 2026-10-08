#include "hardware/PatternPlayer.h"

namespace coffee {

PatternPlayer::PatternPlayer(DigitalOutput& output) : output_(output) {}

void PatternPlayer::play(const Pattern& pattern, uint32_t nowMs) {
    if (pattern.steps == nullptr || pattern.length == 0) {
        stop();
        return;
    }
    pattern_ = pattern;
    stepIndex_ = 0;
    playing_ = true;
    applyCurrentStep(nowMs);
}

void PatternPlayer::stop() {
    playing_ = false;
    output_.off();
}

void PatternPlayer::update(uint32_t nowMs) {
    if (!playing_) {
        return;
    }
    const PatternStep& step = pattern_.steps[stepIndex_];
    if (nowMs - stepStartedMs_ < step.durationMs) {
        return;
    }
    const uint8_t next = static_cast<uint8_t>(stepIndex_ + 1);
    if (next < pattern_.length) {
        stepIndex_ = next;
        applyCurrentStep(nowMs);
    } else if (pattern_.loop) {
        stepIndex_ = 0;
        applyCurrentStep(nowMs);
    } else {
        stop();
    }
}

void PatternPlayer::applyCurrentStep(uint32_t nowMs) {
    stepStartedMs_ = nowMs;
    output_.set(pattern_.steps[stepIndex_].on);
}

}  // namespace coffee
