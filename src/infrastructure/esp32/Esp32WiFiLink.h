#pragma once

#include <cstdint>

#include "hardware/Logger.h"
#include "hardware/NetworkLink.h"

namespace coffee {

/// Station-mode Wi-Fi link with non-blocking reconnection.
class Esp32WiFiLink : public NetworkLink {
   public:
    Esp32WiFiLink(const char* ssid, const char* password, Logger& logger,
                  uint32_t reconnectIntervalMs = 15'000);

    void begin() override;
    void update(uint32_t nowMs) override;
    bool isConnected() const override;

   private:
    void connect();

    const char* ssid_;
    const char* password_;
    Logger& logger_;
    uint32_t reconnectIntervalMs_;
    uint32_t lastAttemptMs_ = 0;
    bool wasConnected_ = false;
};

}  // namespace coffee
