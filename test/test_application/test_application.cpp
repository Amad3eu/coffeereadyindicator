// CoffeeSentinelApp with every dependency mocked: buzzer, LED, notifier,
// logger, clock, sensor and network.
#include <unity.h>

#include "application/CoffeeSentinelApp.h"

#include "support/AppHarness.h"
#include "support/Assertions.h"

using coffee::AppConfig;
using coffee::CoffeeState;
using coffee::EventType;
using coffee::test::AppHarness;
using coffee::test::assertState;

namespace {
constexpr uint16_t kDark = 300;
constexpr uint16_t kBright = 2800;
constexpr uint32_t kMinute = 60'000;
}  // namespace

void setUp() {}
void tearDown() {}

void test_boot_logs_banner_and_initial_state() {
    AppHarness h;
    TEST_ASSERT_TRUE(h.logger.contains("[INFO] Coffee Sentinel"));
    TEST_ASSERT_TRUE(h.logger.contains("[INFO] State: IDLE"));
    TEST_ASSERT_TRUE(h.network.began);
    assertState(CoffeeState::Idle, h.app.state());
}

void test_full_cycle_beeps_notifies_and_rearms() {
    AppHarness h;
    h.lightAt(kBright, 3100);
    assertState(CoffeeState::Ready, h.app.state());
    TEST_ASSERT_TRUE(h.logger.contains("[STATE] IDLE -> DETECTING"));
    TEST_ASSERT_TRUE(h.logger.contains("[STATE] DETECTING -> READY"));
    TEST_ASSERT_TRUE(h.logger.contains("[EVENT] CoffeeReady"));
    TEST_ASSERT_TRUE(h.logger.contains("[BUZZER] Playing ready pattern"));

    h.runFor(1000);  // ready pattern (420 ms) finishes, notification goes out
    assertState(CoffeeState::Notified, h.app.state());
    TEST_ASSERT_EQUAL_UINT(2, h.buzzer.beeps());
    TEST_ASSERT_FALSE(h.buzzer.isOn());
    TEST_ASSERT_EQUAL_UINT(1, h.notifier.count(EventType::CoffeeReady));
    TEST_ASSERT_TRUE(h.logger.contains("[STATE] READY -> NOTIFIED"));

    h.lightAt(kDark, 3500);
    assertState(CoffeeState::Idle, h.app.state());
    TEST_ASSERT_TRUE(h.logger.contains("[STATE] NOTIFIED -> REARMING"));
    TEST_ASSERT_TRUE(h.logger.contains("[STATE] REARMING -> IDLE"));
    TEST_ASSERT_TRUE(h.logger.contains("[EVENT] CoffeeReset"));
    TEST_ASSERT_EQUAL_UINT(0, h.notifier.count(EventType::CoffeeReset));  // off by default
}

void test_notification_is_sent_after_the_buzzer_pattern_ends() {
    AppHarness h;
    h.lightAt(kBright, 3050);
    assertState(CoffeeState::Ready, h.app.state());
    TEST_ASSERT_TRUE(h.buzzer.isOn());
    TEST_ASSERT_EQUAL_UINT(0, h.notifier.events.size());

    h.runFor(200);  // still inside the 420 ms pattern
    assertState(CoffeeState::Ready, h.app.state());
    TEST_ASSERT_EQUAL_UINT(0, h.notifier.events.size());

    h.runFor(300);
    assertState(CoffeeState::Notified, h.app.state());
    TEST_ASSERT_EQUAL_UINT(1, h.notifier.events.size());
}

