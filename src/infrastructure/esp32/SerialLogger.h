#pragma once

#include <Arduino.h>

#include "hardware/Logger.h"

namespace coffee {

/// Logger that writes to a Print target (normally `Serial`).
class SerialLogger : public Logger {
   public:
    explicit SerialLogger(Print& out, LogLevel minimumLevel = LogLevel::Info)
        : Logger(minimumLevel), out_(out) {}

   protected:
    void write(LogLevel level, const char* tag, const char* message) override;

   private:
    Print& out_;
};

}  // namespace coffee
