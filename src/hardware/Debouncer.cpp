#include "hardware/Debouncer.h"

namespace coffee {

Debouncer::Debouncer(uint32_t debounceMs) : debounceMs_(debounceMs) {}

void Debouncer::update(bool rawPressed, uint32_t nowMs) {
    if (!initialised_) {
        initialised_ = true;
        lastRaw_ = rawPressed;
        stable_ = rawPressed;
        lastChangeMs_ = nowMs;
        return;
    }
    if (rawPressed != lastRaw_) {
        // The level moved; restart the settling window.
        lastRaw_ = rawPressed;
        lastChangeMs_ = nowMs;
        return;
    }
    if (rawPressed != stable_ && (nowMs - lastChangeMs_) >= debounceMs_) {
        stable_ = rawPressed;
        if (stable_) {
            pressPending_ = true;
        }
    }
}

bool Debouncer::consumePress() {
    const bool pressed = pressPending_;
    pressPending_ = false;
    return pressed;
}

}  // namespace coffee