void test_light_on_for_twenty_minutes_alerts_once() {
    AppHarness h;
    h.lightAt(kBright, 20 * kMinute);
    assertState(CoffeeState::Notified, h.app.state());
    TEST_ASSERT_EQUAL_UINT(1, h.notifier.count(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_UINT(2, h.buzzer.beeps());
    TEST_ASSERT_EQUAL_UINT(1, h.logger.count("[EVENT] CoffeeReady"));
}

void test_second_brew_alerts_again() {
    AppHarness h;
    h.lightAt(kBright, 5000);
    h.lightAt(kDark, 5000);
    h.lightAt(kBright, 5000);
    TEST_ASSERT_EQUAL_UINT(2, h.notifier.count(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_UINT(4, h.buzzer.beeps());
}

void test_status_led_follows_state() {
    AppHarness h;
    h.lightAt(kDark, 500);
    TEST_ASSERT_FALSE(h.led.isOn());

    h.lightAt(kBright, 1000);  // DETECTING blinks
    TEST_ASSERT_TRUE(h.led.blinks() >= 3);

    h.lightAt(kBright, 3000);  // READY / NOTIFIED: solid on
    assertState(CoffeeState::Notified, h.app.state());
    TEST_ASSERT_TRUE(h.led.isOn());
    const uint32_t blinksWhenSolid = h.led.blinks();
    h.runFor(2000);
    TEST_ASSERT_TRUE(h.led.isOn());
    TEST_ASSERT_EQUAL_UINT32(blinksWhenSolid, h.led.blinks());

    h.lightAt(kDark, 5000);  // back to IDLE
    TEST_ASSERT_FALSE(h.led.isOn());
}

void test_sensor_failure_is_logged_and_chirped() {
    AppHarness h;
    h.sensor.setInvalid();
    h.runFor(2000);
    TEST_ASSERT_EQUAL_UINT(1, h.logger.count("[EVENT] SensorFailure"));
    TEST_ASSERT_TRUE(h.logger.contains("[WARN][SENSOR] Invalid readings"));
    TEST_ASSERT_EQUAL_UINT(3, h.buzzer.beeps());
    TEST_ASSERT_EQUAL_UINT(0, h.notifier.events.size());

    h.lightAt(kDark, 500);
    TEST_ASSERT_EQUAL_UINT(1, h.logger.count("[EVENT] SensorRecovered"));
}

void test_invalid_configuration_falls_back_to_defaults() {
    AppConfig config;
    config.detection.lightOnThreshold = 1000;
    config.detection.lightOffThreshold = 1500;
    AppHarness h(config);
    TEST_ASSERT_TRUE(h.logger.contains("[ERROR][CONFIG] Invalid configuration"));
    TEST_ASSERT_EQUAL_UINT16(1800, h.app.config().detection.lightOnThreshold);
    TEST_ASSERT_EQUAL_UINT16(1200, h.app.config().detection.lightOffThreshold);
}

void test_failed_notification_is_logged_but_state_moves_on() {
    AppHarness h;
    h.notifier.result = false;
    h.lightAt(kBright, 5000);
    assertState(CoffeeState::Notified, h.app.state());
    TEST_ASSERT_TRUE(h.logger.contains("[WARN][NOTIFY] Delivery failed for CoffeeReady"));
}

void test_notify_on_reset_when_enabled() {
    AppConfig config;
    config.notifyOnReset = true;
    AppHarness h(config);
    h.lightAt(kBright, 5000);
    h.lightAt(kDark, 5000);
    TEST_ASSERT_EQUAL_UINT(1, h.notifier.count(EventType::CoffeeReset));
}

void test_network_changes_become_events() {
    AppHarness h;
    h.runFor(100);
    TEST_ASSERT_FALSE(h.logger.contains("WiFi"));

    h.network.connected = true;
    h.runFor(100);
    TEST_ASSERT_EQUAL_UINT(1, h.logger.count("[EVENT] WiFiConnected"));

    h.network.connected = false;
    h.runFor(100);
    TEST_ASSERT_EQUAL_UINT(1, h.logger.count("[EVENT] WiFiDisconnected"));
    TEST_ASSERT_TRUE(h.network.updates > 0);
}

void test_sensor_is_sampled_at_the_configured_interval() {
    AppConfig config;
    config.sampleIntervalMs = 100;
    AppHarness h(config);
    h.sensor.reads = 0;
    h.runFor(1000);
    TEST_ASSERT_EQUAL_UINT32(10, h.sensor.reads);
}

void test_zero_sample_interval_falls_back_to_default() {
    AppConfig config;
    config.sampleIntervalMs = 0;  // would otherwise sample on every loop iteration
    AppHarness h(config);
    TEST_ASSERT_EQUAL_UINT32(AppConfig{}.sampleIntervalMs, h.app.config().sampleIntervalMs);
}

void test_invalid_reading_is_logged_at_debug_level() {
    AppConfig config;
    config.sensorLogIntervalMs = 500;
    AppHarness h(config);
    h.sensor.setInvalid();
    h.runFor(1500);
    TEST_ASSERT_TRUE(h.logger.contains("[SENSOR] value=invalid"));
}

void test_sensor_value_is_logged_at_debug_level_only() {
    AppConfig config;
    config.sensorLogIntervalMs = 1000;
    AppHarness h(config);
    h.lightAt(kDark, 3000);
    TEST_ASSERT_TRUE(h.logger.contains("[SENSOR] value=300"));

    h.logger.lines.clear();
    h.logger.setMinimumLevel(coffee::LogLevel::Info);
    h.lightAt(kDark, 3000);
    TEST_ASSERT_FALSE(h.logger.contains("[SENSOR] value="));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_boot_logs_banner_and_initial_state);
    RUN_TEST(test_full_cycle_beeps_notifies_and_rearms);
    RUN_TEST(test_notification_is_sent_after_the_buzzer_pattern_ends);
    RUN_TEST(test_light_on_for_twenty_minutes_alerts_once);
    RUN_TEST(test_second_brew_alerts_again);
    RUN_TEST(test_status_led_follows_state);
    RUN_TEST(test_sensor_failure_is_logged_and_chirped);
    RUN_TEST(test_invalid_configuration_falls_back_to_defaults);
    RUN_TEST(test_failed_notification_is_logged_but_state_moves_on);
    RUN_TEST(test_notify_on_reset_when_enabled);
    RUN_TEST(test_network_changes_become_events);
    RUN_TEST(test_sensor_is_sampled_at_the_configured_interval);
    RUN_TEST(test_zero_sample_interval_falls_back_to_default);
    RUN_TEST(test_invalid_reading_is_logged_at_debug_level);
    RUN_TEST(test_sensor_value_is_logged_at_debug_level_only);
    return UNITY_END();
}
