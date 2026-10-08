#pragma once

#include <cstddef>
#include <cstdint>

namespace coffee {

/// One step of an on/off pattern.
struct PatternStep {
    bool on;
    uint16_t durationMs;
};

/// A sequence of steps played by a PatternPlayer. Points at static data, so
/// patterns are cheap to copy and can be `constexpr`.
struct Pattern {
    const PatternStep* steps;
    uint8_t length;
    bool loop;
};

template <size_t N>
constexpr Pattern makePattern(const PatternStep (&steps)[N], bool loop = false) {
    return Pattern{steps, static_cast<uint8_t>(N), loop};
}

}  // namespace coffee
