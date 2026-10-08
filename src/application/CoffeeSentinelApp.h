#pragma once

#include <cstdint>

#include "application/AppConfig.h"
#include "core/CoffeeDetector.h"
#include "hardware/Buzzer.h"
#include "hardware/Clock.h"
#include "hardware/LightSensor.h"
#include "hardware/Logger.h"
#include "hardware/NetworkLink.h"
#include "hardware/Notifier.h"
#include "hardware/PatternPlayer.h"
#include "hardware/StatusLed.h"

namespace coffee {

/// Wires the detector to the outside world.
///
/// Responsibilities:
///   - sample the sensor on a fixed interval and feed the detector;
///   - turn detector events into buzzer patterns, LED patterns and notifications;
///   - log state transitions and events;
///   - surface connectivity changes as events.
///
/// Everything is driven by `loop()`; nothing blocks.
class CoffeeSentinelApp {
   public:
    CoffeeSentinelApp(const AppConfig& config, LightSensor& sensor, const Clock& clock, Buzzer& buzzer,
                      StatusLed& statusLed, Notifier& notifier, NetworkLink& network, Logger& logger);

    void begin();
    void loop();

    CoffeeState state() const { return detector_.state(); }
    const AppConfig& config() const { return config_; }
    const CoffeeDetector& detector() const { return detector_; }

   private:
    static AppConfig sanitize(const AppConfig& config);

    void updateNetwork(uint32_t nowMs);
    void sampleSensor(uint32_t nowMs);
    void completeAlertIfFinished(uint32_t nowMs);
    void trackStateChange(CoffeeState before, uint32_t nowMs);
    void handleEvent(EventType event, uint32_t nowMs);
    void applyLedForState(CoffeeState state, uint32_t nowMs);
    void notify(EventType event);

    CoffeeConfig::Error configError_;
    AppConfig config_;

    LightSensor& sensor_;
    const Clock& clock_;
    Notifier& notifier_;
    NetworkLink& network_;
    Logger& logger_;

    CoffeeDetector detector_;
    PatternPlayer buzzerPlayer_;
    PatternPlayer ledPlayer_;

    uint32_t lastSampleMs_ = 0;
    uint32_t lastSensorLogMs_ = 0;
    bool networkConnected_ = false;
};

}  // namespace coffee
