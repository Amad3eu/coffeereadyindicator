#pragma once

// Copy this file to include/secrets.h (git-ignored) and fill in your values.
//
//   cp include/secrets.example.h include/secrets.h
//
// When include/secrets.h is absent the firmware compiles and runs offline:
// it beeps and prints to the serial console, but sends no webhook.
namespace secrets {

constexpr const char* kWifiSsid = "your-wifi-ssid";
constexpr const char* kWifiPassword = "your-wifi-password";

// Endpoint that receives a JSON POST when the coffee is ready, e.g. a Home
// Assistant webhook trigger (see docs/alexa-integration.md).
constexpr const char* kWebhookUrl = "http://homeassistant.local:8123/api/webhook/coffee-sentinel";

// Optional bearer token sent as "Authorization: Bearer <token>". Leave empty
// for Home Assistant webhooks (they are authenticated by their URL).
constexpr const char* kWebhookToken = "";

}  // namespace secrets
