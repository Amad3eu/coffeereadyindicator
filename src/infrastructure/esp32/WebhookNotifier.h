#pragma once

#include <cstdint>

#include "hardware/Logger.h"
#include "hardware/NetworkLink.h"
#include "hardware/Notifier.h"

namespace coffee {

/// POSTs a small JSON payload to an HTTP(S) endpoint, e.g. a Home Assistant
/// webhook trigger:
///
///   {"event":"coffee_ready","message":"Coffee is ready","device":"coffee-sentinel","version":"0.1.0"}
///
/// The request is synchronous (bounded by `timeoutMs`) and runs once per
/// coffee, after the local buzzer alert has finished.
class WebhookNotifier : public Notifier {
   public:
    static constexpr const char* kDeviceName = "coffee-sentinel";

    WebhookNotifier(NetworkLink& link, const char* url, const char* bearerToken, Logger& logger,
                    uint16_t timeoutMs = 3000);

    bool notify(EventType event) override;

   private:
    bool post(const char* payload);

    NetworkLink& link_;
    const char* url_;
    const char* bearerToken_;
    Logger& logger_;
    uint16_t timeoutMs_;
};

}  // namespace coffee
