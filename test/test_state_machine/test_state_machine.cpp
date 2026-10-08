// CoffeeDetector lifecycle: the numbered scenarios from the project brief.
#include <unity.h>

#include <cstdint>

#include "core/CoffeeDetector.h"

#include "support/Assertions.h"
#include "support/DetectorDriver.h"

using coffee::CoffeeConfig;
using coffee::CoffeeState;
using coffee::EventType;
using coffee::LightReading;
using coffee::test::assertEvent;
using coffee::test::assertState;
using coffee::test::DetectorDriver;

namespace {

constexpr uint16_t kDark = 300;     // well below lightOffThreshold (1200)
constexpr uint16_t kBright = 2800;  // well above lightOnThreshold (1800)
constexpr uint32_t kMinute = 60'000;

CoffeeConfig defaultConfig() {
    CoffeeConfig config;
    config.lightOnThreshold = 1800;
    config.lightOffThreshold = 1200;
    config.readyConfirmationMs = 3000;
    config.rearmConfirmationMs = 3000;
    return config;
}

/// A detector that has been running with the light off, i.e. a normal boot.
DetectorDriver bootedInTheDark(uint32_t startMs = 10'000) {
    DetectorDriver d(defaultConfig(), startMs);
    d.feedValue(kDark, 100);
    return d;
}

/// Brings a detector to NOTIFIED with the light on.
void brewUntilNotified(DetectorDriver& d) {
    d.feedValue(kBright, 3500);
    d.markNotified();
    assertState(CoffeeState::Notified, d.detector.state());
}

}  // namespace

void setUp() {}
void tearDown() {}

// 1. light never turns on
void test_light_never_on_stays_idle() {
    DetectorDriver d = bootedInTheDark();
    d.feedValue(kDark, 20 * kMinute);
    assertState(CoffeeState::Idle, d.detector.state());
    TEST_ASSERT_EQUAL_UINT(0, d.events.size());
}

// 2. light on for less than the confirmation window
void test_short_flash_does_not_trigger() {
    DetectorDriver d = bootedInTheDark();
    d.feedValue(kBright, 2000);
    assertState(CoffeeState::Detecting, d.detector.state());
    d.feedValue(kDark, 1000);
    assertState(CoffeeState::Idle, d.detector.state());
    TEST_ASSERT_EQUAL_UINT(0, d.events.size());
}

// 3. light on for longer than the confirmation window
void test_sustained_light_triggers_ready() {
    DetectorDriver d = bootedInTheDark();
    d.feedValue(kBright, 2950);
    assertState(CoffeeState::Detecting, d.detector.state());
    d.feedValue(kBright, 200);
    assertState(CoffeeState::Ready, d.detector.state());
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReady));
}

// 4. CoffeeReady is produced exactly once
void test_coffee_ready_event_fires_exactly_once() {
    DetectorDriver d = bootedInTheDark();
    d.feedValue(kBright, 10'000);
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReady));
    d.markNotified();
    d.feedValue(kBright, 10'000);
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReady));
}

// 5 + 6. light stays on for 20 minutes, no second alert
void test_light_on_for_twenty_minutes_alerts_once() {
    DetectorDriver d = bootedInTheDark();
    brewUntilNotified(d);
    d.feedValue(kBright, 20 * kMinute);
    assertState(CoffeeState::Notified, d.detector.state());
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_UINT(1, d.events.size());
}

// 7. light turns off
void test_light_off_moves_to_rearming() {
    DetectorDriver d = bootedInTheDark();
    brewUntilNotified(d);
    d.feedValue(kDark, 100);
    assertState(CoffeeState::Rearming, d.detector.state());
}

// 8. system re-arms after the light stays off
void test_rearms_after_light_stays_off() {
    DetectorDriver d = bootedInTheDark();
    brewUntilNotified(d);
    d.feedValue(kDark, 2900);
    assertState(CoffeeState::Rearming, d.detector.state());
    d.feedValue(kDark, 300);
    assertState(CoffeeState::Idle, d.detector.state());
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReset));
}

void test_light_returning_during_rearm_does_not_alert_again() {
    DetectorDriver d = bootedInTheDark();
    brewUntilNotified(d);
    d.feedValue(kDark, 1500);
    assertState(CoffeeState::Rearming, d.detector.state());
    d.feedValue(kBright, 5000);
    assertState(CoffeeState::Notified, d.detector.state());
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_UINT(0, d.count(EventType::CoffeeReset));
}

