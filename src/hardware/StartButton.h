#pragma once

namespace coffee {

/// The "START COFFEE" push button, as a raw level: true while the button is
/// physically held. Debouncing and edge detection happen in the application
/// (see Debouncer), so adapters stay trivial and the logic stays testable.
class StartButton {
   public:
    virtual ~StartButton() = default;
    virtual bool isPressed() = 0;
};

}  // namespace coffee
