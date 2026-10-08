#include "core/Event.h"

namespace coffee {

const char* toString(EventType event) {
    switch (event) {
        case EventType::None:
            return "None";
        case EventType::CoffeeReady:
            return "CoffeeReady";
        case EventType::CoffeeReset:
            return "CoffeeReset";
        case EventType::SensorFailure:
            return "SensorFailure";
        case EventType::SensorRecovered:
            return "SensorRecovered";
        case EventType::WiFiConnected:
            return "WiFiConnected";
        case EventType::WiFiDisconnected:
            return "WiFiDisconnected";
    }
    return "Unknown";
}

const char* toSlug(EventType event) {
    switch (event) {
        case EventType::None:
            return "none";
        case EventType::CoffeeReady:
            return "coffee_ready";
        case EventType::CoffeeReset:
            return "coffee_reset";
        case EventType::SensorFailure:
            return "sensor_failure";
        case EventType::SensorRecovered:
            return "sensor_recovered";
        case EventType::WiFiConnected:
            return "wifi_connected";
        case EventType::WiFiDisconnected:
            return "wifi_disconnected";
    }
    return "unknown";
}

const char* toMessage(EventType event) {
    switch (event) {
        case EventType::None:
            return "";
        case EventType::CoffeeReady:
            return "Coffee is ready";
        case EventType::CoffeeReset:
            return "Coffee machine reset, watching for the next brew";
        case EventType::SensorFailure:
            return "Light sensor failure";
        case EventType::SensorRecovered:
            return "Light sensor recovered";
        case EventType::WiFiConnected:
            return "Wi-Fi connected";
        case EventType::WiFiDisconnected:
            return "Wi-Fi disconnected";
    }
    return "";
}

}  // namespace coffee