// 9 + 10. a second brew produces a second event
void test_second_brew_produces_second_event() {
    DetectorDriver d = bootedInTheDark();
    brewUntilNotified(d);
    d.feedValue(kDark, 5000);
    assertState(CoffeeState::Idle, d.detector.state());

    d.feedValue(kBright, 3500);
    assertState(CoffeeState::Ready, d.detector.state());
    TEST_ASSERT_EQUAL_UINT(2, d.count(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReset));
}

// 11 + 12. values oscillating around the threshold do not flicker
void test_oscillation_inside_dead_band_keeps_light_on() {
    DetectorDriver d = bootedInTheDark();
    d.feedValue(1800, 50);  // exactly the on threshold: light turns on
    for (int i = 0; i < 100; ++i) {
        d.step(LightReading::of(i % 2 == 0 ? 1300 : 1790));  // inside the band
    }
    assertState(CoffeeState::Ready, d.detector.state());
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReady));
}

void test_oscillation_below_on_threshold_never_triggers() {
    DetectorDriver d = bootedInTheDark();
    for (int i = 0; i < 400; ++i) {
        d.step(LightReading::of(i % 2 == 0 ? 1790 : 1500));  // never reaches 1800
    }
    assertState(CoffeeState::Idle, d.detector.state());
    TEST_ASSERT_EQUAL_UINT(0, d.events.size());
}

void test_noisy_light_after_notified_does_not_rearm() {
    DetectorDriver d = bootedInTheDark();
    brewUntilNotified(d);
    for (int i = 0; i < 2000; ++i) {
        d.step(LightReading::of(i % 3 == 0 ? 1250 : 1700));  // never reaches 1200
    }
    assertState(CoffeeState::Notified, d.detector.state());
    TEST_ASSERT_EQUAL_UINT(1, d.events.size());
}

// 13. sensor produces invalid values
void test_invalid_readings_raise_sensor_failure_once() {
    DetectorDriver d(defaultConfig());  // fails from the very first sample
    d.feedInvalid(2000);
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::SensorFailure));
    TEST_ASSERT_FALSE(d.detector.isSensorHealthy());
    assertState(CoffeeState::Idle, d.detector.state());
}

void test_out_of_range_reading_is_invalid() {
    DetectorDriver d = bootedInTheDark();
    d.feedValue(4096, 100);  // above sensorMaxValue
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::SensorFailure));
}

void test_invalid_readings_freeze_the_state_machine() {
    DetectorDriver d = bootedInTheDark();
    d.feedValue(kBright, 1000);
    assertState(CoffeeState::Detecting, d.detector.state());
    d.feedInvalid(10'000);
    assertState(CoffeeState::Detecting, d.detector.state());
    TEST_ASSERT_EQUAL_UINT(0, d.count(EventType::CoffeeReady));
}

void test_sensor_recovery_restarts_confirmation_window() {
    DetectorDriver d = bootedInTheDark();
    d.feedValue(kBright, 2000);
    d.feedInvalid(500);
    d.feedValue(kBright, 50);
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::SensorRecovered));
    TEST_ASSERT_TRUE(d.detector.isSensorHealthy());

    // 2 s before the failure do not count: a full 3 s window is needed again.
    d.feedValue(kBright, 2500);
    assertState(CoffeeState::Detecting, d.detector.state());
    d.feedValue(kBright, 600);
    assertState(CoffeeState::Ready, d.detector.state());
}

// 14. millis() overflow
void test_timers_survive_millis_overflow() {
    DetectorDriver d = bootedInTheDark(UINT32_MAX - 1000);
    d.feedValue(kBright, 3500);  // the clock wraps around after ~1 s
    TEST_ASSERT_TRUE(d.clock.nowMs() < 10'000);
    assertState(CoffeeState::Ready, d.detector.state());
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReady));

    d.markNotified();
    d.feedValue(kDark, 3500);
    assertState(CoffeeState::Idle, d.detector.state());
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReset));
}

void test_rearm_timer_survives_millis_overflow() {
    DetectorDriver d = bootedInTheDark(UINT32_MAX - 5000);
    brewUntilNotified(d);
    d.feedValue(kDark, 3500);  // wraps during the re-arm window
    assertState(CoffeeState::Idle, d.detector.state());
}

// 15. reboot in different states (a reboot == a fresh detector)
void test_reboot_while_light_on_does_not_alert_again_by_default() {
    DetectorDriver before = bootedInTheDark();
    brewUntilNotified(before);

    DetectorDriver after(defaultConfig());  // reboot, light still on
    after.feedValue(kBright, 10 * kMinute);
    assertState(CoffeeState::Notified, after.detector.state());
    TEST_ASSERT_EQUAL_UINT(0, after.events.size());

    after.feedValue(kDark, 3500);  // the machine is switched off -> normal re-arm
    assertState(CoffeeState::Idle, after.detector.state());
    TEST_ASSERT_EQUAL_UINT(1, after.count(EventType::CoffeeReset));
}

