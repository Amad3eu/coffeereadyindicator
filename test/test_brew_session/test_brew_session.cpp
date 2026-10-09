// BrewSession lifecycle: a green light only means coffee inside a session
// that was started with the button and armed by a real cycle start.
#include <unity.h>

#include <cstdint>

#include "core/BrewSession.h"

#include "support/Assertions.h"
#include "support/SessionDriver.h"

using coffee::BrewState;
using coffee::CoffeeConfig;
using coffee::EventType;
using coffee::LightReading;
using coffee::test::assertEvent;
using coffee::test::assertState;
using coffee::test::SessionDriver;

namespace {

constexpr uint16_t kDark = 300;    // well below lightOffThreshold (1200)
constexpr uint16_t kGreen = 2800;  // well above lightOnThreshold (1800)
constexpr uint32_t kMinute = 60'000;

CoffeeConfig defaultConfig() {
    CoffeeConfig config;
    config.lightOnThreshold = 1800;
    config.lightOffThreshold = 1200;
    config.cycleStartConfirmationMs = 3000;
    config.readyConfirmationMs = 3000;
    return config;
}

/// START pressed with the light off, then armed by a confirmed OFF period.
void startAndArm(SessionDriver& d) {
    d.feedValue(kDark, 100);
    assertEvent(EventType::BrewStarted, d.pressStart());
    d.feedValue(kDark, 3500);
    assertState(BrewState::Brewing, d.state());
}

/// A full session up to COMPLETED.
void brewToCompletion(SessionDriver& d) {
    startAndArm(d);
    d.feedValue(kGreen, 3500);
    assertState(BrewState::Ready, d.state());
    assertEvent(EventType::BrewCompleted, d.complete());
    assertState(BrewState::Completed, d.state());
}

}  // namespace

void setUp() {}
void tearDown() {}

// --- outside a session --------------------------------------------------------

void test_green_before_start_never_alerts() {
    SessionDriver d(defaultConfig());
    d.feedValue(kGreen, 20 * kMinute);
    assertState(BrewState::Idle, d.state());
    TEST_ASSERT_EQUAL_UINT(0, d.events.size());
}

void test_start_creates_a_session() {
    SessionDriver d(defaultConfig());
    assertEvent(EventType::BrewStarted, d.pressStart());
    assertState(BrewState::Starting, d.state());
    TEST_ASSERT_TRUE(d.session.isActive());
}

// --- arming -----------------------------------------------------------------

void test_start_with_green_already_on_does_not_alert() {
    SessionDriver d(defaultConfig());
    d.feedValue(kGreen, 1000);  // the machine is idle and green
    d.pressStart();
    d.feedValue(kGreen, 20 * kMinute);  // and stays green
    assertState(BrewState::Starting, d.state());
    TEST_ASSERT_EQUAL_UINT(0, d.count(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_UINT(0, d.count(EventType::BrewArmed));
}

void test_session_requires_a_confirmed_off_period_to_arm() {
    SessionDriver d(defaultConfig());
    d.feedValue(kGreen, 500);
    d.pressStart();
    d.feedValue(kDark, 2900);  // not long enough yet
    assertState(BrewState::Starting, d.state());
    d.feedValue(kDark, 200);
    assertState(BrewState::Brewing, d.state());
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::BrewArmed));
}

void test_light_returning_during_arming_restarts_the_window() {
    SessionDriver d(defaultConfig());
    d.pressStart();
    d.feedValue(kDark, 2000);
    d.feedValue(kGreen, 200);  // the machine flashed green again
    d.feedValue(kDark, 2000);  // 2 s + 2 s is not 3 s continuous
    assertState(BrewState::Starting, d.state());
    d.feedValue(kDark, 1100);
    assertState(BrewState::Brewing, d.state());
}

void test_off_before_start_counts_only_from_start() {
    SessionDriver d(defaultConfig());
    d.feedValue(kDark, 10 * kMinute);  // dark for ages before START
    d.pressStart();
    d.feedValue(kDark, 2900);
    assertState(BrewState::Starting, d.state());  // the window starts at START
    d.feedValue(kDark, 200);
    assertState(BrewState::Brewing, d.state());
}

// --- ready ------------------------------------------------------------------

void test_green_shorter_than_confirmation_does_not_alert() {
    SessionDriver d(defaultConfig());
    startAndArm(d);
    d.feedValue(kGreen, 2000);
    d.feedValue(kDark, 1000);
    assertState(BrewState::Brewing, d.state());
    TEST_ASSERT_EQUAL_UINT(0, d.count(EventType::CoffeeReady));
}

void test_stable_green_while_armed_alerts() {
    SessionDriver d(defaultConfig());
    startAndArm(d);
    d.feedValue(kGreen, 2950);
    assertState(BrewState::Brewing, d.state());
    d.feedValue(kGreen, 200);
    assertState(BrewState::Ready, d.state());
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReady));
}

