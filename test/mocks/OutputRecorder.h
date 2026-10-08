#pragma once

#include <cstdint>
#include <vector>

namespace coffee {
namespace test {

/// Records every level written to a DigitalOutput.
struct OutputRecorder {
    void record(bool on) {
        if (on && !level) {
            ++risingEdges;
        }
        level = on;
        history.push_back(on);
    }

    bool level = false;
    uint32_t risingEdges = 0;  ///< off -> on transitions (number of beeps / blinks)
    std::vector<bool> history;
};

}  // namespace test
}  // namespace coffee
