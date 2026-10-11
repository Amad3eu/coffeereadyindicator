// CoffeeConfig validation, CompositeNotifier and InMemoryConfigStore.
#include <unity.h>

#include "core/CoffeeConfig.h"
#include "hardware/CompositeNotifier.h"
#include "hardware/InMemoryConfigStore.h"

#include "mocks/MockNotifier.h"

using coffee::CoffeeConfig;
using coffee::CompositeNotifier;
using coffee::EventType;
using coffee::InMemoryConfigStore;
using coffee::test::MockNotifier;

void setUp() {}
void tearDown() {}

void test_defaults_are_valid() {
    const CoffeeConfig config;
    TEST_ASSERT_TRUE(config.isValid());
    TEST_ASSERT_EQUAL_STRING("ok", toString(config.validate()));
}

void test_inverted_thresholds_are_rejected() {
    CoffeeConfig config;
    config.lightOnThreshold = 1000;
    config.lightOffThreshold = 1000;
    TEST_ASSERT_EQUAL(static_cast<int>(CoffeeConfig::Error::ThresholdsInverted),
                      static_cast<int>(config.validate()));
    config.lightOffThreshold = 1500;
    TEST_ASSERT_FALSE(config.isValid());
}

void test_threshold_above_sensor_range_is_rejected() {
    CoffeeConfig config;
    config.lightOnThreshold = 5000;
    TEST_ASSERT_EQUAL(static_cast<int>(CoffeeConfig::Error::ThresholdAboveSensorMax),
                      static_cast<int>(config.validate()));
}

void test_zero_confirmation_is_rejected() {
    CoffeeConfig config;
    config.readyConfirmationMs = 0;
    TEST_ASSERT_EQUAL(static_cast<int>(CoffeeConfig::Error::ZeroConfirmation),
                      static_cast<int>(config.validate()));
    config.readyConfirmationMs = 1;
    config.cycleStartConfirmationMs = 0;
    TEST_ASSERT_FALSE(config.isValid());
}

void test_in_memory_store_round_trips() {
    InMemoryConfigStore store;
    CoffeeConfig loaded;
    TEST_ASSERT_FALSE(store.load(loaded));

    CoffeeConfig saved;
    saved.lightOnThreshold = 2222;
    saved.cycleStartConfirmationMs = 7000;
    TEST_ASSERT_TRUE(store.save(saved));
    TEST_ASSERT_TRUE(store.load(loaded));
    TEST_ASSERT_EQUAL_UINT16(2222, loaded.lightOnThreshold);
    TEST_ASSERT_EQUAL_UINT32(7000, loaded.cycleStartConfirmationMs);
}

void test_composite_notifier_fans_out() {
    MockNotifier a;
    MockNotifier b;
    CompositeNotifier composite;
    TEST_ASSERT_TRUE(composite.notify(EventType::CoffeeReady));  // empty composite succeeds
    TEST_ASSERT_TRUE(composite.add(a));
    TEST_ASSERT_TRUE(composite.add(b));
    TEST_ASSERT_TRUE(composite.notify(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_UINT(1, a.count(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_UINT(1, b.count(EventType::CoffeeReady));
}

void test_composite_notifier_reports_partial_failure() {
    MockNotifier ok;
    MockNotifier failing;
    failing.result = false;
    CompositeNotifier composite;
    composite.add(ok);
    composite.add(failing);
    TEST_ASSERT_FALSE(composite.notify(EventType::CoffeeReady));
    TEST_ASSERT_EQUAL_UINT(1, ok.events.size());
}

void test_composite_notifier_has_fixed_capacity() {
    MockNotifier n;
    CompositeNotifier composite;
    for (size_t i = 0; i < CompositeNotifier::kMaxNotifiers; ++i) {
        TEST_ASSERT_TRUE(composite.add(n));
    }
    TEST_ASSERT_FALSE(composite.add(n));
    TEST_ASSERT_EQUAL_UINT(CompositeNotifier::kMaxNotifiers, composite.size());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_defaults_are_valid);
    RUN_TEST(test_inverted_thresholds_are_rejected);
    RUN_TEST(test_threshold_above_sensor_range_is_rejected);
    RUN_TEST(test_zero_confirmation_is_rejected);
    RUN_TEST(test_in_memory_store_round_trips);
    RUN_TEST(test_composite_notifier_fans_out);
    RUN_TEST(test_composite_notifier_reports_partial_failure);
    RUN_TEST(test_composite_notifier_has_fixed_capacity);
    return UNITY_END();
}
