#include "application/CoffeeSentinelApp.h"

#include "application/AlertPatterns.h"
#include "application/Version.h"

namespace coffee {

namespace {
constexpr const char* kTagApp = "INFO";
constexpr const char* kTagConfig = "CONFIG";
constexpr const char* kTagState = "STATE";
constexpr const char* kTagSensor = "SENSOR";
constexpr const char* kTagEvent = "EVENT";
constexpr const char* kTagBuzzer = "BUZZER";
constexpr const char* kTagNotify = "NOTIFY";
}  // namespace

CoffeeSentinelApp::CoffeeSentinelApp(const AppConfig& config, LightSensor& sensor, const Clock& clock,
                                     Buzzer& buzzer, StatusLed& statusLed, Notifier& notifier,
                                     NetworkLink& network, Logger& logger)
    : configError_(config.detection.validate()),
      config_(sanitize(config)),
      sensor_(sensor),
      clock_(clock),
      notifier_(notifier),
      network_(network),
      logger_(logger),
      detector_(config_.detection),
      buzzerPlayer_(buzzer),
      ledPlayer_(statusLed) {}

AppConfig CoffeeSentinelApp::sanitize(const AppConfig& config) {
    AppConfig result = config;
    if (!config.detection.isValid()) {
        result.detection = CoffeeConfig{};
    }
    if (result.sampleIntervalMs == 0) {
        result.sampleIntervalMs = AppConfig{}.sampleIntervalMs;
    }
    return result;
}

void CoffeeSentinelApp::begin() {
    const uint32_t now = clock_.nowMs();

    logger_.info(kTagApp, "Coffee Sentinel %s booting", kVersion);
    if (configError_ != CoffeeConfig::Error::None) {
        logger_.error(kTagConfig, "Invalid configuration (%s); using defaults", toString(configError_));
    }
    const CoffeeConfig& d = config_.detection;
    logger_.info(kTagConfig, "lightOn>=%u lightOff<=%u ready=%lums rearm=%lums sample=%lums",
                 static_cast<unsigned>(d.lightOnThreshold), static_cast<unsigned>(d.lightOffThreshold),
                 static_cast<unsigned long>(d.readyConfirmationMs),
                 static_cast<unsigned long>(d.rearmConfirmationMs),
                 static_cast<unsigned long>(config_.sampleIntervalMs));

    buzzerPlayer_.stop();
    ledPlayer_.stop();
    network_.begin();

    // Make the first loop() iteration sample immediately.
    lastSampleMs_ = now - config_.sampleIntervalMs;
    lastSensorLogMs_ = now - config_.sensorLogIntervalMs;

    logger_.info(kTagApp, "State: %s", toString(detector_.state()));
}

void CoffeeSentinelApp::loop() {
    const uint32_t now = clock_.nowMs();

    updateNetwork(now);

    if (now - lastSampleMs_ >= config_.sampleIntervalMs) {
        sampleSensor(now);
    }

    buzzerPlayer_.update(now);
    ledPlayer_.update(now);

    completeAlertIfFinished(now);
}

void CoffeeSentinelApp::updateNetwork(uint32_t nowMs) {
    network_.update(nowMs);
    const bool connected = network_.isConnected();
    if (connected != networkConnected_) {
        networkConnected_ = connected;
        handleEvent(connected ? EventType::WiFiConnected : EventType::WiFiDisconnected, nowMs);
    }
}

void CoffeeSentinelApp::sampleSensor(uint32_t nowMs) {
    lastSampleMs_ = nowMs;

    const LightReading reading = sensor_.read();
    if (nowMs - lastSensorLogMs_ >= config_.sensorLogIntervalMs) {
        lastSensorLogMs_ = nowMs;
        if (reading.valid) {
            logger_.debug(kTagSensor, "value=%u light=%s", static_cast<unsigned>(reading.value),
                          detector_.isLightOn() ? "on" : "off");
        } else {
            logger_.debug(kTagSensor, "value=invalid");
        }
    }

    const CoffeeState before = detector_.state();
    const EventType event = detector_.update(reading, nowMs);
    trackStateChange(before, nowMs);
    if (event != EventType::None) {
        handleEvent(event, nowMs);
    }
}

void CoffeeSentinelApp::completeAlertIfFinished(uint32_t nowMs) {
    // READY lasts while the local alert plays. Once it is over, deliver the
    // notification and let the detector move on to NOTIFIED.
    if (detector_.state() != CoffeeState::Ready || buzzerPlayer_.isPlaying()) {
        return;
    }
    notify(EventType::CoffeeReady);
    const CoffeeState before = detector_.state();
    detector_.markNotified(nowMs);
    trackStateChange(before, nowMs);
}

void CoffeeSentinelApp::trackStateChange(CoffeeState before, uint32_t nowMs) {
    const CoffeeState after = detector_.state();
    if (after == before) {
        return;
    }
    logger_.info(kTagState, "%s -> %s", toString(before), toString(after));
    applyLedForState(after, nowMs);
}

void CoffeeSentinelApp::handleEvent(EventType event, uint32_t nowMs) {
    logger_.info(kTagEvent, "%s", toString(event));

    switch (event) {
        case EventType::CoffeeReady:
            logger_.info(kTagBuzzer, "Playing ready pattern");
            buzzerPlayer_.play(patterns::kCoffeeReady, nowMs);
            break;

        case EventType::CoffeeReset:
            if (config_.notifyOnReset) {
                notify(event);
            }
            break;

        case EventType::SensorFailure:
            logger_.warn(kTagSensor, "Invalid readings; detection paused");
            buzzerPlayer_.play(patterns::kSensorFailure, nowMs);
            break;

        case EventType::SensorRecovered:
            logger_.info(kTagSensor, "Readings valid again; confirmation window restarted");
            break;

        case EventType::WiFiConnected:
        case EventType::WiFiDisconnected:
        case EventType::None:
            break;
    }
}

void CoffeeSentinelApp::applyLedForState(CoffeeState state, uint32_t nowMs) {
    switch (state) {
        case CoffeeState::Idle:
            ledPlayer_.stop();
            break;
        case CoffeeState::Detecting:
            ledPlayer_.play(patterns::kDetectingBlink, nowMs);
            break;
        case CoffeeState::Ready:
        case CoffeeState::Notified:
            ledPlayer_.play(patterns::kSolidOn, nowMs);
            break;
        case CoffeeState::Rearming:
            ledPlayer_.play(patterns::kRearmingBlink, nowMs);
            break;
    }
}

void CoffeeSentinelApp::notify(EventType event) {
    if (!notifier_.notify(event)) {
        logger_.warn(kTagNotify, "Delivery failed for %s", toString(event));
    }
}

}  // namespace coffee
