#pragma once

#include "hardware/ConfigStore.h"

namespace coffee {

/// Volatile store: keeps the configuration for the lifetime of the object.
class InMemoryConfigStore : public ConfigStore {
   public:
    bool load(CoffeeConfig& out) override {
        if (!hasValue_) {
            return false;
        }
        out = config_;
        return true;
    }

    bool save(const CoffeeConfig& config) override {
        config_ = config;
        hasValue_ = true;
        return true;
    }

   private:
    CoffeeConfig config_;
    bool hasValue_ = false;
};

}  // namespace coffee
