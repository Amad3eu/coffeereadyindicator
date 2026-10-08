// Hysteresis (software Schmitt trigger) behaviour.
#include <unity.h>

#include "core/Hysteresis.h"

using coffee::Hysteresis;

namespace {
constexpr uint16_t kOn = 1800;
constexpr uint16_t kOff = 1200;
}  // namespace

void setUp() {}
void tearDown() {}

void test_starts_off() {
    Hysteresis h(kOn, kOff);
    TEST_ASSERT_FALSE(h.isOn());
}

void test_turns_on_at_or_above_on_threshold() {
    Hysteresis h(kOn, kOff);
    TEST_ASSERT_FALSE(h.update(kOn - 1));
    TEST_ASSERT_TRUE(h.update(kOn));
}

void test_stays_on_between_thresholds() {
    Hysteresis h(kOn, kOff);
    h.update(kOn);
    TEST_ASSERT_TRUE(h.update(kOff + 1));
    TEST_ASSERT_TRUE(h.update(kOn - 1));
}

void test_turns_off_at_or_below_off_threshold() {
    Hysteresis h(kOn, kOff);
    h.update(kOn);
    TEST_ASSERT_TRUE(h.update(kOff + 1));
    TEST_ASSERT_FALSE(h.update(kOff));
}

void test_stays_off_between_thresholds() {
    Hysteresis h(kOn, kOff);
    TEST_ASSERT_FALSE(h.update(kOff + 1));
    TEST_ASSERT_FALSE(h.update(kOn - 1));
}

void test_values_oscillating_around_on_threshold_do_not_flicker() {
    // A single-threshold comparator would toggle on every sample here.
    Hysteresis h(kOn, kOff);
    h.update(kOn);
    for (int i = 0; i < 1000; ++i) {
        const uint16_t value = (i % 2 == 0) ? kOn + 20 : kOn - 20;
        TEST_ASSERT_TRUE_MESSAGE(h.update(value), "output must stay on inside the dead band");
    }
}

void test_values_oscillating_around_off_threshold_do_not_flicker() {
    Hysteresis h(kOn, kOff);
    for (int i = 0; i < 1000; ++i) {
        const uint16_t value = (i % 2 == 0) ? kOff + 20 : kOff - 20;
        TEST_ASSERT_FALSE_MESSAGE(h.update(value), "output must stay off inside the dead band");
    }
}

void test_reset_returns_to_off() {
    Hysteresis h(kOn, kOff);
    h.update(kOn);
    h.reset();
    TEST_ASSERT_FALSE(h.isOn());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_starts_off);
    RUN_TEST(test_turns_on_at_or_above_on_threshold);
    RUN_TEST(test_stays_on_between_thresholds);
    RUN_TEST(test_turns_off_at_or_below_off_threshold);
    RUN_TEST(test_stays_off_between_thresholds);
    RUN_TEST(test_values_oscillating_around_on_threshold_do_not_flicker);
    RUN_TEST(test_values_oscillating_around_off_threshold_do_not_flicker);
    RUN_TEST(test_reset_returns_to_off);
    return UNITY_END();
}
