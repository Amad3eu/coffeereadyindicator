// Threshold suggestion math and the interactive Calibrator flow.
#include <unity.h>

#include "application/Calibrator.h"
#include "core/Calibration.h"

#include "mocks/MockClock.h"
#include "mocks/MockLightSensor.h"
#include "mocks/MockLogger.h"

using coffee::CalibrationConfig;
using coffee::CalibrationPhase;
using coffee::Calibrator;
using coffee::SampleStats;
using coffee::suggestThresholds;
using coffee::ThresholdSuggestion;
using coffee::test::MockClock;
using coffee::test::MockLightSensor;
using coffee::test::MockLogger;

namespace {

struct Fixture {
    MockClock clock;
    MockLightSensor sensor;
    MockLogger logger;
    CalibrationConfig config;
    Calibrator calibrator;

    Fixture() : calibrator(sensor, clock, logger, config) { calibrator.begin(); }

    void runFor(uint32_t ms) {
        for (uint32_t elapsed = 0; elapsed < ms; elapsed += 10) {
            calibrator.update();
            clock.advance(10);
        }
    }

    void samplePhase(uint16_t value) {
        sensor.setValue(value);
        calibrator.confirm();
        runFor(config.sampleWindowMs + 10);
    }
};

}  // namespace

void setUp() {}
void tearDown() {}

void test_sample_stats_track_min_max_mean() {
    SampleStats stats;
    TEST_ASSERT_EQUAL_UINT16(0, stats.mean());
    stats.add(200);
    stats.add(300);  // new maximum
    stats.add(100);  // new minimum
    TEST_ASSERT_EQUAL_UINT32(3, stats.count());
    TEST_ASSERT_EQUAL_UINT16(100, stats.minimum());
    TEST_ASSERT_EQUAL_UINT16(300, stats.maximum());
    TEST_ASSERT_EQUAL_UINT16(200, stats.mean());
    stats.reset();
    TEST_ASSERT_EQUAL_UINT32(0, stats.count());
}

void test_suggestion_places_thresholds_inside_the_span() {
    const ThresholdSuggestion s = suggestThresholds(341, 2870);
    TEST_ASSERT_TRUE(s.valid);
    TEST_ASSERT_EQUAL_UINT16(1858, s.lightOnThreshold);
    TEST_ASSERT_EQUAL_UINT16(1099, s.lightOffThreshold);
    TEST_ASSERT_TRUE(s.lightOffThreshold < s.lightOnThreshold);
}

void test_suggestion_rejects_inverted_or_narrow_span() {
    TEST_ASSERT_FALSE(suggestThresholds(2000, 1000).valid);
    TEST_ASSERT_FALSE(suggestThresholds(1000, 1000).valid);
    TEST_ASSERT_FALSE(suggestThresholds(1000, 1050).valid);
    TEST_ASSERT_TRUE(suggestThresholds(1000, 1100).valid);
}

void test_calibrator_starts_with_ambient_prompt() {
    Fixture f;
    TEST_ASSERT_EQUAL(static_cast<int>(CalibrationPhase::Ambient), static_cast<int>(f.calibrator.phase()));
    TEST_ASSERT_TRUE(f.logger.contains("[CALIBRATION] Phase 1/3 AMBIENT"));
    TEST_ASSERT_FALSE(f.calibrator.isSampling());
}

void test_calibrator_logs_live_values_while_waiting() {
    Fixture f;
    f.sensor.setValue(345);
    f.runFor(1600);
    TEST_ASSERT_TRUE(f.logger.count("[CALIBRATION] live value=345") >= 3);
}

