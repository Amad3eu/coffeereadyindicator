// CoffeeSentinelApp with every dependency mocked: button, sensor, clock,
// buzzer, LED, notifier, network and logger.
#include <unity.h>

#include "application/CoffeeSentinelApp.h"

#include "support/AppHarness.h"
#include "support/Assertions.h"

using coffee::AppConfig;
using coffee::BrewState;
using coffee::EventType;
using coffee::test::AppHarness;
using coffee::test::assertState;

namespace {
constexpr uint16_t kDark = 300;
constexpr uint16_t kGreen = 2800;
constexpr uint32_t kMinute = 60'000;

/// START with the light off, then the confirmed OFF period: the app is armed.
void startAndArm(AppHarness& h) {
    h.lightAt(kDark, 100);
    h.pressStart();
    assertState(BrewState::Starting, h.app.state());
    h.lightAt(kDark, 3500);
    assertState(BrewState::Brewing, h.app.state());
}
}  // namespace

void setUp() {}
void tearDown() {}

void test_boot_logs_banner_and_initial_state() {
    AppHarness h;
    TEST_ASSERT_TRUE(h.logger.contains("[INFO] Coffee Sentinel"));
    TEST_ASSERT_TRUE(h.logger.contains("[INFO] State: IDLE"));
    TEST_ASSERT_TRUE(h.logger.contains("[INFO] Press START"));
    TEST_ASSERT_TRUE(h.network.began);
    assertState(BrewState::Idle, h.app.state());
}

void test_green_before_start_does_nothing() {
    AppHarness h;
    h.lightAt(kGreen, 20 * kMinute);
    assertState(BrewState::Idle, h.app.state());
    TEST_ASSERT_EQUAL_UINT(0, h.notifier.events.size());
    TEST_ASSERT_EQUAL_UINT(0, h.buzzer.beeps());
    TEST_ASSERT_FALSE(h.led.isOn());
}

void test_start_opens_a_session_with_a_blip_and_log() {
    AppHarness h;
    h.pressStart();
    assertState(BrewState::Starting, h.app.state());
    TEST_ASSERT_TRUE(h.logger.contains("[BUTTON] START pressed"));
    TEST_ASSERT_TRUE(h.logger.contains("[STATE] IDLE -> STARTING"));
    TEST_ASSERT_TRUE(h.logger.contains("[EVENT] BrewStarted"));
    TEST_ASSERT_EQUAL_UINT(1, h.buzzer.beeps());
}

