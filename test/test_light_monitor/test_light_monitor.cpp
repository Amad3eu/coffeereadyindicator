// LightMonitor: hysteresis on readings plus sensor health transitions.
#include <unity.h>

#include "core/LightMonitor.h"

#include "support/Assertions.h"

using coffee::CoffeeConfig;
using coffee::EventType;
using coffee::LightMonitor;
using coffee::LightReading;
using coffee::test::assertEvent;

void setUp() {}
void tearDown() {}

void test_starts_off_and_healthy() {
    LightMonitor m{CoffeeConfig{}};
    TEST_ASSERT_FALSE(m.isOn());
    TEST_ASSERT_TRUE(m.isHealthy());
}

void test_follows_thresholds_with_hysteresis() {
    LightMonitor m{CoffeeConfig{}};  // on >= 1800, off <= 1200
    assertEvent(EventType::None, m.update(LightReading::of(1799)));
    TEST_ASSERT_FALSE(m.isOn());
    m.update(LightReading::of(1800));
    TEST_ASSERT_TRUE(m.isOn());
    m.update(LightReading::of(1201));
    TEST_ASSERT_TRUE(m.isOn());
    m.update(LightReading::of(1200));
    TEST_ASSERT_FALSE(m.isOn());
}

void test_invalid_reading_raises_sensor_failure_once() {
    LightMonitor m{CoffeeConfig{}};
    assertEvent(EventType::SensorFailure, m.update(LightReading::invalid()));
    assertEvent(EventType::None, m.update(LightReading::invalid()));
    TEST_ASSERT_FALSE(m.isHealthy());
}

void test_out_of_range_value_is_invalid() {
    LightMonitor m{CoffeeConfig{}};
    assertEvent(EventType::SensorFailure, m.update(LightReading::of(4096)));
}

void test_failure_keeps_the_last_light_state() {
    LightMonitor m{CoffeeConfig{}};
    m.update(LightReading::of(3000));
    TEST_ASSERT_TRUE(m.isOn());
    m.update(LightReading::invalid());
    TEST_ASSERT_TRUE(m.isOn());
}

void test_recovery_is_reported_once_and_applies_the_reading() {
    LightMonitor m{CoffeeConfig{}};
    m.update(LightReading::invalid());
    assertEvent(EventType::SensorRecovered, m.update(LightReading::of(3000)));
    TEST_ASSERT_TRUE(m.isHealthy());
    TEST_ASSERT_TRUE(m.isOn());
    assertEvent(EventType::None, m.update(LightReading::of(3000)));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_starts_off_and_healthy);
    RUN_TEST(test_follows_thresholds_with_hysteresis);
    RUN_TEST(test_invalid_reading_raises_sensor_failure_once);
    RUN_TEST(test_out_of_range_value_is_invalid);
    RUN_TEST(test_failure_keeps_the_last_light_state);
    RUN_TEST(test_recovery_is_reported_once_and_applies_the_reading);
    return UNITY_END();
}
