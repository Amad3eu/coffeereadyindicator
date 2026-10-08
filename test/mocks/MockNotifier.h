#pragma once

#include <algorithm>
#include <vector>

#include "hardware/Notifier.h"

namespace coffee {
namespace test {

class MockNotifier : public Notifier {
   public:
    bool notify(EventType event) override {
        events.push_back(event);
        return result;
    }

    size_t count(EventType event) const {
        return static_cast<size_t>(std::count(events.begin(), events.end(), event));
    }

    std::vector<EventType> events;
    bool result = true;
};

}  // namespace test
}  // namespace coffee
