// Non-blocking on/off pattern sequencing.
#include <unity.h>

#include "hardware/Pattern.h"
#include "hardware/PatternPlayer.h"

#include "mocks/MockBuzzer.h"
#include "mocks/MockClock.h"

using coffee::makePattern;
using coffee::Pattern;
using coffee::PatternPlayer;
using coffee::PatternStep;
using coffee::test::MockBuzzer;
using coffee::test::MockClock;

namespace {
constexpr PatternStep kDoubleBeepSteps[] = {{true, 150}, {false, 100}, {true, 150}};
constexpr Pattern kDoubleBeep = makePattern(kDoubleBeepSteps);
constexpr PatternStep kBlinkSteps[] = {{true, 100}, {false, 100}};
constexpr Pattern kBlink = makePattern(kBlinkSteps, true);
}  // namespace

void setUp() {}
void tearDown() {}

void test_idle_player_leaves_output_untouched() {
    MockBuzzer buzzer;
    MockClock clock;
    PatternPlayer player(buzzer);
    player.update(clock.nowMs());
    TEST_ASSERT_FALSE(player.isPlaying());
    TEST_ASSERT_EQUAL_UINT(0, buzzer.recorder.history.size());
}

void test_plays_steps_in_order_and_stops() {
    MockBuzzer buzzer;
    MockClock clock;
    PatternPlayer player(buzzer);

    player.play(kDoubleBeep, clock.nowMs());
    TEST_ASSERT_TRUE(player.isPlaying());
    TEST_ASSERT_TRUE(buzzer.isOn());

    clock.advance(149);
    player.update(clock.nowMs());
    TEST_ASSERT_TRUE(buzzer.isOn());

    clock.advance(1);  // 150 ms: first beep over
    player.update(clock.nowMs());
    TEST_ASSERT_FALSE(buzzer.isOn());

    clock.advance(100);  // pause over
    player.update(clock.nowMs());
    TEST_ASSERT_TRUE(buzzer.isOn());

    clock.advance(150);  // second beep over
    player.update(clock.nowMs());
    TEST_ASSERT_FALSE(buzzer.isOn());
    TEST_ASSERT_FALSE(player.isPlaying());
    TEST_ASSERT_EQUAL_UINT32(2, buzzer.beeps());
}

void test_loop_pattern_repeats() {
    MockBuzzer buzzer;
    MockClock clock;
    PatternPlayer player(buzzer);
    player.play(kBlink, clock.nowMs());
    for (int i = 0; i < 100; ++i) {  // 1000 ms: rising edges at 0, 200, ..., 1000 ms
        clock.advance(10);
        player.update(clock.nowMs());
    }
    TEST_ASSERT_TRUE(player.isPlaying());
    TEST_ASSERT_EQUAL_UINT32(6, buzzer.beeps());
}

void test_stop_turns_output_off() {
    MockBuzzer buzzer;
    MockClock clock;
    PatternPlayer player(buzzer);
    player.play(kBlink, clock.nowMs());
    TEST_ASSERT_TRUE(buzzer.isOn());
    player.stop();
    TEST_ASSERT_FALSE(buzzer.isOn());
    TEST_ASSERT_FALSE(player.isPlaying());
}

void test_empty_pattern_is_ignored() {
    MockBuzzer buzzer;
    MockClock clock;
    PatternPlayer player(buzzer);
    player.play(Pattern{nullptr, 0, false}, clock.nowMs());
    TEST_ASSERT_FALSE(player.isPlaying());
    TEST_ASSERT_FALSE(buzzer.isOn());
}

void test_stalled_loop_delays_instead_of_skipping_steps() {
    MockBuzzer buzzer;
    MockClock clock;
    PatternPlayer player(buzzer);
    player.play(kDoubleBeep, clock.nowMs());

    clock.advance(5000);  // the main loop was blocked for a long time
    player.update(clock.nowMs());
    TEST_ASSERT_TRUE(player.isPlaying());
    TEST_ASSERT_FALSE(buzzer.isOn());  // only advanced to the pause

    clock.advance(100);
    player.update(clock.nowMs());
    TEST_ASSERT_TRUE(buzzer.isOn());  // second beep still happens
}

void test_restarting_a_pattern_begins_from_the_first_step() {
    MockBuzzer buzzer;
    MockClock clock;
    PatternPlayer player(buzzer);
    player.play(kDoubleBeep, clock.nowMs());
    clock.advance(150);
    player.update(clock.nowMs());
    TEST_ASSERT_FALSE(buzzer.isOn());

    player.play(kDoubleBeep, clock.nowMs());
    TEST_ASSERT_TRUE(buzzer.isOn());
}

void test_pattern_survives_millis_overflow() {
    MockBuzzer buzzer;
    MockClock clock(UINT32_MAX - 100);
    PatternPlayer player(buzzer);
    player.play(kDoubleBeep, clock.nowMs());
    clock.advance(150);  // wraps around
    player.update(clock.nowMs());
    TEST_ASSERT_FALSE(buzzer.isOn());
    TEST_ASSERT_TRUE(player.isPlaying());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_idle_player_leaves_output_untouched);
    RUN_TEST(test_plays_steps_in_order_and_stops);
    RUN_TEST(test_loop_pattern_repeats);
    RUN_TEST(test_stop_turns_output_off);
    RUN_TEST(test_empty_pattern_is_ignored);
    RUN_TEST(test_stalled_loop_delays_instead_of_skipping_steps);
    RUN_TEST(test_restarting_a_pattern_begins_from_the_first_step);
    RUN_TEST(test_pattern_survives_millis_overflow);
    return UNITY_END();
}
