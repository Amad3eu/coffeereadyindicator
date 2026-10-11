#pragma once

#include <cstdint>

namespace coffee {

/// States of a brew session.
///
///   IDLE --START--> STARTING --light off for cycleStartConfirmationMs--> BREWING
///   BREWING --light on for readyConfirmationMs--> READY --alert delivered--> COMPLETED
///   COMPLETED --START--> STARTING
///
/// The light is only interpreted in STARTING and BREWING. See
/// docs/diagrams/state-machine.mmd for the full transition table.
enum class BrewState : uint8_t {
    Idle,       ///< No session. Any light is ignored until START is pressed.
    Starting,   ///< START pressed; waiting for evidence that the machine left its green/idle state.
    Brewing,    ///< Armed: the next stable green light means the coffee is ready.
    Ready,      ///< Coffee confirmed; local alert in progress.
    Completed,  ///< Alert delivered. The light is ignored until the next START.
};

const char* toString(BrewState state);

}  // namespace coffee
