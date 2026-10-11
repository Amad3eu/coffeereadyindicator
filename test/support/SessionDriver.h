#pragma once

#include <cstdint>
#include <vector>

#include "core/BrewSession.h"
#include "core/LightMonitor.h"

#include "mocks/MockClock.h"

namespace coffee {
namespace test {

/// Drives a LightMonitor + BrewSession pair through time, feeding one reading
/// per sample interval and collecting every event they produce.
class SessionDriver {
   public:
    static constexpr uint32_t kSampleIntervalMs = 50;

    explicit SessionDriver(const CoffeeConfig& config, uint32_t startMs = 10'000)
        : clock(startMs), monitor(config), session(config) {}

    /// Feeds `reading` for `durationMs`, one sample every kSampleIntervalMs.
    void feed(LightReading reading, uint32_t durationMs) {
        for (uint32_t elapsed = 0; elapsed < durationMs; elapsed += kSampleIntervalMs) {
            step(reading);
        }
    }

    void feedValue(uint16_t value, uint32_t durationMs) { feed(LightReading::of(value), durationMs); }
    void feedInvalid(uint32_t durationMs) { feed(LightReading::invalid(), durationMs); }

    /// Feeds one sample and advances the clock. Returns the session event.
    EventType step(LightReading reading) {
        record(monitor.update(reading));
        const EventType event = session.update(monitor.isOn(), monitor.isHealthy(), clock.nowMs());
        record(event);
        clock.advance(kSampleIntervalMs);
        return event;
    }

    EventType pressStart() { return record(session.start(clock.nowMs())); }
    EventType complete() { return record(session.complete(clock.nowMs())); }

    BrewState state() const { return session.state(); }

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
    LightMonitor monitor;
    BrewSession session;
    std::vector<EventType> events;

   private:
    EventType record(EventType event) {
        if (event != EventType::None) {
            events.push_back(event);
        }
        return event;
    }
};

}  // namespace test
}  // namespace coffee
