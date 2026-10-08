#pragma once

#include <cstdint>

#include "hardware/Clock.h"

namespace coffee {
namespace test {

/// Clock the tests advance by hand. Starts at an arbitrary non-zero value so
/// code that accidentally assumes "boot == 0 ms" is caught.
class MockClock : public Clock {
   public:
    explicit MockClock(uint32_t startMs = 10'000) : now_(startMs) {}

    uint32_t nowMs() const override { return now_; }
    void advance(uint32_t ms) { now_ += ms; }
    void set(uint32_t ms) { now_ = ms; }

   private:
    uint32_t now_;
};

}  // namespace test
}  // namespace coffee