void test_red_green_flicker_while_brewing_does_not_alert() {
    SessionDriver d(defaultConfig());
    startAndArm(d);
    for (int i = 0; i < 20; ++i) {  // 1.5 s green / 1 s not green, repeatedly
        d.feedValue(kGreen, 1500);
        d.feedValue(kDark, 1000);
    }
    assertState(BrewState::Brewing, d.state());
    TEST_ASSERT_EQUAL_UINT(0, d.count(EventType::CoffeeReady));
    d.feedValue(kGreen, 3500);  // finally steady
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReady));
}

void test_only_one_coffee_ready_per_session() {
    SessionDriver d(defaultConfig());
    startAndArm(d);
    d.feedValue(kGreen, 10 * kMinute);
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReady));
    d.complete();
    d.feedValue(kDark, 5000);
    d.feedValue(kGreen, 10 * kMinute);
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReady));
}

void test_ready_ignores_the_light_until_completed() {
    SessionDriver d(defaultConfig());
    startAndArm(d);
    d.feedValue(kGreen, 3500);
    assertState(BrewState::Ready, d.state());
    d.feedValue(kDark, 5000);
    d.feedValue(kGreen, 5000);
    assertState(BrewState::Ready, d.state());
    TEST_ASSERT_EQUAL_UINT(1,
                           d.events.size() - d.count(EventType::BrewStarted) - d.count(EventType::BrewArmed));
}

// --- completed --------------------------------------------------------------

void test_oscillations_after_completed_produce_no_event() {
    SessionDriver d(defaultConfig());
    brewToCompletion(d);
    const size_t eventsBefore = d.events.size();
    for (int i = 0; i < 50; ++i) {
        d.feedValue(kDark, 4000);
        d.feedValue(kGreen, 4000);
    }
    assertState(BrewState::Completed, d.state());
    TEST_ASSERT_EQUAL_UINT(eventsBefore, d.events.size());
}

void test_new_start_after_completed_allows_a_new_cycle() {
    SessionDriver d(defaultConfig());
    brewToCompletion(d);
    d.feedValue(kGreen, 1000);  // the machine still shows green from the last coffee
    assertEvent(EventType::BrewStarted, d.pressStart());
    assertState(BrewState::Starting, d.state());
    d.feedValue(kGreen, 5000);  // still green: nothing
    d.feedValue(kDark, 3500);   // the new cycle began
    assertState(BrewState::Brewing, d.state());
    d.feedValue(kGreen, 3500);
    TEST_ASSERT_EQUAL_UINT(2, d.count(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_UINT(2, d.count(EventType::BrewStarted));
}

void test_complete_outside_ready_is_a_no_op() {
    SessionDriver d(defaultConfig());
    assertEvent(EventType::None, d.complete());
    d.pressStart();
    assertEvent(EventType::None, d.complete());
    assertState(BrewState::Starting, d.state());
}

// --- duplicate START ----------------------------------------------------------

void test_start_during_starting_is_ignored() {
    SessionDriver d(defaultConfig());
    d.pressStart();
    d.feedValue(kDark, 1000);
    assertEvent(EventType::StartIgnored, d.pressStart());
    assertState(BrewState::Starting, d.state());
    d.feedValue(kDark, 2100);  // the original window keeps counting
    assertState(BrewState::Brewing, d.state());
}

void test_start_during_brewing_and_ready_is_ignored() {
    SessionDriver d(defaultConfig());
    startAndArm(d);
    assertEvent(EventType::StartIgnored, d.pressStart());
    assertState(BrewState::Brewing, d.state());
    d.feedValue(kGreen, 3500);
    assertEvent(EventType::StartIgnored, d.pressStart());
    assertState(BrewState::Ready, d.state());
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::BrewStarted));
}

// --- sensor failure ---------------------------------------------------------

void test_invalid_readings_during_brewing_never_alert() {
    SessionDriver d(defaultConfig());
    startAndArm(d);
    d.feedValue(kGreen, 1000);
    d.feedInvalid(10 * kMinute);
    assertState(BrewState::Brewing, d.state());
    TEST_ASSERT_EQUAL_UINT(0, d.count(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::SensorFailure));
}

void test_sensor_recovery_restarts_the_confirmation_window() {
    SessionDriver d(defaultConfig());
    startAndArm(d);
    d.feedValue(kGreen, 2000);
    d.feedInvalid(500);
    d.feedValue(kGreen, 50);
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::SensorRecovered));
    d.feedValue(kGreen, 2500);  // 2 s before the gap do not count
    assertState(BrewState::Brewing, d.state());
    d.feedValue(kGreen, 600);
    assertState(BrewState::Ready, d.state());
}

