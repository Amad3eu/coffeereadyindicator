#include "infrastructure/esp32/Esp32WiFiLink.h"

#include <WiFi.h>

namespace coffee {

namespace {
constexpr const char* kTag = "WIFI";
}

Esp32WiFiLink::Esp32WiFiLink(const char* ssid, const char* password, Logger& logger,
                             uint32_t reconnectIntervalMs)
    : ssid_(ssid), password_(password), logger_(logger), reconnectIntervalMs_(reconnectIntervalMs) {}

void Esp32WiFiLink::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    connect();
}

void Esp32WiFiLink::update(uint32_t nowMs) {
    const bool connected = isConnected();
    if (connected && !wasConnected_) {
        logger_.info(kTag, "Connected to %s, IP %s, RSSI %d dBm", ssid_, WiFi.localIP().toString().c_str(),
                     WiFi.RSSI());
    } else if (!connected && wasConnected_) {
        logger_.warn(kTag, "Connection lost; retrying every %lu ms",
                     static_cast<unsigned long>(reconnectIntervalMs_));
    }
    wasConnected_ = connected;

    if (!connected && nowMs - lastAttemptMs_ >= reconnectIntervalMs_) {
        lastAttemptMs_ = nowMs;
        connect();
    }
}

bool Esp32WiFiLink::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

void Esp32WiFiLink::connect() {
    logger_.info(kTag, "Connecting to %s", ssid_);
    WiFi.begin(ssid_, password_);
}

}  // namespace coffee
