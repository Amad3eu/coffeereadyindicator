#include "infrastructure/esp32/SerialLogger.h"

namespace coffee {

void SerialLogger::write(LogLevel level, const char* tag, const char* message) {
    if (level >= LogLevel::Warn) {
        out_.print('[');
        out_.print(toString(level));
        out_.print(']');
    }
    out_.print('[');
    out_.print(tag);
    out_.print("] ");
    out_.println(message);
}

}  // namespace coffee