void test_calibrator_walks_through_the_three_phases() {
    Fixture f;
    f.samplePhase(322);
    TEST_ASSERT_EQUAL(static_cast<int>(CalibrationPhase::LedOff), static_cast<int>(f.calibrator.phase()));
    TEST_ASSERT_TRUE(f.logger.contains("[CALIBRATION] AMBIENT: 322"));
    TEST_ASSERT_TRUE(f.logger.contains("[CALIBRATION] Phase 2/3 LED OFF"));

    f.samplePhase(341);
    TEST_ASSERT_EQUAL(static_cast<int>(CalibrationPhase::LedOn), static_cast<int>(f.calibrator.phase()));
    TEST_ASSERT_TRUE(f.logger.contains("[CALIBRATION] Phase 3/3 LED ON"));

    f.samplePhase(2870);
    TEST_ASSERT_TRUE(f.calibrator.isDone());
    TEST_ASSERT_TRUE(f.logger.contains("[CALIBRATION] Ambient: 322"));
    TEST_ASSERT_TRUE(f.logger.contains("[CALIBRATION] LED OFF: 341"));
    TEST_ASSERT_TRUE(f.logger.contains("[CALIBRATION] LED ON: 2870"));
    TEST_ASSERT_TRUE(f.logger.contains("lightOnThreshold=1858 lightOffThreshold=1099"));
    TEST_ASSERT_TRUE(f.calibrator.result().suggestion.valid);
}

void test_calibrator_warns_when_led_on_is_too_close_to_led_off() {
    Fixture f;
    f.samplePhase(322);
    f.samplePhase(341);
    f.samplePhase(360);
    TEST_ASSERT_TRUE(f.calibrator.isDone());
    TEST_ASSERT_FALSE(f.calibrator.result().suggestion.valid);
    TEST_ASSERT_TRUE(f.logger.contains("[WARN][CALIBRATION] LED ON (360) is too close"));
}

void test_calibrator_retries_phase_without_valid_samples() {
    Fixture f;
    f.sensor.setInvalid();
    f.calibrator.confirm();
    f.runFor(f.config.sampleWindowMs + 10);
    TEST_ASSERT_EQUAL(static_cast<int>(CalibrationPhase::Ambient), static_cast<int>(f.calibrator.phase()));
    TEST_ASSERT_TRUE(f.logger.contains("[WARN][CALIBRATION] No valid samples"));
    TEST_ASSERT_FALSE(f.calibrator.isSampling());
}

void test_live_value_warns_when_the_sensor_is_invalid() {
    Fixture f;
    f.sensor.setInvalid();
    f.runFor(600);
    TEST_ASSERT_TRUE(f.logger.contains("[WARN][CALIBRATION] live value=invalid"));
}

void test_finished_calibration_stays_quiet() {
    Fixture f;
    f.samplePhase(300);
    f.samplePhase(320);
    f.samplePhase(2000);
    TEST_ASSERT_TRUE(f.calibrator.isDone());
    TEST_ASSERT_TRUE(f.logger.contains("[CALIBRATION] Calibration complete"));
    const size_t linesWhenDone = f.logger.lines.size();
    f.runFor(5000);  // no more live values or prompts
    TEST_ASSERT_EQUAL_UINT(linesWhenDone, f.logger.lines.size());
}

void test_confirm_is_ignored_while_sampling_or_done() {
    Fixture f;
    f.sensor.setValue(300);
    f.calibrator.confirm();
    TEST_ASSERT_TRUE(f.calibrator.isSampling());
    f.calibrator.confirm();  // ignored
    f.runFor(f.config.sampleWindowMs + 10);
    TEST_ASSERT_EQUAL(static_cast<int>(CalibrationPhase::LedOff), static_cast<int>(f.calibrator.phase()));

    f.samplePhase(300);
    f.samplePhase(2000);
    TEST_ASSERT_TRUE(f.calibrator.isDone());
    f.calibrator.confirm();  // ignored
    TEST_ASSERT_FALSE(f.calibrator.isSampling());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sample_stats_track_min_max_mean);
    RUN_TEST(test_suggestion_places_thresholds_inside_the_span);
    RUN_TEST(test_suggestion_rejects_inverted_or_narrow_span);
    RUN_TEST(test_calibrator_starts_with_ambient_prompt);
    RUN_TEST(test_calibrator_logs_live_values_while_waiting);
    RUN_TEST(test_calibrator_walks_through_the_three_phases);
    RUN_TEST(test_calibrator_warns_when_led_on_is_too_close_to_led_off);
    RUN_TEST(test_calibrator_retries_phase_without_valid_samples);
    RUN_TEST(test_live_value_warns_when_the_sensor_is_invalid);
    RUN_TEST(test_finished_calibration_stays_quiet);
    RUN_TEST(test_confirm_is_ignored_while_sampling_or_done);
    return UNITY_END();
}
