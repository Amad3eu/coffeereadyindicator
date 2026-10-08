#include "core/Hysteresis.h"

namespace coffee {

Hysteresis::Hysteresis(uint16_t onThreshold, uint16_t offThreshold)
    : onThreshold_(onThreshold), offThreshold_(offThreshold) {}

bool Hysteresis::update(uint16_t value) {
    if (!on_ && value >= onThreshold_) {
        on_ = true;
    } else if (on_ && value <= offThreshold_) {
        on_ = false;
    }
    return on_;
}

}  // namespace coffee
