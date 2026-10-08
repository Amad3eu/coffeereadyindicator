#pragma once

#include "hardware/ConfigStore.h"

namespace coffee {

/// Persists the detection configuration in the ESP32's NVS flash partition
/// through the Preferences library. Survives reboots and re-flashing of the
/// firmware (not a full flash erase).
class NvsConfigStore : public ConfigStore {
   public:
    static constexpr const char* kNamespace = "coffee";

    bool load(CoffeeConfig& out) override;
    bool save(const CoffeeConfig& config) override;

    /// Removes the stored configuration so the defaults apply again.
    bool clear();
};

}  // namespace coffee
