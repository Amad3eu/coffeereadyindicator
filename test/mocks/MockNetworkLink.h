#pragma once

#include "hardware/NetworkLink.h"

namespace coffee {
namespace test {

class MockNetworkLink : public NetworkLink {
   public:
    void begin() override { began = true; }
    void update(uint32_t /*nowMs*/) override { ++updates; }
    bool isConnected() const override { return connected; }

    bool connected = false;
    bool began = false;
    uint32_t updates = 0;
};

}  // namespace test
}  // namespace coffee
