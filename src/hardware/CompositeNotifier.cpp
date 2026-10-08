#include "hardware/CompositeNotifier.h"

namespace coffee {

bool CompositeNotifier::add(Notifier& notifier) {
    if (count_ >= kMaxNotifiers) {
        return false;
    }
    notifiers_[count_++] = &notifier;
    return true;
}

bool CompositeNotifier::notify(EventType event) {
    bool allDelivered = true;
    for (size_t i = 0; i < count_; ++i) {
        if (!notifiers_[i]->notify(event)) {
            allDelivered = false;
        }
    }
    return allDelivered;
}

}  // namespace coffee
