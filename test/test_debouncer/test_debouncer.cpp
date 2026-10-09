// Non-blocking button debouncing and press-edge detection.
#include <unity.h>

#include "hardware/Debouncer.h"

using coffee::Debouncer;

namespace {
constexpr uint32_t kDebounceMs = 50;

/// Feeds `level` for `durationMs` in 1 ms steps starting at `now`; returns the new time.
uint32_t hold(Debouncer& d, bool level, uint32_t now, uint32_t durationMs) {
    for (uint32_t i = 0; i < durationMs; ++i) {
        d.update(level, now++);
    }
    return now;
}
}  // namespace

void setUp() {}
void tearDown() {}

void test_no_press_before_any_input() {
    Debouncer d(kDebounceMs);
    TEST_ASSERT_FALSE(d.consumePress());
    TEST_ASSERT_FALSE(d.isPressed());
}

void test_clean_press_is_reported_once() {
    Debouncer d(kDebounceMs);
    uint32_t now = hold(d, false, 1000, 10);
    now = hold(d, true, now, kDebounceMs + 5);
    TEST_ASSERT_TRUE(d.isPressed());
    TEST_ASSERT_TRUE(d.consumePress());
    TEST_ASSERT_FALSE(d.consumePress());  // consumed
    hold(d, true, now, 5000);             // holding does not repeat
    TEST_ASSERT_FALSE(d.consumePress());
}

void test_bounces_shorter_than_the_window_are_ignored() {
    Debouncer d(kDebounceMs);
    uint32_t now = hold(d, false, 1000, 10);
    for (int i = 0; i < 20; ++i) {  // 20 ms of chatter
        now = hold(d, true, now, 1);
        now = hold(d, false, now, 1);
    }
    hold(d, false, now, kDebounceMs * 2);
    TEST_ASSERT_FALSE(d.isPressed());
    TEST_ASSERT_FALSE(d.consumePress());
}

void test_chatter_before_a_real_press_still_yields_one_press() {
    Debouncer d(kDebounceMs);
    uint32_t now = hold(d, false, 1000, 10);
    for (int i = 0; i < 10; ++i) {
        now = hold(d, true, now, 2);
        now = hold(d, false, now, 2);
    }
    now = hold(d, true, now, kDebounceMs + 1);
    TEST_ASSERT_TRUE(d.consumePress());
    TEST_ASSERT_FALSE(d.consumePress());
}

void test_release_and_second_press_are_separate() {
    Debouncer d(kDebounceMs);
    uint32_t now = hold(d, false, 1000, 10);
    now = hold(d, true, now, kDebounceMs + 1);
    TEST_ASSERT_TRUE(d.consumePress());
    now = hold(d, false, now, kDebounceMs + 1);
    TEST_ASSERT_FALSE(d.isPressed());
    now = hold(d, true, now, kDebounceMs + 1);
    TEST_ASSERT_TRUE(d.consumePress());
}

void test_button_held_at_boot_does_not_count_as_a_press() {
    Debouncer d(kDebounceMs);
    hold(d, true, 1000, kDebounceMs * 3);
    TEST_ASSERT_TRUE(d.isPressed());
    TEST_ASSERT_FALSE(d.consumePress());
}

void test_press_survives_millis_overflow() {
    Debouncer d(kDebounceMs);
    uint32_t now = hold(d, false, UINT32_MAX - 20, 10);
    now = hold(d, true, now, kDebounceMs + 1);  // the clock wraps during the hold
    TEST_ASSERT_TRUE(now < 1000);
    TEST_ASSERT_TRUE(d.consumePress());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_no_press_before_any_input);
    RUN_TEST(test_clean_press_is_reported_once);
    RUN_TEST(test_bounces_shorter_than_the_window_are_ignored);
    RUN_TEST(test_chatter_before_a_real_press_still_yields_one_press);
    RUN_TEST(test_release_and_second_press_are_separate);
    RUN_TEST(test_button_held_at_boot_does_not_count_as_a_press);
    RUN_TEST(test_press_survives_millis_overflow);
    return UNITY_END();
}
