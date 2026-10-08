#pragma once

#include "hardware/DigitalOutput.h"

namespace coffee {

/// Audible alert output. Distinct type so a buzzer can never be passed where a
/// status LED is expected (and vice versa).
class Buzzer : public DigitalOutput {};

}  // namespace coffee