void test_reboot_while_light_on_alerts_when_configured() {
    CoffeeConfig config = defaultConfig();
    config.alertIfLightOnAtBoot = true;
    DetectorDriver after(config);
    after.feedValue(kBright, 3500);
    assertState(CoffeeState::Ready, after.detector.state());
    TEST_ASSERT_EQUAL_UINT(1, after.count(EventType::CoffeeReady));
}

void test_reboot_while_light_off_starts_idle() {
    DetectorDriver before = bootedInTheDark();
    brewUntilNotified(before);
    before.feedValue(kDark, 1000);
    assertState(CoffeeState::Rearming, before.detector.state());

    DetectorDriver after(defaultConfig());  // reboot during REARMING
    after.feedValue(kDark, 1000);
    assertState(CoffeeState::Idle, after.detector.state());
    after.feedValue(kBright, 3500);  // the next brew is detected normally
    TEST_ASSERT_EQUAL_UINT(1, after.count(EventType::CoffeeReady));
}

void test_reboot_with_light_on_then_off_then_on_detects_next_brew() {
    DetectorDriver d(defaultConfig());
    d.feedValue(kBright, 1000);  // boot: light already on -> NOTIFIED
    assertState(CoffeeState::Notified, d.detector.state());
    d.feedValue(kDark, 3500);
    d.feedValue(kBright, 3500);
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReady));
}

// READY / markNotified semantics
void test_ready_waits_for_mark_notified() {
    DetectorDriver d = bootedInTheDark();
    d.feedValue(kBright, 3500);
    assertState(CoffeeState::Ready, d.detector.state());
    d.feedValue(kDark, 5000);  // light changes are ignored until acknowledged
    assertState(CoffeeState::Ready, d.detector.state());
    d.markNotified();
    assertState(CoffeeState::Notified, d.detector.state());
    d.feedValue(kDark, 50);
    assertState(CoffeeState::Rearming, d.detector.state());
}

void test_mark_notified_is_a_no_op_outside_ready() {
    DetectorDriver d = bootedInTheDark();
    d.markNotified();
    assertState(CoffeeState::Idle, d.detector.state());
    d.feedValue(kBright, 1000);
    d.markNotified();
    assertState(CoffeeState::Detecting, d.detector.state());
}

void test_time_in_state_is_tracked() {
    DetectorDriver d = bootedInTheDark();
    d.feedValue(kBright, 1000);
    TEST_ASSERT_EQUAL_UINT32(1000, d.detector.timeInStateMs(d.clock.nowMs()));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_light_never_on_stays_idle);
    RUN_TEST(test_short_flash_does_not_trigger);
    RUN_TEST(test_sustained_light_triggers_ready);
    RUN_TEST(test_coffee_ready_event_fires_exactly_once);
    RUN_TEST(test_light_on_for_twenty_minutes_alerts_once);
    RUN_TEST(test_light_off_moves_to_rearming);
    RUN_TEST(test_rearms_after_light_stays_off);
    RUN_TEST(test_light_returning_during_rearm_does_not_alert_again);
    RUN_TEST(test_second_brew_produces_second_event);
    RUN_TEST(test_oscillation_inside_dead_band_keeps_light_on);
    RUN_TEST(test_oscillation_below_on_threshold_never_triggers);
    RUN_TEST(test_noisy_light_after_notified_does_not_rearm);
    RUN_TEST(test_invalid_readings_raise_sensor_failure_once);
    RUN_TEST(test_out_of_range_reading_is_invalid);
    RUN_TEST(test_invalid_readings_freeze_the_state_machine);
    RUN_TEST(test_sensor_recovery_restarts_confirmation_window);
    RUN_TEST(test_timers_survive_millis_overflow);
    RUN_TEST(test_rearm_timer_survives_millis_overflow);
    RUN_TEST(test_reboot_while_light_on_does_not_alert_again_by_default);
    RUN_TEST(test_reboot_while_light_on_alerts_when_configured);
    RUN_TEST(test_reboot_while_light_off_starts_idle);
    RUN_TEST(test_reboot_with_light_on_then_off_then_on_detects_next_brew);
    RUN_TEST(test_ready_waits_for_mark_notified);
    RUN_TEST(test_mark_notified_is_a_no_op_outside_ready);
    RUN_TEST(test_time_in_state_is_tracked);
    return UNITY_END();
}
