#pragma once

#include "core/CoffeeConfig.h"

namespace coffee {

/// Persistence port for the detection configuration (NVS on the ESP32,
/// memory in tests).
class ConfigStore {
   public:
    virtual ~ConfigStore() = default;

    /// Fills `out` with the stored configuration. Returns false (and leaves
    /// `out` untouched) when nothing has been stored yet.
    virtual bool load(CoffeeConfig& out) = 0;

    virtual bool save(const CoffeeConfig& config) = 0;
};

}  // namespace coffee
