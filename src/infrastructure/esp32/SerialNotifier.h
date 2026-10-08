#pragma once

#include <Arduino.h>

#include "hardware/Notifier.h"

namespace coffee {

/// Notifier that prints to the serial console, e.g. "[NOTIFY] Coffee is ready".
/// Always succeeds; useful on its own during development and as a trace next
/// to the webhook notifier in production.
class SerialNotifier : public Notifier {
   public:
    explicit SerialNotifier(Print& out) : out_(out) {}

    bool notify(EventType event) override {
        out_.print("[NOTIFY] ");
        out_.println(toMessage(event));
        return true;
    }

   private:
    Print& out_;
};

}  // namespace coffee