void test_start_with_green_already_on_does_not_alert() {
    AppHarness h;
    h.lightAt(kGreen, 1000);
    h.pressStart();
    h.lightAt(kGreen, 10 * kMinute);
    assertState(BrewState::Starting, h.app.state());
    TEST_ASSERT_EQUAL_UINT(0, h.notifier.count(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_UINT(1, h.buzzer.beeps());  // only the START blip
}

void test_full_cycle_beeps_notifies_and_completes() {
    AppHarness h;
    startAndArm(h);
    TEST_ASSERT_TRUE(h.logger.contains("[STATE] STARTING -> BREWING"));
    TEST_ASSERT_TRUE(h.logger.contains("[EVENT] BrewArmed"));

    h.lightAt(kGreen, 3100);
    assertState(BrewState::Ready, h.app.state());
    TEST_ASSERT_TRUE(h.logger.contains("[STATE] BREWING -> READY"));
    TEST_ASSERT_TRUE(h.logger.contains("[EVENT] CoffeeReady"));
    TEST_ASSERT_TRUE(h.logger.contains("[BUZZER] Playing ready pattern"));

    h.runFor(1000);  // ready pattern (420 ms) finishes, notification goes out
    assertState(BrewState::Completed, h.app.state());
    TEST_ASSERT_EQUAL_UINT(3, h.buzzer.beeps());  // START blip + BEEP BEEP
    TEST_ASSERT_FALSE(h.buzzer.isOn());
    TEST_ASSERT_EQUAL_UINT(1, h.notifier.count(EventType::CoffeeReady));
    TEST_ASSERT_TRUE(h.logger.contains("[STATE] READY -> COMPLETED"));
    TEST_ASSERT_TRUE(h.logger.contains("[EVENT] BrewCompleted"));
}

void test_notification_is_sent_after_the_buzzer_pattern_ends() {
    AppHarness h;
    startAndArm(h);
    h.lightAt(kGreen, 3050);
    assertState(BrewState::Ready, h.app.state());
    TEST_ASSERT_TRUE(h.buzzer.isOn());
    TEST_ASSERT_EQUAL_UINT(0, h.notifier.events.size());

    h.runFor(200);  // still inside the 420 ms pattern
    assertState(BrewState::Ready, h.app.state());
    TEST_ASSERT_EQUAL_UINT(0, h.notifier.events.size());

    h.runFor(300);
    assertState(BrewState::Completed, h.app.state());
    TEST_ASSERT_EQUAL_UINT(1, h.notifier.events.size());
}

void test_green_shorter_than_confirmation_does_not_alert() {
    AppHarness h;
    startAndArm(h);
    h.lightAt(kGreen, 2000);
    h.lightAt(kDark, 1000);
    assertState(BrewState::Brewing, h.app.state());
    TEST_ASSERT_EQUAL_UINT(0, h.notifier.events.size());
}

void test_only_one_alert_per_session() {
    AppHarness h;
    startAndArm(h);
    h.lightAt(kGreen, 20 * kMinute);
    assertState(BrewState::Completed, h.app.state());
    for (int i = 0; i < 20; ++i) {
        h.lightAt(kDark, 4000);
        h.lightAt(kGreen, 4000);
    }
    TEST_ASSERT_EQUAL_UINT(1, h.notifier.count(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_UINT(3, h.buzzer.beeps());
    TEST_ASSERT_EQUAL_UINT(1, h.logger.count("[EVENT] CoffeeReady"));
}

void test_new_start_allows_a_new_cycle() {
    AppHarness h;
    startAndArm(h);
    h.lightAt(kGreen, 5000);
    assertState(BrewState::Completed, h.app.state());

    h.pressStart();  // machine still green from the last coffee
    assertState(BrewState::Starting, h.app.state());
    TEST_ASSERT_TRUE(h.logger.contains("[STATE] COMPLETED -> STARTING"));
    h.lightAt(kGreen, 5000);
    h.lightAt(kDark, 3500);
    h.lightAt(kGreen, 5000);
    TEST_ASSERT_EQUAL_UINT(2, h.notifier.count(EventType::CoffeeReady));
}

void test_duplicate_start_is_ignored_and_logged() {
    AppHarness h;
    startAndArm(h);
    h.pressStart();
    assertState(BrewState::Brewing, h.app.state());
    TEST_ASSERT_TRUE(h.logger.contains("[EVENT] StartIgnored"));
    TEST_ASSERT_TRUE(
        h.logger.contains("[WARN][BUTTON] START ignored: a brew cycle is already active (BREWING)"));
    TEST_ASSERT_EQUAL_UINT(1, h.logger.count("[EVENT] BrewStarted"));
}

void test_button_bounce_counts_as_one_press() {
    AppHarness h;
    h.lightAt(kDark, 100);
    for (int i = 0; i < 6; ++i) {  // contact chatter, 10 ms per level
        h.button.pressed = true;
        h.runFor(10);
        h.button.pressed = false;
        h.runFor(10);
    }
    h.button.pressed = true;
    h.runFor(200);  // then held steadily
    h.button.pressed = false;
    h.runFor(200);
    TEST_ASSERT_EQUAL_UINT(1, h.logger.count("[EVENT] BrewStarted"));
    TEST_ASSERT_EQUAL_UINT(0, h.logger.count("[EVENT] StartIgnored"));
}

void test_holding_the_button_is_a_single_press() {
    AppHarness h;
    h.button.pressed = true;
    h.runFor(5000);
    h.button.pressed = false;
    h.runFor(200);
    TEST_ASSERT_EQUAL_UINT(1, h.logger.count("[BUTTON] START pressed"));
}

void test_status_led_follows_state() {
    AppHarness h;
    h.lightAt(kDark, 500);
    TEST_ASSERT_FALSE(h.led.isOn());  // IDLE: off

    h.pressStart();
    h.lightAt(kDark, 1000);  // STARTING: fast blink
    TEST_ASSERT_TRUE(h.led.blinks() >= 3);

    h.lightAt(kDark, 3000);  // BREWING: slow heartbeat
    assertState(BrewState::Brewing, h.app.state());
    const uint32_t blinksWhenArmed = h.led.blinks();
    h.lightAt(kDark, 3000);
    TEST_ASSERT_TRUE(h.led.blinks() - blinksWhenArmed >= 2);
    TEST_ASSERT_TRUE(h.led.blinks() - blinksWhenArmed <= 4);

    h.lightAt(kGreen, 5000);  // READY then COMPLETED: solid on
    assertState(BrewState::Completed, h.app.state());
    TEST_ASSERT_TRUE(h.led.isOn());
    const uint32_t blinksWhenSolid = h.led.blinks();
    h.runFor(2000);
    TEST_ASSERT_TRUE(h.led.isOn());
    TEST_ASSERT_EQUAL_UINT32(blinksWhenSolid, h.led.blinks());
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

void test_sensor_failure_during_brewing_never_alerts() {
    AppHarness h;
    startAndArm(h);
    h.sensor.setInvalid();
    h.runFor(10 * kMinute);
    assertState(BrewState::Brewing, h.app.state());
    TEST_ASSERT_EQUAL_UINT(0, h.notifier.events.size());
    h.lightAt(kGreen, 3500);  // recovery + full confirmation
    TEST_ASSERT_EQUAL_UINT(1, h.notifier.count(EventType::CoffeeReady));
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

void test_zero_sample_interval_falls_back_to_default() {
    AppConfig config;
    config.sampleIntervalMs = 0;
    AppHarness h(config);
    TEST_ASSERT_EQUAL_UINT32(AppConfig{}.sampleIntervalMs, h.app.config().sampleIntervalMs);
}

void test_failed_notification_is_logged_but_session_completes() {
    AppHarness h;
    h.notifier.result = false;
    startAndArm(h);
    h.lightAt(kGreen, 5000);
    assertState(BrewState::Completed, h.app.state());
    TEST_ASSERT_TRUE(h.logger.contains("[WARN][NOTIFY] Delivery failed for CoffeeReady"));
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

void test_sensor_value_is_logged_at_debug_level_only() {
    AppConfig config;
    config.sensorLogIntervalMs = 1000;
    AppHarness h(config);
    h.lightAt(kDark, 3000);
    TEST_ASSERT_TRUE(h.logger.contains("[SENSOR] value=300"));
    h.sensor.setInvalid();
    h.runFor(1500);
    TEST_ASSERT_TRUE(h.logger.contains("[SENSOR] value=invalid"));

    h.logger.lines.clear();
    h.logger.setMinimumLevel(coffee::LogLevel::Info);
    h.lightAt(kDark, 3000);
    TEST_ASSERT_FALSE(h.logger.contains("[SENSOR] value="));
}

void test_reboot_returns_to_idle_and_needs_a_new_start() {
    AppHarness before;
    startAndArm(before);
    before.lightAt(kGreen, 5000);
    assertState(BrewState::Completed, before.app.state());

    AppHarness after;  // reboot while the machine is still green
    after.lightAt(kGreen, 10 * kMinute);
    assertState(BrewState::Idle, after.app.state());
    TEST_ASSERT_EQUAL_UINT(0, after.notifier.events.size());
    after.pressStart();
    after.lightAt(kDark, 3500);
    after.lightAt(kGreen, 5000);
    TEST_ASSERT_EQUAL_UINT(1, after.notifier.count(EventType::CoffeeReady));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_boot_logs_banner_and_initial_state);
    RUN_TEST(test_green_before_start_does_nothing);
    RUN_TEST(test_start_opens_a_session_with_a_blip_and_log);
    RUN_TEST(test_start_with_green_already_on_does_not_alert);
    RUN_TEST(test_full_cycle_beeps_notifies_and_completes);
    RUN_TEST(test_notification_is_sent_after_the_buzzer_pattern_ends);
    RUN_TEST(test_green_shorter_than_confirmation_does_not_alert);
    RUN_TEST(test_only_one_alert_per_session);
    RUN_TEST(test_new_start_allows_a_new_cycle);
    RUN_TEST(test_duplicate_start_is_ignored_and_logged);
    RUN_TEST(test_button_bounce_counts_as_one_press);
    RUN_TEST(test_holding_the_button_is_a_single_press);
    RUN_TEST(test_status_led_follows_state);
    RUN_TEST(test_sensor_failure_is_logged_and_chirped);
    RUN_TEST(test_sensor_failure_during_brewing_never_alerts);
    RUN_TEST(test_invalid_configuration_falls_back_to_defaults);
    RUN_TEST(test_zero_sample_interval_falls_back_to_default);
    RUN_TEST(test_failed_notification_is_logged_but_session_completes);
    RUN_TEST(test_network_changes_become_events);
    RUN_TEST(test_sensor_is_sampled_at_the_configured_interval);
    RUN_TEST(test_sensor_value_is_logged_at_debug_level_only);
    RUN_TEST(test_reboot_returns_to_idle_and_needs_a_new_start);
    return UNITY_END();
}
