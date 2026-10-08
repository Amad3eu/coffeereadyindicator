#pragma once

// The version string is injected by platformio.ini (-DCOFFEE_SENTINEL_VERSION="x.y.z").
// The fallback keeps ad-hoc builds (IDE syntax checks, scripts) compiling.
#ifndef COFFEE_SENTINEL_VERSION
#define COFFEE_SENTINEL_VERSION "dev"
#endif

namespace coffee {

constexpr const char* kVersion = COFFEE_SENTINEL_VERSION;

}  // namespace coffee
