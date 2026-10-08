#pragma once

#include "hardware/Buzzer.h"

#include "mocks/OutputRecorder.h"

namespace coffee {
namespace test {

class MockBuzzer : public Buzzer {
   public:
    void set(bool on) override { recorder.record(on); }

    bool isOn() const { return recorder.level; }
    uint32_t beeps() const { return recorder.risingEdges; }

    OutputRecorder recorder;
};

}  // namespace test
}  // namespace coffee
