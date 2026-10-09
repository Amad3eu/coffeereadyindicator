#include "application/CoffeeSentinelApp.h"

#include "application/AlertPatterns.h"
#include "application/Version.h"

namespace coffee {

namespace {
constexpr const char* kTagApp = "INFO";
constexpr const char* kTagConfig = "CONFIG";
constexpr const char* kTagState = "STATE";
constexpr const char* kTagSensor = "SENSOR";
constexpr const char* kTagButton = "BUTTON";
constexpr const char* kTagEvent = "EVENT";
constexpr const char* kTagBuzzer = "BUZZER";
constexpr const char* kTagNotify = "NOTIFY";
}  // namespace

CoffeeSentinelApp::CoffeeSentinelApp(const AppConfig& config, LightSensor& sensor, StartButton& startButton,
                                     const Clock& clock, Buzzer& buzzer, StatusLed& statusLed,
                                     Notifier& notifier, NetworkLink& network, Logger& logger)
    : configError_(config.detection.validate()),
      config_(sanitize(config)),
      sensor_(sensor),
      startButton_(startButton),
      clock_(clock),
      notifier_(notifier),
      network_(network),
      logger_(logger),
      monitor_(config_.detection),
      session_(config_.detection),
      startDebouncer_(config_.buttonDebounceMs),
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
    logger_.info(kTagConfig,
                 "lightOn>=%u lightOff<=%u cycleStart=%lums ready=%lums sample=%lums debounce=%lums",
                 static_cast<unsigned>(d.lightOnThreshold), static_cast<unsigned>(d.lightOffThreshold),
                 static_cast<unsigned long>(d.cycleStartConfirmationMs),
                 static_cast<unsigned long>(d.readyConfirmationMs),
                 static_cast<unsigned long>(config_.sampleIntervalMs),
                 static_cast<unsigned long>(config_.buttonDebounceMs));

    buzzerPlayer_.stop();
    applyLedForState(session_.state(), now);
    network_.begin();

    // Make the first loop() iteration sample immediately.
    lastSampleMs_ = now - config_.sampleIntervalMs;
    lastSensorLogMs_ = now - config_.sensorLogIntervalMs;

    logger_.info(kTagApp, "State: %s", toString(session_.state()));
    logger_.info(kTagApp, "Press START to begin a brew cycle");
}

void CoffeeSentinelApp::loop() {
    const uint32_t now = clock_.nowMs();

    updateNetwork(now);
    pollStartButton(now);

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

void CoffeeSentinelApp::pollStartButton(uint32_t nowMs) {
    startDebouncer_.update(startButton_.isPressed(), nowMs);
    if (!startDebouncer_.consumePress()) {
        return;
    }
    logger_.info(kTagButton, "START pressed");
    const BrewState before = session_.state();
    const EventType event = session_.start(nowMs);
    trackStateChange(before, nowMs);
    handleEvent(event, nowMs);
}

void CoffeeSentinelApp::sampleSensor(uint32_t nowMs) {
    lastSampleMs_ = nowMs;

    const LightReading reading = sensor_.read();
    if (nowMs - lastSensorLogMs_ >= config_.sensorLogIntervalMs) {
        lastSensorLogMs_ = nowMs;
        if (reading.valid) {
            logger_.debug(kTagSensor, "value=%u light=%s", static_cast<unsigned>(reading.value),
                          monitor_.isOn() ? "on" : "off");
        } else {
            logger_.debug(kTagSensor, "value=invalid");
        }
    }

    const EventType sensorEvent = monitor_.update(reading);
    if (sensorEvent != EventType::None) {
        handleEvent(sensorEvent, nowMs);
    }

    const BrewState before = session_.state();
    const EventType event = session_.update(monitor_.isOn(), monitor_.isHealthy(), nowMs);
    trackStateChange(before, nowMs);
    if (event != EventType::None) {
        handleEvent(event, nowMs);
    }
}

void CoffeeSentinelApp::completeAlertIfFinished(uint32_t nowMs) {
    // READY lasts while the local alert plays. Once it is over, deliver the
    // notification and close the session.
    if (session_.state() != BrewState::Ready || buzzerPlayer_.isPlaying()) {
        return;
    }
    notify(EventType::CoffeeReady);
    const BrewState before = session_.state();
    const EventType event = session_.complete(nowMs);
    trackStateChange(before, nowMs);
    handleEvent(event, nowMs);
}

void CoffeeSentinelApp::trackStateChange(BrewState before, uint32_t nowMs) {
    const BrewState after = session_.state();
    if (after == before) {
        return;
    }
    logger_.info(kTagState, "%s -> %s", toString(before), toString(after));
    applyLedForState(after, nowMs);
}

void CoffeeSentinelApp::handleEvent(EventType event, uint32_t nowMs) {
    logger_.info(kTagEvent, "%s", toString(event));

    switch (event) {
        case EventType::BrewStarted:
            buzzerPlayer_.play(patterns::kBrewStarted, nowMs);
            logger_.info(kTagApp, "Waiting for the machine to leave its green state");
            break;

        case EventType::BrewArmed:
            logger_.info(kTagApp, "Armed: the next stable green light means coffee");
            break;

        case EventType::CoffeeReady:
            logger_.info(kTagBuzzer, "Playing ready pattern");
            buzzerPlayer_.play(patterns::kCoffeeReady, nowMs);
            break;

        case EventType::BrewCompleted:
            logger_.info(kTagApp, "Session closed; press START for the next brew");
            break;

        case EventType::StartIgnored:
            logger_.warn(kTagButton, "START ignored: a brew cycle is already active (%s)",
                         toString(session_.state()));
            break;

        case EventType::SensorFailure:
            logger_.warn(kTagSensor, "Invalid readings; session frozen");
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

void CoffeeSentinelApp::applyLedForState(BrewState state, uint32_t nowMs) {
    switch (state) {
        case BrewState::Idle:
            ledPlayer_.stop();
            break;
        case BrewState::Starting:
            ledPlayer_.play(patterns::kStartingBlink, nowMs);
            break;
        case BrewState::Brewing:
            ledPlayer_.play(patterns::kBrewingBlink, nowMs);
            break;
        case BrewState::Ready:
        case BrewState::Completed:
            ledPlayer_.play(patterns::kSolidOn, nowMs);
            break;
    }
}

void CoffeeSentinelApp::notify(EventType event) {
    if (!notifier_.notify(event)) {
        logger_.warn(kTagNotify, "Delivery failed for %s", toString(event));
    }
}

}  // namespace coffee
