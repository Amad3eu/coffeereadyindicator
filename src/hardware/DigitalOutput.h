#pragma once

namespace coffee {

/// Anything that can be switched on or off: buzzers, LEDs, relays...
class DigitalOutput {
   public:
    virtual ~DigitalOutput() = default;
    virtual void set(bool on) = 0;

    void on() { set(true); }
    void off() { set(false); }
};

}  // namespace coffee
