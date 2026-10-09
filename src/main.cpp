// Composition root: builds the object graph and hands control to the
// application. Everything interesting lives in the layers below.
//
//   src/core            - detection logic (platform independent)
//   src/hardware        - ports (interfaces) and platform-independent drivers
//   src/application     - coordination
//   src/infrastructure  - ESP32 adapters
#include <Arduino.h>

#include <optional>

#include "application/AppConfig.h"
#include "application/Calibrator.h"
#include "application/CoffeeSentinelApp.h"
#include "hardware/CompositeNotifier.h"
#include "hardware/NetworkLink.h"
#include "infrastructure/esp32/ArduinoClock.h"
#include "infrastructure/esp32/BoardPins.h"
#include "infrastructure/esp32/Esp32AnalogLightSensor.h"
#include "infrastructure/esp32/Esp32Buzzer.h"
#include "infrastructure/esp32/Esp32StartButton.h"
#include "infrastructure/esp32/Esp32StatusLed.h"
#include "infrastructure/esp32/NvsConfigStore.h"
#include "infrastructure/esp32/SerialLogger.h"
#include "infrastructure/esp32/SerialNotifier.h"

// Wi-Fi credentials and the webhook URL live in include/secrets.h, which is
// git-ignored. Without it the firmware runs fully offline (serial notifier only).
#if __has_include("secrets.h")
#include "infrastructure/esp32/Esp32WiFiLink.h"
#include "infrastructure/esp32/WebhookNotifier.h"

#include "secrets.h"
#define COFFEE_HAS_SECRETS 1
#else
#define COFFEE_HAS_SECRETS 0
#endif

namespace {

using namespace coffee;

constexpr unsigned long kSerialBaudRate = 115200;
constexpr LogLevel kLogLevel = LogLevel::Info;  // LogLevel::Debug also prints sensor samples

ArduinoClock arduinoClock;
SerialLogger logger(Serial, kLogLevel);
Esp32AnalogLightSensor lightSensor(pins::kLightSensor);
Esp32StartButton startButton(pins::kStartButton);
Esp32Buzzer buzzer(pins::kBuzzer);
Esp32StatusLed statusLed(pins::kStatusLed);
SerialNotifier serialNotifier(Serial);
CompositeNotifier notifier;
NvsConfigStore configStore;

#if COFFEE_HAS_SECRETS
Esp32WiFiLink network(secrets::kWifiSsid, secrets::kWifiPassword, logger);
WebhookNotifier webhookNotifier(network, secrets::kWebhookUrl, secrets::kWebhookToken, logger);
#else
NullNetworkLink network;
#endif

#ifdef COFFEE_CALIBRATION_MODE
std::optional<Calibrator> calibrator;
#else
std::optional<CoffeeSentinelApp> app;
#endif

void initHardware() {
    Serial.begin(kSerialBaudRate);
    lightSensor.begin();
    startButton.begin();
    buzzer.begin();
    statusLed.begin();
}

#ifndef COFFEE_CALIBRATION_MODE
/// Detection settings: NVS if something valid was stored, defaults otherwise.
AppConfig loadConfig() {
    AppConfig config;
    CoffeeConfig stored;
    if (!configStore.load(stored)) {
        logger.info("CONFIG", "No stored settings; using defaults");
    } else if (!stored.isValid()) {
        logger.warn("CONFIG", "Stored settings are invalid (%s); using defaults",
                    toString(stored.validate()));
    } else {
        config.detection = stored;
        logger.info("CONFIG", "Loaded detection settings from NVS");
    }
    return config;
}
#endif

}  // namespace

void setup() {
    initHardware();

#ifdef COFFEE_CALIBRATION_MODE
    calibrator.emplace(lightSensor, arduinoClock, logger);
    calibrator->begin();
#else
    notifier.add(serialNotifier);
#if COFFEE_HAS_SECRETS
    notifier.add(webhookNotifier);
#else
    logger.info("INFO", "No include/secrets.h found: running offline (serial notifier only)");
#endif
    app.emplace(loadConfig(), lightSensor, startButton, arduinoClock, buzzer, statusLed, notifier, network,
                logger);
    app->begin();
#endif
}

void loop() {
#ifdef COFFEE_CALIBRATION_MODE
    if (Serial.available() > 0) {
        while (Serial.available() > 0) {
            Serial.read();
        }
        calibrator->confirm();
    }
    calibrator->update();
#else
    app->loop();
#endif
}
