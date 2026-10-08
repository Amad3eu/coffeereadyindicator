#pragma once

#include <unity.h>

#include "core/CoffeeState.h"
#include "core/Event.h"

namespace coffee {
namespace test {

/// Compares states by name so a failure prints "IDLE" instead of "0".
inline void assertState(CoffeeState expected, CoffeeState actual) {
    TEST_ASSERT_EQUAL_STRING(toString(expected), toString(actual));
}

inline void assertEvent(EventType expected, EventType actual) {
    TEST_ASSERT_EQUAL_STRING(toString(expected), toString(actual));
}

}  // namespace test
}  // namespace coffee
