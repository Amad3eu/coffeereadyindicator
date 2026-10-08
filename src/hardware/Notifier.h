#pragma once

#include "core/Event.h"

namespace coffee {

/// Delivers a domain event to the outside world (serial console, webhook, ...).
/// Returns true when the notification was handed over successfully.
class Notifier {
   public:
    virtual ~Notifier() = default;
    virtual bool notify(EventType event) = 0;
};

}  // namespace coffee
