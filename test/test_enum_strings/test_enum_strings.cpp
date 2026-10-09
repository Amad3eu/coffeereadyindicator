// Human-readable names of every enum used in logs and payloads, including the
// defensive fallbacks for values outside the enumerators.
#include <unity.h>

#include "application/Calibrator.h"
#include "core/CoffeeConfig.h"
#include "core/CoffeeState.h"
#include "core/Event.h"
#include "hardware/Logger.h"

using coffee::CalibrationPhase;
using coffee::CoffeeConfig;
using coffee::CoffeeState;
using coffee::EventType;
using coffee::LogLevel;

namespace {
constexpr uint8_t kOutOfRange = 0xFF;
}

void setUp() {}
void tearDown() {}

void test_coffee_state_names() {
    TEST_ASSERT_EQUAL_STRING("IDLE", toString(CoffeeState::Idle));
    TEST_ASSERT_EQUAL_STRING("DETECTING", toString(CoffeeState::Detecting));
    TEST_ASSERT_EQUAL_STRING("READY", toString(CoffeeState::Ready));
    TEST_ASSERT_EQUAL_STRING("NOTIFIED", toString(CoffeeState::Notified));
    TEST_ASSERT_EQUAL_STRING("REARMING", toString(CoffeeState::Rearming));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", toString(static_cast<CoffeeState>(kOutOfRange)));
}

void test_event_log_names() {
    TEST_ASSERT_EQUAL_STRING("None", toString(EventType::None));
    TEST_ASSERT_EQUAL_STRING("CoffeeReady", toString(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_STRING("CoffeeReset", toString(EventType::CoffeeReset));
    TEST_ASSERT_EQUAL_STRING("SensorFailure", toString(EventType::SensorFailure));
    TEST_ASSERT_EQUAL_STRING("SensorRecovered", toString(EventType::SensorRecovered));
    TEST_ASSERT_EQUAL_STRING("WiFiConnected", toString(EventType::WiFiConnected));
    TEST_ASSERT_EQUAL_STRING("WiFiDisconnected", toString(EventType::WiFiDisconnected));
    TEST_ASSERT_EQUAL_STRING("Unknown", toString(static_cast<EventType>(kOutOfRange)));
}

void test_event_payload_slugs() {
    TEST_ASSERT_EQUAL_STRING("none", toSlug(EventType::None));
    TEST_ASSERT_EQUAL_STRING("coffee_ready", toSlug(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_STRING("coffee_reset", toSlug(EventType::CoffeeReset));
    TEST_ASSERT_EQUAL_STRING("sensor_failure", toSlug(EventType::SensorFailure));
    TEST_ASSERT_EQUAL_STRING("sensor_recovered", toSlug(EventType::SensorRecovered));
    TEST_ASSERT_EQUAL_STRING("wifi_connected", toSlug(EventType::WiFiConnected));
    TEST_ASSERT_EQUAL_STRING("wifi_disconnected", toSlug(EventType::WiFiDisconnected));
    TEST_ASSERT_EQUAL_STRING("unknown", toSlug(static_cast<EventType>(kOutOfRange)));
}

void test_event_messages() {
    TEST_ASSERT_EQUAL_STRING("", toMessage(EventType::None));
    TEST_ASSERT_EQUAL_STRING("Coffee is ready", toMessage(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_STRING("Coffee machine reset, watching for the next brew",
                             toMessage(EventType::CoffeeReset));
    TEST_ASSERT_EQUAL_STRING("Light sensor failure", toMessage(EventType::SensorFailure));
    TEST_ASSERT_EQUAL_STRING("Light sensor recovered", toMessage(EventType::SensorRecovered));
    TEST_ASSERT_EQUAL_STRING("Wi-Fi connected", toMessage(EventType::WiFiConnected));
    TEST_ASSERT_EQUAL_STRING("Wi-Fi disconnected", toMessage(EventType::WiFiDisconnected));
    TEST_ASSERT_EQUAL_STRING("", toMessage(static_cast<EventType>(kOutOfRange)));
}

void test_log_level_names() {
    TEST_ASSERT_EQUAL_STRING("DEBUG", toString(LogLevel::Debug));
    TEST_ASSERT_EQUAL_STRING("INFO", toString(LogLevel::Info));
    TEST_ASSERT_EQUAL_STRING("WARN", toString(LogLevel::Warn));
    TEST_ASSERT_EQUAL_STRING("ERROR", toString(LogLevel::Error));
    TEST_ASSERT_EQUAL_STRING("?", toString(static_cast<LogLevel>(kOutOfRange)));
}

void test_config_error_descriptions() {
    TEST_ASSERT_EQUAL_STRING("ok", toString(CoffeeConfig::Error::None));
    TEST_ASSERT_EQUAL_STRING("lightOffThreshold must be below lightOnThreshold",
                             toString(CoffeeConfig::Error::ThresholdsInverted));
    TEST_ASSERT_EQUAL_STRING("lightOnThreshold exceeds sensorMaxValue",
                             toString(CoffeeConfig::Error::ThresholdAboveSensorMax));
    TEST_ASSERT_EQUAL_STRING("confirmation windows must be greater than 0 ms",
                             toString(CoffeeConfig::Error::ZeroConfirmation));
    TEST_ASSERT_EQUAL_STRING("unknown", toString(static_cast<CoffeeConfig::Error>(kOutOfRange)));
}

void test_calibration_phase_names() {
    TEST_ASSERT_EQUAL_STRING("AMBIENT", toString(CalibrationPhase::Ambient));
    TEST_ASSERT_EQUAL_STRING("LED OFF", toString(CalibrationPhase::LedOff));
    TEST_ASSERT_EQUAL_STRING("LED ON", toString(CalibrationPhase::LedOn));
    TEST_ASSERT_EQUAL_STRING("DONE", toString(CalibrationPhase::Done));
    TEST_ASSERT_EQUAL_STRING("?", toString(static_cast<CalibrationPhase>(kOutOfRange)));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_coffee_state_names);
    RUN_TEST(test_event_log_names);
    RUN_TEST(test_event_payload_slugs);
    RUN_TEST(test_event_messages);
    RUN_TEST(test_log_level_names);
    RUN_TEST(test_config_error_descriptions);
    RUN_TEST(test_calibration_phase_names);
    return UNITY_END();
}
