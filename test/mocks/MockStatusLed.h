#pragma once

#include "hardware/StatusLed.h"

#include "mocks/OutputRecorder.h"

namespace coffee {
namespace test {

class MockStatusLed : public StatusLed {
   public:
    void set(bool on) override { recorder.record(on); }

    bool isOn() const { return recorder.level; }
    uint32_t blinks() const { return recorder.risingEdges; }

    OutputRecorder recorder;
};

}  // namespace test
}  // namespace coffee
