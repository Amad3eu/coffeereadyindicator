#include "core/CoffeeState.h"

namespace coffee {

const char* toString(CoffeeState state) {
    switch (state) {
        case CoffeeState::Idle:
            return "IDLE";
        case CoffeeState::Detecting:
            return "DETECTING";
        case CoffeeState::Ready:
            return "READY";
        case CoffeeState::Notified:
            return "NOTIFIED";
        case CoffeeState::Rearming:
            return "REARMING";
    }
    return "UNKNOWN";
}

}  // namespace coffee
