#pragma once

#include "hardware/DigitalOutput.h"

namespace coffee {

/// Visual status output (on-board or external LED).
class StatusLed : public DigitalOutput {};

}  // namespace coffee
