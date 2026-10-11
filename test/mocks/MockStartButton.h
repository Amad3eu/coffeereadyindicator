#pragma once

#include "hardware/StartButton.h"

namespace coffee {
namespace test {

/// START button whose raw level is set by the test.
class MockStartButton : public StartButton {
   public:
    bool isPressed() override { return pressed; }

    bool pressed = false;
};

}  // namespace test
}  // namespace coffee
