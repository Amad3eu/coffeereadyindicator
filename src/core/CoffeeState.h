#pragma once

#include <cstdint>

namespace coffee {

/// States of the coffee detection lifecycle.
///
///   IDLE -> DETECTING -> READY -> NOTIFIED -> REARMING -> IDLE
///
/// See docs/diagrams/state-machine.mmd for the full transition table.
enum class CoffeeState : uint8_t {
    Idle,       ///< Light off; waiting for it to turn on.
    Detecting,  ///< Light on; waiting for it to stay on for readyConfirmationMs.
    Ready,      ///< Coffee confirmed; local alert in progress.
    Notified,   ///< Alert delivered; waiting for the light to turn off.
    Rearming,   ///< Light off; waiting for it to stay off for rearmConfirmationMs.
};

const char* toString(CoffeeState state);

}  // namespace coffee
