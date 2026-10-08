#pragma once

#include <cstddef>

#include "hardware/Notifier.h"

namespace coffee {

/// Fans a notification out to several notifiers (e.g. serial + webhook).
/// Fixed capacity, no heap allocation.
class CompositeNotifier : public Notifier {
   public:
    static constexpr size_t kMaxNotifiers = 4;

    /// Returns false when the composite is full.
    bool add(Notifier& notifier);
    size_t size() const { return count_; }

    /// Returns true only if every registered notifier succeeded.
    bool notify(EventType event) override;

   private:
    Notifier* notifiers_[kMaxNotifiers] = {};
    size_t count_ = 0;
};

}  // namespace coffee
