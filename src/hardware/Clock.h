#pragma once

#include <cstdint>

namespace coffee {

/// Monotonic millisecond clock. The firmware wraps `millis()`; tests use a
/// mock they can advance by hand.
class Clock {
   public:
    virtual ~Clock() = default;
    virtual uint32_t nowMs() const = 0;
};

}  // namespace coffee
