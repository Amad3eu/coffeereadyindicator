#pragma once

#include <cstdint>

namespace coffee {

/// Connectivity port. The application only needs to know whether a link is up;
/// how it is (re)established is the adapter's business.
class NetworkLink {
   public:
    virtual ~NetworkLink() = default;
    virtual void begin() = 0;
    virtual void update(uint32_t nowMs) = 0;
    virtual bool isConnected() const = 0;
};

/// Link used when the firmware runs without network credentials.
class NullNetworkLink : public NetworkLink {
   public:
    void begin() override {}
    void update(uint32_t /*nowMs*/) override {}
    bool isConnected() const override { return false; }
};

}  // namespace coffee
