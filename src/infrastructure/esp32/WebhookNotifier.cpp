#include "infrastructure/esp32/WebhookNotifier.h"

#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include <cstdio>
#include <cstring>

#include "application/Version.h"

namespace coffee {

namespace {
constexpr const char* kTag = "WEBHOOK";

bool isHttps(const char* url) {
    return std::strncmp(url, "https://", 8) == 0;
}
}  // namespace

WebhookNotifier::WebhookNotifier(NetworkLink& link, const char* url, const char* bearerToken, Logger& logger,
                                 uint16_t timeoutMs)
    : link_(link), url_(url), bearerToken_(bearerToken), logger_(logger), timeoutMs_(timeoutMs) {}

bool WebhookNotifier::notify(EventType event) {
    if (url_ == nullptr || url_[0] == '\0') {
        logger_.debug(kTag, "No URL configured; skipping %s", toString(event));
        return false;
    }
    if (!link_.isConnected()) {
        logger_.warn(kTag, "Network down; cannot deliver %s", toString(event));
        return false;
    }

    char payload[192];
    std::snprintf(payload, sizeof(payload), R"({"event":"%s","message":"%s","device":"%s","version":"%s"})",
                  toSlug(event), toMessage(event), kDeviceName, kVersion);
    return post(payload);
}

bool WebhookNotifier::post(const char* payload) {
    HTTPClient http;
    http.setTimeout(timeoutMs_);
    http.setConnectTimeout(timeoutMs_);

    WiFiClient plainClient;
    WiFiClientSecure secureClient;
    bool began = false;
    if (isHttps(url_)) {
        // Certificate pinning is on the roadmap; for a LAN Home Assistant
        // instance the connection is still encrypted, just not authenticated.
        secureClient.setInsecure();
        began = http.begin(secureClient, url_);
    } else {
        began = http.begin(plainClient, url_);
    }
    if (!began) {
        logger_.error(kTag, "Invalid URL: %s", url_);
        return false;
    }

    http.addHeader("Content-Type", "application/json");
    if (bearerToken_ != nullptr && bearerToken_[0] != '\0') {
        http.addHeader("Authorization", String("Bearer ") + bearerToken_);
    }

    const int status = http.POST(payload);
    http.end();

    if (status >= 200 && status < 300) {
        logger_.info(kTag, "Delivered (HTTP %d)", status);
        return true;
    }
    logger_.warn(kTag, "Delivery failed: %s", status < 0 ? HTTPClient::errorToString(status).c_str() : "");
    if (status >= 0) {
        logger_.warn(kTag, "HTTP status %d", status);
    }
    return false;
}

}  // namespace coffee