void test_sensor_failure_while_arming_restarts_the_off_window() {
    SessionDriver d(defaultConfig());
    d.pressStart();
    d.feedValue(kDark, 2000);
    d.feedInvalid(1000);
    d.feedValue(kDark, 2500);
    assertState(BrewState::Starting, d.state());
    d.feedValue(kDark, 600);
    assertState(BrewState::Brewing, d.state());
}

void test_start_while_sensor_is_failed_waits_for_recovery() {
    SessionDriver d(defaultConfig());
    d.feedInvalid(1000);
    assertEvent(EventType::BrewStarted, d.pressStart());
    d.feedInvalid(10 * kMinute);
    assertState(BrewState::Starting, d.state());
    d.feedValue(kDark, 3500);
    assertState(BrewState::Brewing, d.state());
}

// --- millis() overflow ---------------------------------------------------------

void test_windows_survive_millis_overflow() {
    SessionDriver d(defaultConfig(), UINT32_MAX - 1500);
    d.pressStart();
    d.feedValue(kDark, 3500);  // the clock wraps while confirming the start
    TEST_ASSERT_TRUE(d.clock.nowMs() < 10'000);
    assertState(BrewState::Brewing, d.state());
    d.feedValue(kGreen, 3500);
    assertState(BrewState::Ready, d.state());
    TEST_ASSERT_EQUAL_UINT(1, d.count(EventType::CoffeeReady));
}

void test_ready_window_survives_millis_overflow() {
    SessionDriver d(defaultConfig(), UINT32_MAX - 5000);
    startAndArm(d);             // ends ~1.4 s before the wrap
    d.feedValue(kGreen, 3500);  // wraps while confirming the green
    assertState(BrewState::Ready, d.state());
}

// --- reboot (a reboot == a fresh session object) -------------------------------

void test_reboot_in_any_state_returns_to_idle_and_waits_for_start() {
    SessionDriver before(defaultConfig());
    brewToCompletion(before);

    SessionDriver after(defaultConfig());  // reboot: machine still green
    after.feedValue(kGreen, 10 * kMinute);
    assertState(BrewState::Idle, after.state());
    TEST_ASSERT_EQUAL_UINT(0, after.events.size());

    after.pressStart();  // the user starts the next coffee
    after.feedValue(kDark, 3500);
    after.feedValue(kGreen, 3500);
    TEST_ASSERT_EQUAL_UINT(1, after.count(EventType::CoffeeReady));
}

void test_reboot_during_brewing_loses_the_session() {
    SessionDriver before(defaultConfig());
    startAndArm(before);

    SessionDriver after(defaultConfig());  // reboot while armed
    after.feedValue(kGreen, 3500);         // the coffee finishes: no session, no alert
    assertState(BrewState::Idle, after.state());
    TEST_ASSERT_EQUAL_UINT(0, after.count(EventType::CoffeeReady));
}

void test_time_in_state_is_tracked() {
    SessionDriver d(defaultConfig());
    d.pressStart();
    d.feedValue(kDark, 1000);
    TEST_ASSERT_EQUAL_UINT32(1000, d.session.timeInStateMs(d.clock.nowMs()));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_green_before_start_never_alerts);
    RUN_TEST(test_start_creates_a_session);
    RUN_TEST(test_start_with_green_already_on_does_not_alert);
    RUN_TEST(test_session_requires_a_confirmed_off_period_to_arm);
    RUN_TEST(test_light_returning_during_arming_restarts_the_window);
    RUN_TEST(test_off_before_start_counts_only_from_start);
    RUN_TEST(test_green_shorter_than_confirmation_does_not_alert);
    RUN_TEST(test_stable_green_while_armed_alerts);
    RUN_TEST(test_red_green_flicker_while_brewing_does_not_alert);
    RUN_TEST(test_only_one_coffee_ready_per_session);
    RUN_TEST(test_ready_ignores_the_light_until_completed);
    RUN_TEST(test_oscillations_after_completed_produce_no_event);
    RUN_TEST(test_new_start_after_completed_allows_a_new_cycle);
    RUN_TEST(test_complete_outside_ready_is_a_no_op);
    RUN_TEST(test_start_during_starting_is_ignored);
    RUN_TEST(test_start_during_brewing_and_ready_is_ignored);
    RUN_TEST(test_invalid_readings_during_brewing_never_alert);
    RUN_TEST(test_sensor_recovery_restarts_the_confirmation_window);
    RUN_TEST(test_sensor_failure_while_arming_restarts_the_off_window);
    RUN_TEST(test_start_while_sensor_is_failed_waits_for_recovery);
    RUN_TEST(test_windows_survive_millis_overflow);
    RUN_TEST(test_ready_window_survives_millis_overflow);
    RUN_TEST(test_reboot_in_any_state_returns_to_idle_and_waits_for_start);
    RUN_TEST(test_reboot_during_brewing_loses_the_session);
    RUN_TEST(test_time_in_state_is_tracked);
    return UNITY_END();
}
