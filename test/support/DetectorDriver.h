#pragma once

#include <cstdint>
#include <vector>

#include "core/CoffeeDetector.h"

#include "mocks/MockClock.h"

namespace coffee {
namespace test {

/// Drives a CoffeeDetector through time, feeding one reading per sample
/// interval and collecting every event it produces.
class DetectorDriver {
   public:
    static constexpr uint32_t kSampleIntervalMs = 50;

    explicit DetectorDriver(const CoffeeConfig& config, uint32_t startMs = 10'000)
        : clock(startMs), detector(config) {}

    /// Feeds `reading` for `durationMs`, one sample every kSampleIntervalMs.
    void feed(LightReading reading, uint32_t durationMs) {
        for (uint32_t elapsed = 0; elapsed < durationMs; elapsed += kSampleIntervalMs) {
            step(reading);
        }
    }

    void feedValue(uint16_t value, uint32_t durationMs) { feed(LightReading::of(value), durationMs); }
    void feedInvalid(uint32_t durationMs) { feed(LightReading::invalid(), durationMs); }

    /// Feeds one sample and advances the clock.
    EventType step(LightReading reading) {
        const EventType event = detector.update(reading, clock.nowMs());
        if (event != EventType::None) {
            events.push_back(event);
        }
        clock.advance(kSampleIntervalMs);
        return event;
    }

    void markNotified() { detector.markNotified(clock.nowMs()); }

    size_t count(EventType event) const {
        size_t n = 0;
        for (EventType e : events) {
            if (e == event) {
                ++n;
            }
        }
        return n;
    }

    MockClock clock;
    CoffeeDetector detector;
    std::vector<EventType> events;
};

}  // namespace test
}  // namespace coffee
