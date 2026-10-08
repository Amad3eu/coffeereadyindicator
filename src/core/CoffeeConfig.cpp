#include "core/CoffeeConfig.h"

namespace coffee {

CoffeeConfig::Error CoffeeConfig::validate() const {
    if (lightOffThreshold >= lightOnThreshold) {
        return Error::ThresholdsInverted;
    }
    if (lightOnThreshold > sensorMaxValue) {
        return Error::ThresholdAboveSensorMax;
    }
    if (readyConfirmationMs == 0 || rearmConfirmationMs == 0) {
        return Error::ZeroConfirmation;
    }
    return Error::None;
}

const char* toString(CoffeeConfig::Error error) {
    switch (error) {
        case CoffeeConfig::Error::None:
            return "ok";
        case CoffeeConfig::Error::ThresholdsInverted:
            return "lightOffThreshold must be below lightOnThreshold";
        case CoffeeConfig::Error::ThresholdAboveSensorMax:
            return "lightOnThreshold exceeds sensorMaxValue";
        case CoffeeConfig::Error::ZeroConfirmation:
            return "confirmation windows must be greater than 0 ms";
    }
    return "unknown";
}

}  // namespace coffee
