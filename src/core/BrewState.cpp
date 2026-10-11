#include "core/BrewState.h"

namespace coffee {

const char* toString(BrewState state) {
    switch (state) {
        case BrewState::Idle:
            return "IDLE";
        case BrewState::Starting:
            return "STARTING";
        case BrewState::Brewing:
            return "BREWING";
        case BrewState::Ready:
            return "READY";
        case BrewState::Completed:
            return "COMPLETED";
    }
    return "UNKNOWN";
}

}  // namespace coffee
