#include "core/Event.h"

namespace coffee {

const char* toString(EventType event) {
    switch (event) {
        case EventType::None:
            return "None";
        case EventType::BrewStarted:
            return "BrewStarted";
        case EventType::BrewArmed:
            return "BrewArmed";
        case EventType::CoffeeReady:
            return "CoffeeReady";
        case EventType::BrewCompleted:
            return "BrewCompleted";
        case EventType::StartIgnored:
            return "StartIgnored";
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
        case EventType::BrewStarted:
            return "brew_started";
        case EventType::BrewArmed:
            return "brew_armed";
        case EventType::CoffeeReady:
            return "coffee_ready";
        case EventType::BrewCompleted:
            return "brew_completed";
        case EventType::StartIgnored:
            return "start_ignored";
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
        case EventType::BrewStarted:
            return "Brew cycle started";
        case EventType::BrewArmed:
            return "Watching for the ready light";
        case EventType::CoffeeReady:
            return "Coffee is ready";
        case EventType::BrewCompleted:
            return "Brew cycle completed";
        case EventType::StartIgnored:
            return "Start ignored: a brew cycle is already active";
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
